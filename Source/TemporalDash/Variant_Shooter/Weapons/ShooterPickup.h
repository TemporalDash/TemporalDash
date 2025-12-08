// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "Engine/DataAsset.h"
#include "ShooterPickup.generated.h"

class USphereComponent;
class UPrimitiveComponent;
class AShooterWeapon;

UENUM(BlueprintType)
enum class EPickupType : uint8
{
	Weapon UMETA(DisplayName = "Weapon"),
	Skill  UMETA(DisplayName = "Skill")
};

/**
 *  Holds information about a type of weapon pickup
 */
USTRUCT(BlueprintType)
struct FWeaponTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Mesh to display on the pickup */
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UStaticMesh> StaticMesh;

	/** Weapon class to grant on pickup */
	UPROPERTY(EditAnywhere)
	TSubclassOf<AShooterWeapon> WeaponToSpawn;
};


/**
 *  Simple shooter game weapon pickup
 */
UCLASS(abstract)
class TEMPORALDASH_API AShooterPickup : public AActor
{
	GENERATED_BODY()

	/** Collision sphere */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* SphereCollision;

	/** Weapon pickup mesh. Its mesh asset is set from the weapon data table */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;
	
protected:

	// [新增] 1. 核心数据资产变量
    // =========================================================
    /** 存放你的纯蓝图数据资产 (BP_CardData) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card Config")
    UPrimaryDataAsset* CardAsset;

    // ... (原来的 PickupCategory / WeaponType / SkillCardToGive 可以删掉或者留着不用) ...

    // =========================================================
    // [修改] 2. 暴露 WeaponClass 给蓝图
    // =========================================================
    /** * 原本这里只是 TSubclassOf<AShooterWeapon> WeaponClass;
     * 我们加上 UPROPERTY(BlueprintReadWrite)，这样你在蓝图的 Construction Script
     * 就能把 CardData 里的 WeaponClass 塞进这个变量里。
     */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Pickup Logic") 
	TSubclassOf<AShooterWeapon> WeaponClass;
	
	/** Time to wait before respawning this pickup */
	UPROPERTY(EditAnywhere, Category="Pickup", meta = (ClampMin = 0, ClampMax = 120, Units = "s"))
	float RespawnTime = 4.0f;

	/** Timer to respawn the pickup */
	FTimerHandle RespawnTimer;


public:	
	
	/** Constructor */
	AShooterPickup();

protected:

	/** Native construction script */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Gameplay Initialization*/
	virtual void BeginPlay() override;

	/** Gameplay cleanup */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Handles collision overlap */
	UFUNCTION()
	virtual void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

protected:

	/** Called when it's time to respawn this pickup */
	UFUNCTION(BlueprintCallable, Category = "Pickup Logic")
	void RespawnPickup();

	/** Passes control to Blueprint to animate the pickup respawn. Should end by calling FinishRespawn */
	UFUNCTION(BlueprintImplementableEvent, Category="Pickup", meta = (DisplayName = "OnRespawn"))
	void BP_OnRespawn();

	/** Enables this pickup after respawning */
	UFUNCTION(BlueprintCallable, Category="Pickup")
	void FinishRespawn();
};
