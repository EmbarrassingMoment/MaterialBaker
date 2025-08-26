// Copyright 2025 kurorekish. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture.h"
#include "MaterialBakerTypes.generated.h"

UENUM(BlueprintType)
enum class EMaterialBakeOutputType : uint8
{
	Texture UMETA(DisplayName = "Texture Asset"),
	PNG UMETA(DisplayName = "PNG"),
	JPEG UMETA(DisplayName = "JPEG"),
};

USTRUCT(BlueprintType)
struct FMaterialBakeSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Baker")
	TObjectPtr<class UMaterialInterface> Material = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Baker")
	FString BakedName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Baker", meta = (ClampMin = "1", ClampMax = "8192"))
	int32 TextureWidth = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Baker", meta = (ClampMin = "1", ClampMax = "8192"))
	int32 TextureHeight = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Baker")
	TEnumAsByte<TextureCompressionSettings> CompressionSettings = TEnumAsByte<TextureCompressionSettings>(TC_Default);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Baker")
	bool bSRGB = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Baker")
	EMaterialBakeOutputType OutputType = EMaterialBakeOutputType::Texture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Baker")
	FString OutputPath;

	FMaterialBakeSettings() = default;
};
