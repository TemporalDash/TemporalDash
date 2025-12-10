// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Variant_Shooter/Weapons/ShooterWeapon.h"
#include "ShooterSkill.generated.h"

class AShooterCharacter;

/**
 * 
 */
UCLASS()
class TEMPORALDASH_API AShooterSkill : public AShooterWeapon
{
	GENERATED_BODY()
protected:
	virtual void FireProjectile(const FVector& TargetLocation) override;

public:
	virtual void DestroyWeapon() override;

	virtual void ActivateWeapon();

	/** Deactivates this weapon */
	virtual void DeactivateWeapon();

	/** Called when the active weapon changes */
	UFUNCTION(BlueprintImplementableEvent, Category = "Shooter", meta = (DisplayName = "On Skill Activate"))
	void BP_OnSkillActivate(const AShooterCharacter* caster, const FVector& TargetLocation);

	UFUNCTION(BlueprintImplementableEvent, Category = "Cleanup", meta = (DisplayName = "On Skill Added"))
	void BP_OnSkillAdded();

	UFUNCTION(BlueprintImplementableEvent, Category="Cleanup", meta = (DisplayName = "On Skill Destroy"))
	void BP_OnSkillDestroy();

	UFUNCTION(BlueprintImplementableEvent, Category = "Cleanup", meta = (DisplayName = "On Skill Enable"))
	void BP_OnSkillEnable();

	UFUNCTION(BlueprintImplementableEvent, Category = "Cleanup", meta = (DisplayName = "On Skill Disable"))
	void BP_OnSkillDisable();
};
