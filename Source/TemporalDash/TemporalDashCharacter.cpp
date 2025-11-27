// Copyright Epic Games, Inc. All Rights Reserved.

#include "TemporalDashCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TemporalDash.h"

ATemporalDashCharacter::ATemporalDashCharacter()
{
	// Enable Tick for dash linear decay
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;

	// Double-jump defaults
	MaxJumpCount = 2;
	JumpCount = 0;
	SecondJumpStrength = 600.0f; // tweak to taste

	// Setup Camera roll for wall slide
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	// IMPORTANT: 
	// The Camera must follow the controller for Pitch/Yaw...
	FirstPersonCamera->bUsePawnControlRotation = true;
	// ...BUT we will offset the Roll manually, so standard Controller Roll must be zero.
}

void ATemporalDashCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ATemporalDashCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ATemporalDashCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATemporalDashCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATemporalDashCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ATemporalDashCharacter::LookInput);
	}
	else
	{
		UE_LOG(LogTemporalDash, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void ATemporalDashCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void ATemporalDashCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void ATemporalDashCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ATemporalDashCharacter::DoMove(float Right, float Forward)
{
	// Cache input for Hook steering
	LastMovementInput = FVector(Right, Forward, 0.0f);

	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void ATemporalDashCharacter::DoJumpStart()
{
	// If we're on the ground, perform the normal jump and count it
	if (GetCharacterMovement()->IsMovingOnGround())
	{
		Jump();
		JumpCount = 1;
	}
	else if (bIsWallSliding)
	{
		// 1. Unset Z component (Kill vertical momentum before launching)
		// We modify the movement component directly to ensure the physics engine knows Z is zeroed.
		FVector CurrentVel = GetCharacterMovement()->Velocity;
		CurrentVel.Z = 0.0f;
		GetCharacterMovement()->Velocity = CurrentVel;

		// 2. Calculate Launch Direction
		// A: Push AWAY from the wall (Critical to stop re-grabbing the wall instantly)
		FVector WallPush = CurrentWallNormal * SecondJumpStrength * 2 * sin(abs(TargetWallRunRoll));

		// B: Push UP (The jump height)
		// Note: If your camera is tilted, "Up" is technically diagonal, but FVector::UpVector 
		// is safer to ensure consistent jump height regardless of look angle.
		FVector UpPush = FVector::UpVector * SecondJumpStrength;

		// C: Push FORWARD (To keep flow)
		//FVector ForwardPush = GetActorForwardVector() * 400.f;

		// Combine them
		FVector LaunchDirection = WallPush + UpPush; // +ForwardPush;

		// 3. Stop Sliding State
		// We must call this BEFORE LaunchCharacter to reset MovementMode to MOVE_Falling
		// and stop the 'Tick' logic from forcing us back into the wall.
		StopWallSliding();

		// 4. Launch the Character
		// bXYOverride = false: Adds to existing horizontal momentum (feels fluid)
		// bZOverride = true: Sets exact vertical velocity (feels crisp)
		LaunchCharacter(LaunchDirection, false, true);

		// 5. Handle Jump Counts
		// Set to 1 so the system knows we have used a jump (allows for double jump if you have it)
		JumpCount = 1;
	}
	else
	{
		// If we still have jumps left, perform an in-air jump by applying an upward launch
		if (JumpCount < MaxJumpCount)
		{
			// Optionally zero any existing Z velocity to make jumps consistent
			FVector CurrentVel = GetCharacterMovement()->Velocity;
			CurrentVel.Z = 0.f;
			GetCharacterMovement()->Velocity = CurrentVel;

			// Launch upward for the second jump
			LaunchCharacter(FVector(0.f, 0.f, SecondJumpStrength), false, true);
			JumpCount++;
		}
	}
}

void ATemporalDashCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}

void ATemporalDashCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// Reset jump counter when touching the ground again
	JumpCount = 0;
}
