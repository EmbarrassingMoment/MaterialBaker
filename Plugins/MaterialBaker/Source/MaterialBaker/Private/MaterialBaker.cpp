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

	// 圧縮設定の選択肢を初期化
	const UEnum* CompressionSettingsEnum = StaticEnum<TextureCompressionSettings>();
	if (CompressionSettingsEnum)
	{
		for (int32 i = 0; i < CompressionSettingsEnum->NumEnums() - 1; ++i)
		{
			CompressionSettingOptions.Add(MakeShareable(new FString(CompressionSettingsEnum->GetDisplayNameTextByIndex(i).ToString())));
		}
	}
	// デフォルトの圧縮設定
	if (CompressionSettingOptions.Num() > 0)
	{
		SelectedCompressionSetting = CompressionSettingOptions[0];
	}


	// sRGBのデフォルト値を設定
	bSRGBEnabled = false;

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


	TSharedRef<SVerticalBox> WidgetContent = SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(STextBlock)
				.Text(LOCTEXT("SelectMaterialLabel", "Target Material"))
		]
		// サムネイルとマテリアル選択欄の底辺を揃える
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 5.0f, 0.0f)
				.VAlign(VAlign_Bottom) // 底辺を揃える
				[
					ThumbnailBox.ToSharedRef()
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Bottom) // 底辺を揃える
				[
					// SObjectPropertyEntryBox を使用してアセットピッカーを作成
					SNew(SObjectPropertyEntryBox)
						.AllowedClass(UMaterialInterface::StaticClass()) // 選択をマテリアルインターフェースに限定
						.OnObjectChanged(FOnSetObject::CreateRaw(this, &FMaterialBakerModule::OnMaterialChanged)) // アセット変更時のハンドラ
						.ObjectPath_Lambda([this]() -> FString { // 現在選択中のアセットをUIに表示
						return SelectedMaterial ? SelectedMaterial->GetPathName() : FString("");
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
						.Value(TextureWidth)
						.OnValueChanged(FOnInt32ValueChanged::CreateRaw(this, &FMaterialBakerModule::OnTextureWidthChanged))
						.MinValue(1)
						.MaxValue(8192)
				]
			+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SSpinBox<int32>)
						.Value(TextureHeight)
						.OnValueChanged(FOnInt32ValueChanged::CreateRaw(this, &FMaterialBakerModule::OnTextureHeightChanged))
						.MinValue(1)
						.MaxValue(8192)
				]
		]
	// 圧縮設定のドロップダウンリストを追加
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
				.InitiallySelectedItem(SelectedCompressionSetting)
				[
					SNew(STextBlock)
						.Text_Lambda([this] { return FText::FromString(*SelectedCompressionSetting.Get()); })
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
						.IsChecked_Lambda([this]() -> ECheckBoxState { return bSRGBEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
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
		.HAlign(HAlign_Right)
		.Padding(10.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("BakeButton", "Bake Material"))
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
	// アセットをUMaterialInterfaceにキャストして保持
	SelectedMaterial = Cast<UMaterialInterface>(AssetData.GetAsset());

	if (ThumbnailBox.IsValid() && SelectedMaterial)
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
	CustomBakedName = InText.ToString();
}

void FMaterialBakerModule::OnSRGBCheckBoxChanged(ECheckBoxState NewState)
{
	bSRGBEnabled = (NewState == ECheckBoxState::Checked);
}


FReply FMaterialBakerModule::OnBakeButtonClicked()
{
	if (!SelectedMaterial)
	{
		// マテリアルが選択されていない場合、ダイアログを表示
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoMaterialSelectedWarning", "Please select a material to bake."));
		return FReply::Handled();
	}

	if (CustomBakedName.IsEmpty())
	{
		// テクスチャ名が空の場合、ダイアログを表示
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("EmptyNameWarning", "Please enter a name for the baked texture."));
		return FReply::Handled();
	}


	FIntPoint TextureSize(TextureWidth, TextureHeight);

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot get world context!"));
		return FReply::Handled();
	}

	const int32 TotalSteps = 5; // 処理の総ステップ数
	FScopedSlowTask SlowTask(TotalSteps, LOCTEXT("BakingMaterial", "Baking Material..."));
	SlowTask.MakeDialog(); // プログレスバーのダイアログを表示

	// ステップ1: Render Targetの作成
	SlowTask.EnterProgressFrame(1, LOCTEXT("CreateRenderTarget", "Step 1/5: Creating Render Target..."));

	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
	RenderTarget->AddToRoot();

	RenderTarget->RenderTargetFormat = RTF_RGBA16f;
	// PF_FloatRGBAを指定し、ガンマはリニアに強制
	RenderTarget->InitCustomFormat(TextureSize.X, TextureSize.Y, PF_FloatRGBA, true);
	RenderTarget->UpdateResourceImmediate(true);

	// ステップ2: マテリアルを描画
	SlowTask.EnterProgressFrame(1, LOCTEXT("DrawMaterial", "Step 2/5: Drawing Material..."));
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(World, RenderTarget, SelectedMaterial);

	// レンダリングコマンドをフラッシュして、描画が完了するのを待つ
	FlushRenderingCommands();

	// ステップ3: アセットの作成準備
	SlowTask.EnterProgressFrame(1, LOCTEXT("PrepareAsset", "Step 3/5: Preparing Asset..."));
	FAssetData SelectedMaterialAssetData(SelectedMaterial);
	FString PackagePath = SelectedMaterialAssetData.PackagePath.ToString();
	FString AssetName = CustomBakedName;

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	FString UniquePackageName;
	FString UniqueAssetName;
	AssetToolsModule.Get().CreateUniqueAssetName(PackagePath / AssetName, TEXT(""), UniquePackageName, UniqueAssetName);

	UPackage* Package = CreatePackage(*UniquePackageName);
	Package->FullyLoad();

	UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *UniqueAssetName, RF_Public | RF_Standalone | RF_MarkAsRootSet);
	NewTexture->AddToRoot();

	// 選択された圧縮設定を適用
	const UEnum* CompressionSettingsEnum = StaticEnum<TextureCompressionSettings>();
	if (CompressionSettingsEnum)
	{
		int64 SelectedValue = INDEX_NONE;
		// ドロップダウンで選択された表示名を取得
		const FString SelectedDisplayName = *SelectedCompressionSetting.Get();

		// Enumの全要素をループして、表示名が一致するものを探す
		for (int32 i = 0; i < CompressionSettingsEnum->NumEnums() - 1; ++i)
		{
			if (SelectedDisplayName == CompressionSettingsEnum->GetDisplayNameTextByIndex(i).ToString())
			{
				// 表示名が一致したら、そのインデックスのEnum値を取得
				SelectedValue = CompressionSettingsEnum->GetValueByIndex(i);
				break;
			}
		}

		if (SelectedValue != INDEX_NONE)
		{
			NewTexture->CompressionSettings = static_cast<TextureCompressionSettings>(SelectedValue);
		}
	}

	// UIのチェックボックスに基づいてsRGBを設定 (リニアデータを保持する場合はfalseを推奨)
	NewTexture->SRGB = bSRGBEnabled;


	// ステップ4: ピクセルデータを読み込み
	SlowTask.EnterProgressFrame(1, LOCTEXT("ReadPixels", "Step 4/5: Reading Pixels..."));
	FRenderTarget* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();

	TArray<FFloat16Color> RawPixels;
	if (RenderTargetResource->ReadFloat16Pixels(RawPixels))
	{
		// ステップ5: テクスチャを更新して保存
		SlowTask.EnterProgressFrame(1, LOCTEXT("UpdateTexture", "Step 5/5: Updating and Saving Texture..."));

		NewTexture->Source.Init(TextureSize.X, TextureSize.Y, 1, 1, TSF_RGBA16F, (const uint8*)RawPixels.GetData());
		NewTexture->UpdateResource();

		Package->MarkPackageDirty();
		FAssetRegistryModule::GetRegistry().AssetCreated(NewTexture);
		NewTexture->PostEditChange();
	}

	NewTexture->RemoveFromRoot();
	RenderTarget->RemoveFromRoot();


	return FReply::Handled();
}


void FMaterialBakerModule::OnTextureWidthChanged(int32 NewValue)
{
	TextureWidth = NewValue;
}

void FMaterialBakerModule::OnTextureHeightChanged(int32 NewValue)
{
	TextureHeight = NewValue;
}

void FMaterialBakerModule::OnCompressionSettingChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		SelectedCompressionSetting = NewSelection;
	}
}

TSharedRef<SWidget> FMaterialBakerModule::MakeWidgetForCompressionOption(TSharedPtr<FString> InOption)
{
	return SNew(STextBlock).Text(FText::FromString(*InOption));
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