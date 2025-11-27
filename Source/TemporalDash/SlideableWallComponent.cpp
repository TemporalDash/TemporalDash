#include "SlideableWallComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"

// CRITICAL: Ensure this path matches where your character file actually lives
#include "ShooterCharacter.h"

USlideableWallComponent::USlideableWallComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.
	// We disable ticking for performance since a wall usually doesn't need per-frame logic.
	PrimaryComponentTick.bCanEverTick = false;

	// 1. Setup Detection Box
	DetectionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("DetectionBox"));
	DetectionBox->SetupAttachment(this); // Attach to the Root of this component
	DetectionBox->SetBoxExtent(FVector(50.f, 50.f, 50.f));

	// 'Trigger' profile is standard for things that detect overlaps but don't block movement
	DetectionBox->SetCollisionProfileName(TEXT("Trigger"));

	// 2. Setup Normal Indicator (Arrow)
	NormalIndicator = CreateDefaultSubobject<UArrowComponent>(TEXT("NormalIndicator"));
	NormalIndicator->SetupAttachment(this);
	NormalIndicator->ArrowColor = FColor::Cyan; // Cyan is easy to see against grey walls
	NormalIndicator->ArrowSize = 1.0f;
}

void USlideableWallComponent::BeginPlay()
{
	Super::BeginPlay();

	// Bind the Overlap Events dynamically
	if (DetectionBox)
	{
		DetectionBox->OnComponentBeginOverlap.AddDynamic(this, &USlideableWallComponent::OnBoxBeginOverlap);
		DetectionBox->OnComponentEndOverlap.AddDynamic(this, &USlideableWallComponent::OnBoxEndOverlap);
	}
}

FVector USlideableWallComponent::GetInteractionNormal() const
{
	if (NormalIndicator)
	{
		// Return the direction the arrow is pointing in world space
		return NormalIndicator->GetComponentRotation().Vector();
	}
	return GetForwardVector();
}

void USlideableWallComponent::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	// Check if OtherActor is valid and not this component's owner
	if (!OtherActor || OtherActor == GetOwner())
	{
		return;
	}
	// Attempt to cast to your specific character class
	ATemporalDashCharacter* DashChar = Cast<ATemporalDashCharacter>(OtherActor);

	if (DashChar)
	{
		// --- SUCCESS LOGIC HERE ---

		UE_LOG(LogTemp, Log, TEXT("Entered Wall Zone: %s"), *GetName());
		DashChar->StartWallSliding(this);
	}
}

void USlideableWallComponent::OnBoxEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor) return;
	// Cast again to make sure it was the character leaving, not something else
	ATemporalDashCharacter* DashChar = Cast<ATemporalDashCharacter>(OtherActor);

	if (DashChar)
	{
		// --- SUCCESS LOGIC HERE ---

		UE_LOG(LogTemp, Log, TEXT("Left Wall Zone: %s"), *GetName());
		DashChar->StopWallSliding();
	}
}