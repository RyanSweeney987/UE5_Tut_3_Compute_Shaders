// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "SceneTexturesConfig.h"
#include "PostProcess/PostProcessInputs.h"
#include "RenderPasses/ComputePassBase.h"

struct FColourReplace
{
	FVector3f TargetColour;
	float Tolerance;
	FVector3f ReplacementColour;
};

// This can be included in your FGlobalShader class
// Handy to keep them separate as you can use the same Params for multiple shaders
// BEGIN_SHADER_PARAMETER_STRUCT(FColourReplaceParams,)
// 	// Texture type is same as set in shader 
// 	SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture)
// 	SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
// 	SHADER_PARAMETER_STRUCT_INCLUDE(FSceneTextureShaderParameters, SceneTextures)
//
// 	// The different colours we want to replace
// 	SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBufffer<FColourReplace>, ColourReplacementDataBuffer)
//
// 	// Only needed if we're outputting to a render target
// 	RENDER_TARGET_BINDING_SLOTS()
// END_SHADER_PARAMETER_STRUCT()

// class FColourReplaceCS : public FGlobalShader
// {
// 	DECLARE_EXPORTED_SHADER_TYPE(FColourReplaceCS, Global, );
// 	using FParameters = FColourReplaceParams;
// 	SHADER_USE_PARAMETER_STRUCT(FColourReplaceCS, FGlobalShader);
//
// 	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
// 	{
// 		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
// 	}
// 	
// 	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
// 	{
// 		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
//
// 		// When changing this, you may need to change something in the shader for it to take effect
// 		// A simple comment with a bit of gibberish seems to be enough
// 		SET_SHADER_DEFINE(OutEnvironment, USE_UNLIT_SCENE_COLOUR, 0);
// 	}
// };

/**
 * Override for making it easier to pass in the required parameters
 * Can override the FRenderPassOutputParams to return more than just a texture reference
 *
 * Delete this struct if you don't have the UE5ShaderUtils plugin
 */
struct UE5_TUT_3_COMPUTE_SHADERS_API FColourReplaceInputParams : public FComputePassInputParams
{
	const FIntPoint ThreadCount;
	const FPostProcessingInputs& Inputs;
	const FSceneView& View;
	
	FColourReplaceInputParams(FRDGBuilder &InGraphBuilder, const FGlobalShaderMap *InGlobalShaderMap, FIntPoint InThreadCount, const FPostProcessingInputs& InInputs,  const FSceneView& InView)
		: FComputePassInputParams(InGraphBuilder, InGlobalShaderMap), ThreadCount(InThreadCount), Inputs(InInputs), View(InView)
	{}
};

/**
 * 
 */
class UE5_TUT_3_COMPUTE_SHADERS_API FColourReplaceComputePass : public FComputePassBase
{
public:
	FColourReplaceComputePass() = default;
	
	virtual FComputePassOutputParams AddPass(FComputePassInputParams& InParams) override;
};
