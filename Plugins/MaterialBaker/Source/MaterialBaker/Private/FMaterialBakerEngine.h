// Copyright 2025 EmbarrassingMoment. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MaterialBakerTypes.h"

namespace MaterialBakerEngineConstants
{
	const int32 TotalSteps = 5;
	const FVector DefaultCaptureActorLocation(0, 0, 100.0f);
	const FRotator DefaultCaptureActorRotation(-90.f, 0.f, -90.f);
	const float DefaultPlaneOrthoWidth = 200.0f;
}

class FMaterialBakerEngine
{
public:
	static bool BakeMaterial(const FMaterialBakeSettings& BakeSettings);
};
