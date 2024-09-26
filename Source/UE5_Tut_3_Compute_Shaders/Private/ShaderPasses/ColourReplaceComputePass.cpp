// Fill out your copyright notice in the Description page of Project Settings.


#include "ShaderPasses/ColourReplaceComputePass.h"

IMPLEMENT_SHADER_TYPE(, FTutorialColourReplaceCS, TEXT("/TutorialShaders/private/ComputeTutorialShader.usf"), TEXT("ColourChangeMaskCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FTutorialIndirectComputeCS, TEXT("/TutorialShaders/private/ComputeTutorialShader.usf"), TEXT("IndirectDispatchCS"), SF_Compute);
