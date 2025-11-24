// Copyright 2025 Kurorekishi. All Rights Reserved.

#pragma once

#include "Framework/Commands/Commands.h"
#include "MaterialBakerStyle.h"

class FMaterialBakerCommands : public TCommands<FMaterialBakerCommands>
{
public:

	FMaterialBakerCommands()
		: TCommands<FMaterialBakerCommands>(TEXT("MaterialBaker"), NSLOCTEXT("Contexts", "MaterialBaker", "MaterialBaker Plugin"), NAME_None, FMaterialBakerStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};