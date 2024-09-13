// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomComputeShaderSubsystem.h"

#include "SceneViewExtension.h"

void UCustomComputeShaderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	CustomSceneViewExtension = FSceneViewExtensions::NewExtension<FCustomSceneViewExtension>();
}

void UCustomComputeShaderSubsystem::Deinitialize()
{
	Super::Deinitialize();

	CustomSceneViewExtension.Reset();
	CustomSceneViewExtension = nullptr;
}
