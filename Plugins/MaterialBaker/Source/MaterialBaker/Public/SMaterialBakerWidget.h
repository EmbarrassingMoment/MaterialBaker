// Copyright 2025 kurorekish. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "MaterialBakerTypes.h"
#include "Widgets/Views/SListView.h"

class FAssetThumbnailPool;
struct FAssetData;
struct FMaterialBakeSettings;


class SMaterialBakerWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMaterialBakerWidget) {}
	SLATE_END_ARGS()

	/** Constructs this widget */
	void Construct(const FArguments& InArgs);

	~SMaterialBakerWidget();

private:
	// -- UI Event Handlers --

	void OnMaterialChanged(const FAssetData& AssetData);
	void OnBakedNameTextChanged(const FText& InText);
	void OnTextureWidthChanged(int32 NewValue);
	void OnTextureHeightChanged(int32 NewValue);
	void OnCompressionSettingChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	TSharedRef<SWidget> MakeWidgetForCompressionOption(TSharedPtr<FString> InOption);
	void OnSRGBCheckBoxChanged(ECheckBoxState NewState);
	void OnOutputTypeChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	TSharedRef<SWidget> MakeWidgetForOutputTypeOption(TSharedPtr<FString> InOption);
	void OnOutputPathTextChanged(const FText& InText);
	FReply OnBrowseButtonClicked();
	FReply OnAddToQueueClicked();
	FReply OnUpdateSelectedClicked();
	FReply OnRemoveSelectedClicked();
	FReply OnBakeButtonClicked();

	// -- Bake Queue ListView Handlers --
	TSharedRef<ITableRow> OnGenerateRowForBakeQueue(TSharedPtr<FMaterialBakeSettings> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void OnBakeQueueSelectionChanged(TSharedPtr<FMaterialBakeSettings> InItem, ESelectInfo::Type SelectInfo);

private:
	// -- UI Data and State --
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;

	FMaterialBakeSettings CurrentBakeSettings;
	TArray<TSharedPtr<FString>> CompressionSettingOptions;
	TArray<TSharedPtr<FString>> OutputTypeOptions;

	TArray<TSharedPtr<FMaterialBakeSettings>> BakeQueue;
	TSharedPtr<FMaterialBakeSettings> SelectedQueueItem;

	// -- UI Widgets --
	TSharedPtr<SBox> ThumbnailBox;
	TSharedPtr<SListView<TSharedPtr<FMaterialBakeSettings>>> BakeQueueListView;
};
