// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "OverlappedCardWidget.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class TEMPORALDASH_API UOverlappedCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    UOverlappedCardWidget(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UCanvasPanel* CardContainer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cards")
    TSubclassOf<UUserWidget> CardWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cards")
    float CardWidth = 200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cards")
    float Overlap = 160.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cards")
    float HighlightOverlap = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cards")
    float MoveSpeed = 10.f;

    UPROPERTY(BlueprintReadOnly, Category = "Cards")
    int32 HighlightIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "Cards")
    TArray<UUserWidget*> CardWidgets;

    UPROPERTY(BlueprintReadOnly, Category = "Cards")
    TArray<float> TargetPositions;

    /** Optional data references for each card (Blueprint can use them). */
    UPROPERTY(BlueprintReadOnly, Category = "Cards")
    TArray<UObject*> CardData;

    /** Called when a new card widget is created (for BP to set visuals). */
    UFUNCTION(BlueprintImplementableEvent, Category = "Cards")
    void OnCardCreated(UUserWidget* CardWidget, UObject* CardObject);

    /** Create cards from a data array. */
    UFUNCTION(BlueprintCallable, Category = "Cards")
    virtual void SetCards(const TArray<UObject*>& NewCards);

    /** Insert a new card widget at a given index. */
    UFUNCTION(BlueprintCallable, Category = "Cards")
    virtual int InsertCard(int32 Index, UObject* CardDataObject);

    /** Remove a card widget at a given index. */
    UFUNCTION(BlueprintCallable, Category = "Cards")
    virtual void RemoveCard(int32 Index);

    /** Clear all cards. */
    UFUNCTION(BlueprintCallable, Category = "Cards")
    virtual void ClearCards();

    /** Highlight one card. */
    UFUNCTION(BlueprintCallable, Category = "Cards")
    virtual UObject* HighlightCard(int32 Index);

    /** Recalculate layout positions. */
    UFUNCTION(BlueprintCallable, Category = "Cards")
    virtual void UpdateLayoutTargets();

protected:
    void SetCardPosition(UUserWidget* Card, float X);
};
