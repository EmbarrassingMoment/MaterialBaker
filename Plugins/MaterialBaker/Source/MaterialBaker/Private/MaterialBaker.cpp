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


static const FName MaterialBakerTabName("MaterialBaker");

#define LOCTEXT_NAMESPACE "FMaterialBakerModule"

void FMaterialBakerModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FMaterialBakerStyle::Initialize();
	FMaterialBakerStyle::ReloadTextures();

	FMaterialBakerCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FMaterialBakerCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FMaterialBakerModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMaterialBakerModule::RegisterMenus));

	// サムネイルプールを初期化
	ThumbnailPool = MakeShareable(new FAssetThumbnailPool(10));
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(MaterialBakerTabName, FOnSpawnTab::CreateRaw(this, &FMaterialBakerModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FMaterialBakerTabTitle", "MaterialBaker"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

// (ShutdownModule と OnSpawnPluginTab 以降のコードは変更ありません)
void FMaterialBakerModule::ShutdownModule()
{
    // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
    // we call this function before unloading the module.

    UToolMenus::UnRegisterStartupCallback(this);

    UToolMenus::UnregisterOwner(this);

    FMaterialBakerStyle::Shutdown();

    FMaterialBakerCommands::Unregister();

    // サムネイルプールを解放
    ThumbnailPool.Reset();

    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MaterialBakerTabName);

}

TSharedRef<SDockTab> FMaterialBakerModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	// ThumbnailBoxをSAssignNewでメンバ変数に保持
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
			SNew(SMaterialDropTarget)
				.OnMaterialDropped_Lambda([this](UMaterialInterface* Material)
					{
						// ドロップされたマテリアルをメンバ変数に保持
						SelectedMaterial = Material;

						if (ThumbnailBox.IsValid() && SelectedMaterial)
						{
							// アセットデータからサムネイルを生成
							FAssetData AssetData(SelectedMaterial);
							TSharedPtr<FAssetThumbnail> Thumbnail = MakeShareable(new FAssetThumbnail(AssetData, 128, 128, ThumbnailPool));

							// ThumbnailBoxの中身をサムネイルウィジェットで更新
							ThumbnailBox->SetContent(Thumbnail->MakeThumbnailWidget());
						}
					})
		]

	// 選択されたマテリアルのサムネイルを表示するウィジェット
	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			// 先に生成したThumbnailBoxをレイアウトに追加
			ThumbnailBox.ToSharedRef()
		]

		// テクスチャサイズのラベル
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(STextBlock)
				.Text(LOCTEXT("TextureSizeLabel", "Bake Texture Size"))
		]

		// テクスチャサイズを選択するドロップダウン（後で実装）
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SBox)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("TextureSizeDropdown", " Texture Size Dropdown will go here "))
				]
		]

	// ベイク実行ボタン
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


void FMaterialBakerModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(MaterialBakerTabName);
}

void FMaterialBakerModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
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