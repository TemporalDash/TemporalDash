// Fill out your copyright notice in the Description page of Project Settings.


#include "Variant_Shooter/Weapons/ShooterSkill.h"
#include "Variant_Shooter/ShooterCharacter.h"

void AShooterSkill::FireProjectile(const FVector& TargetLocation) {
	if (AShooterCharacter* OwnerCharacter = Cast<AShooterCharacter>(WeaponOwner))
	BP_OnSkillActivate(OwnerCharacter, TargetLocation);
}
