// Additional dash implementation for ATemporalDashCharacter
// Implements DoDashStart, PerformDash, EndDash, ResetDashCooldown

#include "TemporalDashCharacter.h"
#include "TemporalDash.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "EnhancedInputComponent.h"

void ATemporalDashCharacter::BeginPlay()
{
	Super::BeginPlay();

	// DEBUG: Check BeginPlay is called
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Magenta, TEXT("Character BeginPlay called"));

	// Try to bind actions if InputComponent is available and is an EnhancedInputComponent
	if (UInputComponent* IC = InputComponent)
	{
		if (UEnhancedInputComponent* Enhanced = Cast<UEnhancedInputComponent>(IC))
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Enhanced Input Component found!"));

			// Bind Dash action
			if (DashAction)
			{
				Enhanced->BindAction(DashAction, ETriggerEvent::Started, this, &ATemporalDashCharacter::DoDashStart);
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("Dash action bound!"));
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("WARNING: DashAction is NULL!"));
			}

			// Bind Hook actions
			if (Hook)
			{
				Enhanced->BindAction(Hook, ETriggerEvent::Started, this, &ATemporalDashCharacter::DoHookStart);
				Enhanced->BindAction(Hook, ETriggerEvent::Completed, this, &ATemporalDashCharacter::DoHookEnd);
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("Hook action bound!"));
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("WARNING: Hook is NULL!"));
			}
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("ERROR: Not Enhanced Input Component!"));
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("ERROR: No Input Component!"));
	}
}

void ATemporalDashCharacter::DoDashStart(const FInputActionValue& ActionValue)
{
	// Guards
	if (bIsDashing)
	{
		return;
	}
	
	if (!GetCharacterMovement())
	{
		return;
	}
	
	if (GetWorldTimerManager().IsTimerActive(DashCooldownHandle))
	{
		return;
	}

	// Determine direction: prefer last movement input, then controller forward, then actor forward
	FVector InputDir = GetLastMovementInputVector();
	if (InputDir.IsNearlyZero())
	{
		if (AController* C = GetController())
		{
			const FRotator ControlRot = C->GetControlRotation();
			InputDir = FRotationMatrix(ControlRot).GetScaledAxis(EAxis::X); // forward
		}
	}
	if (InputDir.IsNearlyZero())
	{
		InputDir = GetActorForwardVector();
	}

	PerformDash(InputDir);
}

void ATemporalDashCharacter::PerformDash(const FVector& Direction)
{
	FVector Dir = Direction.GetSafeNormal();
	if (Dir.IsNearlyZero()) 
	{
		Dir = GetActorForwardVector();
	}

	// Store dash parameters for Tick to apply
	DashDirection = FVector(Dir.X, Dir.Y, 0.f).GetSafeNormal(); // Horizontal only
	DashTimeRemaining = DashDuration;
	
	// Calculate target speed and velocity for smooth interpolation
	float TargetSpeed = DashDistance / DashDuration;
	DashTargetVelocity = DashDirection * TargetSpeed;
	DashInitialSpeed = TargetSpeed; // Keep for reference
	
	bIsDashing = true;

	// Store original movement settings to restore later
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		// Reduce friction and braking during dash for smoother feel
		Movement->GroundFriction = 0.f;
		Movement->BrakingDecelerationWalking = 0.f;
		Movement->BrakingFrictionFactor = 0.f;
		
		// Disable gravity during dash for consistent horizontal movement
		Movement->GravityScale = 0.f;
	}

	// Start cooldown immediately
	GetWorldTimerManager().SetTimer(DashCooldownHandle, this, &ATemporalDashCharacter::ResetDashCooldown, DashCooldown, false);
}

void ATemporalDashCharacter::EndDash()
{
	bIsDashing = false;
	DashTimeRemaining = 0.f;

	// Restore original movement settings
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->GroundFriction = 8.f; // UE default
		Movement->BrakingDecelerationWalking = 2000.f; // UE default
		Movement->BrakingFrictionFactor = 2.f; // UE default
		Movement->GravityScale = 1.f; // Restore gravity
	}
}

void ATemporalDashCharacter::ResetDashCooldown()
{
	// intentionally empty
}

void ATemporalDashCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update Dash
	if (bIsDashing && DashTimeRemaining > 0.f)
	{
		if (UCharacterMovementComponent* Movement = GetCharacterMovement())
		{
			FVector CurrentVelocity = Movement->Velocity;
			
			// Calculate progress: 0→1 over dash duration
			float Alpha = 1.f - (DashTimeRemaining / DashDuration);
			FVector TargetVel;
			
			// Phase 1: Accelerate to target velocity (first 20% of dash duration)
			// Phase 2: Maintain target velocity (middle 60%)
			// Phase 3: Decelerate to zero (last 20%)
			if (Alpha < 0.2f)
			{
				// Accelerate: 0→100% over first 20%
				float AccelAlpha = Alpha / 0.2f;
				TargetVel = DashTargetVelocity * AccelAlpha;
			}
			else if (Alpha < 0.8f)
			{
				// Maintain: 100% speed
				TargetVel = DashTargetVelocity;
			}
			else
			{
				// Decelerate: 100%→0% over last 20%
				float DecelAlpha = (1.f - Alpha) / 0.2f;
				TargetVel = DashTargetVelocity * DecelAlpha;
			}
			
			// Smoothly interpolate horizontal velocity toward target (like hook does)
			FVector NewVelocity = FMath::VInterpTo(
				FVector(CurrentVelocity.X, CurrentVelocity.Y, 0.f),
				TargetVel,
				DeltaTime,
				10.f // Interp speed (higher = snappier, lower = smoother)
			);
			
			// Preserve vertical velocity (gravity/jumping)
			NewVelocity.Z = CurrentVelocity.Z;
			Movement->Velocity = NewVelocity;
		}

		// Decrease remaining time
		DashTimeRemaining -= DeltaTime;

		// End dash when time expires
		if (DashTimeRemaining <= 0.f)
		{
			EndDash();
		}
	}

	// Update Hook
	if (bIsHooked)
	{
		UpdateHookMovement(DeltaTime);
	}
}
