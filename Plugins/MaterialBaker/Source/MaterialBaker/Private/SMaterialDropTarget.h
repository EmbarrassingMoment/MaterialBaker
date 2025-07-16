// Copyright 2025 kurorekish. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Materials/MaterialInterface.h"

DECLARE_DELEGATE_OneParam(FOnMaterialDropped, UMaterialInterface*);

class SMaterialDropTarget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMaterialDropTarget) {}
		SLATE_EVENT(FOnMaterialDropped, OnMaterialDropped)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

private:
	FOnMaterialDropped OnMaterialDropped;
};