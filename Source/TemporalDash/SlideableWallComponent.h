// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "SlideableWallComponent.generated.h"

class UBoxComponent;
class UArrowComponent;
class ATemporalDashCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TEMPORALDASH_API USlideableWallComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USlideableWallComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	/** The area that triggers the interaction */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	UBoxComponent* DetectionBox;

	/** Visual indicator for the wall's facing direction */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	UArrowComponent* NormalIndicator;

	/** Helper to get the direction this wall is facing */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	FVector GetInteractionNormal() const;

	// --- Overlap Events ---

	UFUNCTION()
	void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnBoxEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};