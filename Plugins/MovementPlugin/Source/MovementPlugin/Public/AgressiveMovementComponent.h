// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MovementCableActor.h"
#include "FloatModificatorContextV1.h"
#include "Camera/CameraShakeBase.h"
#include "VisibilityTricks/VisibilityTrickObjects.h"
#include "Tricks/TrickObjects.h"
#include "CameraManagers/CameraManagers.h"
#include "AgressiveMovementComponent.generated.h"
/**
 * Небольшое объявление  о струтктуре плагина.

Плагин работате поверх текущих состояний movement Component почти не перезаписывая эвенты компонента ниже. 
Плагин содержит UAgressiveMoveMode. Это модификаторы передвижения, которые могут накладываться сразу рядами и перезаписывать друг друга при необходимости. 
Например, игрок одновременно может как бежать, так и бежать по стене. 

Все накладываемые статусы должны иметь следующую структуру - 
Структура Input. 
- первый обработчик любых запросов активации того или иного Agressive Status. Его вызывает клиент. Как правило он добавляет Agressive Mode в NotActiveAgressiveMoveMode. 
Эта группа содержит в себе все желаемые для активации MoveMode. 
Это позволяет например, забиндить на одну клавишу все MoveMode которые должны проигрываться при ускорении и обеспечить плавность их исполнения, 
так как в таком случае их активация будет задействована по мере необходимости. 
Похожая система например содержиться в Dying Light когда Shift позволяла игроку лазить, бегать, бегать по стенам и тд. 

Все NotActiveAgressiveMoveMode обрабатываются CheckActiveMoveMode. Эта функция вычисляет время перехода неактивной способности в активную. 
Затем все активированные способности обрабатываются в AddMoveStatus. Тут запускаются все Start функции статусов, а также решаются конфликты. 
Start функции выступают сугубо запуском статусов, а в AddMove Status прописываются решения конфликтов статусов.
Например, Climb должен отменить все остальные статусы при появлении. 

Каждый Status имеет функцию Status Tick. Она определяет то как статус будет обрабатываться в любом из Tick компонента или на таймере.
В Tick содержиться проверка валидности статуса при окружении,а также его изменение. 

Удаление статуса происходит в соответствующим tick стаутса либо при input от игрока. 
Удаление статуса происходит через функцию RemoveStatus, также отвечающей за обработку конфликтов, который вызывает функции Remove "Status". 
На момент 04.06.2023 удаление не содержит дополнительной оболочки запросов на удаление и удаляется без возврата в NotActiveAgressiveMoveMode. Я исправлю это позже.

Статусы не имеют отдельной имплементации например по разным UObject ввиду сложностей в их обработке. 
Возможно, ситуация измениться позже. 

В плагине прописаны разнообразные вспомогательные функции, например система трюков и шагов. 

В плагине сущетсвуют некоторые исключения, однако те несущественны, поэтому данное послание рекомендуется считать истинным в последней инстанции и придерживаться его.
 */

/*

A small announcement about the plugin structure.

The plugin works on top of the current states of the movement Component almost without overwriting the events of the component below.
The plugin contains a UAgressiveMoveMode. These are movement modifiers that can overlap in rows at once and overwrite each other if necessary. 
For example, a player can both run and run along a wall at the same time.

All imposed statuses should have the following structure -
Input structure.- the first handler of any activation requests of an Aggressive Status. It is called by the client. 
As a rule, it adds Aggressive Mode to NotActiveAgressiveMoveMode.
This group contains everything you want to activate MoveMode. 
This allows, for example, to bind on one key all movemodes that should be played during acceleration and ensure smoothness of their execution,
since in this case their activation will be activated as needed
. A similar system, for example, is contained in Dying Light when Shift allowed the player to climb, run, run on walls, etc.

All notactiveagressivemovemodes are handled by CheckActiveMoveMode. This function calculates the transition time of an inactive ability to an active one. 
Then all activated abilities are processed in AddMoveStatus.
All the Start functions of statuses are started here, as well as conflicts are resolved. 
Start functions are purely the launch of statuses, and AddMove Status prescribes solutions to status conflicts. 
For example, Climb should cancel all other statuses when it appears.

Each Status has a Status Tick function.
It determines how the status will be processed in any of the Tick component or on the timer. Tick contains validation of the status in the environment, as well as its change.

The status is deleted in the corresponding tick stouts or with input from the player.
The status is deleted via the RemoveStatus function, also responsible for conflict handling, which calls the Remove "Status" function. At the time of 06/04/2023, the deletion does not contain an additional shell of deletion requests and is deleted without returning to NotActiveAgressiveMoveMode. 
I'll fix it later.

Statuses do not have a separate implementation, for example, for different UObjects due to difficulties in processing them. 
Perhaps the situation will change later.

The plugin has a variety of auxiliary functions, such as a system of tricks and steps.

There are some exceptions in the plugin, but those are insignificant, so it is recommended that this message be considered true in the last instance and stick to it.
*/


//Система трюков делиться на Trick Object и Trick Visibility Object. Первый отвечает за физическое представление действия, а второй -  за графическое отображение
//The trick system is divided into Trick Object and Trick Visibility Object. The first one is responsible for the physical representation of the action, and the second one is responsible for the graphical representation

UENUM(Blueprintable)
enum class EAgressiveMoveMode : uint8
{
	None,
	Slide,
	RunOnWall,
	Run,
	Climb
};
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FReadActorDestroyed, AActor, OnDestroyed, AActor*, DestroyedActor);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEndStamina);


USTRUCT(Blueprintable)
struct FAgressiveMoveModeInput
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	EAgressiveMoveMode AgressiveMoveMode;

	UPROPERTY(BlueprintReadWrite)
	int Priority = 0;

	UPROPERTY(BlueprintReadWrite)
	float DefaultReloadTime = 0.2;

	UPROPERTY(BlueprintReadOnly)
	float Time = 0.2;

	void InitializeTimeAsDefaultTime()
	{
		Time = DefaultReloadTime;
	}
};

UCLASS()
class MOVEMENTPLUGIN_API UAgressiveMovementComponent : public UCharacterMovementComponent
{
	friend class UFloatModificatorContext;

	GENERATED_BODY()

	UAgressiveMovementComponent();
	
public:

	UPROPERTY(BlueprintReadWrite,EditAnywhere)
	bool Debug = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool DebugNotActiveMoveMode = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool DebugActiveMoveMode = true;

	UPROPERTY(BlueprintReadOnly)
	TArray<FAgressiveMoveModeInput> ActiveAgressiveMoveMode = {};

	UPROPERTY(BlueprintReadOnly)
	TArray<FAgressiveMoveModeInput> NotActiveAgressiveMoveMode = {};

	UFUNCTION(BlueprintCallable)
	void CheckActivateMoveMode(float DeltaTime);

	UFUNCTION(BlueprintCallable)
	void AddNotActiveAgressiveModeInput(FAgressiveMoveModeInput NewAgressiveMoveModeInput, float DirectTimeToActive = -1.0);

	UFUNCTION(BlueprintCallable)
	void RemoveActiveAgressiveModeInputByMode(EAgressiveMoveMode RemoveMoveMode);

	UFUNCTION(BlueprintCallable)
	void RemoveNotActiveAgressiveModeInputByMode(EAgressiveMoveMode RemoveMoveMode);


	UFUNCTION(BlueprintCallable)
	FAgressiveMoveModeInput FindActiveMoveModeInput(EAgressiveMoveMode MoveMode, bool &Sucsess);

	UFUNCTION(BlueprintCallable)
	FAgressiveMoveModeInput FindNotActiveMoveModeInput(EAgressiveMoveMode MoveMode, bool& Sucsess);
	
	UFUNCTION(BlueprintCallable)
	bool ContainsActiveMoveModeInput(EAgressiveMoveMode MoveMode);

	UFUNCTION(BlueprintCallable)
	bool ContainsNotActiveMoveModeInput(EAgressiveMoveMode MoveMode);

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
	UFloatModificator* AddStaminaModificator(float Value, FString Name);

	UFUNCTION(BlueprintCallable)
	void RemoveStaminaModificator(FString Name, bool Force = false);



	//Core
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void BeginDestroy() override;

	UFUNCTION(BlueprintCallable)
	void AddMoveStatus(FAgressiveMoveModeInput NewAgressiveMoveModeInput);

	UFUNCTION(BlueprintCallable)
	void RemoveMoveStatusByMode(EAgressiveMoveMode RemoveAgressiveMoveMode, bool SendToInput = true, float DirectReloadTime = -1.0);

	UFUNCTION(BlueprintCallable)
	void RemoveMoveStatusByInput(FAgressiveMoveModeInput RemoveAgressiveMoveModeInput, bool SendToInput = true, float DirectReloadTime = -1.0);

	UFUNCTION(BlueprintCallable)
	void ForceRemoveMoveStatusAndInputByMode(EAgressiveMoveMode RemoveAgressiveMoveMode);

protected:
	virtual void SwitchRemoveModeStatus(EAgressiveMoveMode RemoveAgressiveMoveMode);

public:
	UFUNCTION(BlueprintCallable)
	void RemoveAllMoveStatus(bool SendToInput = true);
	//End Core

	//Trick

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<TSubclassOf<UTrickObject>> StartTrickObjects = { UCrossbarJumpTrickObject::StaticClass() };

	UPROPERTY(BlueprintReadWrite)
	TArray<UTrickObject*> TrickObjects;

	UFUNCTION()
	UTrickObject* GetEnableTrick();

	UPROPERTY(BlueprintReadOnly)
	UTrickObject* ExecutedTrick;

	UFUNCTION()
	void TrickEnd(UTrickObject* FinisherTrickObject);

	UFUNCTION()
	void TickTrick(float DeltaTime);

	//TrickEnd

	//CameraManagerSystem
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<TSubclassOf<UDinamicCameraManager>> StartDynamicCameraManagerClasses;

	UPROPERTY(BlueprintReadWrite)
	TArray<UDinamicCameraManager*> DynamicCameraManagers;

	UFUNCTION()
	void TickCameraManagersUpdate(float DeltaTime);

	//SpeedStart
	UPROPERTY(BlueprintReadOnly)
	float DefaultMaxWalkSpeed;

	UPROPERTY(BlueprintReadOnly)
	float DefaultMaxAcceleration;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float SpeedRunMultiply = 2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float AccelerationRunMultiply = 2;

	UPROPERTY(Transient,BlueprintReadOnly)
    UFloatModificatorContext* AccelerationModificator;

	UPROPERTY(Transient, BlueprintReadOnly)
	UFloatModificatorContext* MaxWalkSpeedModificator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StepSensorEvent|Shake")
	TSubclassOf<UCameraShakeBase> WalkCameraShake;

	//SoundStep
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StepSensorEvent|Sound")
	USoundBase* SoundWalkStep;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StepSensorEvent")
	bool PlaySensorEvents = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StepSensorEvent|Sound")
	bool PlayMoveSounds = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StepSensorEvent|Shake")
	bool PlayCameraShakes = true;

	UFUNCTION()
	void PlayStepTick(float DeltaTime);

	UPROPERTY(BlueprintReadOnly, Category = "StepSensorEvent")
	float TickTimePlayStep = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StepSensorEvent")
	UCurveFloat* VelocityToDelayPerStep;
	//SoundStepEnd

	//RunStatus
	UPROPERTY(EditAnywhere, BlueprintGetter = GetEnableRun, BlueprintSetter = SetEnableRun)
	bool EnableRun = true;

	UFUNCTION(BlueprintSetter)
	void SetEnableRun(bool NewEnableRun);

	UFUNCTION(BlueprintGetter)
	bool GetEnableRun();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StepSensorEvent|Shake")
	TSubclassOf<UCameraShakeBase> RunCameraShake;

	UPROPERTY(BlueprintReadOnly)
	UFloatModificator* RunAccelerationModification;

	UPROPERTY(BlueprintReadOnly)
	UFloatModificator* RunMaxSpeedModification;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StepSensorEvent|Sound")
	USoundBase* SoundRunStep;

	UFUNCTION(BlueprintCallable)
	void StartRunInput();

	UFUNCTION()
	void StartRun();

	UFUNCTION()
	void TickRun();

	UPROPERTY(BlueprintReadOnly)
	UFloatModificator* RunStaminaModificator;

	UPROPERTY(BlueprintReadWrite)
	float LowStaminaRunValue = -10;

	UFUNCTION(BlueprintCallable)
	void EndRunInput();

	UFUNCTION()
	void EndRun();

	UFUNCTION()
	void LowStaminaEndRun();
	//EndRunStatus

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "SetMaxWalkSpeed"))
	void SetMaxWalkSpeed(float Speed)
	{
		DefaultMaxWalkSpeed = Speed;
	}
	//Speed End

	//Slide Start


	UPROPERTY(EditAnywhere, BlueprintGetter = GetEnableSlide, BlueprintSetter = SetEnableSlide)
	bool EnableSlide = true;

	UFUNCTION(BlueprintSetter)
	void SetEnableSlide(bool NewEnableSlide);

	UFUNCTION(BlueprintGetter)
	bool GetEnableSlide();


	UPROPERTY(BlueprintReadWrite)
	float DefaultGroundFriction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GroundFrictionWhenSliding = 0.05;

	UPROPERTY(BlueprintReadWrite)
	float DefailtGroundBraking;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GroundBrakingWhenSliding = 200;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinSpeedForSliding = 100;

	UPROPERTY(BlueprintReadWrite)
	float DefaultCharacterSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SlideCharacterSize = 40;

	UFUNCTION(BlueprintCallable)
	void SetSlideCharacterSize(float Size = 40);
	
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

	UPROPERTY(EditAnywhere, BlueprintGetter = GetEnableCruck, BlueprintSetter = SetEnableCruck)
	bool EnableCruck = true;

	UFUNCTION(BlueprintSetter)
	void SetEnableCruck(bool NewEnableCruck);

	UFUNCTION(BlueprintGetter)
	bool GetEnableCruck();

	UFUNCTION()
	FVector GetNormalizeCruckVector();

	UFUNCTION()
	void CruckWalkingTick(float DeltaTime);

	UFUNCTION()
	void CruckFlyTick(float DeltaTime);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LengthCruck = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StrengthPushCruck = 20000;

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

	UPROPERTY(BlueprintReadOnly)
	bool TraceSphereSucsess = false;

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

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool EnableDoubleJump = true;

	UPROPERTY(BlueprintReadWrite)
	float DotToIgnoteJumpWalls = 0.5;

	UFUNCTION(BlueprintCallable)
	void ReloadDoubleJump();

	UFUNCTION(BlueprintCallable)
	void JumpInSky();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool CanDoubleJump = true;



	//JummFromWallEnd
	
	//MoveOnWallStart

	UPROPERTY(EditAnywhere, BlueprintGetter = GetEnableMoveOnWall, BlueprintSetter = SetEnableMoveOnWall)
	bool EnableMoveOnWall = true;

	UFUNCTION(BlueprintSetter)
	void SetEnableMoveOnWall(bool NewEnableMoveOnWall);

	UFUNCTION(BlueprintGetter)
	bool GetEnableMoveOnWall();

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

	UFUNCTION()
	void LowStaminaEndRunOnWall();

	UPROPERTY(BlueprintReadWrite,EditAnywhere)
	float MinMoveOnWallVelocity = 50;

	UFUNCTION(BlueprintCallable)
	void CheckVelocityMoveOnWall();

	UFUNCTION(BlueprintCallable)
	void EndRunOnWallDirectly();

public:
	//MoveOnWallEnd

	//Parkor Start

	UPROPERTY(EditAnywhere, BlueprintGetter = GetEnableClimb, BlueprintSetter = SetEnableClimb)
	bool EnableClimb = true;

	UFUNCTION(BlueprintSetter)
	void SetEnableClimb(bool NewEnableClimb);

	UFUNCTION(BlueprintGetter)
	bool GetEnableClimb();

	UFUNCTION(BlueprintCallable)
	void StartClimbInput();

	UFUNCTION()
	void StartClimb();

	UFUNCTION()
	void TickClimb();

	UFUNCTION(BlueprintCallable)
	void EndClimbInput();

	UFUNCTION()
	void EndClimb();

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "BlueprintCallable for DEBUG, never use it"))
	bool FindClimbeLeadge();

	UFUNCTION()
	bool FindClimbByTrace(TArray<FHitResult>& OptimalResults);

	UFUNCTION()
	bool FindClimbByMeshPoligon(TArray<FHitResult>& OptimalResults);

	UFUNCTION()
	bool CheckInputClimb();

	UPROPERTY(BlueprintReadOnly)
	FHitResult WallInFrontClimbCheck;

	UPROPERTY(BlueprintReadOnly)
	FHitResult OptimalLeadge;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool EnableTraceByMesh = false;

	UFUNCTION(BlueprintCallable)
	virtual FVector GetForwardClimbVector();

	UPROPERTY(BlueprintReadWrite,EditAnywhere)
	int TraceClimbNumber = 3;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float TraceClimbAngle = 45;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MinDotAngleForClimb = 0.5;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MinDIstanceDepthToClimb = 10;

	UPROPERTY(BlueprintReadWrite)
	FVector DirectionalClimbVector = { 0, 0, 0 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "Same as LeadgeTraceSceneVectorDirect"))
	USceneComponent* LeadgeTraceSceneComponentDirect;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,meta = (ToolTip = "Use for trace to find optimal Leadge, when LeadgeTraceSceneComponentDirect not valid"))
	FVector LeadgeTraceLocalVectorDirect = {0,0,100};

	UFUNCTION()
	FVector GetLeadgeTraceDirect();

	UPROPERTY(BlueprintReadWrite)
	TArray<UClimbTrickObject*> ClimbTrickObjects;

	UFUNCTION()
	FVector GetOptimalClimbVectorDirection();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float FindLeadgeTraceLength = 100;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float FindLeadgeTraceRadius = 100;

	//ParkorEnd

	//MainInterface
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode);
	virtual void PhysFalling(float deltaTime, int32 Iterations);
	virtual void PhysWalking(float deltaTime, int32 Iterations);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AirCableFrenselCoifficient = 0.01;

	//HelperFunction
};
