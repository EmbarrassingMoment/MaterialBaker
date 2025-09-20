// Copyright 2025 kurorekish. All Rights Reserved.

#include "MaterialBaker.h"
#include "MaterialBakerStyle.h"
#include "MaterialBakerCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "PropertyCustomizationHelpers.h"
#include "SMaterialDropTarget.h"
#include "AssetRegistry/AssetData.h"
#include "AssetThumbnail.h"
#include "AssetToolsModule.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "IAssetTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/TextureFactory.h"
#include "Misc/FileHelper.h"
#include "Editor.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "IDesktopPlatform.h"
#include "DesktopPlatformModule.h"
#include "Misc/ScopedSlowTask.h"
#include "Widgets/Input/SCheckBox.h"
#include "RenderCore.h"
#include "Misc/MessageDialog.h"
#include "Engine/Texture.h"
#include "UObject/EnumProperty.h"
#include "Widgets/Input/SSpinBox.h"



static const FName MaterialBakerTabName("MaterialBaker");

#define LOCTEXT_NAMESPACE "FMaterialBakerModule"

void FMaterialBakerModule::StartupModule()
{
	FMaterialBakerStyle::Initialize();
	FMaterialBakerStyle::ReloadTextures();
	FMaterialBakerCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);
	PluginCommands->MapAction(
		FMaterialBakerCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FMaterialBakerModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMaterialBakerModule::RegisterMenus));

	ThumbnailPool = MakeShareable(new FAssetThumbnailPool(10));

	CurrentBakeSettings = FMaterialBakeSettings();

	// 圧縮設定の選択肢を初期化
	const UEnum* CompressionSettingsEnum = StaticEnum<TextureCompressionSettings>();
	if (CompressionSettingsEnum)
	{
		for (int32 i = 0; i < CompressionSettingsEnum->NumEnums() - 1; ++i)
		{
			CompressionSettingOptions.Add(MakeShareable(new FString(CompressionSettingsEnum->GetDisplayNameTextByIndex(i).ToString())));
		}
	}

	// 出力形式の選択肢を初期化
	const UEnum* OutputTypeEnum = StaticEnum<EMaterialBakeOutputType>();
	if (OutputTypeEnum)
	{
		for (int32 i = 0; i < OutputTypeEnum->NumEnums(); ++i)
		{
			OutputTypeOptions.Add(MakeShareable(new FString(OutputTypeEnum->GetDisplayNameTextByIndex(i).ToString())));
		}
	}


	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(MaterialBakerTabName, FOnSpawnTab::CreateRaw(this, &FMaterialBakerModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FMaterialBakerTabTitle", "MaterialBaker"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FMaterialBakerModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FMaterialBakerStyle::Shutdown();
	FMaterialBakerCommands::Unregister();
	ThumbnailPool.Reset();
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MaterialBakerTabName);
}

TSharedRef<SDockTab> FMaterialBakerModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	SAssignNew(ThumbnailBox, SBox)
		.WidthOverride(64.f)
		.HeightOverride(64.f)
		[
			SNew(SBorder)
			.Padding(4.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("NoMaterialSelected", "No Material Selected"))
				.Justification(ETextJustify::Center)
			]
		];

        SAssignNew(BakeQueueListView, SListView<TSharedPtr<FMaterialBakeSettings>>)
                .ListItemsSource(&BakeQueue)
                .OnGenerateRow(SListView<TSharedPtr<FMaterialBakeSettings>>::FOnGenerateRow::CreateRaw(this, &FMaterialBakerModule::OnGenerateRowForBakeQueue))
                .OnSelectionChanged(SListView<TSharedPtr<FMaterialBakeSettings>>::FOnSelectionChanged::CreateRaw(this, &FMaterialBakerModule::OnBakeQueueSelectionChanged))
		.HeaderRow
		(
			SNew(SHeaderRow)
			+ SHeaderRow::Column("Material").DefaultLabel(LOCTEXT("MaterialColumn", "Material"))
			+ SHeaderRow::Column("BakedName").DefaultLabel(LOCTEXT("BakedNameColumn", "Baked Name"))
		);

	TSharedRef<SVerticalBox> WidgetContent = SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SelectMaterialLabel", "Target Material"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 5.0f, 0.0f)
			.VAlign(VAlign_Bottom)
			[
				ThumbnailBox.ToSharedRef()
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Bottom)
			[
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(UMaterialInterface::StaticClass())
				.OnObjectChanged(FOnSetObject::CreateRaw(this, &FMaterialBakerModule::OnMaterialChanged))
				.ObjectPath_Lambda([this]() -> FString {
					return CurrentBakeSettings.Material ? CurrentBakeSettings.Material->GetPathName() : FString("");
				})
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("BakedNameLabel", "Baked Texture Name"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SEditableTextBox)
			.HintText(LOCTEXT("BakedNameHint", "Enter baked texture name"))
			.Text_Lambda([this]() { return FText::FromString(CurrentBakeSettings.BakedName); })
			.OnTextChanged(FOnTextChanged::CreateRaw(this, &FMaterialBakerModule::OnBakedNameTextChanged))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("TextureSizeLabel", "Bake Texture Size"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0.0f, 0.0f, 5.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("TextureWidthLabel", "Width"))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("TextureHeightLabel", "Height"))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0.0f, 0.0f, 5.0f, 0.0f)
			[
				SNew(SSpinBox<int32>)
				.Value_Lambda([this]() { return CurrentBakeSettings.TextureWidth; })
				.OnValueChanged(FOnInt32ValueChanged::CreateRaw(this, &FMaterialBakerModule::OnTextureWidthChanged))
				.MinValue(1)
				.MaxValue(8192)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SSpinBox<int32>)
				.Value_Lambda([this]() { return CurrentBakeSettings.TextureHeight; })
				.OnValueChanged(FOnInt32ValueChanged::CreateRaw(this, &FMaterialBakerModule::OnTextureHeightChanged))
				.MinValue(1)
				.MaxValue(8192)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("CompressionSettingLabel", "Compression Setting"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&CompressionSettingOptions)
			.OnSelectionChanged(SComboBox<TSharedPtr<FString>>::FOnSelectionChanged::CreateRaw(this, &FMaterialBakerModule::OnCompressionSettingChanged))
			.OnGenerateWidget(SComboBox<TSharedPtr<FString>>::FOnGenerateWidget::CreateRaw(this, &FMaterialBakerModule::MakeWidgetForCompressionOption))
			.InitiallySelectedItem(CompressionSettingOptions.Num() > 0 ? CompressionSettingOptions[0] : nullptr)
			[
				SNew(STextBlock)
				.Text_Lambda([this] {
					const UEnum* Enum = StaticEnum<TextureCompressionSettings>();
					if (Enum)
					{
						return Enum->GetDisplayNameTextByValue(CurrentBakeSettings.CompressionSettings);
					}
					return FText::GetEmpty();
				})
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this]() -> ECheckBoxState { return CurrentBakeSettings.bSRGB ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged(FOnCheckStateChanged::CreateRaw(this, &FMaterialBakerModule::OnSRGBCheckBoxChanged))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(5.0f, 0.0f, 0.0f, 0.0f))
			[
				SNew(STextBlock)
				.Text(LOCTEXT("sRGBLabel", "sRGB"))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("OutputTypeLabel", "Output Type"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&OutputTypeOptions)
			.OnSelectionChanged(SComboBox<TSharedPtr<FString>>::FOnSelectionChanged::CreateRaw(this, &FMaterialBakerModule::OnOutputTypeChanged))
			.OnGenerateWidget(SComboBox<TSharedPtr<FString>>::FOnGenerateWidget::CreateRaw(this, &FMaterialBakerModule::MakeWidgetForOutputTypeOption))
			.InitiallySelectedItem(OutputTypeOptions.Num() > 0 ? OutputTypeOptions[0] : nullptr)
			[
				SNew(STextBlock)
				.Text_Lambda([this] {
					const UEnum* Enum = StaticEnum<EMaterialBakeOutputType>();
					if (Enum)
					{
						return Enum->GetDisplayNameTextByValue((int64)CurrentBakeSettings.OutputType);
					}
					return FText::GetEmpty();
				})
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("OutputPathLabel", "Output Path"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SEditableTextBox)
				.Text_Lambda([this] { return FText::FromString(CurrentBakeSettings.OutputPath); })
				.OnTextChanged(FOnTextChanged::CreateRaw(this, &FMaterialBakerModule::OnOutputPathTextChanged))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("BrowseButton", "Browse..."))
				.OnClicked(FOnClicked::CreateRaw(this, &FMaterialBakerModule::OnBrowseButtonClicked))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(2.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("AddToQueueButton", "Add to Queue"))
                                .OnClicked(FOnClicked::CreateRaw(this, &FMaterialBakerModule::OnAddToQueueClicked))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(2.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("UpdateSelectedButton", "Update Selected"))
                                .OnClicked(FOnClicked::CreateRaw(this, &FMaterialBakerModule::OnUpdateSelectedClicked))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(2.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("RemoveSelectedButton", "Remove Selected"))
                                .OnClicked(FOnClicked::CreateRaw(this, &FMaterialBakerModule::OnRemoveSelectedClicked))
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(5.0f)
		[
			BakeQueueListView.ToSharedRef()
		]
		+ SVerticalBox::Slot()
		.HAlign(HAlign_Right)
		.AutoHeight()
		.Padding(10.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("BakeButton", "Bake All"))
			.OnClicked(FOnClicked::CreateRaw(this, &FMaterialBakerModule::OnBakeButtonClicked))
		];

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			WidgetContent
		];
}



void FMaterialBakerModule::OnMaterialChanged(const FAssetData& AssetData)
{
	CurrentBakeSettings.Material = Cast<UMaterialInterface>(AssetData.GetAsset());

	if (CurrentBakeSettings.Material)
	{
		CurrentBakeSettings.OutputPath = FPackageName::GetLongPackagePath(CurrentBakeSettings.Material->GetPathName());
	}


	if (ThumbnailBox.IsValid() && CurrentBakeSettings.Material)
	{
		// 選択されたアセットのサムネイルを更新
		TSharedPtr<FAssetThumbnail> Thumbnail = MakeShareable(new FAssetThumbnail(AssetData, 64, 64, ThumbnailPool));
		ThumbnailBox->SetContent(Thumbnail->MakeThumbnailWidget());
	}
	else if (ThumbnailBox.IsValid())
	{
		// アセットの選択が解除された場合は、プレースホルダーテキストに戻す
		ThumbnailBox->SetContent(
			SNew(SBorder)
			.Padding(4.f)
			[
				SNew(STextBlock)
					.Text(LOCTEXT("NoMaterialSelected", "No Material Selected"))
					.Justification(ETextJustify::Center)
			]
		);
	}
}

void FMaterialBakerModule::OnBakedNameTextChanged(const FText& InText)
{
	CurrentBakeSettings.BakedName = InText.ToString();
}

void FMaterialBakerModule::OnSRGBCheckBoxChanged(ECheckBoxState NewState)
{
	CurrentBakeSettings.bSRGB = (NewState == ECheckBoxState::Checked);
}


void FMaterialBakerModule::BakeMaterial(const FMaterialBakeSettings& BakeSettings)
{
	FIntPoint TextureSize(BakeSettings.TextureWidth, BakeSettings.TextureHeight);

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot get world context!"));
		return;
	}

	const int32 TotalSteps = 5; // 処理の総ステップ数
	FScopedSlowTask SlowTask(TotalSteps, FText::Format(LOCTEXT("BakingMaterial", "Baking Material: {0}..."), FText::FromString(BakeSettings.BakedName)));
	SlowTask.MakeDialog(); // プログレスバーのダイアログを表示

	// ステップ1: Render Targetの作成
	SlowTask.EnterProgressFrame(1, LOCTEXT("CreateRenderTarget", "Step 1/5: Creating Render Target..."));

	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
	if (!RenderTarget)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("CreateRenderTargetFailed", "Failed to create Render Target."));
		return;
	}

	RenderTarget->AddToRoot();
	RenderTarget->RenderTargetFormat = RTF_RGBA16f;
	// PF_FloatRGBAを指定し、ガンマはリニアに強制
	RenderTarget->InitCustomFormat(TextureSize.X, TextureSize.Y, PF_FloatRGBA, true);
	RenderTarget->UpdateResourceImmediate(true);

	// ステップ2: マテリアルを描画
	SlowTask.EnterProgressFrame(1, LOCTEXT("DrawMaterial", "Step 2/5: Drawing Material..."));
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(World, RenderTarget, BakeSettings.Material);

	// レンダリングコマンドをフラッシュして、描画が完了するのを待つ
	FlushRenderingCommands();

	// ステップ3: ピクセルデータを読み込み
	SlowTask.EnterProgressFrame(1, LOCTEXT("ReadPixels", "Step 3/5: Reading Pixels..."));
	FRenderTarget* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
	TArray<FFloat16Color> RawPixels;
	if (!RenderTargetResource || !RenderTargetResource->ReadFloat16Pixels(RawPixels))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ReadPixelFailed", "Failed to read pixels from Render Target."));
		RenderTarget->RemoveFromRoot();
		return;
	}


	if (BakeSettings.OutputType == EMaterialBakeOutputType::Texture)
	{
		// ステップ4: アセットの作成準備
		SlowTask.EnterProgressFrame(1, LOCTEXT("PrepareAsset", "Step 4/5: Preparing Asset..."));
		FString PackagePath = BakeSettings.OutputPath;
		FString AssetName = BakeSettings.BakedName;

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		FString UniquePackageName;
		FString UniqueAssetName;
		AssetToolsModule.Get().CreateUniqueAssetName(BakeSettings.OutputPath / AssetName, TEXT(""), UniquePackageName, UniqueAssetName);

		UPackage* Package = CreatePackage(*UniquePackageName);
		Package->FullyLoad();

		UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *UniqueAssetName, RF_Public | RF_Standalone | RF_MarkAsRootSet);
		if (!NewTexture)
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("CreateTextureFailed", "Failed to create new texture asset."));
			RenderTarget->RemoveFromRoot();
			return;
		}
		NewTexture->AddToRoot();

		NewTexture->CompressionSettings = BakeSettings.CompressionSettings;
		NewTexture->SRGB = BakeSettings.bSRGB;

		// ステップ5: テクスチャを更新して保存
		SlowTask.EnterProgressFrame(1, LOCTEXT("UpdateTexture", "Step 5/5: Updating and Saving Texture..."));
		NewTexture->Source.Init(TextureSize.X, TextureSize.Y, 1, 1, TSF_RGBA16F, (const uint8*)RawPixels.GetData());
		NewTexture->UpdateResource();
		Package->MarkPackageDirty();
		FAssetRegistryModule::GetRegistry().AssetCreated(NewTexture);
		NewTexture->PostEditChange();
		NewTexture->RemoveFromRoot();
	}
	else if (BakeSettings.OutputType == EMaterialBakeOutputType::PNG || BakeSettings.OutputType == EMaterialBakeOutputType::JPEG)
	{
		// ステップ4: 画像ファイルとしてエクスポート
		SlowTask.EnterProgressFrame(1, LOCTEXT("ExportImage", "Step 4/5: Exporting Image..."));

		TArray<FColor> OutPixels;
		OutPixels.AddUninitialized(TextureSize.X * TextureSize.Y);
		for (int32 i = 0; i < RawPixels.Num(); ++i)
		{
			OutPixels[i] = FLinearColor(RawPixels[i]).ToFColor(BakeSettings.bSRGB);
		}

		// ファイルダイアログを開く
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		if (DesktopPlatform)
		{
			TArray<FString> OutFiles;
			FString Filter = (BakeSettings.OutputType == EMaterialBakeOutputType::PNG) ? TEXT("PNG Image|*.png") : TEXT("JPEG Image|*.jpg;*.jpeg");
			bool bOpened = DesktopPlatform->SaveFileDialog(
				FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
				TEXT("Save Baked Texture"),
				FPaths::ProjectSavedDir(),
				BakeSettings.BakedName + ((BakeSettings.OutputType == EMaterialBakeOutputType::PNG) ? ".png" : ".jpg"),
				Filter,
				EFileDialogFlags::None,
				OutFiles
			);

			if (bOpened && OutFiles.Num() > 0)
			{
				FString SaveFilePath = OutFiles[0];
				IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
				EImageFormat ImageFormat = (BakeSettings.OutputType == EMaterialBakeOutputType::PNG) ? EImageFormat::PNG : EImageFormat::JPEG;
				TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);

				if (ImageWrapper.IsValid() && ImageWrapper->SetRaw(OutPixels.GetData(), OutPixels.Num() * sizeof(FColor), TextureSize.X, TextureSize.Y, ERGBFormat::BGRA, 8))
				{
					const TArray64<uint8>& CompressedData = ImageWrapper->GetCompressed();
					FFileHelper::SaveArrayToFile(CompressedData, *SaveFilePath);
				}
			}
		}
	}

	RenderTarget->RemoveFromRoot();
}

FReply FMaterialBakerModule::OnBakeButtonClicked()
{
	if (BakeQueue.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("EmptyQueueWarning", "Please add at least one material to the bake queue."));
		return FReply::Handled();
	}

	// --- Validation ---
	TSet<FString> UniqueNames;
	for (const auto& Settings : BakeQueue)
	{
		if (!Settings->Material)
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("InvalidMaterialInQueue", "An item in the queue has no material selected."), FText::FromString(Settings->BakedName)));
			return FReply::Handled();
		}
		if (Settings->BakedName.IsEmpty())
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("EmptyNameInQueue", "An item in the queue has no name."));
			return FReply::Handled();
		}

		FString FullPath = FPaths::Combine(Settings->OutputPath, Settings->BakedName);
		if (UniqueNames.Contains(FullPath))
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("DuplicateNameInQueue", "Duplicate output name and path found in queue: {0}"), FText::FromString(FullPath)));
			return FReply::Handled();
		}
		UniqueNames.Add(FullPath);
	}


	FScopedSlowTask SlowTask(BakeQueue.Num(), LOCTEXT("BakingMaterials", "Baking Materials..."));
	SlowTask.MakeDialog();

	bool bSuccess = true;
	for (const auto& Settings : BakeQueue)
	{
		FText ProgressText = FText::Format(LOCTEXT("BakingMaterialItem", "Baking {0} ({1}/{2})"), FText::FromString(Settings->BakedName), FText::AsNumber(SlowTask.CompletedWork + 1), FText::AsNumber(BakeQueue.Num()));
		SlowTask.EnterProgressFrame(1, ProgressText);

		if (SlowTask.ShouldCancel())
		{
			bSuccess = false;
			break;
		}

		BakeMaterial(*Settings);
	}

	if (bSuccess)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("BakeComplete", "Batch bake completed successfully."));
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("BakeCancelled", "Batch bake was cancelled."));
	}

	BakeQueue.Empty();
	BakeQueueListView->RequestListRefresh();

	return FReply::Handled();
}


void FMaterialBakerModule::OnTextureWidthChanged(int32 NewValue)
{
	CurrentBakeSettings.TextureWidth = NewValue;
}

void FMaterialBakerModule::OnTextureHeightChanged(int32 NewValue)
{
	CurrentBakeSettings.TextureHeight = NewValue;
}

void FMaterialBakerModule::OnCompressionSettingChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		const UEnum* Enum = StaticEnum<TextureCompressionSettings>();
		if (Enum)
		{
			for (int32 i = 0; i < Enum->NumEnums() - 1; ++i)
			{
				if (*NewSelection == Enum->GetDisplayNameTextByIndex(i).ToString())
				{
					CurrentBakeSettings.CompressionSettings = static_cast<TextureCompressionSettings>(Enum->GetValueByIndex(i));
					break;
				}
			}
		}
	}
}

TSharedRef<SWidget> FMaterialBakerModule::MakeWidgetForCompressionOption(TSharedPtr<FString> InOption)
{
	return SNew(STextBlock).Text(FText::FromString(*InOption));
}

void FMaterialBakerModule::OnOutputTypeChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		const UEnum* Enum = StaticEnum<EMaterialBakeOutputType>();
		if (Enum)
		{
			for (int32 i = 0; i < Enum->NumEnums(); ++i)
			{
				if (*NewSelection == Enum->GetDisplayNameTextByIndex(i).ToString())
				{
					CurrentBakeSettings.OutputType = static_cast<EMaterialBakeOutputType>(Enum->GetValueByIndex(i));
					break;
				}
			}
		}
	}
}

TSharedRef<SWidget> FMaterialBakerModule::MakeWidgetForOutputTypeOption(TSharedPtr<FString> InOption)
{
	return SNew(STextBlock).Text(FText::FromString(*InOption));
}

void FMaterialBakerModule::OnOutputPathTextChanged(const FText& InText)
{
	CurrentBakeSettings.OutputPath = InText.ToString();
}

FReply FMaterialBakerModule::OnBrowseButtonClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		FString FolderName;
		const FString Title = TEXT("Select Output Folder");
		const FString DefaultPath = FPaths::ProjectContentDir();
		if (DesktopPlatform->OpenDirectoryDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			Title,
			DefaultPath,
			FolderName
		))
		{
			if (FPaths::MakePathRelativeTo(FolderName, *FPaths::ProjectContentDir()))
			{
				CurrentBakeSettings.OutputPath = FString("/Game/") + FolderName;
			}
			else
			{
				CurrentBakeSettings.OutputPath = FolderName;
			}
		}
	}
	return FReply::Handled();
}


FReply FMaterialBakerModule::OnAddToQueueClicked()
{
	if (!CurrentBakeSettings.Material)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoMaterialForQueue", "Please select a material first."));
		return FReply::Handled();
	}
	if (CurrentBakeSettings.BakedName.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoNameForQueue", "Please enter a name for the baked texture."));
		return FReply::Handled();
	}

	BakeQueue.Add(MakeShared<FMaterialBakeSettings>(CurrentBakeSettings));
	if (BakeQueueListView.IsValid())
	{
		BakeQueueListView->RequestListRefresh();
	}

	return FReply::Handled();
}

TSharedRef<ITableRow> FMaterialBakerModule::OnGenerateRowForBakeQueue(TSharedPtr<FMaterialBakeSettings> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	FString MaterialName = InItem->Material ? InItem->Material->GetName() : TEXT("None");
	FString BakedName = InItem->BakedName;

	return SNew(STableRow<TSharedPtr<FMaterialBakeSettings>>, OwnerTable)
		.Padding(2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(0.5f)
			[
				SNew(STextBlock).Text(FText::FromString(MaterialName))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.5f)
			[
				SNew(STextBlock).Text(FText::FromString(BakedName))
			]
		];
}

void FMaterialBakerModule::OnBakeQueueSelectionChanged(TSharedPtr<FMaterialBakeSettings> InItem, ESelectInfo::Type SelectInfo)
{
	if (InItem.IsValid())
	{
		SelectedQueueItem = InItem;
		CurrentBakeSettings = *InItem;

		if (ThumbnailBox.IsValid() && CurrentBakeSettings.Material)
		{
			FAssetData AssetData(CurrentBakeSettings.Material);
			TSharedPtr<FAssetThumbnail> Thumbnail = MakeShareable(new FAssetThumbnail(AssetData, 64, 64, ThumbnailPool));
			ThumbnailBox->SetContent(Thumbnail->MakeThumbnailWidget());
		}
		else if (ThumbnailBox.IsValid())
		{
			ThumbnailBox->SetContent(
				SNew(SBorder)
				.Padding(4.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("NoMaterialSelected", "No Material Selected"))
					.Justification(ETextJustify::Center)
				]
			);
		}
	}
}

FReply FMaterialBakerModule::OnRemoveSelectedClicked()
{
	if (SelectedQueueItem.IsValid())
	{
		BakeQueue.Remove(SelectedQueueItem);
		SelectedQueueItem.Reset();
		BakeQueueListView->RequestListRefresh();
	}
	return FReply::Handled();
}

FReply FMaterialBakerModule::OnUpdateSelectedClicked()
{
	if (SelectedQueueItem.IsValid())
	{
		*SelectedQueueItem = CurrentBakeSettings;
		BakeQueueListView->RequestListRefresh();
	}
	return FReply::Handled();
}


void FMaterialBakerModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(MaterialBakerTabName);
}

void FMaterialBakerModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);
	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
		{
			FToolMenuSection& Section = Menu->AddSection("MaterialBaker", TAttribute<FText>(LOCTEXT("MaterialBakerMenuHeading", "Material Baker")));
			Section.AddMenuEntryWithCommandList(FMaterialBakerCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMaterialBakerModule, MaterialBaker)