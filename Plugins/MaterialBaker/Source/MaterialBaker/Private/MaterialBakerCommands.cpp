// Copyright 2025 Kurorekishi. All Rights Reserved.

#include "MaterialBakerCommands.h"

#define LOCTEXT_NAMESPACE "FMaterialBakerModule"

void FMaterialBakerCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "MaterialBaker", "Bring up MaterialBaker window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE