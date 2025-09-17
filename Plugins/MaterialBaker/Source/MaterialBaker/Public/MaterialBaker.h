// Copyright 2025 kurorekish. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Materials/MaterialInterface.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/Texture.h" // ETextureCompressionSettings を使用するためにインクルード
#include "MaterialBakerTypes.h"
#include "Widgets/Views/SListView.h"

class FToolBarBuilder;
class FMenuBuilder;
class SBox;
class FAssetThumbnailPool;
class ITableRow;
class STableViewBase;

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

	/** Handler for when the "Add to Queue" button is clicked */
	FReply OnAddToQueueClicked();

	/** Generates a row widget for the bake queue list view */
	TSharedRef<ITableRow> OnGenerateRowForBakeQueue(TSharedPtr<FMaterialBakeSettings> InItem, const TSharedRef<STableViewBase>& OwnerTable);

	/** Handler for when an item is selected in the bake queue */
	void OnBakeQueueSelectionChanged(TSharedPtr<FMaterialBakeSettings> InItem, ESelectInfo::Type SelectInfo);

	/** Handler for when the "Remove Selected" button is clicked */
	FReply OnRemoveSelectedClicked();

	/** Handler for when the "Update Selected" button is clicked */
	FReply OnUpdateSelectedClicked();

	void BakeMaterial(const FMaterialBakeSettings& BakeSettings);

private:
	TSharedPtr<class FUICommandList> PluginCommands;

	/** Thumbnail pool for the asset thumbnail */
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;

	/** Box to hold the thumbnail widget */
	TSharedPtr<SBox> ThumbnailBox;

	/** Options for compression setting dropdown */
	TArray<TSharedPtr<FString>> CompressionSettingOptions;

	/** Options for output type dropdown */
	TArray<TSharedPtr<FString>> OutputTypeOptions;

	/** Current bake settings */
	FMaterialBakeSettings CurrentBakeSettings;

	/** Bake queue */
	TArray<TSharedPtr<FMaterialBakeSettings>> BakeQueue;

	/** Bake queue list view widget */
	TSharedPtr<SListView<TSharedPtr<FMaterialBakeSettings>>> BakeQueueListView;

	/** Currently selected item in the bake queue */
	TSharedPtr<FMaterialBakeSettings> SelectedQueueItem;
};