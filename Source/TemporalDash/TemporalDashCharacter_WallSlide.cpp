#include "TemporalDash.h"
#include "TemporalDashCharacter.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "SlideableWallComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

void ATemporalDashCharacter::StartWallSliding(const USlideableWallComponent* Wall)
{
    if (!Wall) return;
    GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("StartWallSliding Called!"));
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
    GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("StopWallSliding Called!"));
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
    float TargetRoll = (DotResult > 0) ? -WallRunCameraRoll : WallRunCameraRoll;

    SetCameraRoll(TargetRoll);
}

void ATemporalDashCharacter::SetCameraRoll(float TargetRoll)
{
    AController* MyController = GetController();
    if (MyController)
    {
        FRotator ControlRot = MyController->GetControlRotation();

        // Smoothly interpolate the Roll
        // Note: ControlRotation usually doesn't use Roll, so this is a visual effect
        float NewRoll = FMath::FInterpTo(ControlRot.Roll, TargetRoll, GetWorld()->GetDeltaSeconds(), 10.f);

        ControlRot.Roll = NewRoll;
        MyController->SetControlRotation(ControlRot);
    }
}
