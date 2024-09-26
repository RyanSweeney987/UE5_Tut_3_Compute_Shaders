// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "SceneTexturesConfig.h"
#include "PostProcess/PostProcessInputs.h"
#include "RenderPasses/ComputePassBase.h"

namespace ColourReplaceCompute
{
	static constexpr int32 THREADS_X = 16;
	static constexpr int32 THREADS_Y = 16;
}

struct FColourReplace
{
	FVector3f TargetColourLab;
	float PerceptionThreshold;
	FVector3f ReplacementColourHSL;
};

// This can be included in your FGlobalShader class
// Handy to keep them separate as you can use the same Params for multiple shaders
BEGIN_SHADER_PARAMETER_STRUCT(FTutorialColourReplaceParams,)
	SHADER_PARAMETER(int, ColourCount)

	// The texture we're going to be reading from and writing to
	SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, SceneColorTexture)

	// Texture type is same as set in shader - for getting the unlit colour
	SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
	SHADER_PARAMETER_STRUCT_INCLUDE(FSceneTextureShaderParameters, SceneTextures)

	// The different colours we want to replace
	SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBufffer<FColourReplace>, ColourReplacementDataBuffer)

	// How many pixels we've replaced
	SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructureBuffer<int>, ColourReplacementCount)

	// For setting up the indirect dispatch
	SHADER_PARAMETER_RDG_BUFFER_UAV(RWByteAddressBuffer, ExecuteIndirectBuffer)
END_SHADER_PARAMETER_STRUCT()

class FTutorialColourReplaceCS : public FGlobalShader
{
	DECLARE_EXPORTED_SHADER_TYPE(FTutorialColourReplaceCS, Global, );
	using FParameters = FTutorialColourReplaceParams;
	SHADER_USE_PARAMETER_STRUCT(FTutorialColourReplaceCS, FGlobalShader);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
	
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);

		// When changing these, you may need to change something in the shader for it to take effect
		// A simple comment with a bit of gibberish seems to be enough
		SET_SHADER_DEFINE(OutEnvironment, USE_UNLIT_SCENE_COLOUR, 0);
		SET_SHADER_DEFINE(OutEnvironment, THREADS_X, ColourReplaceCompute::THREADS_X);
		SET_SHADER_DEFINE(OutEnvironment, THREADS_Y, ColourReplaceCompute::THREADS_Y);
	}
};

BEGIN_SHADER_PARAMETER_STRUCT(FTutorialIndirectComputeParams,)
	SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, SceneColorTexture)
END_SHADER_PARAMETER_STRUCT()

class FTutorialIndirectComputeCS : public FGlobalShader
{
	DECLARE_EXPORTED_SHADER_TYPE(FTutorialIndirectComputeCS, Global, );
	using FParameters = FTutorialIndirectComputeParams;
	SHADER_USE_PARAMETER_STRUCT(FTutorialIndirectComputeCS, FGlobalShader);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
	
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}
};