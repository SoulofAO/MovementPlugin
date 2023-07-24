// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Curves/CurveVector.h"
#include "TrickObjects.generated.h"

/**
 * 
 */
class UAgressiveMovementComponent;
class UTrickVisibilityObject;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFinishTrick, UTrickObject*, FinisherTrickObject);

UCLASS(Blueprintable, Abstract)
class MOVEMENTPLUGIN_API UTrickObject : public UObject
{
	GENERATED_BODY()
public:

	virtual UWorld* GetWorld() const override;

	//TrickVisisbilityObject Start
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<UTrickVisibilityObject> TrickVisibilityClass;

	UPROPERTY(BlueprintReadOnly)
	UTrickVisibilityObject* TrickVisibilityObject;

	//»спользуетс€ дл€ того, что бы оптимизировать пам€ть. ѕри окончании Trick перенесет сюда его графическую часть и восстановит, если та останетс€ в сохранности. 
	//It is used to optimize memory. At the end, Trick will transfer its graphic part here and restore it if it remains intact.
	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<UTrickVisibilityObject> WeakTrickVisibilityObject;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool SaveTrickVisibilityObject = false;
	//TrickVisisbilityObject End

	UPROPERTY(BlueprintReadOnly)
	UAgressiveMovementComponent* MovementComponent;

	UFUNCTION(BlueprintNativeEvent)
	bool CheckTrickEnable();

	virtual bool CheckTrickEnable_Implementation();

	UFUNCTION(BlueprintNativeEvent)
	void UseTrick();

	virtual void UseTrick_Implementation();

	UFUNCTION(BlueprintNativeEvent)
	void Tick(float DeltaTick);

	virtual void Tick_Implementation(float DeltaTick);

	UPROPERTY(BlueprintCallable, meta = (ToolTip = "Call when want end Trick"))
	FFinishTrick FinishTrickDelegate;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool Enable = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float ReloadTime = 0.5;

	UPROPERTY(BlueprintReadOnly)
	FTimerHandle ReloadTimer;

	UFUNCTION(BlueprintNativeEvent)
	void StartReloadTimer();

	virtual void StartReloadTimer_Implementation();

	UFUNCTION()
	void EndReloadTimer();

};

UCLASS(Blueprintable, Abstract)
class MOVEMENTPLUGIN_API UClimbTrickObject : public UTrickObject
{
	GENERATED_BODY()
public:
	virtual bool CheckTrickEnable_Implementation();

	virtual void UseTrick_Implementation();

};


UCLASS(Blueprintable)
class MOVEMENTPLUGIN_API UCrossbarJumpTrickObject : public UClimbTrickObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	float TraceRadiusForward = 40;

	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	float TraceLengthForward = 100;

	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	float DirectStrength = 0;

	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	bool ApplyStrengthAsVelocity = true;

	virtual bool CheckTrickEnable_Implementation();

	virtual void UseTrick_Implementation();

};

UCLASS(Blueprintable)
class MOVEMENTPLUGIN_API UClimbingUpTrickObject : public UClimbTrickObject
{
	GENERATED_BODY()
public:

	virtual bool CheckTrickEnable_Implementation();

	virtual void UseTrick_Implementation();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float DirectTime = 1.0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UCurveVector* CurveAddLocation;

	UPROPERTY(BlueprintReadOnly)
	float TickTime = 0;

	UPROPERTY(BlueprintReadOnly)
	bool EnableMove = false;

	UPROPERTY(BlueprintReadOnly)
	FVector StartLocation;

	UPROPERTY(BlueprintReadOnly)
	FVector EndLocation;

	UFUNCTION(BlueprintNativeEvent)
	void MoveToPoint(FVector Point);

	virtual void MoveToPoint_Implementation(FVector Point);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void EndMovePoint();

	virtual void EndMovePoint_Implementation();

	virtual void Tick_Implementation(float DeltaTime);
};

UCLASS(Blueprintable)
class MOVEMENTPLUGIN_API UClimbingTopEndTrickObject : public UClimbingUpTrickObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MinDotForEndTop = 0.8;

	virtual bool CheckTrickEnable_Implementation();

	virtual void UseTrick_Implementation();
};
