#include "OverlappedCardWidget.h"
#include "Components/CanvasPanelSlot.h"
#include <Kismet/KismetSystemLibrary.h>

UOverlappedCardWidget::UOverlappedCardWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UOverlappedCardWidget::NativeConstruct()
{
    Super::NativeConstruct();
    CardWidgets.Empty();
    CardData.Empty();
    TargetPositions.Empty();
}

void UOverlappedCardWidget::SetCards(const TArray<UObject*>& NewCards)
{
    ClearCards();
    for (UObject* CardObj : NewCards)
    {
        InsertCard(CardWidgets.Num(), CardObj);
    }
}

void UOverlappedCardWidget::InsertCard(int32 Index, UObject* CardDataObject)
{
    if (!CardContainer || !CardWidgetClass || !CardDataObject)
        return;

    Index = FMath::Clamp(Index, 0, CardWidgets.Num());

    UUserWidget* NewCard = CreateWidget<UUserWidget>(GetWorld(), CardWidgetClass);
    if (!NewCard) return;

    UPanelSlot* NewSlot = CardContainer->AddChild(NewCard);
    CardWidgets.Insert(NewCard, Index);
    CardData.Insert(CardDataObject, Index);
    TargetPositions.Insert(0.f, Index);

    OnCardCreated(NewCard, CardDataObject);

    UpdateLayoutTargets();
}

void UOverlappedCardWidget::RemoveCard(int32 Index)
{
    if (Index < 0 || Index >= CardWidgets.Num() || !CardContainer)
        return;

    UUserWidget* Card = CardWidgets[Index];
    CardContainer->RemoveChild(Card);

    CardWidgets.RemoveAt(Index);
    CardData.RemoveAt(Index);
    TargetPositions.RemoveAt(Index);

    UpdateLayoutTargets();
}

void UOverlappedCardWidget::ClearCards()
{
    if (CardContainer)
        CardContainer->ClearChildren();

    CardWidgets.Empty();
    CardData.Empty();
    TargetPositions.Empty();
    HighlightIndex = INDEX_NONE;
}

void UOverlappedCardWidget::HighlightCard(int32 Index)
{
    if (Index < 0 || Index >= CardWidgets.Num())
        return;

    HighlightIndex = Index;
    UpdateLayoutTargets();
}

void UOverlappedCardWidget::UpdateLayoutTargets()
{
    const int32 N = CardWidgets.Num();
    TargetPositions.SetNum(N);

    if (N == 0 || !CardContainer)
        return;

    // Canvas (container) width
    float ContainerWidth = 0.f;
    if (UCanvasPanelSlot* ContainerSlot = Cast<UCanvasPanelSlot>(CardContainer->Slot))
    {
        ContainerWidth = ContainerSlot->GetSize().X;
    }

    // Fallback if container has no size
    if (ContainerWidth <= 0.f)
    {
        ContainerWidth = 1920.f; // default for testing
    }

    // Effective overlap range control
    const float MinOverlapRatio = 0.80f; // 80% overlap minimum
    const float MaxOverlapRatio = 1.00f; // 100% (no gap)
    const float HighlightOverlapRatio = 0.10f; // 10% overlap (less covered)

    const float HighlightOverlapPixels = CardWidth * HighlightOverlapRatio;

    // Compute ideal overlap based on container width
    // Total visual width (from leftmost to rightmost edge) should fit container

    float OverlapNormal = CardWidth; // start with fully overlapped (100%)
    if (N > 1)
    {
        float AvailableWidth = ContainerWidth - HighlightOverlapPixels;
        // Try to fit deck into container
        float RequiredOverlap = CardWidth - (AvailableWidth - (CardWidth - HighlightOverlapPixels) - CardWidth) / (float)(N - 1);
        OverlapNormal = FMath::Clamp(RequiredOverlap, CardWidth * MinOverlapRatio, CardWidth * MaxOverlapRatio);
    }

    float CurrentX = ContainerWidth; // - CardWidth; // rightmost card start

    for (int32 i = N - 1; i >= 0; --i)
    {

        if (i == HighlightIndex)
        {
            CurrentX -= (CardWidth - HighlightOverlapPixels);
        }
        else
        {
            CurrentX -= (CardWidth - OverlapNormal);
        }
        TargetPositions[i] = CurrentX;
    }
}

void UOverlappedCardWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    const int32 N = CardWidgets.Num();
    if (N == 0 || TargetPositions.Num() != N) return;

    for (int32 i = 0; i < N; ++i)
    {
        UUserWidget* Card = CardWidgets[i];
        if (!Card) continue;

        UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Card->Slot);
        if (!CanvasSlot) continue;

        FVector2D CurrentPos = CanvasSlot->GetPosition();
        FVector2D TargetPos(TargetPositions[i], CurrentPos.Y);
        FVector2D NewPos;
        NewPos.X = FMath::FInterpTo(CurrentPos.X, TargetPos.X, InDeltaTime, MoveSpeed);
        NewPos.Y = CurrentPos.Y;

        CanvasSlot->SetPosition(NewPos);
        CanvasSlot->SetZOrder(i);
    }
}

void UOverlappedCardWidget::SetCardPosition(UUserWidget* Card, float X)
{
    if (!Card) return;
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Card->Slot))
    {
        FVector2D Pos = CanvasSlot->GetPosition();
        CanvasSlot->SetPosition(FVector2D(X, Pos.Y));
    }
}
