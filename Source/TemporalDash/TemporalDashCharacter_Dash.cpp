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

	// Check if DashAction is assigned
	if (!DashAction)
	{
		return;
	}

	// Try to bind DashAction if InputComponent is available and is an EnhancedInputComponent
	if (UInputComponent* IC = InputComponent)
	{
		if (UEnhancedInputComponent* Enhanced = Cast<UEnhancedInputComponent>(IC))
		{
			Enhanced->BindAction(DashAction, ETriggerEvent::Started, this, &ATemporalDashCharacter::DoDashStart);
		}
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
	DashInitialSpeed = DashDistance / DashDuration; // Speed at start of dash
	
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

	if (bIsDashing && DashTimeRemaining > 0.f)
	{
		// Linear decay: speed decreases from DashInitialSpeed to 0 over DashDuration
		float Alpha = DashTimeRemaining / DashDuration; // 1.0 at start, 0.0 at end
		float CurrentSpeed = DashInitialSpeed * Alpha;

		// Apply velocity in dash direction
		FVector DashVelocity = DashDirection * CurrentSpeed;
		
		// Preserve vertical velocity (jumping/falling)
		if (GetCharacterMovement())
		{
			FVector CurrentVelocity = GetCharacterMovement()->Velocity;
			DashVelocity.Z = CurrentVelocity.Z;
			GetCharacterMovement()->Velocity = DashVelocity;
		}

		// Decrease remaining time
		DashTimeRemaining -= DeltaTime;

		// End dash when time expires
		if (DashTimeRemaining <= 0.f)
		{
			EndDash();
		}
	}
}
