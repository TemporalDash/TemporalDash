// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "Field/FieldSystemComponent.h"
#include "Field/FieldSystemObjects.h" // For RadialFalloff, etc.
#include "Field/FieldSystemTypes.h"
#include "BreakableStructure.generated.h"

UCLASS()
class TEMPORALDASH_API ABreakableStructure : public AActor
{
	GENERATED_BODY()

public:
    ABreakableStructure();

    // Call this from your Weapon/Projectile class
    UFUNCTION(BlueprintImplementableEvent, Category = "Chaos")
    void OnDestruction(const FVector& HitLocation);
};
