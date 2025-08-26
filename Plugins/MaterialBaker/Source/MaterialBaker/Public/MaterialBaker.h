// Copyright 2025 kurorekish. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Materials/MaterialInterface.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/Texture.h" // ETextureCompressionSettings を使用するためにインクルード
#include "MaterialBakerTypes.h"

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

	/** Handler for when texture width is changed */
	void OnTextureWidthChanged(int32 NewValue);

	/** Handler for when texture height is changed */
	void OnTextureHeightChanged(int32 NewValue);

	/** Handler for when compression setting is changed */
	void OnCompressionSettingChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	/** Generates a widget for a compression setting option */
	TSharedRef<SWidget> MakeWidgetForCompressionOption(TSharedPtr<FString> InOption);

	/** Handler for when the bake button is clicked */
	FReply OnBakeButtonClicked();

	/** Handler for when material selection changes */
	void OnMaterialChanged(const FAssetData& AssetData);

	/** Handler for when baked name text changes */
	void OnBakedNameTextChanged(const FText& InText);

	/** Handler for when sRGB checkbox changes */
	void OnSRGBCheckBoxChanged(ECheckBoxState NewState);

	/** Handler for when output type is changed */
	void OnOutputTypeChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	/** Generates a widget for a output type option */
	TSharedRef<SWidget> MakeWidgetForOutputTypeOption(TSharedPtr<FString> InOption);

	/** Handler for when the browse button is clicked */
	FReply OnBrowseButtonClicked();

	/** Handler for when output path text changes */
	void OnOutputPathTextChanged(const FText& InText);


private:
	TSharedPtr<class FUICommandList> PluginCommands;

	/** Material asset selected in the UI */
	class UMaterialInterface* SelectedMaterial = nullptr;

	/** Thumbnail pool for the asset thumbnail */
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;

	/** Box to hold the thumbnail widget */
	TSharedPtr<SBox> ThumbnailBox;

	/** Texture width for baking */
	int32 TextureWidth = 1024;

	/** Texture height for baking */
	int32 TextureHeight = 1024;

	/** Options for compression setting dropdown */
	TArray<TSharedPtr<FString>> CompressionSettingOptions;

	/** Currently selected compression setting */
	TSharedPtr<FString> SelectedCompressionSetting;

	/** Custom name for the baked texture */
	FString CustomBakedName;

	/** Whether to enable sRGB for the baked texture */
	bool bSRGBEnabled = false;

	/** Options for output type dropdown */
	TArray<TSharedPtr<FString>> OutputTypeOptions;

	/** Currently selected output type */
	TSharedPtr<FString> SelectedOutputType;

	/** Output path for the baked texture */
	FString OutputPath;
};