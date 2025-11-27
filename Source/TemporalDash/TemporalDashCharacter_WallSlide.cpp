#include "TemporalDash.h"
#include "TemporalDashCharacter.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "SlideableWallComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "InputAction.h"

void ATemporalDashCharacter::Look(const FInputActionValue& Value)
{
    // 1. Get Raw Mouse Input (X = Yaw, Y = Pitch)
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // 2. THE FIX: Counter-Rotate the input by the current Roll.
        // If the screen is rolled 15 degrees, we rotate the input -15 degrees.
        // This realigns "Mouse Up" to "Screen Up" instead of "World Up".

        FVector2D RotatedInput = LookAxisVector;

        if (FMath::Abs(CurrentWallRunRoll) > 0.1f)
        {
            // Convert degrees to radians
            float RollRad = FMath::DegreesToRadians(CurrentWallRunRoll);

            // Standard 2D Rotation Matrix
            // NewX = X * cos(a) - Y * sin(a)
            // NewY = X * sin(a) + Y * cos(a)

            // Note: We might need to invert the angle depending on mouse setup,
            // generally rotating the INPUT opposite to the CAMERA corrects it.
            float CosA = FMath::Cos(-RollRad);
            float SinA = FMath::Sin(-RollRad);

            RotatedInput.X = LookAxisVector.X * CosA - LookAxisVector.Y * SinA;
            RotatedInput.Y = LookAxisVector.X * SinA + LookAxisVector.Y * CosA;
        }

        // 3. Apply the Rotated Input
        AddControllerYawInput(RotatedInput.X);
        AddControllerPitchInput(RotatedInput.Y);
    }
}

void ATemporalDashCharacter::StartWallSliding(const USlideableWallComponent* Wall)
{
    if (!Wall) return;
    // 1. State Setup
    bIsWallSliding = true;
    CurrentWallNormal = Wall->GetInteractionNormal();

    // 2. Physics Change
    // We switch to Flying so standard Gravity/Friction doesn't mess with our custom logic
    GetCharacterMovement()->SetMovementMode(MOVE_Flying);

    // Optional: Stop all current vertical momentum so we don't slide up/down immediately
    FVector CurrentVel = GetVelocity();
    GetCharacterMovement()->Velocity = FVector(CurrentVel.X, CurrentVel.Y, 0.0f);
}

void ATemporalDashCharacter::StopWallSliding()
{
    if (bIsWallSliding)
    {
        bIsWallSliding = false;

        // Reset Physics
        GetCharacterMovement()->SetMovementMode(MOVE_Falling);

        // Optional: Give a little "Pop" off the wall so we don't immediately re-trigger overlap
        // FVector PopOffForce = CurrentWallNormal * 300.f;
        // LaunchCharacter(PopOffForce, false, false);
    }
}

void ATemporalDashCharacter::UpdateWallSliding(float DeltaTime)
{
    // --- 1. MOVEMENT DIRECTION ---

    // Get the player's intended forward input from the Controller
    FVector ForwardInput = GetActorForwardVector();

    // MATH: Project that forward vector onto the wall plane.
    // This removes any part of the vector pointing INTO the wall, leaving only
    // the part that runs PARALLEL to the wall.
    FVector WallRunDirection = FVector::VectorPlaneProject(ForwardInput, CurrentWallNormal);
    WallRunDirection.Normalize();

    // --- 2. CALCULATE VELOCITY ---

    // Apply the wall run speed along the calculated direction
    FVector NewVelocity = WallRunDirection * WallRunSpeed;

    // Apply Custom "Wall Gravity" (Slide down slowly)
    NewVelocity.Z -= WallGravity;

    // IMPORTANT: Apply a tiny force INTO the wall.
    // If we move perfectly parallel, floating point errors might drift us 
    // 0.001 units away from the box, triggering "OnEndOverlap" instantly.
    // Pushing slightly (-Normal) ensures we stay inside the trigger volume.
    NewVelocity += (CurrentWallNormal * -100.f * DeltaTime);

    // Apply to component
    GetCharacterMovement()->Velocity = NewVelocity;

    // --- 3. ROTATION (Face the run direction) ---

    // Smoothly rotate character to face the direction we are running
    FRotator TargetRotation = WallRunDirection.Rotation();
    // Keep the Z (Pitch) upright so we don't look at the floor
    TargetRotation.Pitch = 0.0f;

    FRotator NewRot = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, 10.f);
    SetActorRotation(NewRot);

    // --- 4. CAMERA TILT ---

    // Calculate which side the wall is on relative to the player
    // Cross Product of Up and Normal gives the "Right" vector of the wall
    FVector WallRight = FVector::CrossProduct(FVector::UpVector, CurrentWallNormal);

    // Dot Product determines direction: >0 means wall is on Left, <0 means Right (approx)
    float DotResult = FVector::DotProduct(GetActorForwardVector(), WallRight);

    // If DotResult is positive, wall is on Left -> Roll Right (+Roll)
    // If DotResult is negative, wall is on Right -> Roll Left (-Roll)
    TargetWallRunRoll = (DotResult > 0) ? -WallRunCameraRoll : WallRunCameraRoll;
}
