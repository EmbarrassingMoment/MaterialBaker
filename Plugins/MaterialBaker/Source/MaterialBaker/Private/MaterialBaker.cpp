// Copyright Epic Games, Inc. All Rights Reserved.

#include "MaterialBaker.h"
#include "MaterialBakerStyle.h"
#include "MaterialBakerCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"

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
	// �����ŃE�B���h�E�̒��g�ƂȂ�E�B�W�F�b�g���쐬���܂��B
	// SVerticalBox�̓E�B�W�F�b�g���c�ɕ��ׂ邽�߂̃R���e�i�ł��B
	TSharedRef<SWidget> WidgetContent = SNew(SVerticalBox)

		// �}�e���A���I���̃��x��
		+ SVerticalBox::Slot()
		.AutoHeight() // �������R���e���c�ɍ��킹��
		.Padding(5.0f)   // ���͂ɗ]��
		[
			SNew(STextBlock)
				.Text(LOCTEXT("SelectMaterialLabel", "Target Material"))
		]

		// �}�e���A����I������A�Z�b�g�s�b�J�[�i��Ŏ����j
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			// ���̓v���[�X�z���_�[�Ƃ���SBox��z�u
			SNew(SBox)
				.MinDesiredWidth(300.f)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("MaterialAssetPicker", " Material Asset Picker will go here ")) // "Material Asset Picker will go here"
				]
		]

	// �e�N�X�`���T�C�Y�̃��x��
	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(STextBlock)
				.Text(LOCTEXT("TextureSizeLabel", "Bake Texture Size"))
		]

		// �e�N�X�`���T�C�Y��I������h���b�v�_�E���i��Ŏ����j
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			// ���̓v���[�X�z���_�[�Ƃ���SBox��z�u
			SNew(SBox)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("TextureSizeDropdown", " Texture Size Dropdown will go here ")) // "Texture Size Dropdown will go here"
				]
		]

	// �x�C�N���s�{�^��
	+ SVerticalBox::Slot()
		.HAlign(HAlign_Right) // �E��
		.Padding(10.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("BakeButton", "Bake Material"))
				/* .OnClicked( ... ) */ // �{�^���������ꂽ���̏�������Œǉ�
		];


	// �쐬�����E�B�W�F�b�g��DockTab�̃R���e���c�Ƃ��Đݒ�
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
            // "Tools"�Z�N�V������T���̂ł͂Ȃ��A"MaterialBaker"�Ƃ������O�ŐV�����Z�N�V������ǉ�����
            FToolMenuSection& Section = Menu->AddSection("MaterialBaker", TAttribute<FText>(LOCTEXT("MaterialBakerMenuHeading", "Material Baker")));
            Section.AddMenuEntryWithCommandList(FMaterialBakerCommands::Get().OpenPluginWindow, PluginCommands);
        }
    }
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FMaterialBakerModule, MaterialBaker)