#include "SMaterialDropTarget.h"
#include "Widgets/Text/STextBlock.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "Materials/MaterialInterface.h"

void SMaterialDropTarget::Construct(const FArguments& InArgs)
{
	OnMaterialDropped = InArgs._OnMaterialDropped;

	ChildSlot
		[
			SNew(SBox)
				.WidthOverride(300.f)
				.HeightOverride(100.f)
				[
					SNew(SBorder)
						.Padding(4.f)
						[
							SNew(STextBlock)
								.Text(FText::FromString(TEXT("Drop Material Here")))
								.Justification(ETextJustify::Center)
						]
				]
		];
}

FReply SMaterialDropTarget::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FDragDropOperation> Operation = DragDropEvent.GetOperation();
	if (Operation.IsValid() && Operation->IsOfType<FAssetDragDropOp>())
	{
		TSharedPtr<FAssetDragDropOp> AssetOperation = StaticCastSharedPtr<FAssetDragDropOp>(Operation);
		if (AssetOperation->GetAssets().Num() > 0 && AssetOperation->GetAssets()[0].GetAsset()->IsA(UMaterialInterface::StaticClass()))
		{
			return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}

FReply SMaterialDropTarget::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FDragDropOperation> Operation = DragDropEvent.GetOperation();
	if (Operation.IsValid() && Operation->IsOfType<FAssetDragDropOp>())
	{
		TSharedPtr<FAssetDragDropOp> AssetOperation = StaticCastSharedPtr<FAssetDragDropOp>(Operation);
		if (AssetOperation->GetAssets().Num() > 0)
		{
			UMaterialInterface* Material = Cast<UMaterialInterface>(AssetOperation->GetAssets()[0].GetAsset());
			if (Material)
			{
				OnMaterialDropped.ExecuteIfBound(Material);
				return FReply::Handled();
			}
		}
	}
	return FReply::Unhandled();
}