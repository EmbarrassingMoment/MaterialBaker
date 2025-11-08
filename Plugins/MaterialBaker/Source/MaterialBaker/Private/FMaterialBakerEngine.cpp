// Copyright 2025 kurorekish. All Rights Reserved.

#include "FMaterialBakerEngine.h"
#include "MaterialBakerTypes.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/TextureFactory.h"
#include "Misc/FileHelper.h"
#include "Editor.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "IDesktopPlatform.h"
#include "DesktopPlatformModule.h"
#include "Misc/ScopedSlowTask.h"
#include "RenderCore.h"
#include "Misc/MessageDialog.h"
#include "Engine/Texture.h"
#include "UObject/EnumProperty.h"
#include "Engine/SceneCapture2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "HAL/IConsoleManager.h"


#define LOCTEXT_NAMESPACE "FMaterialBakerModule"

bool FMaterialBakerEngine::BakeMaterial(const FMaterialBakeSettings& BakeSettings)
{
	FIntPoint TextureSize(BakeSettings.TextureWidth, BakeSettings.TextureHeight);

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot get world context!"));
		return false;
	}

	FScopedSlowTask SlowTask(MaterialBakerEngineConstants::TotalSteps, FText::Format(LOCTEXT("BakingMaterial", "Baking Material: {0}..."), FText::FromString(BakeSettings.BakedName)));
	SlowTask.MakeDialog();

	// Step 1: Create Render Target
	SlowTask.EnterProgressFrame(1, FText::Format(LOCTEXT("CreateRenderTarget", "Step 1/{0}: Creating Render Target..."), MaterialBakerEngineConstants::TotalSteps));

	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
	if (!RenderTarget)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("CreateRenderTargetFailed", "Failed to create Render Target."));
		return false;
	}
	RenderTarget->AddToRoot();

	EPixelFormat PixelFormat;
	ETextureRenderTargetFormat RenderTargetFormat;
	int32 BitDepthValue;

	switch (BakeSettings.BitDepth)
	{
	case EMaterialBakeBitDepth::Bake_8Bit:
		PixelFormat = PF_B8G8R8A8;
		RenderTargetFormat = RTF_RGBA8;
		BitDepthValue = 8;
		break;
	case EMaterialBakeBitDepth::Bake_16Bit:
	default:
		PixelFormat = PF_FloatRGBA;
		RenderTargetFormat = RTF_RGBA16f;
		BitDepthValue = 16;
		break;
	}

	const bool bIsHdr = BakeSettings.BitDepth == EMaterialBakeBitDepth::Bake_16Bit;
	bool bIsColorData = BakeSettings.PropertyType == EMaterialPropertyType::FinalColor || BakeSettings.PropertyType == EMaterialPropertyType::BaseColor || BakeSettings.PropertyType == EMaterialPropertyType::EmissiveColor;
	bool bSRGB = bIsColorData && BakeSettings.bSRGB;

	RenderTarget->RenderTargetFormat = RenderTargetFormat;
	RenderTarget->bForceLinearGamma = !bSRGB;
	RenderTarget->InitCustomFormat(TextureSize.X, TextureSize.Y, PixelFormat, !bSRGB);
	RenderTarget->UpdateResourceImmediate(true);

	// Step 2: Draw Material
	SlowTask.EnterProgressFrame(1, FText::Format(LOCTEXT("DrawMaterial", "Step 2/{0}: Drawing Material..."), MaterialBakerEngineConstants::TotalSteps));

	if (BakeSettings.PropertyType == EMaterialPropertyType::FinalColor)
	{
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(World, RenderTarget, BakeSettings.Material);
	}
	else
	{
		// Scene Capture path for specific properties
		UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
		if (!PlaneMesh)
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("PlaneMeshNotFound", "Could not find /Engine/BasicShapes/Plane.Plane"));
			RenderTarget->RemoveFromRoot();
			return false;
		}

		AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>();
		MeshActor->SetActorLocation(FVector(0, 0, 0));
		MeshActor->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);
		MeshActor->GetStaticMeshComponent()->SetMaterial(0, BakeSettings.Material);

		ASceneCapture2D* CaptureActor = World->SpawnActor<ASceneCapture2D>();
		USceneCaptureComponent2D* CaptureComponent = CaptureActor->GetCaptureComponent2D();

		CaptureActor->SetActorLocation(FVector(0, 0, 100.0f));
		CaptureActor->SetActorRotation(FRotator(-90.f, 0.f, -90.f));

		CaptureComponent->TextureTarget = RenderTarget;
		CaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
		CaptureComponent->OrthoWidth = 200; // Default plane is 200x200
		CaptureComponent->bCaptureEveryFrame = false;
		CaptureComponent->bCaptureOnMovement = false;
		CaptureComponent->ShowFlags.SetAtmosphere(false);
		CaptureComponent->ShowFlags.SetFog(false);
		CaptureComponent->ShowFlags.SetAmbientOcclusion(false);
		CaptureComponent->ShowFlags.SetScreenSpaceReflections(false);
		CaptureComponent->ShowFlags.SetLighting(false);
		CaptureComponent->ShowFlags.SetPostProcessing(false);
		CaptureComponent->CaptureSource = bIsHdr ? SCS_FinalColorHDR : SCS_FinalColorLDR;

		IConsoleVariable* CVar_BufferVisualizationTarget = nullptr;
		FString PreviousBufferVisualizationValue;
		FEngineShowFlags PreviousShowFlags = CaptureComponent->ShowFlags;
		bool bShowFlagsChanged = false;

		switch (BakeSettings.PropertyType)
		{
		case EMaterialPropertyType::BaseColor:
			CaptureComponent->CaptureSource = SCS_BaseColor;
			break;
		case EMaterialPropertyType::Normal:
			CaptureComponent->CaptureSource = SCS_Normal;
			break;
		case EMaterialPropertyType::Roughness:
		case EMaterialPropertyType::Metallic:
		case EMaterialPropertyType::Specular:
		case EMaterialPropertyType::Opacity:
			{
				CVar_BufferVisualizationTarget = IConsoleManager::Get().FindConsoleVariable(TEXT("r.BufferVisualizationTarget"));
				if (CVar_BufferVisualizationTarget)
				{
					PreviousBufferVisualizationValue = CVar_BufferVisualizationTarget->GetString();
					FString TargetBufferName;
					switch (BakeSettings.PropertyType)
					{
						case EMaterialPropertyType::Roughness: TargetBufferName = TEXT("Roughness"); break;
						case EMaterialPropertyType::Metallic:  TargetBufferName = TEXT("Metallic"); break;
						case EMaterialPropertyType::Specular:  TargetBufferName = TEXT("Specular"); break;
						case EMaterialPropertyType::Opacity:   TargetBufferName = TEXT("Opacity"); break;
						default: break;
					}
					CVar_BufferVisualizationTarget->Set(*TargetBufferName, ECVF_SetByCode);
				}
			}
			break;
		case EMaterialPropertyType::EmissiveColor:
			CaptureComponent->ShowFlags.SetLighting(false);
			CaptureComponent->ShowFlags.SetPostProcessing(false);
			CaptureComponent->ShowFlags.SetTonemapper(false);
			CaptureComponent->CaptureSource = SCS_FinalColorHDR;
			bShowFlagsChanged = true;
			break;
		default:
			break;
		}

		CaptureComponent->CaptureScene();

		if (CVar_BufferVisualizationTarget)
		{
			CVar_BufferVisualizationTarget->Set(*PreviousBufferVisualizationValue, ECVF_SetByCode);
		}

		if (bShowFlagsChanged)
		{
			CaptureComponent->ShowFlags = PreviousShowFlags;
		}

		if (MeshActor)
		{
			MeshActor->Destroy();
			MeshActor = nullptr;
		}

		if (CaptureActor)
		{
			CaptureActor->Destroy();
			CaptureActor = nullptr;
		}
	}
	FlushRenderingCommands();

	// Step 3: Read Pixels
	SlowTask.EnterProgressFrame(1, FText::Format(LOCTEXT("ReadPixels", "Step 3/{0}: Reading Pixels..."), MaterialBakerEngineConstants::TotalSteps));
	FRenderTarget* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
	if (!RenderTargetResource)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ReadPixelFailed", "Failed to get Render Target Resource."));
		RenderTarget->RemoveFromRoot();
		return false;
	}

	TArray<uint8> RawPixels;
	bool bReadSuccess = false;
	if (BakeSettings.BitDepth == EMaterialBakeBitDepth::Bake_8Bit)
	{
		TArray<FColor> Pixels;
		bReadSuccess = RenderTargetResource->ReadPixels(Pixels);
		if (bReadSuccess)
		{
			RawPixels.SetNum(Pixels.Num() * sizeof(FColor));
			FMemory::Memcpy(RawPixels.GetData(), Pixels.GetData(), RawPixels.Num());
		}
	}
	else if (BakeSettings.BitDepth == EMaterialBakeBitDepth::Bake_16Bit)
	{
		TArray<FFloat16Color> Pixels;
		bReadSuccess = RenderTargetResource->ReadFloat16Pixels(Pixels);
		if (bReadSuccess)
		{
			RawPixels.SetNum(Pixels.Num() * sizeof(FFloat16Color));
			FMemory::Memcpy(RawPixels.GetData(), Pixels.GetData(), RawPixels.Num());
		}
	}

	if (!bReadSuccess)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ReadPixelFailed", "Failed to read pixels from Render Target."));
		RenderTarget->RemoveFromRoot();
		return false;
	}

	// Post-process for specific property types
	if (BakeSettings.PropertyType == EMaterialPropertyType::Opacity)
	{
		if (BakeSettings.BitDepth == EMaterialBakeBitDepth::Bake_8Bit)
		{
			TArray<FColor> Pixels;
			Pixels.AddUninitialized(TextureSize.X * TextureSize.Y);
			FMemory::Memcpy(Pixels.GetData(), RawPixels.GetData(), RawPixels.Num());

			for (FColor& Pixel : Pixels)
			{
				// For Opacity, the value is in the R channel.
				// Copy it to G and B to make it grayscale, and also to Alpha.
				Pixel.G = Pixel.R;
				Pixel.B = Pixel.R;
				Pixel.A = Pixel.R;
			}
			FMemory::Memcpy(RawPixels.GetData(), Pixels.GetData(), RawPixels.Num());
		}
		else // 16-bit
		{
			TArray<FFloat16Color> Pixels;
			Pixels.AddUninitialized(TextureSize.X * TextureSize.Y);
			FMemory::Memcpy(Pixels.GetData(), RawPixels.GetData(), RawPixels.Num());

			for (FFloat16Color& Pixel : Pixels)
			{
				// For Opacity, the value is in the R channel.
				// Copy it to G and B to make it grayscale, and also to Alpha.
				Pixel.G = Pixel.R;
				Pixel.B = Pixel.R;
				Pixel.A = Pixel.R;
			}
			FMemory::Memcpy(RawPixels.GetData(), Pixels.GetData(), RawPixels.Num());
		}
	}

	if (BakeSettings.OutputType == EMaterialBakeOutputType::Texture)
	{
		SlowTask.EnterProgressFrame(1, FText::Format(LOCTEXT("PrepareAsset", "Step 4/{0}: Preparing Asset..."), MaterialBakerEngineConstants::TotalSteps));
		FString AssetName = BakeSettings.BakedName;

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		FString UniquePackageName;
		FString UniqueAssetName;
		AssetToolsModule.Get().CreateUniqueAssetName(BakeSettings.OutputPath / AssetName, TEXT(""), UniquePackageName, UniqueAssetName);

		UPackage* Package = CreatePackage(*UniquePackageName);
		Package->FullyLoad();

		UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *UniqueAssetName, RF_Public | RF_Standalone | RF_MarkAsRootSet);
		if (!NewTexture)
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("CreateTextureFailed", "Failed to create new texture asset."));
			RenderTarget->RemoveFromRoot();
			return false;
		}
		NewTexture->AddToRoot();

		NewTexture->CompressionSettings = BakeSettings.CompressionSettings;
		NewTexture->SRGB = bSRGB;

		ETextureSourceFormat TextureFormat;
		switch (BakeSettings.BitDepth)
		{
		case EMaterialBakeBitDepth::Bake_8Bit:
			TextureFormat = TSF_BGRA8;
			break;
		case EMaterialBakeBitDepth::Bake_16Bit:
		default:
			TextureFormat = TSF_RGBA16F;
			break;
		}

		SlowTask.EnterProgressFrame(1, FText::Format(LOCTEXT("UpdateTexture", "Step 5/{0}: Updating and Saving Texture..."), MaterialBakerEngineConstants::TotalSteps));
		NewTexture->Source.Init(TextureSize.X, TextureSize.Y, 1, 1, TextureFormat, RawPixels.GetData());
		NewTexture->UpdateResource();
		Package->MarkPackageDirty();
		FAssetRegistryModule::GetRegistry().AssetCreated(NewTexture);
		NewTexture->PostEditChange();
		NewTexture->RemoveFromRoot();
	}
	else if (BakeSettings.OutputType == EMaterialBakeOutputType::PNG || BakeSettings.OutputType == EMaterialBakeOutputType::JPEG || BakeSettings.OutputType == EMaterialBakeOutputType::TGA)
	{
		SlowTask.EnterProgressFrame(1, FText::Format(LOCTEXT("ExportImage", "Step 4/{0}: Exporting Image..."), MaterialBakerEngineConstants::TotalSteps));

		FString Extension;
		EImageFormat ImageFormat;
		ERGBFormat RGBFormat = ERGBFormat::BGRA;
		int32 ExportBitDepth = BitDepthValue;

		switch (BakeSettings.OutputType)
		{
		case EMaterialBakeOutputType::PNG:
			Extension = TEXT(".png");
			ImageFormat = EImageFormat::PNG;
			break;
		case EMaterialBakeOutputType::JPEG:
			Extension = TEXT(".jpg");
			ImageFormat = EImageFormat::JPEG;
			ExportBitDepth = 8;
			break;
		case EMaterialBakeOutputType::TGA:
			Extension = TEXT(".tga");
			ImageFormat = EImageFormat::TGA;
			break;
		default:
			return false;
		}

		TArray<uint8> ExportPixels = RawPixels;
		if (ExportBitDepth == 8 && BakeSettings.BitDepth == EMaterialBakeBitDepth::Bake_16Bit)
		{
			TArray<FColor> TempPixels;
			TempPixels.AddUninitialized(TextureSize.X * TextureSize.Y);
			const FFloat16Color* Src = reinterpret_cast<const FFloat16Color*>(RawPixels.GetData());
			for (int32 i = 0; i < TempPixels.Num(); ++i)
			{
				TempPixels[i] = FLinearColor(Src[i]).ToFColor(bSRGB);
			}
			ExportPixels.SetNum(TempPixels.Num() * sizeof(FColor));
			FMemory::Memcpy(ExportPixels.GetData(), TempPixels.GetData(), ExportPixels.Num());
		}

		FString SaveFilePath = FPaths::Combine(BakeSettings.OutputPath, BakeSettings.BakedName + Extension);
		if (SaveFilePath.StartsWith(TEXT("/Game/")))
		{
			FString ContentDir = FPaths::ProjectContentDir();
			SaveFilePath = SaveFilePath.RightChop(6);
			SaveFilePath = FPaths::Combine(ContentDir, SaveFilePath);
		}

		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);

		if (ImageWrapper.IsValid() && ImageWrapper->SetRaw(ExportPixels.GetData(), ExportPixels.Num(), TextureSize.X, TextureSize.Y, RGBFormat, ExportBitDepth))
		{
			FString DirectoryPath = FPaths::GetPath(SaveFilePath);
			if (!FPaths::DirectoryExists(DirectoryPath))
			{
				FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*DirectoryPath);
			}

			const TArray64<uint8>& CompressedData = ImageWrapper->GetCompressed();
			if (!FFileHelper::SaveArrayToFile(CompressedData, *SaveFilePath))
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("SaveImageFailed", "Failed to save image to {0}."), FText::FromString(SaveFilePath)));
				RenderTarget->RemoveFromRoot();
				return false;
			}
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ImageWrapperFailed", "Failed to create or set image wrapper."));
			RenderTarget->RemoveFromRoot();
			return false;
		}
	}

	RenderTarget->RemoveFromRoot();
	return true;
}

#undef LOCTEXT_NAMESPACE