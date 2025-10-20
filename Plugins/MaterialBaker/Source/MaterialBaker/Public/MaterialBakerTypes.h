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
	TGA UMETA(DisplayName = "TGA"),
};

UENUM(BlueprintType)
enum class EMaterialBakeBitDepth : uint8
{
	Bake_8Bit UMETA(DisplayName = "8-bit"),
	Bake_16Bit UMETA(DisplayName = "16-bit"),
};

UENUM(BlueprintType)
enum class EMaterialPropertyType : uint8
{
	FinalColor UMETA(DisplayName = "Final Color"),
	BaseColor UMETA(DisplayName = "Base Color"),
	Normal UMETA(DisplayName = "Normal"),
	Roughness UMETA(DisplayName = "Roughness"),
	Metallic UMETA(DisplayName = "Metallic"),
	Specular UMETA(DisplayName = "Specular"),
	Opacity UMETA(DisplayName = "Opacity"),
	EmissiveColor UMETA(DisplayName = "Emissive Color"),
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
	bool bSRGB = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Baker")
	EMaterialBakeOutputType OutputType = EMaterialBakeOutputType::Texture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Baker")
	EMaterialBakeBitDepth BitDepth = EMaterialBakeBitDepth::Bake_16Bit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Baker")
	EMaterialPropertyType PropertyType = EMaterialPropertyType::FinalColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Baker")
	FString OutputPath;

	FMaterialBakeSettings() = default;
};
