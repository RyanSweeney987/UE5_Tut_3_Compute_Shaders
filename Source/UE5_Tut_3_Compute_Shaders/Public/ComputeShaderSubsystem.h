// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "Rendering/ComputeSceneViewExtension.h"
#include "ComputeShaderSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class UE5_TUT_3_COMPUTE_SHADERS_API UComputeShaderSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

private:
	TSharedPtr< class FComputeSceneViewExtension, ESPMode::ThreadSafe > CustomSceneViewExtension;
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
};
