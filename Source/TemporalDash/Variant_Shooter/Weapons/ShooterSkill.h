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
	/** Called when the active weapon changes */
	UFUNCTION(BlueprintImplementableEvent, Category = "Shooter", meta = (DisplayName = "On Skill Activate"))
	void BP_OnSkillActivate(const AShooterCharacter* caster, const FVector& TargetLocation);
};
