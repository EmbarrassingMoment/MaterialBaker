// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Materials/MaterialInterface.h"

class FToolBarBuilder;
class FMenuBuilder;

class FMaterialBakerModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();
	
private:

	void RegisterMenus();

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

private:
       TSharedPtr<class FUICommandList> PluginCommands;

       /** Material asset selected in the UI */
       class UMaterialInterface* SelectedMaterial = nullptr;
};
