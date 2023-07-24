// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VisibilityTrickObjects.generated.h"

/**
 * 
 */

class UAgressiveMovementComponent;
class UTrickObject;

UCLASS(Blueprintable, Abstract)
class MOVEMENTPLUGIN_API UTrickVisibilityObject : public UObject
{
	GENERATED_BODY()
public:

	virtual UWorld* GetWorld() const override;

	UPROPERTY(BlueprintReadOnly)
	UTrickObject* TrickObject;

	UPROPERTY(BlueprintReadOnly)
	UAgressiveMovementComponent* MovementComponent;

	UFUNCTION(BlueprintNativeEvent)
	void StartTrick();

	virtual void StartTrick_Implementation();

	UFUNCTION(BlueprintNativeEvent)
	void EndTrick();

	virtual void EndTrick_Implementation();

	UFUNCTION(BlueprintNativeEvent)
	void Tick(float DeltaTime);

	virtual void Tick_Implementation(float DeltaTime);

};

USTRUCT(Blueprintable)
struct FAnimClimbingStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UAnimMontage* AnimMontage;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float DistanceClimbStart = 10;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float NormalizeDistance = 20;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MultyplyPriority = 2.0;
};

UCLASS(Blueprintable, Abstract)
class MOVEMENTPLUGIN_API UMontageTrickVisibility : public UTrickVisibilityObject
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FAnimClimbingStruct> AnimClimbingEndMontage;

	virtual void StartTrick_Implementation();

	virtual void EndTrick_Implementation();

	UPROPERTY(BlueprintReadOnly)
	FAnimClimbingStruct OptimalAnimClimbingEndMontage;

	UFUNCTION()
	void MontagePlayEndBind(UAnimMontage* Montage, bool bInterrupted);
};

UCLASS(Blueprintable, Abstract)
class MOVEMENTPLUGIN_API UPlayOneMontageTrickVisibility : public UTrickVisibilityObject
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UAnimMontage* AnimMontage;

	virtual void StartTrick_Implementation();

};