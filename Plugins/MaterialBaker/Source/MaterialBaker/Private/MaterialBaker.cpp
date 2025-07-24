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
#include "PropertyCustomizationHelpers.h"

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

	// テクスチャサイズの選択肢を初期化
	TextureSizeOptions.Add(MakeShareable(new FString(TEXT("32x32"))));
	TextureSizeOptions.Add(MakeShareable(new FString(TEXT("64x64"))));
	TextureSizeOptions.Add(MakeShareable(new FString(TEXT("128x128"))));
	TextureSizeOptions.Add(MakeShareable(new FString(TEXT("256x256"))));
	TextureSizeOptions.Add(MakeShareable(new FString(TEXT("512x512"))));
	TextureSizeOptions.Add(MakeShareable(new FString(TEXT("1024x1024"))));
	TextureSizeOptions.Add(MakeShareable(new FString(TEXT("2048x2048"))));


	// デフォルトの選択値を設定 (32x32)
	SelectedTextureSize = TextureSizeOptions[0];

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
		.WidthOverride(128.f)
		.HeightOverride(128.f)
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

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			// SObjectPropertyEntryBox を使用してアセットピッカーを作成
			SNew(SObjectPropertyEntryBox)
				.AllowedClass(UMaterialInterface::StaticClass()) // 選択をマテリアルインターフェースに限定
				// ★★★ 修正点1: CreateRaw を使用 ★★★
				.OnObjectChanged(FOnSetObject::CreateRaw(this, &FMaterialBakerModule::OnMaterialChanged)) // アセット変更時のハンドラ
				.ObjectPath_Lambda([this]() -> FString { // 現在選択中のアセットをUIに表示
				return SelectedMaterial ? SelectedMaterial->GetPathName() : FString("");
					})
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			ThumbnailBox.ToSharedRef()
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
				// ★★★ 修正点2: CreateRaw を使用 ★★★
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
			SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&TextureSizeOptions)
				.OnSelectionChanged(SComboBox<TSharedPtr<FString>>::FOnSelectionChanged::CreateRaw(this, &FMaterialBakerModule::OnTextureSizeChanged))
				.OnGenerateWidget(SComboBox<TSharedPtr<FString>>::FOnGenerateWidget::CreateRaw(this, &FMaterialBakerModule::MakeWidgetForOption))
				.InitiallySelectedItem(SelectedTextureSize)
				[
					SNew(STextBlock)
						.Text_Lambda([this] { return FText::FromString(*SelectedTextureSize.Get()); })
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
		TSharedPtr<FAssetThumbnail> Thumbnail = MakeShareable(new FAssetThumbnail(AssetData, 128, 128, ThumbnailPool));
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


bool FMaterialBakerModule::ParseTextureSize(const FString& SizeString, FIntPoint& OutSize)
{
	TArray<FString> Parts;
	SizeString.ParseIntoArray(Parts, TEXT("x"), true);

	if (Parts.Num() == 2)
	{
		OutSize.X = FCString::Atoi(*Parts[0]);
		OutSize.Y = FCString::Atoi(*Parts[1]);
		return true;
	}

	return false;
}

FReply FMaterialBakerModule::OnBakeButtonClicked()
{
	if (!SelectedMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("No material selected!"));
		return FReply::Handled();
	}

	if (CustomBakedName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Baked texture name is empty!"));
		return FReply::Handled();
	}


	FIntPoint TextureSize;
	if (!ParseTextureSize(*SelectedTextureSize, TextureSize))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid texture size format!"));
		return FReply::Handled();
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot get world context!"));
		return FReply::Handled();
	}

	// 1. Render Targetの作成
	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
	RenderTarget->AddToRoot(); // GCに回収されないようにルートに追加

	RenderTarget->RenderTargetFormat = RTF_RGBA16f;
	RenderTarget->InitCustomFormat(TextureSize.X, TextureSize.Y, PF_FloatRGBA, true);

	RenderTarget->UpdateResourceImmediate(true);

	// 2. マテリアルをRender Targetに描画
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(World, RenderTarget, SelectedMaterial);

	FAssetData SelectedMaterialAssetData(SelectedMaterial);
	FString PackagePath = SelectedMaterialAssetData.PackagePath.ToString();
	FString AssetName = CustomBakedName; // カスタム名を使用

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	FString UniquePackageName;
	FString UniqueAssetName;
	AssetToolsModule.Get().CreateUniqueAssetName(PackagePath / AssetName, TEXT(""), UniquePackageName, UniqueAssetName);

	UPackage* Package = CreatePackage(*UniquePackageName);
	Package->FullyLoad();

	UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *UniqueAssetName, RF_Public | RF_Standalone | RF_MarkAsRootSet);
	NewTexture->AddToRoot();

	NewTexture->CompressionSettings = TC_Grayscale;
	NewTexture->SRGB = false;


	// Render Targetからピクセルデータを読み込み
	FRenderTarget* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
	TArray<FColor> RawPixels;
	if (RenderTargetResource->ReadPixels(RawPixels))
	{
		// ソースデータを設定してテクスチャを更新 (安全な方法)
		NewTexture->Source.Init(TextureSize.X, TextureSize.Y, 1, 1, TSF_BGRA8, (const uint8*)RawPixels.GetData());
		NewTexture->UpdateResource();

		Package->MarkPackageDirty();
		FAssetRegistryModule::GetRegistry().AssetCreated(NewTexture);
		NewTexture->PostEditChange();
	}

	NewTexture->RemoveFromRoot();
	RenderTarget->RemoveFromRoot();

	return FReply::Handled();
}


void FMaterialBakerModule::OnTextureSizeChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		SelectedTextureSize = NewSelection;
	}
}

TSharedRef<SWidget> FMaterialBakerModule::MakeWidgetForOption(TSharedPtr<FString> InOption)
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