// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HookableActor.generated.h"

/**
 * Base class for all hookable actors in the game.
 * Derive Blueprint classes from this to create hookable objects that can be placed in levels.
 * The hook system will detect any actor that inherits from this class as a valid hook target.
 */
UCLASS(Blueprintable)
class TEMPORALDASH_API AHookableActor : public AActor
{
	GENERATED_BODY()

public:
	AHookableActor();

protected:
	/** The visual mesh for this hookable actor */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** Optional: Collision sphere for easier hook detection */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class USphereComponent> HookCollision;

public:
	/** Whether this hook point is currently active and can be hooked */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hook")
	bool bIsHookable = true;

	/** Optional: Custom hook offset from actor origin (useful for precise hook points) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hook")
	FVector HookPointOffset = FVector::ZeroVector;

	/** Returns the world location where the hook should attach */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Hook")
	FVector GetHookPoint() const;

	/** Called when a player successfully hooks to this actor */
	UFUNCTION(BlueprintImplementableEvent, Category = "Hook")
	void OnHooked(AActor* HookingActor);

	/** Called when a player releases the hook from this actor */
	UFUNCTION(BlueprintImplementableEvent, Category = "Hook")
	void OnHookReleased(AActor* HookingActor);

	/** Check if this actor can currently be hooked */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Hook")
	bool CanBeHooked() const { return bIsHookable; }

	/** Enable or disable this hook point at runtime */
	UFUNCTION(BlueprintCallable, Category = "Hook")
	void SetHookable(bool bNewHookable) { bIsHookable = bNewHookable; }
};

