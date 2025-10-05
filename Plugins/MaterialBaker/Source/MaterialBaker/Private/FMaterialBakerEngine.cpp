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

	const int32 TotalSteps = 5; // 処理の総ステップ数
	FScopedSlowTask SlowTask(TotalSteps, FText::Format(LOCTEXT("BakingMaterial", "Baking Material: {0}..."), FText::FromString(BakeSettings.BakedName)));
	SlowTask.MakeDialog(); // プログレスバーのダイアログを表示

	// ステップ1: Render Targetの作成
	SlowTask.EnterProgressFrame(1, LOCTEXT("CreateRenderTarget", "Step 1/5: Creating Render Target..."));

	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
	if (!RenderTarget)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("CreateRenderTargetFailed", "Failed to create Render Target."));
		return false;
	}

	RenderTarget->AddToRoot();
	RenderTarget->RenderTargetFormat = RTF_RGBA16f;
	// PF_FloatRGBAを指定し、ガンマはリニアに強制
	RenderTarget->InitCustomFormat(TextureSize.X, TextureSize.Y, PF_FloatRGBA, true);
	RenderTarget->UpdateResourceImmediate(true);

	// ステップ2: マテリアルを描画
	SlowTask.EnterProgressFrame(1, LOCTEXT("DrawMaterial", "Step 2/5: Drawing Material..."));
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(World, RenderTarget, BakeSettings.Material);

	// レンダリングコマンドをフラッシュして、描画が完了するのを待つ
	FlushRenderingCommands();

	// ステップ3: ピクセルデータを読み込み
	SlowTask.EnterProgressFrame(1, LOCTEXT("ReadPixels", "Step 3/5: Reading Pixels..."));
	FRenderTarget* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
	TArray<FFloat16Color> RawPixels;
	if (!RenderTargetResource || !RenderTargetResource->ReadFloat16Pixels(RawPixels))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ReadPixelFailed", "Failed to read pixels from Render Target."));
		RenderTarget->RemoveFromRoot();
		return false;
	}


	if (BakeSettings.OutputType == EMaterialBakeOutputType::Texture)
	{
		// ステップ4: アセットの作成準備
		SlowTask.EnterProgressFrame(1, LOCTEXT("PrepareAsset", "Step 4/5: Preparing Asset..."));
		FString PackagePath = BakeSettings.OutputPath;
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
		NewTexture->SRGB = BakeSettings.bSRGB;

		// ステップ5: テクスチャを更新して保存
		SlowTask.EnterProgressFrame(1, LOCTEXT("UpdateTexture", "Step 5/5: Updating and Saving Texture..."));
		NewTexture->Source.Init(TextureSize.X, TextureSize.Y, 1, 1, TSF_RGBA16F, (const uint8*)RawPixels.GetData());
		NewTexture->UpdateResource();
		Package->MarkPackageDirty();
		FAssetRegistryModule::GetRegistry().AssetCreated(NewTexture);
		NewTexture->PostEditChange();
		NewTexture->RemoveFromRoot();
	}
	else if (BakeSettings.OutputType == EMaterialBakeOutputType::PNG || BakeSettings.OutputType == EMaterialBakeOutputType::JPEG || BakeSettings.OutputType == EMaterialBakeOutputType::TGA)
	{
		// ステップ4: 画像ファイルとしてエクスポート
		SlowTask.EnterProgressFrame(1, LOCTEXT("ExportImage", "Step 4/5: Exporting Image..."));

		TArray<FColor> OutPixels;
		OutPixels.AddUninitialized(TextureSize.X * TextureSize.Y);
		for (int32 i = 0; i < RawPixels.Num(); ++i)
		{
			OutPixels[i] = FLinearColor(RawPixels[i]).ToFColor(BakeSettings.bSRGB);
		}

		FString Extension;
		EImageFormat ImageFormat;
		switch (BakeSettings.OutputType)
		{
		case EMaterialBakeOutputType::PNG:
			Extension = TEXT(".png");
			ImageFormat = EImageFormat::PNG;
			break;
		case EMaterialBakeOutputType::JPEG:
			Extension = TEXT(".jpg");
			ImageFormat = EImageFormat::JPEG;
			break;
		case EMaterialBakeOutputType::TGA:
			Extension = TEXT(".tga");
			ImageFormat = EImageFormat::TGA;
			break;
		default:
			// Should not happen
			return false;
		}

		FString SaveFilePath = FPaths::Combine(BakeSettings.OutputPath, BakeSettings.BakedName + Extension);
		if (SaveFilePath.StartsWith(TEXT("/Game/")))
		{
			FString ContentDir = FPaths::ProjectContentDir();
			SaveFilePath = SaveFilePath.RightChop(6); // Remove "/Game/"
			SaveFilePath = FPaths::Combine(ContentDir, SaveFilePath);
		}

		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);

		if (ImageWrapper.IsValid() && ImageWrapper->SetRaw(OutPixels.GetData(), OutPixels.Num() * sizeof(FColor), TextureSize.X, TextureSize.Y, ERGBFormat::BGRA, 8))
		{
			// ディレクトリが存在しない場合は作成
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
