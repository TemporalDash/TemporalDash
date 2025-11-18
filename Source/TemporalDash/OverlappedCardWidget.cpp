// OverlappedCardWidget.cpp

#include "OverlappedCardWidget.h"
#include "Components/CanvasPanelSlot.h"

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

    // Let Blueprint fill visuals (icon, texture, etc.)
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

    for (int32 i = 0; i < N; ++i)
    {
        if (HighlightIndex == INDEX_NONE)
        {
            TargetPositions[i] = i * (CardWidth - Overlap);
        }
        else if (i <= HighlightIndex)
        {
            TargetPositions[i] = i * (CardWidth - Overlap);
        }
        else
        {
            float HighlightOffset = (CardWidth - HighlightOverlap) - (CardWidth - Overlap);
            TargetPositions[i] = i * (CardWidth - Overlap) + HighlightOffset;
        }
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

        if (i == HighlightIndex)
            CanvasSlot->SetZOrder(999);
        else
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
