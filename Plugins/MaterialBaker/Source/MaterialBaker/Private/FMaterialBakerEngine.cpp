// Copyright 2025 EmbarrassingMoment. All Rights Reserved.

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
#include "PreviewScene.h"

#define LOCTEXT_NAMESPACE "FMaterialBakerModule"

bool FMaterialBakerEngine::BakeMaterial(const FMaterialBakeSettings& BakeSettings)
{
	// Create a Preview Scene to handle the baking in an isolated world
	FPreviewScene PreviewScene;
	UWorld* World = PreviewScene.GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot get world context!"));
		return false;
	}

	FScopedSlowTask SlowTask(MaterialBakerEngineConstants::TotalSteps, FText::Format(LOCTEXT("BakingMaterial", "Baking Material: {0}..."), FText::FromString(BakeSettings.BakedName)));
	SlowTask.MakeDialog();

	FMaterialBakerContext Context(World, BakeSettings, &SlowTask);

	if (!SetupRenderTarget(Context))
	{
		return false;
	}

	if (!CaptureMaterial(Context))
	{
		return false;
	}

	if (!ReadPixels(Context))
	{
		return false;
	}

	if (BakeSettings.OutputType == EMaterialBakeOutputType::Texture)
	{
		if (!CreateTextureAsset(Context))
		{
			return false;
		}
	}
	else
	{
		if (!ExportImageFile(Context))
		{
			return false;
		}
	}

	return true;
}

bool FMaterialBakerEngine::SetupRenderTarget(FMaterialBakerContext& Context)
{
	Context.SlowTask->EnterProgressFrame(1, FText::Format(LOCTEXT("CreateRenderTarget", "Step 1/{0}: Creating Render Target..."), MaterialBakerEngineConstants::TotalSteps));

	Context.RenderTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), NAME_None, RF_Transient);
	if (!Context.RenderTarget)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("CreateRenderTargetFailed", "Failed to create Render Target."));
		return false;
	}

	EPixelFormat PixelFormat;
	ETextureRenderTargetFormat RenderTargetFormat;

	switch (Context.Settings.BitDepth)
	{
	case EMaterialBakeBitDepth::Bake_8Bit:
		PixelFormat = PF_B8G8R8A8;
		RenderTargetFormat = RTF_RGBA8;
		break;
	case EMaterialBakeBitDepth::Bake_16Bit:
	default:
		PixelFormat = PF_FloatRGBA;
		RenderTargetFormat = RTF_RGBA16f;
		break;
	}

	Context.bIsHdr = Context.Settings.BitDepth == EMaterialBakeBitDepth::Bake_16Bit;
	bool bIsColorData = Context.Settings.PropertyType == EMaterialPropertyType::FinalColor || Context.Settings.PropertyType == EMaterialPropertyType::BaseColor || Context.Settings.PropertyType == EMaterialPropertyType::EmissiveColor;
	Context.bSRGB = bIsColorData && Context.Settings.bSRGB;

	Context.RenderTarget->RenderTargetFormat = RenderTargetFormat;
	Context.RenderTarget->bForceLinearGamma = !Context.bSRGB;
	Context.RenderTarget->InitCustomFormat(Context.TextureSize.X, Context.TextureSize.Y, PixelFormat, !Context.bSRGB);
	Context.RenderTarget->UpdateResourceImmediate(true);

	return true;
}

bool FMaterialBakerEngine::CaptureMaterial(FMaterialBakerContext& Context)
{
	Context.SlowTask->EnterProgressFrame(1, FText::Format(LOCTEXT("DrawMaterial", "Step 2/{0}: Drawing Material..."), MaterialBakerEngineConstants::TotalSteps));

	if (Context.Settings.PropertyType == EMaterialPropertyType::FinalColor)
	{
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(Context.World, Context.RenderTarget, Context.Settings.Material);
	}
	else
	{
		// Scene Capture path for specific properties
		UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
		if (!PlaneMesh)
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("PlaneMeshNotFound", "Could not find /Engine/BasicShapes/Plane.Plane"));
			return false;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;

		AStaticMeshActor* MeshActor = Context.World->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		MeshActor->SetActorLocation(FVector(0, 0, 0));
		MeshActor->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);
		MeshActor->GetStaticMeshComponent()->SetMaterial(0, Context.Settings.Material);

		ASceneCapture2D* CaptureActor = Context.World->SpawnActor<ASceneCapture2D>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		USceneCaptureComponent2D* CaptureComponent = CaptureActor->GetCaptureComponent2D();

		CaptureActor->SetActorLocation(MaterialBakerEngineConstants::DefaultCaptureActorLocation);
		CaptureActor->SetActorRotation(MaterialBakerEngineConstants::DefaultCaptureActorRotation);

		CaptureComponent->TextureTarget = Context.RenderTarget;
		CaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
		CaptureComponent->OrthoWidth = MaterialBakerEngineConstants::DefaultPlaneOrthoWidth;
		CaptureComponent->bCaptureEveryFrame = false;
		CaptureComponent->bCaptureOnMovement = false;
		CaptureComponent->ShowFlags.SetAtmosphere(false);
		CaptureComponent->ShowFlags.SetFog(false);
		CaptureComponent->ShowFlags.SetAmbientOcclusion(false);
		CaptureComponent->ShowFlags.SetScreenSpaceReflections(false);
		CaptureComponent->ShowFlags.SetLighting(false);
		CaptureComponent->ShowFlags.SetPostProcessing(false);
		CaptureComponent->CaptureSource = Context.bIsHdr ? SCS_FinalColorHDR : SCS_FinalColorLDR;

		IConsoleVariable* CVar_BufferVisualizationTarget = nullptr;
		FString PreviousBufferVisualizationValue;
		FEngineShowFlags PreviousShowFlags = CaptureComponent->ShowFlags;
		bool bShowFlagsChanged = false;

		switch (Context.Settings.PropertyType)
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
					switch (Context.Settings.PropertyType)
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
	return true;
}

bool FMaterialBakerEngine::ReadPixels(FMaterialBakerContext& Context)
{
	Context.SlowTask->EnterProgressFrame(1, FText::Format(LOCTEXT("ReadPixels", "Step 3/{0}: Reading Pixels..."), MaterialBakerEngineConstants::TotalSteps));
	FRenderTarget* RenderTargetResource = Context.RenderTarget->GameThread_GetRenderTargetResource();
	if (!RenderTargetResource)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ReadPixelFailed", "Failed to get Render Target Resource."));
		return false;
	}

	bool bReadSuccess = false;
	if (Context.Settings.BitDepth == EMaterialBakeBitDepth::Bake_8Bit)
	{
		TArray<FColor> Pixels;
		bReadSuccess = RenderTargetResource->ReadPixels(Pixels);
		if (bReadSuccess)
		{
			Context.RawPixels.SetNum(Pixels.Num() * sizeof(FColor));
			FMemory::Memcpy(Context.RawPixels.GetData(), Pixels.GetData(), Context.RawPixels.Num());
		}
	}
	else if (Context.Settings.BitDepth == EMaterialBakeBitDepth::Bake_16Bit)
	{
		TArray<FFloat16Color> Pixels;
		bReadSuccess = RenderTargetResource->ReadFloat16Pixels(Pixels);
		if (bReadSuccess)
		{
			Context.RawPixels.SetNum(Pixels.Num() * sizeof(FFloat16Color));
			FMemory::Memcpy(Context.RawPixels.GetData(), Pixels.GetData(), Context.RawPixels.Num());
		}
	}

	if (!bReadSuccess)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ReadPixelFailed", "Failed to read pixels from Render Target."));
		return false;
	}

	// Post-process for specific property types
	if (Context.Settings.PropertyType == EMaterialPropertyType::Opacity)
	{
		int32 NumPixels = Context.TextureSize.X * Context.TextureSize.Y;
		if (Context.Settings.BitDepth == EMaterialBakeBitDepth::Bake_8Bit)
		{
			FColor* Pixels = reinterpret_cast<FColor*>(Context.RawPixels.GetData());
			for (int32 i = 0; i < NumPixels; ++i)
			{
				// For Opacity, the value is in the R channel.
				// Copy it to G and B to make it grayscale, and also to Alpha.
				Pixels[i].G = Pixels[i].R;
				Pixels[i].B = Pixels[i].R;
				Pixels[i].A = Pixels[i].R;
			}
		}
		else // 16-bit
		{
			FFloat16Color* Pixels = reinterpret_cast<FFloat16Color*>(Context.RawPixels.GetData());
			for (int32 i = 0; i < NumPixels; ++i)
			{
				// For Opacity, the value is in the R channel.
				// Copy it to G and B to make it grayscale, and also to Alpha.
				Pixels[i].G = Pixels[i].R;
				Pixels[i].B = Pixels[i].R;
				Pixels[i].A = Pixels[i].R;
			}
		}
	}

	// Enforce Alpha=1 for Opaque materials (unless baking Opacity which handles Alpha itself)
	if (Context.Settings.Material && Context.Settings.Material->GetBlendMode() == BLEND_Opaque && Context.Settings.PropertyType != EMaterialPropertyType::Opacity)
	{
		int32 NumPixels = Context.TextureSize.X * Context.TextureSize.Y;
		if (Context.Settings.BitDepth == EMaterialBakeBitDepth::Bake_8Bit)
		{
			FColor* Pixels = reinterpret_cast<FColor*>(Context.RawPixels.GetData());
			for (int32 i = 0; i < NumPixels; ++i)
			{
				Pixels[i].A = 255;
			}
		}
		else // 16-bit
		{
			FFloat16Color* Pixels = reinterpret_cast<FFloat16Color*>(Context.RawPixels.GetData());
			for (int32 i = 0; i < NumPixels; ++i)
			{
				Pixels[i].A = 1.0f;
			}
		}
	}

	return true;
}

bool FMaterialBakerEngine::CreateTextureAsset(FMaterialBakerContext& Context)
{
	Context.SlowTask->EnterProgressFrame(1, FText::Format(LOCTEXT("PrepareAsset", "Step 4/{0}: Preparing Asset..."), MaterialBakerEngineConstants::TotalSteps));
	FString AssetName = Context.Settings.BakedName;

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	FString UniquePackageName;
	FString UniqueAssetName;
	AssetToolsModule.Get().CreateUniqueAssetName(Context.Settings.OutputPath / AssetName, TEXT(""), UniquePackageName, UniqueAssetName);

	UPackage* Package = CreatePackage(*UniquePackageName);
	Package->FullyLoad();

	UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *UniqueAssetName, RF_Public | RF_Standalone | RF_MarkAsRootSet);
	if (!NewTexture)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("CreateTextureFailed", "Failed to create new texture asset."));
		return false;
	}

	NewTexture->CompressionSettings = Context.Settings.CompressionSettings;
	NewTexture->SRGB = Context.bSRGB;

	ETextureSourceFormat TextureFormat;
	switch (Context.Settings.BitDepth)
	{
	case EMaterialBakeBitDepth::Bake_8Bit:
		TextureFormat = TSF_BGRA8;
		break;
	case EMaterialBakeBitDepth::Bake_16Bit:
	default:
		TextureFormat = TSF_RGBA16F;
		break;
	}

	Context.SlowTask->EnterProgressFrame(1, FText::Format(LOCTEXT("UpdateTexture", "Step 5/{0}: Updating and Saving Texture..."), MaterialBakerEngineConstants::TotalSteps));
	NewTexture->Source.Init(Context.TextureSize.X, Context.TextureSize.Y, 1, 1, TextureFormat, Context.RawPixels.GetData());
	NewTexture->UpdateResource();
	Package->MarkPackageDirty();
	FAssetRegistryModule::GetRegistry().AssetCreated(NewTexture);
	NewTexture->PostEditChange();

	return true;
}

bool FMaterialBakerEngine::ExportImageFile(FMaterialBakerContext& Context)
{
	Context.SlowTask->EnterProgressFrame(1, FText::Format(LOCTEXT("ExportImage", "Step 4/{0}: Exporting Image..."), MaterialBakerEngineConstants::TotalSteps));

	FString Extension;
	EImageFormat ImageFormat;
	ERGBFormat RGBFormat = ERGBFormat::BGRA;
	int32 ExportBitDepth = Context.Settings.BitDepth == EMaterialBakeBitDepth::Bake_8Bit ? 8 : 16;

	switch (Context.Settings.OutputType)
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
	case EMaterialBakeOutputType::EXR:
		Extension = TEXT(".exr");
		ImageFormat = EImageFormat::EXR;
		RGBFormat = ERGBFormat::RGBAF;
		if (Context.Settings.BitDepth != EMaterialBakeBitDepth::Bake_16Bit)
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("EXRRequires16Bit", "EXR format only supports 16-bit float data."));
			return false;
		}
		break;
	default:
		return false;
	}

	TArray<uint8> ExportPixels = Context.RawPixels;
	if (Context.Settings.OutputType == EMaterialBakeOutputType::EXR)
	{
		// The EXR image wrapper expects 32-bit float (FLinearColor) data to correctly save as 16-bit half-float.
		if (Context.Settings.BitDepth == EMaterialBakeBitDepth::Bake_16Bit)
		{
			TArray<FLinearColor> TempLinearPixels;
			TempLinearPixels.AddUninitialized(Context.TextureSize.X * Context.TextureSize.Y);
			const FFloat16Color* Src = reinterpret_cast<const FFloat16Color*>(Context.RawPixels.GetData());
			for (int32 i = 0; i < TempLinearPixels.Num(); ++i)
			{
				TempLinearPixels[i] = FLinearColor(Src[i]);
			}
			ExportPixels.SetNum(TempLinearPixels.Num() * sizeof(FLinearColor));
			FMemory::Memcpy(ExportPixels.GetData(), TempLinearPixels.GetData(), ExportPixels.Num());
			ExportBitDepth = 32; // SetRaw expects 32 for FLinearColor data
		}
	}
	else if (ExportBitDepth == 16 && Context.Settings.BitDepth == EMaterialBakeBitDepth::Bake_16Bit)
	{
		// Convert Float16 data to UNORM16 for PNG/TGA
		// IImageWrapper for 16-bit PNG/TGA expects uint16 data, usually in BGRA order (for TGA/PNG on Windows typically) or RGBA.
		// FFloat16Color is RGBA.
		// We'll convert to BGRA uint16 (0-65535).

		TArray<uint16> TempPixels;
		TempPixels.AddUninitialized(Context.TextureSize.X * Context.TextureSize.Y * 4);
		const FFloat16Color* Src = reinterpret_cast<const FFloat16Color*>(Context.RawPixels.GetData());

		for (int32 i = 0; i < Context.TextureSize.X * Context.TextureSize.Y; ++i)
		{
			FLinearColor Linear = FLinearColor(Src[i]);

			// If sRGB is requested, convert linear to sRGB before quantizing
			if (Context.bSRGB)
			{
				// Apply sRGB curve (standard approx pow(x, 1/2.2))
				// Using FMath::Pow for high precision
				const float InvGamma = 1.0f / 2.2f;
				Linear.R = Linear.R <= 0.0f ? 0.0f : FMath::Pow(Linear.R, InvGamma);
				Linear.G = Linear.G <= 0.0f ? 0.0f : FMath::Pow(Linear.G, InvGamma);
				Linear.B = Linear.B <= 0.0f ? 0.0f : FMath::Pow(Linear.B, InvGamma);
				// Alpha is kept linear
			}

			// Clamp 0-1
			float B = FMath::Clamp(Linear.B, 0.0f, 1.0f);
			float G = FMath::Clamp(Linear.G, 0.0f, 1.0f);
			float R = FMath::Clamp(Linear.R, 0.0f, 1.0f);
			float A = FMath::Clamp(Linear.A, 0.0f, 1.0f);

			// Scale to 0-65535 with rounding
			TempPixels[i * 4 + 0] = (uint16)(B * 65535.0f + 0.5f); // B
			TempPixels[i * 4 + 1] = (uint16)(G * 65535.0f + 0.5f); // G
			TempPixels[i * 4 + 2] = (uint16)(R * 65535.0f + 0.5f); // R
			TempPixels[i * 4 + 3] = (uint16)(A * 65535.0f + 0.5f); // A
		}

		ExportPixels.SetNum(TempPixels.Num() * sizeof(uint16));
		FMemory::Memcpy(ExportPixels.GetData(), TempPixels.GetData(), ExportPixels.Num());

		// Ensure format is BGRA (ImageWrapper default usually)
		RGBFormat = ERGBFormat::BGRA;
	}
	else if (ExportBitDepth == 8 && Context.Settings.BitDepth == EMaterialBakeBitDepth::Bake_16Bit)
	{
		// Convert 16-bit float data to 8-bit for formats like JPEG
		TArray<FColor> TempPixels;
		TempPixels.AddUninitialized(Context.TextureSize.X * Context.TextureSize.Y);
		const FFloat16Color* Src = reinterpret_cast<const FFloat16Color*>(Context.RawPixels.GetData());
		for (int32 i = 0; i < TempPixels.Num(); ++i)
		{
			TempPixels[i] = FLinearColor(Src[i]).ToFColor(Context.bSRGB);
		}
		ExportPixels.SetNum(TempPixels.Num() * sizeof(FColor));
		FMemory::Memcpy(ExportPixels.GetData(), TempPixels.GetData(), ExportPixels.Num());
	}

	FString SaveFilePath = FPaths::Combine(Context.Settings.OutputPath, Context.Settings.BakedName + Extension);
	if (SaveFilePath.StartsWith(TEXT("/Game/")))
	{
		// Explicitly replace the /Game/ path with the full content directory path.
		SaveFilePath = SaveFilePath.Replace(TEXT("/Game/"), *FPaths::ProjectContentDir(), ESearchCase::CaseSensitive);
	}
	// Ensure the path is absolute for the image wrapper, handling both /Game/ paths and other relative paths.
	SaveFilePath = FPaths::ConvertRelativePathToFull(SaveFilePath);

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);

	if (ImageWrapper.IsValid() && ImageWrapper->SetRaw(ExportPixels.GetData(), ExportPixels.Num(), Context.TextureSize.X, Context.TextureSize.Y, RGBFormat, ExportBitDepth))
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
			return false;
		}
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ImageWrapperFailed", "Failed to create or set image wrapper."));
		return false;
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
