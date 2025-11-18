// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "TemporalDashCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A basic first person character
 */
UCLASS(abstract)
class ATemporalDashCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* MouseLookAction;
	
	/** Dash Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* DashAction;
	
	/** Hook Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* Hook;

public:
	ATemporalDashCharacter();

protected:

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	// --- Dash support ---
	/** Distance the dash should cover (used together with Duration to compute speed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash", meta=(AllowPrivateAccess="true", ClampMin = "0.0"))
	float DashDistance = 600.0f;

	/** How long the dash impulse lasts (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash", meta=(AllowPrivateAccess="true", ClampMin = "0.01"))
	float DashDuration = 0.2f;

	/** Cooldown between dashes (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash", meta=(AllowPrivateAccess="true", ClampMin = "0.0"))
	float DashCooldown = 1.0f;

	/** Whether the character is currently dashing */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dash", meta=(AllowPrivateAccess="true"))
	bool bIsDashing = false;

	// Timer handle for dash cooldown
	FTimerHandle DashCooldownHandle;

	// Runtime dash state for linear decay
	FVector DashDirection;
	float DashTimeRemaining = 0.f;
	float DashInitialSpeed = 0.f;

	// Input handler for the dash (bind to ETriggerEvent::Started)
	void DoDashStart(const FInputActionValue& ActionValue);

	// Internal helpers
	void PerformDash(const FVector& Direction);
	void EndDash();
	void ResetDashCooldown();


	// --- Hook Support ---
	/** Maximum range to detect hook points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hook", meta=(AllowPrivateAccess="true", ClampMin = "0.0"))
	float HookMaxRange = 5000.0f;

	/** Pull strength towards hook point (higher = faster pull) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hook", meta=(AllowPrivateAccess="true", ClampMin = "0.0"))
	float HookPullStrength = 3000.0f;

	/** How much player input affects tangential movement during hook (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hook", meta=(AllowPrivateAccess="true", ClampMin = "0.0", ClampMax = "1.0"))
	float HookSteeringInfluence = 0.3f;

	/** Minimum distance to auto-detach from hook point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hook", meta=(AllowPrivateAccess="true", ClampMin = "0.0"))
	float HookMinDetachDistance = 150.0f;

	/** Maximum velocity magnitude during hook (prevents infinite acceleration) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hook", meta=(AllowPrivateAccess="true", ClampMin = "0.0"))
	float HookMaxVelocity = 4000.0f;

	/** Whether the character is currently hooked */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hook", meta=(AllowPrivateAccess="true"))
	bool bIsHooked = false;

	// Runtime hook state
	FVector HookPoint;
	float HookMaxRopeLength = 0.f;
	FVector LastMovementInput;  // Cache player input direction

	// Input handlers
	void DoHookStart(const FInputActionValue& ActionValue);
	void DoHookEnd(const FInputActionValue& ActionValue);

	// Internal helpers
	bool FindHookPoint(FVector& OutHitLocation);
	void PerformHook();
	void UpdateHookMovement(float DeltaTime);
	void EndHook();

protected:

	/** Called when the game starts or when spawned */
	virtual void BeginPlay() override;

	/** Called every frame */
	virtual void Tick(float DeltaTime) override;

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	
	// --- Double-jump support ---
	/** Maximum number of jumps allowed before landing (set to 2 for double-jump) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta=(AllowPrivateAccess="true"))
	int32 MaxJumpCount;

	/** Current number of performed jumps since last grounded */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta=(AllowPrivateAccess="true"))
	int32 JumpCount;

	/** Upwards boost applied when performing an extra (mid-air) jump */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta=(AllowPrivateAccess="true"))
	float SecondJumpStrength;

	/** Reset jump counter when landing */
	virtual void Landed(const FHitResult& Hit) override;
	

public:

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

};

