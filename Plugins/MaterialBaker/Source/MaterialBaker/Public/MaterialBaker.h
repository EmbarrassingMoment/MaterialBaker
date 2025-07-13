// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Materials/MaterialInterface.h"
#include "AssetRegistry/AssetData.h" 

class FToolBarBuilder;
class FMenuBuilder;
class SBox; // SBox�̑O���錾��ǉ�
class FAssetThumbnailPool; // FAssetThumbnailPool�̑O���錾��ǉ�

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

	/** Thumbnail pool for the asset thumbnail */
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;

	/** Box to hold the thumbnail widget */
	TSharedPtr<SBox> ThumbnailBox;
};