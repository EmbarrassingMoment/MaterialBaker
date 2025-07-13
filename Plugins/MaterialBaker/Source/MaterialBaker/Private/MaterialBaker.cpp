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
    // 1. STextBlockウィジェットへのポインタを先に宣言
    TSharedPtr<STextBlock> SelectedMaterialNameWidget;

    // 2. ラムダ式で使うSTextBlockを先にSAssignNewで生成
    SAssignNew(SelectedMaterialNameWidget, STextBlock)
        .Text_Lambda([this]() -> FText {
        // SelectedMaterial の状態に応じて表示テキストを切り替え
        if (SelectedMaterial)
        {
            return FText::FromString(SelectedMaterial->GetName());
        }
        return LOCTEXT("NoMaterialSelected", "No Material Selected");
            });

    TSharedRef<SVerticalBox> WidgetContent = SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5.0f)
        [
            SNew(SMaterialDropTarget)
                // 3. ラムダ式で TSharedPtr を値でキャプチャする
                .OnMaterialDropped_Lambda([this, SelectedMaterialNameWidget](UMaterialInterface* Material)
                    {
                        // ドロップされたマテリアルをメンバ変数に保持
                        SelectedMaterial = Material;
                        // TextBlockの表示を更新
                        if (SelectedMaterialNameWidget.IsValid() && SelectedMaterial)
                        {
                            SelectedMaterialNameWidget->SetText(FText::FromString(SelectedMaterial->GetName()));
                        }
                    })
        ]

    // 選択されたマテリアル名を表示するウィジェット
    + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5.0f)
        [
            // 4. 先に生成したウィジェットをレイアウトに追加
            SelectedMaterialNameWidget.ToSharedRef()
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