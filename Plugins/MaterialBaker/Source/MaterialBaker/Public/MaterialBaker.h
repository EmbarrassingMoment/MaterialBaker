// Copyright 2025 kurorekish. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Materials/MaterialInterface.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/Texture.h" // ETextureCompressionSettings を使用するためにインクルード

class FToolBarBuilder;
class FMenuBuilder;
class SBox;
class FAssetThumbnailPool;

class FMaterialBakerModule : public IModuleInterface, public TSharedFromThis<FMaterialBakerModule>
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

	/** Handler for when compression setting is changed */
	void OnCompressionSettingChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	/** Generates a widget for a compression setting option */
	TSharedRef<SWidget> MakeWidgetForCompressionOption(TSharedPtr<FString> InOption);

	/** Handler for when the bake button is clicked */
	FReply OnBakeButtonClicked();

	/** Parses the texture size string (e.g., "1024x1024") into FIntPoint */
	bool ParseTextureSize(const FString& SizeString, FIntPoint& OutSize);

	/** Handler for when material selection changes */
	void OnMaterialChanged(const FAssetData& AssetData);

	/** Handler for when baked name text changes */
	void OnBakedNameTextChanged(const FText& InText);

	/** Handler for when sRGB checkbox changes */
	void OnSRGBCheckBoxChanged(ECheckBoxState NewState);


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

	/** Options for compression setting dropdown */
	TArray<TSharedPtr<FString>> CompressionSettingOptions;

	/** Currently selected compression setting */
	TSharedPtr<FString> SelectedCompressionSetting;

	/** Custom name for the baked texture */
	FString CustomBakedName;

	/** Whether to enable sRGB for the baked texture */
	bool bSRGBEnabled = false;
};