// Copyright 2025 kurorekish. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MaterialBakerTypes.h"

namespace MaterialBakerEngineConstants
{
	const int32 TotalSteps = 5;
}

class FMaterialBakerEngine
{
public:
	static bool BakeMaterial(const FMaterialBakeSettings& BakeSettings);
};
