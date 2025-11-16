// Copyright 2025 kurorekish. All Rights Reserved.

#include "SMaterialBakerWidget.h"
#include "FMaterialBakerEngine.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "PropertyCustomizationHelpers.h"
#include "AssetRegistry/AssetData.h"
#include "AssetThumbnail.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Engine/Texture.h"
#include "UObject/EnumProperty.h"
#include "Misc/MessageDialog.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Misc/ScopedSlowTask.h"

#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FMaterialBakerModule"

void SMaterialBakerWidget::Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow)
{
	// Initialize data sources
	ThumbnailPool = MakeShareable(new FAssetThumbnailPool(MaterialBakerConstants::ThumbnailPoolSize));
	CurrentBakeSettings = FMaterialBakeSettings();

	PropertySuffixes.Add(EMaterialPropertyType::BaseColor, TEXT("_BC"));
	PropertySuffixes.Add(EMaterialPropertyType::Normal, TEXT("_N"));
	PropertySuffixes.Add(EMaterialPropertyType::Roughness, TEXT("_R"));
	PropertySuffixes.Add(EMaterialPropertyType::Metallic, TEXT("_M"));
	PropertySuffixes.Add(EMaterialPropertyType::EmissiveColor, TEXT("_E"));
	PropertySuffixes.Add(EMaterialPropertyType::Opacity, TEXT("_O"));

	// Initialize compression settings options
	const UEnum* CompressionSettingsEnum = StaticEnum<TextureCompressionSettings>();
	if (CompressionSettingsEnum)
	{
		for (int32 i = 0; i < CompressionSettingsEnum->NumEnums() - 1; ++i)
		{
			CompressionSettingOptions.Add(MakeShareable(new FString(CompressionSettingsEnum->GetDisplayNameTextByIndex(i).ToString())));
		}
	}

	// Initialize output type options
	const UEnum* OutputTypeEnum = StaticEnum<EMaterialBakeOutputType>();
	if (OutputTypeEnum)
	{
		for (int32 i = 0; i < OutputTypeEnum->NumEnums() - 1; ++i)
		{
			OutputTypeOptions.Add(MakeShareable(new FString(OutputTypeEnum->GetDisplayNameTextByIndex(i).ToString())));
		}
	}

	// Initialize bit depth options
	const UEnum* BitDepthEnum = StaticEnum<EMaterialBakeBitDepth>();
	if (BitDepthEnum)
	{
		for (int32 i = 0; i < BitDepthEnum->NumEnums() - 1; ++i)
		{
			BitDepthOptions.Add(MakeShareable(new FString(BitDepthEnum->GetDisplayNameTextByIndex(i).ToString())));
		}
	}

	// Initialize property type options
	const UEnum* PropertyTypeEnum = StaticEnum<EMaterialPropertyType>();
	if (PropertyTypeEnum)
	{
		for (int32 i = 0; i < PropertyTypeEnum->NumEnums() - 1; ++i)
		{
			PropertyTypeOptions.Add(MakeShareable(new FString(PropertyTypeEnum->GetDisplayNameTextByIndex(i).ToString())));
		}
	}


	// -- UI Layout --

	SAssignNew(ThumbnailBox, SBox)
		.WidthOverride(MaterialBakerConstants::ThumbnailSize)
		.HeightOverride(MaterialBakerConstants::ThumbnailSize)
		[
			SNew(SBorder)
			.Padding(4.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("NoMaterialSelected", "No Material Selected"))
				.Justification(ETextJustify::Center)
			]
		];

	SAssignNew(BakeQueueListView, SListView<TSharedPtr<FMaterialBakeSettings>>)
		.ListItemsSource(&BakeQueue)
		.OnGenerateRow(this, &SMaterialBakerWidget::OnGenerateRowForBakeQueue)
		.OnSelectionChanged(this, &SMaterialBakerWidget::OnBakeQueueSelectionChanged)
		.HeaderRow
		(
			SNew(SHeaderRow)
			+ SHeaderRow::Column("Material").DefaultLabel(LOCTEXT("MaterialColumn", "Material")).FillWidth(0.2f)
			+ SHeaderRow::Column("BakedName").DefaultLabel(LOCTEXT("BakedNameColumn", "Baked Name")).FillWidth(0.2f)
			+ SHeaderRow::Column("Property").DefaultLabel(LOCTEXT("PropertyColumn", "Property")).FillWidth(0.15f)
			+ SHeaderRow::Column("OutputType").DefaultLabel(LOCTEXT("OutputTypeColumn", "Output Type")).FillWidth(0.15f)
			+ SHeaderRow::Column("OutputPath").DefaultLabel(LOCTEXT("OutputPathColumn", "Output Path")).FillWidth(0.3f)
		);

	TabManager = FGlobalTabmanager::Get()->NewTabManager(ConstructUnderMajorTab);

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("MaterialBakerLayout")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->AddTab(MaterialBakerConstants::BakeSettingsTabId, ETabState::OpenedTab)
				->AddTab(MaterialBakerConstants::BakeQueueTabId, ETabState::OpenedTab)
				->SetForegroundTab(MaterialBakerConstants::BakeSettingsTabId)
			)
		);

	TabManager->RegisterTabSpawner(MaterialBakerConstants::BakeSettingsTabId, FOnSpawnTab::CreateRaw(this, &SMaterialBakerWidget::OnSpawnTab_BakeSettings));
	TabManager->RegisterTabSpawner(MaterialBakerConstants::BakeQueueTabId, FOnSpawnTab::CreateRaw(this, &SMaterialBakerWidget::OnSpawnTab_BakeQueue));

	ChildSlot
	[
		TabManager->RestoreFrom(Layout, ConstructUnderWindow).ToSharedRef()
	];
}

TSharedRef<SDockTab> SMaterialBakerWidget::OnSpawnTab_BakeSettings(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("PropertyTypeLabel", "Property to Bake"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&PropertyTypeOptions)
			.OnSelectionChanged(this, &SMaterialBakerWidget::OnPropertyTypeChanged)
			.OnGenerateWidget(this, &SMaterialBakerWidget::MakeWidgetForPropertyTypeOption)
			.InitiallySelectedItem(PropertyTypeOptions.Num() > 0 ? PropertyTypeOptions[0] : nullptr)
			[
				SNew(STextBlock)
				.Text_Lambda([this] {
					const UEnum* Enum = StaticEnum<EMaterialPropertyType>();
					if (Enum)
					{
						return Enum->GetDisplayNameTextByValue((int64)CurrentBakeSettings.PropertyType);
					}
					return FText::GetEmpty();
				})
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("BitDepthLabel", "Bit Depth"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&BitDepthOptions)
			.OnSelectionChanged(this, &SMaterialBakerWidget::OnBitDepthChanged)
			.OnGenerateWidget(this, &SMaterialBakerWidget::MakeWidgetForBitDepthOption)
			.InitiallySelectedItem(BitDepthOptions.Num() > 0 ? BitDepthOptions[1] : nullptr) // Default to 16-bit
			[
				SNew(STextBlock)
				.Text_Lambda([this] {
					const UEnum* Enum = StaticEnum<EMaterialBakeBitDepth>();
					if (Enum)
					{
						return Enum->GetDisplayNameTextByValue((int64)CurrentBakeSettings.BitDepth);
					}
					return FText::GetEmpty();
				})
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SelectMaterialLabel", "Target Material"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 5.0f, 0.0f)
			.VAlign(VAlign_Bottom)
			[
				ThumbnailBox.ToSharedRef()
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Bottom)
			[
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(UMaterialInterface::StaticClass())
				.OnObjectChanged(this, &SMaterialBakerWidget::OnMaterialChanged)
				.ObjectPath_Lambda([this]() -> FString {
					return CurrentBakeSettings.Material ? CurrentBakeSettings.Material->GetPathName() : FString("");
				})
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("BakedNameLabel", "Baked Texture Name"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SEditableTextBox)
				.HintText(LOCTEXT("BakedNameHint", "Enter baked texture name"))
				.Text_Lambda([this]() { return FText::FromString(CurrentBakeSettings.BakedName); })
				.OnTextChanged(this, &SMaterialBakerWidget::OnBakedNameTextChanged)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5.0f, 0.0f, 0.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this]() -> ECheckBoxState { return CurrentBakeSettings.bEnableAutomaticSuffix ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged(this, &SMaterialBakerWidget::OnEnableSuffixCheckBoxChanged)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(5.0f, 0.0f, 0.0f, 0.0f))
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("EnableSuffixLabel", "Auto Suffix"))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("TextureSizeLabel", "Bake Texture Size"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0.0f, 0.0f, 5.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("TextureWidthLabel", "Width"))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("TextureHeightLabel", "Height"))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0.0f, 0.0f, 5.0f, 0.0f)
			[
				SNew(SSpinBox<int32>)
				.Value_Lambda([this]() { return CurrentBakeSettings.TextureWidth; })
				.OnValueChanged(this, &SMaterialBakerWidget::OnTextureWidthChanged)
				.MinValue(1)
				.MaxValue(8192)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SSpinBox<int32>)
				.Value_Lambda([this]() { return CurrentBakeSettings.TextureHeight; })
				.OnValueChanged(this, &SMaterialBakerWidget::OnTextureHeightChanged)
				.MinValue(1)
				.MaxValue(8192)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("CompressionSettingLabel", "Compression Setting"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&CompressionSettingOptions)
			.OnSelectionChanged(this, &SMaterialBakerWidget::OnCompressionSettingChanged)
			.OnGenerateWidget(this, &SMaterialBakerWidget::MakeWidgetForCompressionOption)
			.InitiallySelectedItem(CompressionSettingOptions.Num() > 0 ? CompressionSettingOptions[0] : nullptr)
			[
				SNew(STextBlock)
				.Text_Lambda([this] {
					const UEnum* Enum = StaticEnum<TextureCompressionSettings>();
					if (Enum)
					{
						return Enum->GetDisplayNameTextByValue(CurrentBakeSettings.CompressionSettings);
					}
					return FText::GetEmpty();
				})
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this]() -> ECheckBoxState { return CurrentBakeSettings.bSRGB ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged(this, &SMaterialBakerWidget::OnSRGBCheckBoxChanged)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(5.0f, 0.0f, 0.0f, 0.0f))
			[
				SNew(STextBlock)
				.Text(LOCTEXT("sRGBLabel", "sRGB"))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("OutputTypeLabel", "Output Type"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&OutputTypeOptions)
			.OnSelectionChanged(this, &SMaterialBakerWidget::OnOutputTypeChanged)
			.OnGenerateWidget(this, &SMaterialBakerWidget::MakeWidgetForOutputTypeOption)
			.InitiallySelectedItem(OutputTypeOptions.Num() > 0 ? OutputTypeOptions[0] : nullptr)
			[
				SNew(STextBlock)
				.Text_Lambda([this] {
					const UEnum* Enum = StaticEnum<EMaterialBakeOutputType>();
					if (Enum)
					{
						return Enum->GetDisplayNameTextByValue((int64)CurrentBakeSettings.OutputType);
					}
					return FText::GetEmpty();
				})
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("OutputPathLabel", "Output Path"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SEditableTextBox)
				.Text_Lambda([this] { return FText::FromString(CurrentBakeSettings.OutputPath); })
				.OnTextChanged(this, &SMaterialBakerWidget::OnOutputPathTextChanged)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("BrowseButton", "Browse..."))
				.OnClicked(this, &SMaterialBakerWidget::OnBrowseButtonClicked)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("AddToQueueButton", "Add to Queue"))
			.OnClicked(this, &SMaterialBakerWidget::OnAddToQueueClicked)
		]
	];
}


TSharedRef<SDockTab> SMaterialBakerWidget::OnSpawnTab_BakeQueue(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(5.0f)
			[
				BakeQueueListView.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.Padding(2.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("UpdateSelectedButton", "Update Selected"))
					.OnClicked(this, &SMaterialBakerWidget::OnUpdateSelectedClicked)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.Padding(2.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("RemoveSelectedButton", "Remove Selected"))
					.OnClicked(this, &SMaterialBakerWidget::OnRemoveSelectedClicked)
				]
			]
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Right)
			.AutoHeight()
			.Padding(10.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("BakeButton", "Bake All"))
				.OnClicked(this, &SMaterialBakerWidget::OnBakeButtonClicked)
			]
		];
}


SMaterialBakerWidget::~SMaterialBakerWidget()
{
	if (TabManager.IsValid())
	{
		TabManager->UnregisterTabSpawner(MaterialBakerConstants::BakeSettingsTabId);
		TabManager->UnregisterTabSpawner(MaterialBakerConstants::BakeQueueTabId);
	}
	ThumbnailPool.Reset();
}

void SMaterialBakerWidget::OnMaterialChanged(const FAssetData& AssetData)
{
	CurrentBakeSettings.Material = Cast<UMaterialInterface>(AssetData.GetAsset());

	if (CurrentBakeSettings.Material)
	{
		CurrentBakeSettings.OutputPath = FPackageName::GetLongPackagePath(CurrentBakeSettings.Material->GetPathName());

		FString MaterialName = CurrentBakeSettings.Material->GetName();
		if (MaterialName.StartsWith(TEXT("M_")))
		{
			CurrentBakeSettings.BakedName = TEXT("T_") + MaterialName.RightChop(2);
		}
		else if (MaterialName.StartsWith(TEXT("MI_")))
		{
			CurrentBakeSettings.BakedName = TEXT("T_") + MaterialName.RightChop(3);
		}
		else
		{
			CurrentBakeSettings.BakedName = TEXT("T_") + MaterialName;
		}

		UpdateBakedNameWithSuffix();
	}

	if (ThumbnailBox.IsValid() && CurrentBakeSettings.Material)
	{
		TSharedPtr<FAssetThumbnail> Thumbnail = MakeShareable(new FAssetThumbnail(AssetData, MaterialBakerConstants::ThumbnailSize, MaterialBakerConstants::ThumbnailSize, ThumbnailPool));
		ThumbnailBox->SetContent(Thumbnail->MakeThumbnailWidget());
	}
	else if (ThumbnailBox.IsValid())
	{
		ThumbnailBox->SetContent(
			SNew(SBorder)
			.Padding(4.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("NoMaterialSelected", "No Material Selected"))
				.Justification(ETextJustify::Center)
			]
		);
	}
}

void SMaterialBakerWidget::OnBakedNameTextChanged(const FText& InText)
{
	CurrentBakeSettings.BakedName = InText.ToString();
}

void SMaterialBakerWidget::OnEnableSuffixCheckBoxChanged(ECheckBoxState NewState)
{
	CurrentBakeSettings.bEnableAutomaticSuffix = (NewState == ECheckBoxState::Checked);
	UpdateBakedNameWithSuffix();
}

void SMaterialBakerWidget::OnTextureWidthChanged(int32 NewValue)
{
	CurrentBakeSettings.TextureWidth = NewValue;
}

void SMaterialBakerWidget::OnTextureHeightChanged(int32 NewValue)
{
	CurrentBakeSettings.TextureHeight = NewValue;
}

void SMaterialBakerWidget::OnCompressionSettingChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		const UEnum* Enum = StaticEnum<TextureCompressionSettings>();
		if (Enum)
		{
			for (int32 i = 0; i < Enum->NumEnums() - 1; ++i)
			{
				if (*NewSelection == Enum->GetDisplayNameTextByIndex(i).ToString())
				{
					CurrentBakeSettings.CompressionSettings = static_cast<TextureCompressionSettings>(Enum->GetValueByIndex(i));
					break;
				}
			}
		}
	}
}

TSharedRef<SWidget> SMaterialBakerWidget::MakeWidgetForCompressionOption(TSharedPtr<FString> InOption)
{
	return SNew(STextBlock).Text(FText::FromString(*InOption));
}

void SMaterialBakerWidget::OnSRGBCheckBoxChanged(ECheckBoxState NewState)
{
	CurrentBakeSettings.bSRGB = (NewState == ECheckBoxState::Checked);
}

void SMaterialBakerWidget::OnOutputTypeChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		const UEnum* Enum = StaticEnum<EMaterialBakeOutputType>();
		if (Enum)
		{
			for (int32 i = 0; i < Enum->NumEnums(); ++i)
			{
				if (*NewSelection == Enum->GetDisplayNameTextByIndex(i).ToString())
				{
					CurrentBakeSettings.OutputType = static_cast<EMaterialBakeOutputType>(Enum->GetValueByIndex(i));
					break;
				}
			}
		}
	}
}

TSharedRef<SWidget> SMaterialBakerWidget::MakeWidgetForOutputTypeOption(TSharedPtr<FString> InOption)
{
	return SNew(STextBlock).Text(FText::FromString(*InOption));
}

void SMaterialBakerWidget::OnBitDepthChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		const UEnum* Enum = StaticEnum<EMaterialBakeBitDepth>();
		if (Enum)
		{
			for (int32 i = 0; i < Enum->NumEnums(); ++i)
			{
				if (*NewSelection == Enum->GetDisplayNameTextByIndex(i).ToString())
				{
					CurrentBakeSettings.BitDepth = static_cast<EMaterialBakeBitDepth>(Enum->GetValueByIndex(i));
					break;
				}
			}
		}
	}
}

TSharedRef<SWidget> SMaterialBakerWidget::MakeWidgetForBitDepthOption(TSharedPtr<FString> InOption)
{
	return SNew(STextBlock).Text(FText::FromString(*InOption));
}

void SMaterialBakerWidget::OnPropertyTypeChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection.IsValid())
	{
		const UEnum* Enum = StaticEnum<EMaterialPropertyType>();
		if (Enum)
		{
			for (int32 i = 0; i < Enum->NumEnums(); ++i)
			{
				if (*NewSelection == Enum->GetDisplayNameTextByIndex(i).ToString())
				{
					CurrentBakeSettings.PropertyType = static_cast<EMaterialPropertyType>(Enum->GetValueByIndex(i));
					UpdateBakedNameWithSuffix();
					break;
				}
			}
		}
	}
}

TSharedRef<SWidget> SMaterialBakerWidget::MakeWidgetForPropertyTypeOption(TSharedPtr<FString> InOption)
{
	return SNew(STextBlock).Text(FText::FromString(*InOption));
}

void SMaterialBakerWidget::OnOutputPathTextChanged(const FText& InText)
{
	CurrentBakeSettings.OutputPath = InText.ToString();
}

FReply SMaterialBakerWidget::OnBrowseButtonClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		FString FolderName;
		const FString Title = TEXT("Select Output Folder");
		const FString DefaultPath = FPaths::ProjectContentDir();
		if (DesktopPlatform->OpenDirectoryDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			Title,
			DefaultPath,
			FolderName
		))
		{
			if (FPaths::MakePathRelativeTo(FolderName, *FPaths::ProjectContentDir()))
			{
				CurrentBakeSettings.OutputPath = FString("/Game/") + FolderName;
			}
			else
			{
				CurrentBakeSettings.OutputPath = FolderName;
			}
		}
	}
	return FReply::Handled();
}

FReply SMaterialBakerWidget::OnAddToQueueClicked()
{
	if (!CurrentBakeSettings.Material)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoMaterialForQueue", "Please select a material first."));
		return FReply::Handled();
	}
	if (CurrentBakeSettings.BakedName.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoNameForQueue", "Please enter a name for the baked texture."));
		return FReply::Handled();
	}

	BakeQueue.Add(MakeShared<FMaterialBakeSettings>(CurrentBakeSettings));
	if (BakeQueueListView.IsValid())
	{
		BakeQueueListView->RequestListRefresh();
	}

	return FReply::Handled();
}

FReply SMaterialBakerWidget::OnUpdateSelectedClicked()
{
	if (SelectedQueueItem.IsValid())
	{
		*SelectedQueueItem = CurrentBakeSettings;
		BakeQueueListView->RequestListRefresh();
	}
	return FReply::Handled();
}

FReply SMaterialBakerWidget::OnRemoveSelectedClicked()
{
	if (SelectedQueueItem.IsValid())
	{
		BakeQueue.Remove(SelectedQueueItem);
		SelectedQueueItem.Reset();
		BakeQueueListView->RequestListRefresh();
	}
	return FReply::Handled();
}

FReply SMaterialBakerWidget::OnBakeButtonClicked()
{
	if (BakeQueue.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("EmptyQueueWarning", "Please add at least one material to the bake queue."));
		return FReply::Handled();
	}

	// --- Validation ---
	TSet<FString> UniqueNames;
	for (const auto& Settings : BakeQueue)
	{
		if (!Settings->Material)
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("InvalidMaterialInQueue", "An item in the queue has no material selected."), FText::FromString(Settings->BakedName)));
			return FReply::Handled();
		}
		if (Settings->BakedName.IsEmpty())
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("EmptyNameInQueue", "An item in the queue has no name."));
			return FReply::Handled();
		}

		FString FullPath = FPaths::Combine(Settings->OutputPath, Settings->BakedName);
		if (UniqueNames.Contains(FullPath))
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("DuplicateNameInQueue", "Duplicate output name and path found in queue: {0}"), FText::FromString(FullPath)));
			return FReply::Handled();
		}
		UniqueNames.Add(FullPath);
	}

	FScopedSlowTask SlowTask(BakeQueue.Num(), LOCTEXT("BakingMaterials", "Baking Materials..."));
	SlowTask.MakeDialog();

	bool bAllSucceeded = true;
	for (const auto& Settings : BakeQueue)
	{
		FText ProgressText = FText::Format(LOCTEXT("BakingMaterialItem", "Baking {0} ({1}/{2})"), FText::FromString(Settings->BakedName), FText::AsNumber(SlowTask.CompletedWork + 1), FText::AsNumber(BakeQueue.Num()));
		SlowTask.EnterProgressFrame(1, ProgressText);

		if (SlowTask.ShouldCancel())
		{
			bAllSucceeded = false;
			break;
		}

		if (!FMaterialBakerEngine::BakeMaterial(*Settings))
		{
			// Even if one fails, continue with the rest unless cancelled.
			// You might want to collect failures and report them all at the end.
			bAllSucceeded = false;
		}
	}

	if (bAllSucceeded)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("BakeComplete", "Batch bake completed successfully."));
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("BakeCancelled", "Batch bake was cancelled or had failures."));
	}

	BakeQueue.Empty();
	BakeQueueListView->RequestListRefresh();

	return FReply::Handled();
}

TSharedRef<ITableRow> SMaterialBakerWidget::OnGenerateRowForBakeQueue(TSharedPtr<FMaterialBakeSettings> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	FString MaterialName = InItem->Material ? InItem->Material->GetName() : TEXT("None");
	FString BakedName = InItem->BakedName;
	FString OutputPath = InItem->OutputPath;

	FText PropertyTypeText = FText::GetEmpty();
	const UEnum* PropertyEnum = StaticEnum<EMaterialPropertyType>();
	if (PropertyEnum)
	{
		PropertyTypeText = PropertyEnum->GetDisplayNameTextByValue((int64)InItem->PropertyType);
	}

	FText OutputTypeText = FText::GetEmpty();
	const UEnum* OutputTypeEnum = StaticEnum<EMaterialBakeOutputType>();
	if (OutputTypeEnum)
	{
		OutputTypeText = OutputTypeEnum->GetDisplayNameTextByValue((int64)InItem->OutputType);
	}


	return SNew(STableRow<TSharedPtr<FMaterialBakeSettings>>, OwnerTable)
		.Padding(2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(0.2f)
			[
				SNew(STextBlock).Text(FText::FromString(MaterialName))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.2f)
			[
				SNew(STextBlock).Text(FText::FromString(BakedName))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.15f)
			[
				SNew(STextBlock).Text(PropertyTypeText)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.15f)
			[
				SNew(STextBlock).Text(OutputTypeText)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.3f)
			[
				SNew(STextBlock).Text(FText::FromString(OutputPath))
			]
		];
}

void SMaterialBakerWidget::OnBakeQueueSelectionChanged(TSharedPtr<FMaterialBakeSettings> InItem, ESelectInfo::Type SelectInfo)
{
	if (InItem.IsValid())
	{
		SelectedQueueItem = InItem;
		CurrentBakeSettings = *InItem;

		if (ThumbnailBox.IsValid() && CurrentBakeSettings.Material)
		{
			FAssetData AssetData(CurrentBakeSettings.Material);
			TSharedPtr<FAssetThumbnail> Thumbnail = MakeShareable(new FAssetThumbnail(AssetData, MaterialBakerConstants::ThumbnailSize, MaterialBakerConstants::ThumbnailSize, ThumbnailPool));
			ThumbnailBox->SetContent(Thumbnail->MakeThumbnailWidget());
		}
		else if (ThumbnailBox.IsValid())
		{
			ThumbnailBox->SetContent(
				SNew(SBorder)
				.Padding(4.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("NoMaterialSelected", "No Material Selected"))
					.Justification(ETextJustify::Center)
				]
			);
		}
	}
}

void SMaterialBakerWidget::UpdateBakedNameWithSuffix()
{
	FString BaseName = CurrentBakeSettings.BakedName;

	// First, remove any existing known suffix
	for (const auto& Pair : PropertySuffixes)
	{
		if (BaseName.EndsWith(Pair.Value))
		{
			BaseName.LeftChopInline(Pair.Value.Len());
			break; // Assume only one suffix exists
		}
	}

	// Now, add the correct suffix for the current property type if enabled
	if (CurrentBakeSettings.bEnableAutomaticSuffix)
	{
		if (const FString* Suffix = PropertySuffixes.Find(CurrentBakeSettings.PropertyType))
		{
			CurrentBakeSettings.BakedName = BaseName + *Suffix;
		}
		else
		{
			// If no suffix is defined for the property (e.g., Final Color), just use the base name
			CurrentBakeSettings.BakedName = BaseName;
		}
	}
	else
	{
		CurrentBakeSettings.BakedName = BaseName;
	}
}

#undef LOCTEXT_NAMESPACE
