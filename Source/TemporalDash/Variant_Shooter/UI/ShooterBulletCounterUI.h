// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterBulletCounterUI.generated.h"

/**
 *  Simple bullet counter UI widget for a first person shooter game
 */
UCLASS(abstract)
class TEMPORALDASH_API UShooterBulletCounterUI : public UUserWidget
{
	GENERATED_BODY()
	
public:

	/** Allows Blueprint to update sub-widgets with the new bullet count */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta=(DisplayName = "UpdateBulletCounter"))
	void BP_UpdateBulletCounter(int32 MagazineSize, int32 BulletCount);

	/** Allows Blueprint to update sub-widgets with the new life total and play a damage effect on the HUD */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta=(DisplayName = "Damaged"))
	void BP_Damaged(float LifePercent);

	/** 
	 * Called when a weapon is discarded/removed from the player's inventory.
	 * Implement this event in Blueprint to remove weapon card images from the UI
	 * by calling RemoveCard on the OverlappedCardWidget.
	 * @param WeaponIndex The index of the weapon that was removed (0-based inventory slot)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter|UI", meta=(DisplayName = "OnWeaponDiscarded"))
	void BP_OnWeaponDiscarded(int32 WeaponIndex);
};
