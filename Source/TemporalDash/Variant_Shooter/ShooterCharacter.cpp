// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterCharacter.h"
#include "ShooterWeapon.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "ShooterGameMode.h"

AShooterCharacter::AShooterCharacter()
{
	// create the noise emitter component
	PawnNoiseEmitter = CreateDefaultSubobject<UPawnNoiseEmitterComponent>(TEXT("Pawn Noise Emitter"));

	// configure movement
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 600.0f, 0.0f);
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// reset HP to max
	CurrentHP = MaxHP;

	// update the HUD
	OnDamaged.Broadcast(1.0f);

	// store default AnimInstance classes for when we have no weapon
	DefaultFirstPersonAnimClass = GetFirstPersonMesh()->GetAnimInstance() ? GetFirstPersonMesh()->GetAnimInstance()->GetClass() : nullptr;
	DefaultThirdPersonAnimClass = GetMesh()->GetAnimInstance() ? GetMesh()->GetAnimInstance()->GetClass() : nullptr;
}

void AShooterCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the respawn timer
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
}

void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// base class handles move, aim and jump inputs
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Firing
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AShooterCharacter::DoStartFiring);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AShooterCharacter::DoStopFiring);

		// Switch weapon
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Triggered, this, &AShooterCharacter::DoSwitchWeapon);
	}

}

float AShooterCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// ignore if already dead
	if (CurrentHP <= 0.0f)
	{
		return 0.0f;
	}

	// Reduce HP
	CurrentHP -= Damage;

	// Have we depleted HP?
	if (CurrentHP <= 0.0f)
	{
		Die();
	}

	// update the HUD
	OnDamaged.Broadcast(FMath::Max(0.0f, CurrentHP / MaxHP));

	return Damage;
}

void AShooterCharacter::DoStartFiring()
{
	// fire the current weapon
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->StartFiring();
	}
}

void AShooterCharacter::DoStopFiring()
{
	// stop firing the current weapon
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->StopFiring();
	}
}

void AShooterCharacter::DoSwitchWeapon()
{
	// ensure we have at least two weapons two switch between
	if (OwnedWeapons.Num() > 1)
	{
		// if we don't currently have a weapon index, initialize to first
		if (CurrentWeaponIndex == INDEX_NONE)
		{
			EquipWeaponByIndex(0);
			return;
		}

		int32 NextIndex = (CurrentWeaponIndex + 1) % OwnedWeapons.Num();
		if (NextIndex != CurrentWeaponIndex)
		{
			EquipWeaponByIndex(NextIndex);
		}
	}
}

void AShooterCharacter::AttachWeaponMeshes(AShooterWeapon* Weapon)
{
	const FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, false);

	// attach the weapon actor
	Weapon->AttachToActor(this, AttachmentRule);

	// attach the weapon meshes
	Weapon->GetFirstPersonMesh()->AttachToComponent(GetFirstPersonMesh(), AttachmentRule, FirstPersonWeaponSocket);
	Weapon->GetThirdPersonMesh()->AttachToComponent(GetMesh(), AttachmentRule, FirstPersonWeaponSocket);
	
}

void AShooterCharacter::PlayFiringMontage(UAnimMontage* Montage)
{
	
}

void AShooterCharacter::AddWeaponRecoil(float Recoil)
{
	// apply the recoil as pitch input
	AddControllerPitchInput(Recoil);
}

void AShooterCharacter::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	OnBulletCountUpdated.Broadcast(MagazineSize, CurrentAmmo);
}

FVector AShooterCharacter::GetWeaponTargetLocation()
{
	// trace ahead from the camera viewpoint
	FHitResult OutHit;

	const FVector Start = GetFirstPersonCameraComponent()->GetComponentLocation();
	const FVector End = Start + (GetFirstPersonCameraComponent()->GetForwardVector() * MaxAimDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, QueryParams);

	// return either the impact point or the trace end
	return OutHit.bBlockingHit ? OutHit.ImpactPoint : OutHit.TraceEnd;
}

void AShooterCharacter::AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass)
{
	// do we already own this weapon?
	AShooterWeapon* OwnedWeapon = FindWeaponOfType(WeaponClass);

	if (!OwnedWeapon)
	{
		// spawn the new weapon
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;

		AShooterWeapon* AddedWeapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);

		if (AddedWeapon)
		{
			// add the weapon to the owned list
			OwnedWeapons.Add(AddedWeapon);

			// equip newly added weapon by index (last element)
			EquipWeaponByIndex(OwnedWeapons.Num() - 1);
		}
	}
}

void AShooterCharacter::OnWeaponActivated(AShooterWeapon* Weapon)
{
	// update the bullet counter
	OnBulletCountUpdated.Broadcast(Weapon->GetMagazineSize(), Weapon->GetBulletCount());

	// set the character mesh AnimInstances
	GetFirstPersonMesh()->SetAnimInstanceClass(Weapon->GetFirstPersonAnimInstanceClass());
	GetMesh()->SetAnimInstanceClass(Weapon->GetThirdPersonAnimInstanceClass());
}

void AShooterCharacter::OnWeaponDeactivated(AShooterWeapon* Weapon)
{
	// unused
}

void AShooterCharacter::OnSemiWeaponRefire()
{
	// unused
}

void AShooterCharacter::DiscardWeapon(AShooterWeapon* Weapon)
{
	// validate weapon pointer
	if (!IsValid(Weapon))
	{
		return;
	}

	// find index before removal
	int32 RemovedIndex = OwnedWeapons.Find(Weapon);
	if (RemovedIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("DiscardWeapon: Weapon not found in inventory!"));
		return; // weapon not in inventory
	}

	// broadcast weapon discarded event (for UI/card removal)
	UE_LOG(LogTemp, Warning, TEXT("DiscardWeapon: Broadcasting OnWeaponDiscarded for index %d"), RemovedIndex);
	OnWeaponDiscarded.Broadcast(RemovedIndex);

	// deactivate if currently equipped
	bool bWasCurrent = (Weapon == CurrentWeapon);
	if (bWasCurrent && IsValid(CurrentWeapon))
	{
		CurrentWeapon->DeactivateWeapon();
	}

	OwnedWeapons.RemoveAt(RemovedIndex);

	// adjust current weapon selection
	if (bWasCurrent)
	{
		if (OwnedWeapons.Num() == 0)
		{
			CurrentWeapon = nullptr;
			CurrentWeaponIndex = INDEX_NONE;

			// restore default AnimInstance classes for the hand meshes
			if (DefaultFirstPersonAnimClass)
			{
				GetFirstPersonMesh()->SetAnimInstanceClass(DefaultFirstPersonAnimClass);
			}
			if (DefaultThirdPersonAnimClass)
			{
				GetMesh()->SetAnimInstanceClass(DefaultThirdPersonAnimClass);
			}

			// reset the bullet counter UI
			OnBulletCountUpdated.Broadcast(0, 0);
		}
		else
		{
			// prefer same slot index (now points at next weapon) or clamp
			int32 NewIndex = RemovedIndex;
			if (!OwnedWeapons.IsValidIndex(NewIndex))
			{
				NewIndex = OwnedWeapons.Num() - 1; // removed last, fallback to new last
			}
			EquipWeaponByIndex(NewIndex);
		}
	}
	else
	{
		// if not current and index before current, shift CurrentWeaponIndex left
		if (RemovedIndex < CurrentWeaponIndex)
		{
			--CurrentWeaponIndex;
		}
	}

	// destroy the weapon actor last
	Weapon->Destroy();
}

AShooterWeapon* AShooterCharacter::FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const
{
	// check each owned weapon
	for (AShooterWeapon* Weapon : OwnedWeapons)
	{
		if (Weapon->IsA(WeaponClass))
		{
			return Weapon;
		}
	}

	// weapon not found
	return nullptr;

}

void AShooterCharacter::Die()
{
	// deactivate the weapon
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->DeactivateWeapon();
	}

	// increment the team score
	if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->IncrementTeamScore(TeamByte);
	}
		
	// stop character movement
	GetCharacterMovement()->StopMovementImmediately();

	// disable controls
	DisableInput(nullptr);

	// reset the bullet counter UI
	OnBulletCountUpdated.Broadcast(0, 0);

	// call the BP handler
	BP_OnDeath();

	// schedule character respawn
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &AShooterCharacter::OnRespawn, RespawnTime, false);
}

void AShooterCharacter::OnRespawn()
{
	// destroy the character to force the PC to respawn
	Destroy();
}

void AShooterCharacter::EquipWeaponByIndex(int32 NewIndex)
{
	if (!OwnedWeapons.IsValidIndex(NewIndex))
	{
		CurrentWeapon = nullptr;
		CurrentWeaponIndex = INDEX_NONE;
		return;
	}

	AShooterWeapon* NewWeapon = OwnedWeapons[NewIndex];
	if (NewWeapon == CurrentWeapon)
	{
		// already equipped
		return;
	}

	// deactivate previous
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->DeactivateWeapon();
	}

	CurrentWeapon = NewWeapon;
	CurrentWeaponIndex = NewIndex;

	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->ActivateWeapon();
	}
}
