// Copyright Epic Games, Inc. All Rights Reserved.

#include "UE5_Tut_3_Compute_Shaders.h"

#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FUE5_Tut_3_Compute_ShadersModule"

void FUE5_Tut_3_Compute_ShadersModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	// Shaders is the folder with a private folder inside
	// Requires Projects
	const FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("UE5_Tut_3_Compute_Shaders"))->GetBaseDir(), TEXT("Shaders"));
	// Requires RenderCore
	if(!AllShaderSourceDirectoryMappings().Contains(TEXT("/TutorialShaders")))
	{
		AddShaderSourceDirectoryMapping(TEXT("/TutorialShaders"), PluginShaderDir);
	}
}

void FUE5_Tut_3_Compute_ShadersModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUE5_Tut_3_Compute_ShadersModule, UE5_Tut_3_Compute_Shaders)