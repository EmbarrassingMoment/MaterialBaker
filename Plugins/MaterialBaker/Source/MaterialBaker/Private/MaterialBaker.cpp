// Copyright Epic Games, Inc. All Rights Reserved.

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
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "IAssetTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/TextureFactory.h"
#include "Misc/FileHelper.h"

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
		// ... (SMaterialDropTarget と ThumbnailBox の部分は変更なし) ...
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SMaterialDropTarget)
				.OnMaterialDropped_Lambda([this](UMaterialInterface* Material)
					{
						SelectedMaterial = Material;
						if (ThumbnailBox.IsValid() && SelectedMaterial)
						{
							FAssetData AssetData(SelectedMaterial);
							TSharedPtr<FAssetThumbnail> Thumbnail = MakeShareable(new FAssetThumbnail(AssetData, 128, 128, ThumbnailPool));
							ThumbnailBox->SetContent(Thumbnail->MakeThumbnailWidget());
						}
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
				.Text(LOCTEXT("TextureSizeLabel", "Bake Texture Size"))
		]

		// テクスチャサイズを選択するドロップダウン
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&TextureSizeOptions)
				// デリゲートの作成方法を CreateRaw に変更
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
		];

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			WidgetContent
		];
}

void FMaterialBakerModule::OnTextureSizeChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		SelectedTextureSize = NewSelection;
		// 選択が変更されたことをログに出力（デバッグ用）
		UE_LOG(LogTemp, Log, TEXT("Selected Texture Size: %s"), **NewSelection);
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