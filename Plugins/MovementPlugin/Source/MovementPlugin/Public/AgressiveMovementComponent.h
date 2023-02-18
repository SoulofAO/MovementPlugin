// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MovementCableActor.h"
#include "AgressiveMovementComponent.generated.h"

/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FReadActorDestroyed, AActor, OnDestroyed, AActor*, DestroyedActor);


UCLASS(Blueprintable)
class MOVEMENTPLUGIN_API UAgressiveMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	UAgressiveMovementComponent();

public:

	//Hook Code
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	bool ActiveCruck = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LengthCruck = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StrengthPushCruck = 100;

	//
	UPROPERTY(BlueprintReadOnly)
	float DefaultMaxWalkSpeed;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "SpawnCruck"))
	void SetMaxWalkSpeed(float Speed)
	{
		DefaultMaxWalkSpeed = Speed;
	}
	//

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite)
	USceneComponent* HandComponent;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "SpawnCruck"))
	AMovementCableActor* SpawnCruck(FVector Location, TSubclassOf<AMovementCableActor> CableActorClass, bool CheckLocationDistance);
protected:
	UFUNCTION()
	AMovementCableActor* BaseSpawnCruck(FVector Location, TSubclassOf<AMovementCableActor> CableActorClass);
public:
	UPROPERTY(BlueprintReadOnly)
	TArray<AMovementCableActor*> Cabels;

	UFUNCTION()
	FVector GetApplyCruck();

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "JumpFromCruck"))
	void JumpFromAllCruck(float Strength, FVector AddVector);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "JumpFromCruck"))
	void JumpFromCruckByIndex(float Strength, FVector AddVector, int Index);

	//HookEnd

	//MoveOnWallStart

	UFUNCTION()
	FVector GetMoveToWallVector();

	//MoveOnWallEnd

	//MainInterface

	virtual void PhysFalling(float deltaTime, int32 Iterations);
	virtual void PhysWalking(float deltaTime, int32 Iterations);
};
