// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MovementCableActor.h"
#include "FloatModificatorContextV1.h"
#include "Camera/CameraShakeBase.h"
#include "AgressiveMovementComponent.generated.h"
/**
 * 
 */

UENUM(Blueprintable)
enum class EAgressiveMoveMode : uint8
{
	None,
	Slide,
	RunOnWall,
	Run
};
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FReadActorDestroyed, AActor, OnDestroyed, AActor*, DestroyedActor);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEndStamina);



UCLASS()
class MOVEMENTPLUGIN_API UAgressiveMovementComponent : public UCharacterMovementComponent
{
	friend class UFloatModificatorContext;

	GENERATED_BODY()

	UAgressiveMovementComponent();
	
public:

	UPROPERTY()
	bool Debug = true;

	UPROPERTY(BlueprintReadOnly)
	TArray<EAgressiveMoveMode> AgressiveMoveMode = { EAgressiveMoveMode::None };

	UPROPERTY(BlueprintReadWrite,EditAnywhere)
	float Stamina = 100;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MaxStamina = 100;

	UPROPERTY(BlueprintReadOnly, Transient)
	UFloatModificatorContext* StaminaModificator;

	UFUNCTION(BlueprintCallable)
	bool CanTakeStamina(float StaminaTaken = 20);

	UFUNCTION(BlueprintCallable)
	float TakeStamina(float StaminaTaken = 20);

	UFUNCTION()
	void CheckStamina();

	UFUNCTION()
	void TickCalculateStamina(float DeltaTime);

	UPROPERTY(BlueprintCallable)
	FEndStamina EndStaminaDelegate;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float BaseAddStaminaValue = 20;

	UPROPERTY(BlueprintReadOnly,EditAnywhere)
	bool RemoveStaminaBaseAddWhenSpendStamina = true;

	UFUNCTION(BlueprintCallable)
	void AddStaminaModificator(float Value, FString Name);

	UFUNCTION(BlueprintCallable)
	void RemoveStaminaModificator(FString Name);



	//Core
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void BeginDestroy() override;

	UFUNCTION(BlueprintCallable)
	void AddMoveStatus(EAgressiveMoveMode NewAgressiveMoveMode);

	UFUNCTION(BlueprintCallable)
	void RemoveMoveStatus(EAgressiveMoveMode NewAgressiveMoveMode);

	//SpeedStart
	UPROPERTY(BlueprintReadOnly)
	float DefaultMaxWalkSpeed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float SpeedRunMultiply = 2;

	UPROPERTY(Transient)
    UFloatModificatorContext* SpeedModificator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UCameraShakeBase* WalkCameraShake;

	//RunStatus

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UCameraShakeBase* RunCameraShake;

	UPROPERTY()
	UFloatModificator* SpeedRunModificator;

	UFUNCTION(BlueprintCallable)
	void StartRunInput();

	UFUNCTION()
	void StartRun();

	UFUNCTION(BlueprintCallable)
	void EndRunInput();

	UFUNCTION()
	void EndRun();
	//EndRunStatus

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "SetMaxWalkSpeed"))
		void SetMaxWalkSpeed(float Speed)
	{
		DefaultMaxWalkSpeed = Speed;
	}
	//Speed End

	//Slide Start

	UPROPERTY(BlueprintReadWrite)
	float DefaultGroundFriction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GroundFrictionWhenSliding = 0.01;
	
	UFUNCTION(BlueprintCallable)
	void StartSlideInput();

	UFUNCTION()
	void StartSlide();

	UFUNCTION(BlueprintCallable)
	void EndSlideInput();

	UFUNCTION()
	void EndSlide();
	//Slide End
	
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

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float TakenJumpFromCruckStamina = 30;

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

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float TakenJumpFromWallStamina = 30;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "JumpFromWall"))
	void JumpFromWall();

	UFUNCTION(BlueprintCallable)
	void ReloadJump();


	//JummFromWallEnd
	
	//MoveOnWallStart



	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float DefaultStaminaExpence = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MinDotAngleToRunOnWall = -0.1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpeedRunOnWall = 300;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UCameraShakeBase* RunOnWalkCameraShake;


	UFUNCTION(BlueprintCallable)
	FVector GetMoveToWallVector();

	UFUNCTION()
	void MoveOnWallEvent();

	UFUNCTION(BlueprintCallable)
	void StartRunOnWallInput();

	UFUNCTION()
	void StartRunOnWall();

	UFUNCTION(BlueprintCallable)
	void EndRunOnWallInput();

	UFUNCTION()
	void EndRunOnWall();

	UPROPERTY(BlueprintReadWrite,EditAnywhere)
	float MinMoveOnWallVelocity = 100;

	UFUNCTION(BlueprintCallable)
	void CheckVelocityMoveOnWall();

	UFUNCTION(BlueprintCallable)
	void EndRunOnWallDirectly();

public:
	//MoveOnWallEnd

	//SprintMode


	//
	

	//MainInterface
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode);
	virtual void PhysFalling(float deltaTime, int32 Iterations);
	virtual void PhysWalking(float deltaTime, int32 Iterations);
};
