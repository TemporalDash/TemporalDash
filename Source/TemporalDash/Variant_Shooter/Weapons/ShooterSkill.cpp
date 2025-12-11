// Fill out your copyright notice in the Description page of Project Settings.


#include "Variant_Shooter/Weapons/ShooterSkill.h"
#include "Variant_Shooter/ShooterCharacter.h"
#include "Variant_Shooter/Weapons/ShooterWeapon.h"

void AShooterSkill::FireProjectile(const FVector& TargetLocation) {
	if (AShooterCharacter* OwnerCharacter = Cast<AShooterCharacter>(WeaponOwner))
	BP_OnSkillActivate(OwnerCharacter, TargetLocation);
}

void AShooterSkill::DestroyWeapon() {
	BP_OnSkillDestroy();
}

void AShooterSkill::ActivateWeapon(){
	Super::ActivateWeapon();
	BP_OnSkillEnable();
}

/** Deactivates this weapon */
void AShooterSkill::DeactivateWeapon() {
	Super::DeactivateWeapon();
	BP_OnSkillDisable();
}