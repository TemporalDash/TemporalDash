// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterWeapon.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "ShooterProjectile.h"
#include "ShooterWeaponHolder.h"
#include "Components/SceneComponent.h"
#include "TimerManager.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"

AShooterWeapon::AShooterWeapon()
{
	PrimaryActorTick.bCanEverTick = false; // weapon does not need ticking

	// create the root
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// create the first person mesh
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));
	FirstPersonMesh->SetupAttachment(RootComponent);

	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));
	FirstPersonMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	FirstPersonMesh->bOnlyOwnerSee = true;

	// create the third person mesh
	ThirdPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Third Person Mesh"));
	ThirdPersonMesh->SetupAttachment(RootComponent);

	ThirdPersonMesh->SetCollisionProfileName(FName("NoCollision"));
	ThirdPersonMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::WorldSpaceRepresentation);
	ThirdPersonMesh->bOwnerNoSee = true;
	
	// initialize magazines (MaxMagazines set in BP; RemainingMagazines excludes the one currently loaded)
	RemainingMagazines = FMath::Max(0, MaxMagazines - 1);
}

void AShooterWeapon::BeginPlay()
{
	Super::BeginPlay();

	// subscribe to the owner's destroyed delegate
	GetOwner()->OnDestroyed.AddDynamic(this, &AShooterWeapon::OnOwnerDestroyed);

	// cast the weapon owner
	WeaponOwner = Cast<IShooterWeaponHolder>(GetOwner());
	PawnOwner = Cast<APawn>(GetOwner());

	// initialize ammo
	CurrentBullets = MagazineSize;
	// RemainingMagazines already set in constructor

	// attach the meshes to the owner
	WeaponOwner->AttachWeaponMeshes(this);
}

void AShooterWeapon::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the refire timer
	GetWorld()->GetTimerManager().ClearTimer(RefireTimer);
}

void AShooterWeapon::OnOwnerDestroyed(AActor* DestroyedActor)
{
	// ensure this weapon is destroyed when the owner is destroyed
	Destroy();
}

void AShooterWeapon::ActivateWeapon()
{
	// unhide this weapon
	SetActorHiddenInGame(false);

	// notify the owner
	WeaponOwner->OnWeaponActivated(this);
}

void AShooterWeapon::DeactivateWeapon()
{
	// ensure we're no longer firing this weapon while deactivated
	StopFiring();

	// hide the weapon
	SetActorHiddenInGame(true);

	// notify the owner
	WeaponOwner->OnWeaponDeactivated(this);
}

void AShooterWeapon::StartFiring()
{
	if (IsEmpty())
	{
		// Play a dry-fire sound if needed
		return;
	}
	// raise the firing flag
	bIsFiring = true;

	// check how much time has passed since we last shot
	// this may be under the refire rate if the weapon shoots slow enough and the player is spamming the trigger
	const float TimeSinceLastShot = GetWorld()->GetTimeSeconds() - TimeOfLastShot;

	if (TimeSinceLastShot > RefireRate)
	{
		// fire the weapon right away
		Fire();

	} else {

		// if we're full auto, schedule the next shot
		if (bFullAuto)
		{
			GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &AShooterWeapon::Fire, TimeSinceLastShot, false);
		}

	}
}

void AShooterWeapon::StopFiring()
{
	// lower the firing flag
	bIsFiring = false;

	// clear the refire timer (check for valid world during destruction)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefireTimer);
	}
}

void AShooterWeapon::Fire()
{
	// ensure the player still wants to fire. They may have let go of the trigger
	if (!bIsFiring)
	{
		return;
	}
	
	// guard: ensure we have bullets
	if (CurrentBullets <= 0)
	{
		if (RemainingMagazines > 0)
		{
			--RemainingMagazines;
			CurrentBullets = MagazineSize;
		}
		else
		{
			bIsFiring = false;
			StopFiring();
			if (PawnOwner.IsValid() && PawnOwner->IsPlayerControlled())
			{
				TWeakObjectPtr<AActor> WeakOwner = GetOwner();
				GetWorld()->GetTimerManager().SetTimerForNextTick([this, WeakOwner]()
				{
					if (IsValid(this) && WeakOwner.IsValid())
					{
						if (IShooterWeaponHolder* Holder = Cast<IShooterWeaponHolder>(WeakOwner.Get()))
						{
							Holder->DiscardWeapon(this);
						}
					}
				});
			}
			else
			{
				CurrentBullets = MagazineSize;
			}
			return;
		}
	}
	
	// fire a projectile at the target
	FireProjectile(WeaponOwner->GetWeaponTargetLocation());

	// consume a bullet
	--CurrentBullets;

	// update the weapon HUD (optional per-shot)
	if (WeaponOwner && ShouldUpdateHUDPerShot())
	{
		WeaponOwner->UpdateWeaponHUD(CurrentBullets, MagazineSize);
	}

	// if we just consumed the last bullet and have no spare magazines, handle empty
	if (CurrentBullets <= 0 && RemainingMagazines <= 0)
	{
		bIsFiring = false;
		StopFiring();
		if (PawnOwner.IsValid() && PawnOwner->IsPlayerControlled())
		{
			TWeakObjectPtr<AActor> WeakOwner = GetOwner();
			GetWorld()->GetTimerManager().SetTimerForNextTick([this, WeakOwner]()
			{
				if (IsValid(this) && WeakOwner.IsValid())
				{
					if (IShooterWeaponHolder* Holder = Cast<IShooterWeaponHolder>(WeakOwner.Get()))
					{
						Holder->DiscardWeapon(this);
					}
				}
			});
		}
		else
		{
			CurrentBullets = MagazineSize;
		}
		return;
	}

	// update the time of our last shot
	TimeOfLastShot = GetWorld()->GetTimeSeconds();

	// make noise so the AI perception system can hear us
	APawn* RawPawnOwner = PawnOwner.Get();
	MakeNoise(ShotLoudness, RawPawnOwner, RawPawnOwner ? RawPawnOwner->GetActorLocation() : GetActorLocation(), ShotNoiseRange, ShotNoiseTag);

	// are we full auto?
	if (bFullAuto && bIsFiring)
	{
		// schedule the next shot
		GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &AShooterWeapon::Fire, RefireRate, false);
	}
	else if (bIsFiring)
	{
		// for semi-auto weapons, schedule the cooldown notification
		GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &AShooterWeapon::FireCooldownExpired, RefireRate, false);
	}
}

void AShooterWeapon::FireCooldownExpired()
{
	// notify the owner
	WeaponOwner->OnSemiWeaponRefire();
}

void AShooterWeapon::FireProjectile(const FVector& TargetLocation)
{
    // get the projectile transform
    FTransform ProjectileTransform = CalculateProjectileSpawnTransform(TargetLocation);
    
    // spawn the projectile
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::OverrideRootScale;
    SpawnParams.Owner = GetOwner();
    SpawnParams.Instigator = PawnOwner.Get();

    AShooterProjectile* Projectile = GetWorld()->SpawnActor<AShooterProjectile>(ProjectileClass, ProjectileTransform, SpawnParams);

    // play the firing montage
    WeaponOwner->PlayFiringMontage(FiringMontage);

    // add recoil
    WeaponOwner->AddWeaponRecoil(FiringRecoil);
}

FTransform AShooterWeapon::CalculateProjectileSpawnTransform(const FVector& TargetLocation) const
{
	// find the muzzle location
	const FVector MuzzleLoc = FirstPersonMesh->GetSocketLocation(MuzzleSocketName);

	// calculate the spawn location ahead of the muzzle
	const FVector SpawnLoc = MuzzleLoc + ((TargetLocation - MuzzleLoc).GetSafeNormal() * MuzzleOffset);

	// find the aim rotation vector while applying some variance to the target 
	const FRotator AimRot = UKismetMathLibrary::FindLookAtRotation(SpawnLoc, TargetLocation + (UKismetMathLibrary::RandomUnitVector() * AimVariance));

	// return the built transform
	return FTransform(AimRot, SpawnLoc, FVector::OneVector);
}

const TSubclassOf<UAnimInstance>& AShooterWeapon::GetFirstPersonAnimInstanceClass() const
{
	return FirstPersonAnimInstanceClass;
}

const TSubclassOf<UAnimInstance>& AShooterWeapon::GetThirdPersonAnimInstanceClass() const
{
	return ThirdPersonAnimInstanceClass;
}

void AShooterWeapon::HandleEmpty()
{
	if (IsEmpty())
	{
		StopFiring();
		if (PawnOwner.IsValid() && PawnOwner->IsPlayerControlled())
		{
			if (IShooterWeaponHolder* Holder = Cast<IShooterWeaponHolder>(GetOwner()))
			{
				Holder->DiscardWeapon(this);
			}
		}
	}
}
