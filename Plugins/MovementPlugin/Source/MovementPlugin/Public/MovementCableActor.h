// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CableComponent.h"
#include "MovementCableActor.generated.h"

class UCableComponent;

UCLASS()
class MOVEMENTPLUGIN_API AMovementCableActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMovementCableActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void InitilizeHook();

	UFUNCTION(BlueprintCallable)
	void HookTick();

	UFUNCTION(BlueprintCallable)
	FVector GetLastPoint();

	UFUNCTION(BlueprintCallable)
	FVector GetSecondLastPoint();

	UFUNCTION(BlueprintCallable)
	void NewCableComponent(FVector HitLocation);

	UFUNCTION(BlueprintCallable)
	float GetRealCableLength();

	UFUNCTION(BlueprintCallable)
	void RemoveCableComponent(bool RemoveBeforeAdd);

	UFUNCTION(BlueprintCallable)
	FVector GetJumpUnitVector();

	UPROPERTY(BlueprintReadOnly)
	TArray<FVector> ArrayPoints;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MaxCableLength = 1000;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float CableStiffness = 0.3;

	UPROPERTY(BlueprintReadOnly,EditAnywhere)
	USceneComponent* SceneComponentHand;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	USceneComponent* SceneComponentHook;

	UPROPERTY(BlueprintReadOnly)
	TArray<UCableComponent*> CableComponents;
};
