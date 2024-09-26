// Fill out your copyright notice in the Description page of Project Settings.


#include "Rendering/ComputeSceneViewExtension.h"

#include "PixelShaderUtils.h"
#include "RenderGraphEvent.h"
#include "SceneRenderTargetParameters.h"
#include "SceneTexturesConfig.h"
#include "CompGeom/Delaunay2.h"
#include "ShaderPasses/ColourReplaceComputePass.h"

DECLARE_GPU_DRAWCALL_STAT(ColourReplace); // Unreal Insights
DECLARE_GPU_DRAWCALL_STAT(DownloadReplaceCount); // Unreal Insights


namespace ComputeHelperFunctions
{
	FVector3f RGBToXYZ(const FVector3f Colour)
	{
		float R = Colour.X;
		float G = Colour.Y;
		float B = Colour.Z;

		if(R > 0.04045)
		{
			R = FMath::Pow(((R + 0.055) / 1.055), 2.4);
		} else
		{
			R = R / 12.92;
		}
		if(G > 0.04045)
		{
			G = FMath::Pow(((G + 0.055) / 1.055), 2.4);
		}
		else
		{
			G = G / 12.92;
		}
		if (B > 0.04045)
		{
			B = FMath::Pow(((B + 0.055) / 1.055), 2.4);
		}
		else
		{
			B = B / 12.92;
		}

		R = R * 100;
		G = G * 100;
		B = B * 100;

		const float X = R * 0.4124 + G * 0.3576 + B * 0.1805;
		const float Y = R * 0.2126 + G * 0.7152 + B * 0.0722;
		const float Z = R * 0.0193 + G * 0.1192 + B * 0.9505;
	
		return {X, Y, Z};
	}

	FVector3f XYZToLab(const FVector3f XYZ)
	{
		// Found here https://www.easyrgb.com/en/math.php
		// Under XYZ (Tristimulus) Reference values of a perfect reflecting diffuser
		// D65 illuminant, 2° observer
		constexpr float Xn = 95.047;
		constexpr float Yn = 100.000;
		constexpr float Zn = 108.883;

		const float X = XYZ.X / Xn;
		const float Y = XYZ.Y / Yn;
		const float Z = XYZ.Z / Zn;

		constexpr float Epsilon = 0.008856;
		constexpr float Kappa = 903.3;
		constexpr float Third = 1.0 / 3.0;

		const float fX = (X > Epsilon) ? FMath::Pow(X, Third) : (Kappa * X + 16.0) / 116.0;
		const float fY = (Y > Epsilon) ? FMath::Pow(Y, Third) : (Kappa * Y + 16.0) / 116.0;
		const float fZ = (Z > Epsilon) ? FMath::Pow(Z, Third) : (Kappa * Z + 16.0) / 116.0;

		const float L = (116.0 * fY) - 16.0;
		const float a = 500.0 * (fX - fY);
		const float b = 200.0 * (fY - fZ);
	
		return {L, a, b};
	}

	FVector3f RGBToLab(const FVector3f Colour)
	{
		return XYZToLab(RGBToXYZ(Colour));
	}

	FVector3f RGBToHSL(const FVector3f Colour)
	{
		//Min. value of RGB
		const float Min = FMath::Min3(Colour.X, Colour.Y, Colour.Z);    
		//Max. value of RGB
		const float Max = FMath::Max3(Colour.X, Colour.Y, Colour.Z);    
		//Delta RGB value
		const float Delta = Max - Min;            

		const float L = (Max + Min) / 2;

		float H = 0;
		float S = 0;

		// If delta is not grey, it has chroma
		if (Delta > 0)                                     
		{
			S = L > 0.5 ? Delta / (2 - Max - Min) : Delta / (Max + Min);

			if(Max == Colour.X)
			{
				H = (Colour.Y - Colour.Z) / Delta + (Colour.Y < Colour.Z ? 6 : 0);
			}
			else if(Max == Colour.Y)
			{
				H = (Colour.Z - Colour.X) / Delta + 2;
			}
			else if(Max == Colour.Z)
			{
				H = (Colour.X - Colour.Y) / Delta + 4;
			}

			H /= 6;
		}

		return FVector3f(H, S, L);
	}
}

FComputeSceneViewExtension::FComputeSceneViewExtension(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister)
{
	Readback = new FRHIGPUBufferReadback(TEXT("Colour Replacement Count Readback"));
}

FComputeSceneViewExtension::~FComputeSceneViewExtension()
{
	delete Readback;
	Readback = nullptr;
}

void FComputeSceneViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View,
                                                                 const FPostProcessingInputs& Inputs)
{
	FSceneViewExtensionBase::PrePostProcessPass_RenderThread(GraphBuilder, View, Inputs);
	
	checkSlow(View.bIsViewInfo);
	const FIntRect Viewport = static_cast<const FViewInfo&>(View).ViewRect;
	// Requires RHI & RenderCore
	const FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	
	constexpr bool bUseAsyncCompute = false;
	const bool bAsyncCompute = GSupportsEfficientAsyncCompute && (GNumExplicitGPUsForRendering == 1) && bUseAsyncCompute;
	
	RDG_GPU_STAT_SCOPE(GraphBuilder, ColourReplace); // Unreal Insights
	RDG_EVENT_SCOPE(GraphBuilder,  "Colour Replace Compute"); // RenderDoc
	
	// --------------------------------------------------------------------------------
	
	// Declaring all the buffers, UAVs and SRVs ahead of time
	FRDGTextureUAVRef SceneColourTextureUAV = nullptr;
	FRDGBufferRef ColourReplacementDataBuffer = nullptr;
	FRDGBufferRef ColourReplacementCountBuffer = nullptr;
	FRDGBufferUAVRef ColourReplacementCountBufferUAV = nullptr;
	FRDGBufferRef ExecuteIndirectBuffer = nullptr;
	FRDGBufferUAVRef ExecuteIndirectBufferUAV = nullptr;
	
	// --------------------------------------------------------------------------------
	
	// This is to get the base colour without shading
	const FSceneTextureShaderParameters SceneTextures = CreateSceneTextureShaderParameters(GraphBuilder, View, ESceneTextureSetupMode::SceneColor | ESceneTextureSetupMode::GBuffers);
	// This is colour with shading and shadows
	SceneColourTextureUAV = GraphBuilder.CreateUAV((*Inputs.SceneTextures)->SceneColorTexture);
	
	// Creating the data for what colours we want to replace
	TArray<FColourReplace> ColourReplacements = {
		{
			ComputeHelperFunctions::RGBToLab(FVector3f(1.0f, 0.0f, 0.0f)), // Red
			10.0,
			ComputeHelperFunctions::RGBToHSL(FVector3f(0.0f, 1.0f, 0.0f)) // Blue
		}, {
			ComputeHelperFunctions::RGBToLab(FVector3f(0.0f, 1.0f, 0.0f)), // Blue
			10.0,
			ComputeHelperFunctions::RGBToHSL(FVector3f(0.0f, 0.0f, 1.0f))  // Green
		}, {
			ComputeHelperFunctions::RGBToLab(FVector3f(0.0f, 0.0f, 1.0f)), // Green
			10.0,
			ComputeHelperFunctions::RGBToHSL(FVector3f(1.0f, 0.0f, 0.0f)) // Red
		}
	};
	
	// Create the buffer for uploading the data to the GPU
	ColourReplacementDataBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("Colour Replacement Buffer"), sizeof(FColourReplace), ColourReplacements.Num(), ColourReplacements.GetData(), ColourReplacements.Num() * sizeof(FColourReplace));
	
	// Buffer for how many pixels we've replaced
	const FRDGBufferDesc ColourReplacementCountDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), ColourReplacements.Num());
	ColourReplacementCountBuffer = GraphBuilder.CreateBuffer(ColourReplacementCountDesc, TEXT("Colour Replacement Count Buffer"));
	ColourReplacementCountBufferUAV = GraphBuilder.CreateUAV(ColourReplacementCountBuffer);
	
	// Buffer for setting up the indirect dispatch
	FRDGBufferDesc ExecuteIndirectDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), 4);
	ExecuteIndirectDesc.Usage = EBufferUsageFlags(ExecuteIndirectDesc.Usage | BUF_ByteAddressBuffer | BUF_DrawIndirect);
	ExecuteIndirectBuffer = GraphBuilder.CreateBuffer(ExecuteIndirectDesc, TEXT("Execute Indirect Buffer"));
	ExecuteIndirectBufferUAV = GraphBuilder.CreateUAV(ExecuteIndirectBuffer);
	
	FTutorialColourReplaceCS::FParameters* Parameters = GraphBuilder.AllocParameters<FTutorialColourReplaceCS::FParameters>();
	Parameters->ColourCount = ColourReplacements.Num();
	Parameters->SceneColorTexture = SceneColourTextureUAV;
	Parameters->View = View.ViewUniformBuffer;
	Parameters->SceneTextures = SceneTextures;
	Parameters->ColourReplacementDataBuffer = GraphBuilder.CreateSRV(ColourReplacementDataBuffer);
	Parameters->ColourReplacementCount = ColourReplacementCountBufferUAV;
	Parameters->ExecuteIndirectBuffer = ExecuteIndirectBufferUAV;
	
	const FIntPoint ThreadCount = Viewport.Size();
	const FIntVector GroupCount = FComputeShaderUtils::GetGroupCount(ThreadCount, FIntPoint(ColourReplaceCompute::THREADS_X, ColourReplaceCompute::THREADS_Y));
	
	FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("Colour Replace Compute Pass %u", 1), ERDGPassFlags::Compute, TShaderMapRef<FTutorialColourReplaceCS>(GlobalShaderMap), Parameters, GroupCount);
	
	// --------------------------------------------------------------------------------
	
	{
		RDG_GPU_STAT_SCOPE(GraphBuilder, DownloadReplaceCount); // Unreal Insights
		RDG_EVENT_SCOPE(GraphBuilder,  "Download Replace Count"); // RenderDoc
		
		// Download the data
		const uint32 NumOfBytes = sizeof(uint32) * ColourReplacements.Num();
	
		AddEnqueueCopyPass(GraphBuilder, Readback, ColourReplacementCountBuffer, NumOfBytes);
	
		if(Readback->IsReady())
		{
			ColourReplacementCounts.Empty();
	
			// Read the data
			uint32* Buffer = (uint32*)Readback->Lock(NumOfBytes);
	
			// Copy the data
			for(int i = 0; i < ColourReplacements.Num(); i++)
			{
				ColourReplacementCounts.Add(Buffer[i]);
			}
				
			Readback->Unlock();
		}
	}
	
	// --------------------------------------------------------------------------------
	
	// Execute the indirect compute shader
	FTutorialIndirectComputeCS::FParameters* IndirectParameters = GraphBuilder.AllocParameters<FTutorialIndirectComputeCS::FParameters>();
	IndirectParameters->SceneColorTexture = SceneColourTextureUAV;
	
	const ERDGPassFlags PassFlags = bAsyncCompute ? ERDGPassFlags::AsyncCompute : ERDGPassFlags::Compute;
	
	const TShaderRef<FTutorialIndirectComputeCS> IndirectComputeShader = TShaderMapRef<FTutorialIndirectComputeCS>(GlobalShaderMap);
	
	constexpr uint32 IndirectArgsOffset = 0;
	FComputeShaderUtils::ValidateGroupCount(GroupCount);
	FComputeShaderUtils::ValidateIndirectArgsBuffer(ExecuteIndirectBuffer, IndirectArgsOffset);
	
	GraphBuilder.AddPass(
		RDG_EVENT_NAME("Colour Replace Compute Pass %u", 2),
		IndirectParameters,
		PassFlags,
		[IndirectParameters, IndirectComputeShader, ExecuteIndirectBuffer, IndirectArgsOffset](FRHIComputeCommandList& RHICmdList)
	{
		FComputeShaderUtils::DispatchIndirect(RHICmdList, IndirectComputeShader, *IndirectParameters, ExecuteIndirectBuffer, IndirectArgsOffset);
	});
}
