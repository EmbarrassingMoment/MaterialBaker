// Copyright 2025 EmbarrassingMoment. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MaterialBakerTypes.h"

class UTextureRenderTarget2D;
struct FScopedSlowTask;

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

private:
	struct FMaterialBakerContext
	{
		UWorld* World = nullptr;
		const FMaterialBakeSettings& Settings;
		FScopedSlowTask* SlowTask = nullptr;

		UTextureRenderTarget2D* RenderTarget = nullptr;
		TArray<uint8> RawPixels; // Contains FColor or FFloat16Color data
		FIntPoint TextureSize;
		bool bIsHdr = false;
		bool bSRGB = false;

		FMaterialBakerContext(UWorld* InWorld, const FMaterialBakeSettings& InSettings, FScopedSlowTask* InSlowTask)
			: World(InWorld)
			, Settings(InSettings)
			, SlowTask(InSlowTask)
			, TextureSize(InSettings.TextureWidth, InSettings.TextureHeight)
		{}
	};

	static bool SetupRenderTarget(FMaterialBakerContext& Context);
	static bool CaptureMaterial(FMaterialBakerContext& Context);
	static bool ReadPixels(FMaterialBakerContext& Context);
	static bool CreateTextureAsset(FMaterialBakerContext& Context);
	static bool ExportImageFile(FMaterialBakerContext& Context);
};
