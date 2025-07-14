// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Materials/MaterialInterface.h"
#include "AssetRegistry/AssetData.h"
// #include "Templates/SharedPointer.h" // ïsóvÇ»ÇΩÇﬂçÌèú

class FToolBarBuilder;
class FMenuBuilder;
class SBox;
class FAssetThumbnailPool;

// TSharedFromThis ÇÃåpè≥ÇçÌèúÇµÇ‹Ç∑
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

	/** Handler for when texture size is changed */
	void OnTextureSizeChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	/** Generates a widget for a texture size option */
	TSharedRef<SWidget> MakeWidgetForOption(TSharedPtr<FString> InOption);

private:
	TSharedPtr<class FUICommandList> PluginCommands;

	/** Material asset selected in the UI */
	class UMaterialInterface* SelectedMaterial = nullptr;

	/** Thumbnail pool for the asset thumbnail */
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;

	/** Box to hold the thumbnail widget */
	TSharedPtr<SBox> ThumbnailBox;

	/** Options for texture size dropdown */
	TArray<TSharedPtr<FString>> TextureSizeOptions;

	/** Currently selected texture size */
	TSharedPtr<FString> SelectedTextureSize;
};