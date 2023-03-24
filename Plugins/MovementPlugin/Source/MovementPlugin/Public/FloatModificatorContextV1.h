// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "FloatModificatorContextV1.generated.h"

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

	UPROPERTY()
	EModificatorOperation OperationType = EModificatorOperation::Add;

	UPROPERTY()
	float SelfValue = 0;

	UPROPERTY()
	FString Name = "NoneName";

	UFUNCTION()
	float ApplyModificator(float Value);
};

UCLASS(Blueprintable)
class MOVEMENTPLUGIN_API UFloatModificatorContext : public UObject
{
	GENERATED_BODY()
public:

	UPROPERTY()
	TArray<UFloatModificator*> Modificators;

	UFUNCTION()
	UFloatModificator* CreateNewModificator();

	UFUNCTION()
	float ApplyModificators(float Value);

	UFUNCTION()
	void RemoveModificatorByName(FString Name);
};
