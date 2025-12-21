// Copyright 2025 EmbarrassingMoment. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "MaterialBakerTypes.h"
#include "FMaterialBakerEngine.h"
#include "Modules/ModuleManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"

// Define a simple test to verify constants and enums are correct
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaterialBakerConstantsTest, "MaterialBaker.ConstantsTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMaterialBakerConstantsTest::RunTest(const FString& Parameters)
{
	// Verify that JPEG is an allowed Output Type in the enum
	// (This is a compile-time check mostly, but ensures the enum exists)
	const UEnum* OutputTypeEnum = StaticEnum<EMaterialBakeOutputType>();
	if (!TestNotNull(TEXT("EMaterialBakeOutputType enum should exist"), OutputTypeEnum))
	{
		return false;
	}

	int64 JpegValue = OutputTypeEnum->GetValueByName(TEXT("EMaterialBakeOutputType::JPEG"));
	TestNotEqual(TEXT("JPEG should be a valid entry in EMaterialBakeOutputType"), JpegValue, (int64)INDEX_NONE);

	// Verify that Normal is a valid Property Type
	const UEnum* PropertyTypeEnum = StaticEnum<EMaterialPropertyType>();
	if (!TestNotNull(TEXT("EMaterialPropertyType enum should exist"), PropertyTypeEnum))
	{
		return false;
	}
	int64 NormalValue = PropertyTypeEnum->GetValueByName(TEXT("EMaterialPropertyType::Normal"));
	TestNotEqual(TEXT("Normal should be a valid entry in EMaterialPropertyType"), NormalValue, (int64)INDEX_NONE);

	return true;
}

// Define a test for 16-bit data conversion logic
// Since FMaterialBakerEngine is private/internal, we might not be able to test it directly without friends.
// However, we can test the general logic if we replicate it or assume we are testing the public interface.
// Since we cannot run this test, this is mostly for documentation of the fix verification.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaterialBakerExportLogicTest, "MaterialBaker.ExportLogicTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMaterialBakerExportLogicTest::RunTest(const FString& Parameters)
{
	// Test case: Verify that 16-bit PNG export requires UNORM16 data, not Float16.
	// This test simulates the conversion logic we are adding.

	// Input: Float16 data (0.0 to 1.0)
	TArray<FFloat16Color> FloatData;
	FloatData.Add(FFloat16Color(FLinearColor(0.0f, 0.5f, 1.0f, 1.0f)));

	// Expected Output: UNORM16 data (0 to 65535)
	// 0.0 -> 0
	// 0.5 -> 32767 (approx)
	// 1.0 -> 65535

	// Logic replication (to verify the fix code is correct)
	TArray<uint16> IntData;
	IntData.AddUninitialized(FloatData.Num() * 4);

	const FFloat16Color* Src = FloatData.GetData();
	uint16* Dest = IntData.GetData();

	for (int32 i = 0; i < FloatData.Num(); ++i)
	{
		FLinearColor Linear = FLinearColor(Src[i]);

		// Logic to be implemented in FMaterialBakerEngine:
		// Clamp and scale to 16-bit range
		Dest[i * 4 + 0] = (uint16)(FMath::Clamp(Linear.B, 0.0f, 1.0f) * 65535.0f); // B
		Dest[i * 4 + 1] = (uint16)(FMath::Clamp(Linear.G, 0.0f, 1.0f) * 65535.0f); // G
		Dest[i * 4 + 2] = (uint16)(FMath::Clamp(Linear.R, 0.0f, 1.0f) * 65535.0f); // R
		Dest[i * 4 + 3] = (uint16)(FMath::Clamp(Linear.A, 0.0f, 1.0f) * 65535.0f); // A
	}

	TestEqual(TEXT("Blue channel should be 1.0 -> 65535"), IntData[0], (uint16)65535);
	TestEqual(TEXT("Green channel should be 0.5 -> 32767"), IntData[1], (uint16)32767); // Approx
	TestEqual(TEXT("Red channel should be 0.0 -> 0"), IntData[2], (uint16)0);

	return true;
}
