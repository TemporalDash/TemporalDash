// Additional hook implementation for ATemporalDashCharacter
// Implements DoHookStart, DoHookEnd, FindHookPoint, PerformHook, UpdateHookMovement, EndHook

#include "TemporalDashCharacter.h"
#include "TemporalDash.h"
#include "HookableActor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "InputActionValue.h"
#include "Variant_Shooter/Weapons/ShooterProjectile.h"

void ATemporalDashCharacter::DoHookStart(const FInputActionValue& ActionValue)
{

	if (bIsHooked)
	{
		return;
	}

	FVector HitLocation;
	if (FindHookPoint(HitLocation))
	{
		HookPoint = HitLocation;
		PerformHook();
	}
}

void ATemporalDashCharacter::DoHookEnd(const FInputActionValue& ActionValue)
{
	// DEBUG: Check if function is being called

	if (bIsHooked)
	{
		EndHook();
	}
}

bool ATemporalDashCharacter::FindHookPoint(FVector& OutHitLocation)
{
	if (!FirstPersonCameraComponent)
	{
		return false;
	}

	// shoot from camera
	FVector Start = FirstPersonCameraComponent->GetComponentLocation();
	FVector ForwardVector = FirstPersonCameraComponent->GetForwardVector();
	FVector End = Start + ForwardVector * HookMaxRange;

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
			HitResult,
			Start,
			End,
			ECC_Visibility,
			QueryParams
	);

	// Determine if the hit target is hookable
	bool bIsHookable = false;
	AActor* HitActor = nullptr;
	AHookableActor* HookableActor = nullptr;

	if (bHit)
	{
		HitActor = HitResult.GetActor();
		if (HitActor)
		{
			// 1. Check for Projectile (C++ Class) - can hook to projectiles
			bool bIsProjectile = (Cast<AShooterProjectile>(HitActor) != nullptr);

			// 2. Check for HookableActor (C++ base class and all BP children)
			HookableActor = Cast<AHookableActor>(HitActor);
			bool bIsHookableActor = (HookableActor != nullptr) && HookableActor->CanBeHooked();

			bIsHookable = bIsProjectile || bIsHookableActor;

		}
	}

	// Draw debug line with correct color: Green = hookable, Red = not hookable or no hit
	#if !UE_BUILD_SHIPPING
	DrawDebugLine(GetWorld(), Start, End, bIsHookable ? FColor::Green : FColor::Red, false, 2.0f, 0, 2.0f);
	if (bHit)
	{
		DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 25.0f, 12,
			bIsHookable ? FColor::Yellow : FColor::Orange, false, 2.0f);
	}
	#endif

	OutHitLocation = HitResult.ImpactPoint;

	return bIsHookable;
}

void ATemporalDashCharacter::PerformHook()
{
	bIsHooked = true;

	// Record initial rope length (for rope constraint)
	HookMaxRopeLength = FVector::Dist(GetActorLocation(), HookPoint);

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		// Force character into falling state to override ground movement
		MoveComp->SetMovementMode(MOVE_Flying);
		
		// Reduce gravity for better air control
		MoveComp->GravityScale = 0.4f;
		
		// Clear ground friction
		MoveComp->GroundFriction = 0.0f;
		MoveComp->BrakingDecelerationWalking = 0.0f;
		
		// Give initial upward impulse if on ground to break ground contact
		if (MoveComp->IsMovingOnGround())
		{
			FVector LaunchVel = FVector(0, 0, 500.0f); // Upward boost
			LaunchCharacter(LaunchVel, false, false);
		}
	}

}

void ATemporalDashCharacter::UpdateHookMovement(float DeltaTime)
{
	if (!bIsHooked)
	{
			return;
	}

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp)
	{
			return;
	}

	// === Step 1: Calculate direction to hook point ===
	FVector ActorLocation = GetActorLocation();
	FVector ToHook = HookPoint - ActorLocation;
	float DistanceToHook = ToHook.Size();

	// Auto-detach when reaching minimum distance
	if (DistanceToHook < HookMinDetachDistance)
	{
			EndHook();
			return;
	}

	FVector DirToHook = ToHook.GetSafeNormal();

	// === Step 2: Get current velocity ===
	FVector Velocity = MoveComp->Velocity;

	// === Step 3: Decompose into normal and tangent velocity ===
	float NormalSpeed = FVector::DotProduct(Velocity, DirToHook);
	FVector V_normal = DirToHook * NormalSpeed;
	FVector V_tangent = Velocity - V_normal;

	// === Step 4: Add pull force toward hook point (Method A - direct velocity modification) ===
	FVector NewNormal = V_normal + DirToHook * HookPullStrength * DeltaTime;

	// === Step 5: Handle tangent velocity - blend player input ===
	if (!LastMovementInput.IsNearlyZero())
	{
			// Convert player input to world space direction
			FRotator ControlRotation = GetControlRotation();
			FVector InputWorldDir = ControlRotation.RotateVector(LastMovementInput);
			InputWorldDir.Z = 0.0f;  // Keep horizontal component only
			InputWorldDir.Normalize();

			// Add to tangent velocity
			FVector SteeringForce = InputWorldDir * HookSteeringInfluence * 800.0f * DeltaTime;
			V_tangent += SteeringForce;
	}

	// === Step 6: Rope length constraint (prevent passing through hook point) ===
	if (DistanceToHook > HookMaxRopeLength)
	{
			// Only limit outward velocity component
			float OutwardSpeed = FVector::DotProduct(Velocity, -DirToHook);
			if (OutwardSpeed > 0.0f)
			{
					// Remove outward velocity to create "rope tightening" effect
					Velocity -= (-DirToHook) * OutwardSpeed;
					
					// Recalculate normal and tangent (based on constrained velocity)
					NormalSpeed = FVector::DotProduct(Velocity, DirToHook);
					V_normal = DirToHook * NormalSpeed;
					V_tangent = Velocity - V_normal;
			}
	}

	// === Step 7: Compose final velocity ===
	FVector NewVelocity = NewNormal + V_tangent;

	// === Step 8: Clamp maximum velocity (prevent infinite acceleration) ===
	float VelocityMagnitude = NewVelocity.Size();
	if (VelocityMagnitude > HookMaxVelocity)
	{
			NewVelocity = NewVelocity.GetSafeNormal() * HookMaxVelocity;
	}

	// === Step 9: Apply velocity ===
	MoveComp->Velocity = NewVelocity;

	// Debug visualization
	#if !UE_BUILD_SHIPPING
	DrawDebugLine(GetWorld(), ActorLocation, HookPoint, FColor::Cyan, false, 0.0f, 0, 3.0f);
	#endif
}

void ATemporalDashCharacter::EndHook()
{
	bIsHooked = false;

	// Notify the hooked actor that we're releasing
	if (CurrentHookedActor.IsValid())
	{
		CurrentHookedActor->OnHookReleased(this);
		CurrentHookedActor = nullptr;
	}

	// Restore normal movement settings
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		// Restore gravity
		MoveComp->GravityScale = 1.0f;
		
		// Restore ground friction
		MoveComp->GroundFriction = 8.0f; // UE default
		MoveComp->BrakingDecelerationWalking = 2000.0f; // UE default
		
		// Return to walking mode (will auto-switch to falling if in air)
		MoveComp->SetMovementMode(MOVE_Walking);
	}
}