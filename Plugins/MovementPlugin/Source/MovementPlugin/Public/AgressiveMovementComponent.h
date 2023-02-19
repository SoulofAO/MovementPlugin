// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MovementCableActor.h"
#include "AgressiveMovementComponent.generated.h"

/**
 * 
 */
UENUM(Blueprintable)
enum class EModificatorOperation : uint8
{
	Add,
	Multiply
};

UCLASS(Blueprintable)
class MOVEMENTPLUGIN_API UFloatModificator : public UObject
{

	GENERATED_BODY()

public:
	EModificatorOperation OperationType = EModificatorOperation::Add;

	float SelfValue = 0;
	FString Name = "NoneName";

	float ApplyModificator(float Value)
	{
		switch (OperationType)
		{
		case (EModificatorOperation::Add):
			Value = SelfValue + Value;
			break;

		case (EModificatorOperation::Multiply):
			Value = Value * SelfValue;
			break;
		}

		return Value;
	}
};

UCLASS(Blueprintable)
class MOVEMENTPLUGIN_API UFloatModificatorContext : public UObject
{
	GENERATED_BODY()
public:
	TArray<UFloatModificator*> Modificators;

	UFloatModificator* CreateNewModificator()
	{
		UFloatModificator* Object = NewObject<UFloatModificator>(this);
		Modificators.Add(Object);
		return Object;
	}
	float ApplyModificators(float Value)
	{
		for (UFloatModificator* Modificator : Modificators)
		{
			Value = Modificator->ApplyModificator(Value);
		}
		return Value;
	}
	void RemoveModificatorByName(FString Name)
	{
		for (UFloatModificator* Modificator : Modificators)
		{
			if (Name == Modificator->Name)
			{
				Modificators.Remove(Modificator);
				break;
			}
		}
	}
};

DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FReadActorDestroyed, AActor, OnDestroyed, AActor*, DestroyedActor);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEndTimeMoveOnWall);


UCLASS(Blueprintable)
class MOVEMENTPLUGIN_API UAgressiveMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	UAgressiveMovementComponent();

public:

	//SpeedStart
	UPROPERTY(BlueprintReadOnly)
	float DefaultMaxWalkSpeed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float SpeedRunMultiply = 2;

	UPROPERTY(BlueprintReadOnly)
	UFloatModificatorContext* SpeedModificator;

	//RunStatus
	UFUNCTION(BlueprintCallable)
	void StartRun();

	UPROPERTY()
	UFloatModificator* SpeedRunModificator;

	UFUNCTION(BlueprintCallable)
	void EndRun();
	//EndRunStatus

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "SetMaxWalkSpeed"))
		void SetMaxWalkSpeed(float Speed)
	{
		DefaultMaxWalkSpeed = Speed;
	}
	//Speed End
	
	//Rolling Start
	UFUNCTION(BlueprintCallable)
	void StartRolling();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float RollingMultiplyFriction = 0.5;

	UPROPERTY(BlueprintReadOnly)
	float DefaultWalkFriction;

	UFUNCTION(BlueprintCallable)
	void  EndRolling();

	//Rolling End
	//Hook Code
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	bool ActiveCruck = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LengthCruck = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StrengthPushCruck = 100;

	UPROPERTY(BlueprintReadOnly)
	UFloatModificator* SpeedStrengthPushModificator;

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

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "JumpFromAllCruck"))
	void JumpFromAllCruck(float Strength, FVector AddVector);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "JumpFromCruckByIndex"))
	void JumpFromCruckByIndex(float Strength, FVector AddVector, int Index);

	//HookEnd
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float TraceWallCheckRadius = 200;

	UFUNCTION()
	void TraceForWalkChannel();

	UPROPERTY(BlueprintReadOnly)
	TArray<FHitResult> WallTraceHitResults;

	//JumpFromWallStart

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector AppendVectorJumpFromWall = { 0,0,1 };

	UFUNCTION(BlueprintCallable)
	FVector GetJumpFromWallVector();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float StrengthJumpFromWall = 600;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float TimeReloadJumpFromWall = 0.5;

	UPROPERTY(BlueprintReadWrite)
	FTimerHandle ReloadJumpTimeHandle;

	UFUNCTION(BlueprintCallable)
	void Jump();


	//JummFromWallEnd
	
	//MoveOnWallStart



	UPROPERTY(BlueprintReadOnly)
	bool RunOnWall;

	UPROPERTY()
	float DefaultTimeRunOnWall = 5;

	UPROPERTY()
	FTimerHandle MoveOnWallTimeHandle;

	UPROPERTY()
	FEndTimeMoveOnWall EndTimeMoveOnWallDelegate;

	UPROPERTY()
	FEndTimeMoveOnWall RestoreMoveOnWallDelegate;

	UFUNCTION()
	FVector GetMoveToWallVector();

	UFUNCTION()
	void StartRunOnWall();

	UFUNCTION()
	void EndRunOnWall();
	//MoveOnWallEnd

	//SprintMode


	//
	

	//MainInterface
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode);
	virtual void PhysFalling(float deltaTime, int32 Iterations);
	virtual void PhysWalking(float deltaTime, int32 Iterations);
};
