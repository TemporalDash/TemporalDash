// Fill out your copyright notice in the Description page of Project Settings.

#include "HookableActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"

AHookableActor::AHookableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create root scene component
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Create mesh component for visual representation
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(Root);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// Create optional hook collision sphere (can be resized in BP)
	HookCollision = CreateDefaultSubobject<USphereComponent>(TEXT("HookCollision"));
	HookCollision->SetupAttachment(Root);
	HookCollision->SetSphereRadius(50.0f);
	HookCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HookCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	HookCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	HookCollision->SetHiddenInGame(true);
}

FVector AHookableActor::GetHookPoint() const
{
	return GetActorLocation() + GetActorRotation().RotateVector(HookPointOffset);
}

