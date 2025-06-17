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

#include "AssetRegistry/AssetData.h"
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
    
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(MaterialBakerTabName, FOnSpawnTab::CreateRaw(this, &FMaterialBakerModule::OnSpawnPluginTab))
        .SetDisplayName(LOCTEXT("FMaterialBakerTabTitle", "MaterialBaker"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FMaterialBakerModule::ShutdownModule()
{
    // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
    // we call this function before unloading the module.

    UToolMenus::UnRegisterStartupCallback(this);

    UToolMenus::UnregisterOwner(this);

    FMaterialBakerStyle::Shutdown();

    FMaterialBakerCommands::Unregister();

    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MaterialBakerTabName);
}

TSharedRef<SDockTab> FMaterialBakerModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(5.0f)
                [
                    SNew(SObjectPropertyEntryBox)
                        .AllowedClass(UMaterialInterface::StaticClass())
                        .ObjectPath_Lambda([this]() -> FString { return SelectedMaterial ? SelectedMaterial->GetPathName() : FString(); })
                        .OnObjectChanged_Lambda([this](const FAssetData& AssetData) { SelectedMaterial = Cast<UMaterialInterface>(AssetData.GetAsset()); })
                        .MinDesiredWidth(300.f)
                ]

		// マテリアルを選択するアセットピッカー（後で実装）
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			// 今はプレースホルダーとしてSBoxを配置
			SNew(SBox)
				.MinDesiredWidth(300.f)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("MaterialAssetPicker", " Material Asset Picker will go here ")) // "Material Asset Picker will go here"
				]
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
			// 今はプレースホルダーとしてSBoxを配置
			SNew(SBox)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("TextureSizeDropdown", " Texture Size Dropdown will go here ")) // "Texture Size Dropdown will go here"
				]
		]

	// ベイク実行ボタン
	+ SVerticalBox::Slot()
		.HAlign(HAlign_Right) // 右寄せ
		.Padding(10.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("BakeButton", "Bake Material"))
				/* .OnClicked( ... ) */ // ボタンが押された時の処理を後で追加
		];


	// 作成したウィジェットをDockTabのコンテンツとして設定
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
            // "Tools"セクションを探すのではなく、"MaterialBaker"という名前で新しいセクションを追加する
            FToolMenuSection& Section = Menu->AddSection("MaterialBaker", TAttribute<FText>(LOCTEXT("MaterialBakerMenuHeading", "Material Baker")));
            Section.AddMenuEntryWithCommandList(FMaterialBakerCommands::Get().OpenPluginWindow, PluginCommands);
        }
    }
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FMaterialBakerModule, MaterialBaker)