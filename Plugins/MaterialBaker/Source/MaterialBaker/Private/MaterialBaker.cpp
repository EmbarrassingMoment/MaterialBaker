// Copyright 2025 kurorekish. All Rights Reserved.

#include "MaterialBaker.h"
#include "MaterialBakerStyle.h"
#include "MaterialBakerCommands.h"
#include "SMaterialBakerWidget.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "ToolMenus.h"

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
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MaterialBakerTabName);
}

TSharedRef<SDockTab> FMaterialBakerModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	TSharedRef<SDockTab> MyTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab);

	TSharedPtr<SWindow> Window = SpawnTabArgs.GetOwnerWindow();

	MyTab->SetContent(
		SNew(SMaterialBakerWidget, MyTab, Window)
	);

	return MyTab;
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