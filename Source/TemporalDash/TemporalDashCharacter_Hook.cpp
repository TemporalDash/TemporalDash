// Additional hook implementation for ATemporalDashCharacter
// Implements DoHookStart, DoHookEnd, FindHookPoint, PerformHook, UpdateHookMovement, EndHook

#include "TemporalDashCharacter.h"
#include "TemporalDash.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "InputActionValue.h"

void ATemporalDashCharacter::DoHookStart(const FInputActionValue& ActionValue)
{
	// DEBUG: Check if function is being called
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("DoHookStart called!"));

	if (bIsHooked)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("Already hooked, ignoring"));
		return;
	}

	FVector HitLocation;
	if (FindHookPoint(HitLocation))
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, TEXT("Hook point found!"));
		HookPoint = HitLocation;
		PerformHook();
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("No hook point found"));
	}
}

void ATemporalDashCharacter::DoHookEnd(const FInputActionValue& ActionValue)
{
	// DEBUG: Check if function is being called
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, TEXT("DoHookEnd called!"));

	if (bIsHooked)
	{
		EndHook();
	}
}

bool ATemporalDashCharacter::FindHookPoint(FVector& OutHitLocation)
{
	if (!FirstPersonCameraComponent)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("ERROR: No camera component!"));
		return false;
	}

	// shoot from camera
	FVector Start = FirstPersonCameraComponent->GetComponentLocation();
	FVector ForwardVector = FirstPersonCameraComponent->GetForwardVector();
	FVector End = Start + ForwardVector * HookMaxRange;

	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::White,
		FString::Printf(TEXT("Tracing from camera, Range: %.0f"), HookMaxRange));

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

	#if !UE_BUILD_SHIPPING
	DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, 2.0f, 0, 2.0f);
	if (bHit)
	{
			DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 25.0f, 12, FColor::Yellow, false, 2.0f);
	}
	#endif

	if (bHit)
	{
			OutHitLocation = HitResult.ImpactPoint;
			return true;
	}

	return false;
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

	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green,
		FString::Printf(TEXT("HOOK ACTIVATED! Rope Length: %.1f"), HookMaxRopeLength));
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
	DrawDebugString(GetWorld(), ActorLocation + FVector(0, 0, 100), 
			FString::Printf(TEXT("Speed: %.0f | Dist: %.0f"), VelocityMagnitude, DistanceToHook),
			nullptr, FColor::White, 0.0f);
	#endif
}

void ATemporalDashCharacter::EndHook()
{
	bIsHooked = false;

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