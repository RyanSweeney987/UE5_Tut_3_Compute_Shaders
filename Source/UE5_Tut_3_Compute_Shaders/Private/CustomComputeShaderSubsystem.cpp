// Fill out your copyright notice in the Description page of Project Settings.


#include "ComputeShaderSubsystem.h"

#include "SceneViewExtension.h"

void UComputeShaderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	CustomSceneViewExtension = FSceneViewExtensions::NewExtension<FComputeSceneViewExtension>();
}

void UComputeShaderSubsystem::Deinitialize()
{
	Super::Deinitialize();

	CustomSceneViewExtension.Reset();
	CustomSceneViewExtension = nullptr;
}
