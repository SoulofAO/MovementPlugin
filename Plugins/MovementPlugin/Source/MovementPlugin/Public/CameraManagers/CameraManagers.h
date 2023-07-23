// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CameraManagers.generated.h"

/**
 * 
 */

class UAgressiveMovementComponent;

UCLASS(Blueprintable)
class MOVEMENTPLUGIN_API UDinamicCameraManager : public UObject
{
	GENERATED_BODY()
public:

	virtual UWorld* GetWorld() const override;

	UPROPERTY(BlueprintReadOnly)
	UAgressiveMovementComponent* MovementComponent;

	UPROPERTY(BlueprintReadWrite)
	FRotator ApplyRotator;

	UFUNCTION(BlueprintNativeEvent)
	void ApplyCamera(float DeltaTime);

	virtual void ApplyCamera_Implementation(float DeltaTime);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	FRotator CalculateApplyRotator(float DeltaTime);

	virtual FRotator CalculateApplyRotator_Implementation(float DeltaTime);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool CheckApplyCamera();

	virtual bool CheckApplyCamera_Implementation();

};

UCLASS(Blueprintable)
class MOVEMENTPLUGIN_API UBaseDynamicCameraManager : public UDinamicCameraManager
{
	GENERATED_BODY()

		UBaseDynamicCameraManager();

public:
	virtual void ApplyCamera_Implementation(float DeltaTime);

	virtual FRotator CalculateApplyRotator_Implementation(float DeltaTime);

	virtual bool CheckApplyCamera_Implementation();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		UCurveFloat* VelocityToCameraRotation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		UCurveFloat* DotToCameraRotation;
};