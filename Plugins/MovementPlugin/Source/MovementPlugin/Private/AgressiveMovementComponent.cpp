// Fill out your copyright notice in the Description page of Project Settings.


#include "AgressiveMovementComponent.h"
#include "MovementCableActor.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "FloatModificatorContextV1.h"
#include "Camera/CameraShakeBase.h"
#include "Curves/CurveFloat.h"
#include "VisibilityTricks/VisibilityTrickObjects.h"
#include "Tricks/TrickObjects.h"
#include "CameraManagers/CameraManagers.h"

UAgressiveMovementComponent::UAgressiveMovementComponent()
{
	DefaultMaxWalkSpeed = MaxWalkSpeed;
	MaxAcceleration = 600;
}

void UAgressiveMovementComponent::CheckActiveMoveMode()
{
	for (int count = 0; count < NotActiveAgressiveMoveMode.Num(); count++)
	{
		FAgressiveMoveModeByPriority LocalAgressiveMoveMode = NotActiveAgressiveMoveMode[count];
		switch (LocalAgressiveMoveMode.AgressiveMoveMode)
		{
		case EAgressiveMoveMode::Run:
			if ((MovementMode == EMovementMode::MOVE_Walking) && !AgressiveMoveMode.Contains(EAgressiveMoveMode::Slide))
			{
				AddMoveStatus(EAgressiveMoveMode::Run);
				count--;
			}
			break;

		case EAgressiveMoveMode::Slide:
		{
			FVector NormilizeMultiplier = { 1.0,1.0,0 };
			FVector LocalNormaliaizeVelocity = GetCharacterOwner()->GetVelocity() * NormilizeMultiplier;
			if ((LocalNormaliaizeVelocity.Length() > MinSpeedForSliding) && MovementMode == EMovementMode::MOVE_Falling)
			{
				AddMoveStatus(EAgressiveMoveMode::Slide);
				count--;
			}
			break; 
		}
		case EAgressiveMoveMode::RunOnWall:
		{
			if ((MovementMode == EMovementMode::MOVE_Falling) && TraceSphereSucsess)
			{
				AddMoveStatus(EAgressiveMoveMode::RunOnWall);
				count--;
			}
			break;
		}

		case EAgressiveMoveMode::Climb:
		{
			if (CheckInputClimb()&&(MovementMode == EMovementMode::MOVE_Falling) && TraceSphereSucsess)
			{
				AddMoveStatus(EAgressiveMoveMode::Climb);
				count--;
			}
			break;
		};
		break;
		}
	}
}

void UAgressiveMovementComponent::AddNewAgressiveModeInput(EAgressiveMoveMode MoveMode, int Priority)
{
	int count = 0;
	for (FAgressiveMoveModeByPriority LocalMoveMode : NotActiveAgressiveMoveMode)
	{
		if (LocalMoveMode.Priority > Priority)
		{
			break;
		}
		count = count + 1;
	}
	FAgressiveMoveModeByPriority NewAgressiveMoveModeByPriority = { MoveMode,Priority };
	NotActiveAgressiveMoveMode.Insert(NewAgressiveMoveModeByPriority, count);
}

void UAgressiveMovementComponent::RemoveAgressiveModeInput(EAgressiveMoveMode RemoveMoveMode)
{
	int count = 0;
	for (FAgressiveMoveModeByPriority LocalAgressiveMoveMode : NotActiveAgressiveMoveMode)
	{
		if (LocalAgressiveMoveMode.AgressiveMoveMode == RemoveMoveMode)
		{
			NotActiveAgressiveMoveMode.RemoveAt(count);
			break;
		}
		count = count + 1;
	}
}

bool UAgressiveMovementComponent::CanTakeStamina(float StaminaTaken)
{
	return Stamina >= StaminaTaken;
}

float UAgressiveMovementComponent::TakeStamina(float StaminaTaken)
{
	Stamina = Stamina - StaminaTaken;
	Stamina = UKismetMathLibrary::Clamp(Stamina, 0, MaxStamina);
	return Stamina;
}

void UAgressiveMovementComponent::CheckStamina()
{
	if (Stamina <= 0)
	{
		EndStaminaDelegate.Broadcast();
	}
}

void UAgressiveMovementComponent::TickCalculateStamina(float DeltaTime)
{
	float LocalStamina = Stamina+StaminaModificator->ApplyModificators(0) * DeltaTime;
	Stamina = LocalStamina;
	Stamina = FMath::Clamp(Stamina, 0, MaxStamina);
	CheckStamina();
	if (Debug)
	{
		GEngine->AddOnScreenDebugMessage(-1,0.00,FColor::Black,"AddStamina"+FString::SanitizeFloat(StaminaModificator->ApplyModificators(1) * DeltaTime));
	}
}

void UAgressiveMovementComponent::AddMoveStatus(EAgressiveMoveMode NewAgressiveMoveMode)
{
	RemoveAgressiveModeInput(NewAgressiveMoveMode);
	TArray<EAgressiveMoveMode> OldAgressiveMode = AgressiveMoveMode;
	AgressiveMoveMode.Add(NewAgressiveMoveMode);
	{
	if (AgressiveMoveMode.Contains(NewAgressiveMoveMode))
		switch (NewAgressiveMoveMode)
		{
		case EAgressiveMoveMode::None:
			break;
		case EAgressiveMoveMode::Slide:
			RemoveMoveStatus(EAgressiveMoveMode::Run);
			if (AgressiveMoveMode.Contains(EAgressiveMoveMode::RunOnWall))
			{
				RemoveMoveStatus(EAgressiveMoveMode::RunOnWall);
			}
			StartSlide();
			break;
		case EAgressiveMoveMode::RunOnWall:
			StartRunOnWall();
			break;
		case EAgressiveMoveMode::Run:
			RemoveMoveStatus(EAgressiveMoveMode::Slide);
			StartRun();
			break;
		case EAgressiveMoveMode::Climb:
			RemoveAllMoveStatus();
			AgressiveMoveMode.Add(NewAgressiveMoveMode);
			StartClimb();
			break;
		default:
			break;
		}
	}
}

void UAgressiveMovementComponent::RemoveMoveStatus(EAgressiveMoveMode RemoveAgressiveMoveMode, bool SendToInput)
{
	TArray<EAgressiveMoveMode> OldAgressiveMode = AgressiveMoveMode;

	if (AgressiveMoveMode.Contains(RemoveAgressiveMoveMode))
	{
		switch (RemoveAgressiveMoveMode)
		{
		case EAgressiveMoveMode::None:
			break;
		case EAgressiveMoveMode::Slide:
			EndSlide();
			break;
		case EAgressiveMoveMode::RunOnWall:
			EndRunOnWall();
			break;
		case EAgressiveMoveMode::Run:
			EndRun();
			break;
		case EAgressiveMoveMode::Climb:
			EndClimb();
		default:
			break;
		}
	}
	RemoveAgressiveModeInput(RemoveAgressiveMoveMode);
	AgressiveMoveMode.Remove(RemoveAgressiveMoveMode);

}

void UAgressiveMovementComponent::RemoveAllMoveStatus(bool SendToInput)
{
	TArray<EAgressiveMoveMode> LOldAgressiveMode = AgressiveMoveMode;
	for (EAgressiveMoveMode LMoveMode : LOldAgressiveMode)
	{
		RemoveMoveStatus(LMoveMode, SendToInput);
	}
};

void UAgressiveMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	SpeedModificator = NewObject<UFloatModificatorContext>(this, "SpeedModificator");
	StaminaModificator = NewObject<UFloatModificatorContext>(this, "StaminaModificator");


	AddStaminaModificator(BaseAddStaminaValue, "BaseAddModificator");
	DefaultCharacterSize = GetCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	DefaultMaxAcceleration = MaxAcceleration;
	for (TSubclassOf<UTrickObject> LClassTrickObject : StartTrickObjects)
	{
		UTrickObject* LNewTrickObject = NewObject<UTrickObject>(this, LClassTrickObject);
		LNewTrickObject->MovementComponent = this;
		TrickObjects.Add(LNewTrickObject);
		if (Cast<UClimbTrickObject>(LNewTrickObject))
		{
			ClimbTrickObjects.Add(Cast<UClimbTrickObject>(LNewTrickObject));
		}
	}

	for (TSubclassOf<UDinamicCameraManager> LCameraManagerClass : StartDynamicCameraManagerClasses)
	{
		UDinamicCameraManager* LNewDynamicCameraManager = NewObject<UDinamicCameraManager>(this,LCameraManagerClass);
		DynamicCameraManagers.Add(LNewDynamicCameraManager);
	}

}

void UAgressiveMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TickCalculateStamina(DeltaTime);
	TickCameraManagersUpdate(DeltaTime);
	TraceForWalkChannel();
	CheckActiveMoveMode();
	TickTrick(DeltaTime);
	TickRun();
	TickClimb();

	//Debug
	if (Debug)
	{
		for (EAgressiveMoveMode MoveMode : AgressiveMoveMode)
		{
			FString LocalString = GetCharacterOwner()->GetName() + " " + UEnum::GetValueAsString(MoveMode);
			GEngine->AddOnScreenDebugMessage(-1, 0.0, FColor::Green, LocalString);
		}
	}
	if (ExecutedTrick)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0, FColor::Blue, ExecutedTrick->GetName());
	}
}
void UAgressiveMovementComponent::TrickEnd(UTrickObject* FinisherTrickObject)
{
	ExecutedTrick->FinishTrickDelegate.RemoveDynamic(this, &UAgressiveMovementComponent::TrickEnd);
	if (!ExecutedTrick->SaveTrickVisibilityObject)
	{
		ExecutedTrick->WeakTrickVisibilityObject = ExecutedTrick->TrickVisibilityObject;
		ExecutedTrick->TrickVisibilityObject = nullptr;
	}
	if (ExecutedTrick->TrickVisibilityObject)
	{
		ExecutedTrick->TrickVisibilityObject->EndTrick();
	}
	ExecutedTrick->StartReloadTimer();
	ExecutedTrick = nullptr;
}

void UAgressiveMovementComponent::BeginDestroy()
{
	Super::BeginDestroy();
	if (StaminaModificator)
	{
		StaminaModificator->Modificators.Empty();
	}
	if (SpeedModificator)
	{
		SpeedModificator->Modificators.Empty();
	}
}
void UAgressiveMovementComponent::SetEnableRun(bool NewEnableRun)
{
	if (!(NewEnableRun == EnableRun))
	{
		if (!NewEnableRun)
		{
			RemoveMoveStatus(EAgressiveMoveMode::Run);
		}
		EnableRun = NewEnableRun;
	}
}

bool UAgressiveMovementComponent::GetEnableRun()
{
	return EnableRun;
}


void UAgressiveMovementComponent::StartRun()
{
	if (!SpeedRunModificator)
	{
		SpeedRunModificator = SpeedModificator->CreateNewModificator();
		SpeedRunModificator->OperationType = EModificatorOperation::Multiply;
	}
	if (SpeedRunModificator)
	{
		SpeedRunModificator->SelfValue = SpeedRunMultiply;
	}
	MaxAcceleration = SpeedModificator->ApplyModificators(DefaultMaxAcceleration);
	RunStaminaModificator = AddStaminaModificator(LowStaminaRunValue,"Run");
	EndStaminaDelegate.AddDynamic(this, &UAgressiveMovementComponent::LowStaminaEndRun);
}

void UAgressiveMovementComponent::EndRun()
{
	EndStaminaDelegate.RemoveDynamic(this, &UAgressiveMovementComponent::LowStaminaEndRun);
	if (SpeedRunModificator)
	{
		SpeedRunModificator->SelfValue = 1.0;
	}
	MaxAcceleration = SpeedModificator->ApplyModificators(DefaultMaxAcceleration);
	RemoveStaminaModificator("Run");
	RunStaminaModificator = nullptr;
}

void UAgressiveMovementComponent::StartRunInput()
{
	if (GetEnableRun())
	{
		AddNewAgressiveModeInput(EAgressiveMoveMode::Run, 2);
	}
}

void UAgressiveMovementComponent::EndRunInput()
{
	RemoveMoveStatus(EAgressiveMoveMode::Run);
}

void UAgressiveMovementComponent::LowStaminaEndRun()
{
	RemoveMoveStatus(EAgressiveMoveMode::Run);
}

UTrickObject* UAgressiveMovementComponent::GetEnableTrick()
{
	for (UTrickObject* LTrickObject : TrickObjects)
	{
		if (LTrickObject->CheckTrickEnable()&&LTrickObject->Enable&&!LTrickObject->ReloadTimer.IsValid())
		{
			return LTrickObject;
		}
	}
	return nullptr;
}

UFloatModificator* UAgressiveMovementComponent::AddStaminaModificator(float Value, FString Name)
{
	if (StaminaModificator->FindModificator(Name))
	{
		RemoveStaminaModificator(Name, true);
	}
	UFloatModificator* FloatModificator = StaminaModificator->CreateNewModificator();
	FloatModificator->SelfValue = Value;
	FloatModificator->OperationType = EModificatorOperation::Add;
	FloatModificator->Name = Name;
	if (RemoveStaminaBaseAddWhenSpendStamina)
	{
		if (StaminaModificator->FindModificator("BaseAddModificator"))
		{
			bool HasSubstractModificator = false;
			for (UFloatModificator* LocalFloatModificator : StaminaModificator->Modificators)
			{
				if (LocalFloatModificator->SelfValue < 0)
				{
					HasSubstractModificator = true;
					break;
				}
			}
			if (HasSubstractModificator)
			{
				RemoveStaminaModificator("BaseAddModificator");
			}
		}
	}
	return FloatModificator;
}

void UAgressiveMovementComponent::RemoveStaminaModificator(FString Name, bool Force)
{
	StaminaModificator->RemoveModificatorByName(Name);
	if (RemoveStaminaBaseAddWhenSpendStamina)
	{
		if (!(StaminaModificator->FindModificator("BaseAddModificator")))
		{

			bool HasSubstractModificator = false;
			for (UFloatModificator* LocalFloatModificator : StaminaModificator->Modificators)
			{
				if (LocalFloatModificator->SelfValue < 0)
				{
					HasSubstractModificator = true;
					break;
				}
			}
			if (!HasSubstractModificator)
			{
				if (!Force)
				{
					AddStaminaModificator(BaseAddStaminaValue, "BaseAddModificator");
				}
			}
		}
	}
}

void UAgressiveMovementComponent::SetEnableSlide(bool NewEnableSlide)
{
	if (!EnableSlide == NewEnableSlide)
	{
		if(!NewEnableSlide)
		{
			RemoveMoveStatus(EAgressiveMoveMode::Slide);
		}
		EnableSlide = NewEnableSlide;
	}
}

bool UAgressiveMovementComponent::GetEnableSlide()
{
	return EnableSlide;
}

void UAgressiveMovementComponent::SetSlideCharacterSize(float Size)
{
	SlideCharacterSize = Size;
	if (AgressiveMoveMode.Contains(EAgressiveMoveMode::Slide))
	{
		GetCharacterOwner()->GetCapsuleComponent()->SetCapsuleSize(GetCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), Size);
	}
}

void UAgressiveMovementComponent::StartSlideInput()
{
	if (GetEnableSlide())
	{
		AddNewAgressiveModeInput(EAgressiveMoveMode::Slide, 0);
	}
}

void UAgressiveMovementComponent::StartSlide()
{
	DefaultGroundFriction = GroundFriction;
	DefailtGroundBraking = BrakingDecelerationWalking;
	GroundFriction = GroundFrictionWhenSliding;
	BrakingDecelerationWalking = GroundBrakingWhenSliding;
	GetCharacterOwner()->GetCapsuleComponent()->SetCapsuleSize(GetCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), SlideCharacterSize);
	UFloatModificator* FloatModificator = SpeedModificator->CreateNewModificator();
	FloatModificator->SelfValue = 0;
	FloatModificator->Name = "Slide";
	FloatModificator->OperationType = EModificatorOperation::Multiply;
	MaxAcceleration = SpeedModificator->ApplyModificators(DefaultMaxAcceleration);
}


void UAgressiveMovementComponent::EndSlideInput()
{
	RemoveMoveStatus(EAgressiveMoveMode::Slide);
}

void UAgressiveMovementComponent::TickRun()
{
	if (AgressiveMoveMode.Contains(EAgressiveMoveMode::Run))
	{
		if (UKismetMathLibrary::Dot_VectorVector(GetLastInputVector(), GetCharacterOwner()->GetActorForwardVector()) > 0)
		{
			RunStaminaModificator->SelfValue = LowStaminaRunValue;
			SpeedRunModificator->SelfValue = SpeedRunMultiply;
		}
		else
		{
			RunStaminaModificator->SelfValue = 0;
			SpeedRunModificator->SelfValue = 1.0;
		}
	}
}

void UAgressiveMovementComponent::EndSlide()
{
	GroundFriction = DefaultGroundFriction;
	BrakingDecelerationWalking = DefailtGroundBraking;
	GetCharacterOwner()->GetCapsuleComponent()->SetCapsuleSize(GetCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), DefaultCharacterSize);
	SpeedModificator->RemoveModificatorByName("Slide");
	MaxAcceleration = SpeedModificator->ApplyModificators(DefaultMaxAcceleration);
}


AMovementCableActor* UAgressiveMovementComponent::SpawnCruck(FVector Location, TSubclassOf<AMovementCableActor> CableActorClass, bool CheckLocationDistance)
{
	if (GetEnableCruck())
	{
		if (CheckLocationDistance)
		{
			if (UKismetMathLibrary::Vector_Distance(Location, HandComponent->GetComponentLocation()) < LengthCruck)
			{
				return BaseSpawnCruck(Location, CableActorClass);
			}
		}
		else
		{
			return BaseSpawnCruck(Location, CableActorClass);
		}
	}
	return nullptr;
};

AMovementCableActor* UAgressiveMovementComponent::BaseSpawnCruck(FVector Location, TSubclassOf<AMovementCableActor> CableActorClass)
{
	AMovementCableActor* NewCable = GetWorld()->SpawnActor<AMovementCableActor>(CableActorClass);
	NewCable->SceneComponentHook->SetWorldLocation(Location);
	NewCable->SceneComponentHand->SetWorldLocation(GetCharacterOwner()->GetActorLocation());
	FAttachmentTransformRules TransformRules = FAttachmentTransformRules::KeepWorldTransform;
	NewCable->SceneComponentHand->AttachToComponent(HandComponent, TransformRules);
	NewCable->MaxCableLength = LengthCruck;
	NewCable->InitilizeHook();
	Cabels.Add(NewCable);
	return NewCable;
}

FVector UAgressiveMovementComponent::GetApplyCruck()
{
	FVector MovementDirection = { 0, 0, 0 };

	for (AMovementCableActor* CableActor : Cabels)
	{
		FVector CableVector = { 0,0,0 };
		if (CableActor->GetRealCableLength() > CableActor->MaxCableLength)
		{
			CableVector = UKismetMathLibrary::GetDirectionUnitVector(CableActor->SceneComponentHand->GetComponentLocation(), CableActor->GetLastPoint());
			MovementDirection = MovementDirection + CableVector * CableActor->CableStiffness;
		}
	}
	return MovementDirection;
}
void UAgressiveMovementComponent::PlayStepTick(float DeltaTime)
{
	if (PlaySensorEvents)
	{
		if (VelocityToDelayPerStep)
		{
			if ((MovementMode != EMovementMode::MOVE_Falling) && (!AgressiveMoveMode.Contains(EAgressiveMoveMode::Slide)))
			{
				TickTimePlayStep = DeltaTime + TickTimePlayStep;
				float GetTimeToStep = VelocityToDelayPerStep->GetFloatValue(GetCharacterOwner()->GetVelocity().Length());
				if (TickTimePlayStep > GetTimeToStep)
				{
					if (PlayCameraShakes)
					{
						if (AgressiveMoveMode.Contains(EAgressiveMoveMode::Run))
						{
							if (RunCameraShake)
							{
								UGameplayStatics::GetPlayerController(this, 0)->PlayerCameraManager->StartCameraShake(RunCameraShake);
							}
						}
						else
						{
							if (WalkCameraShake)
							{
								UGameplayStatics::GetPlayerController(this, 0)->PlayerCameraManager->StartCameraShake(WalkCameraShake);
							}
						}
					}
					if (PlayMoveSounds)
					{
						USceneComponent* LocalAttachToComponent = GetCharacterOwner()->GetCapsuleComponent();
						FName LocalAttachPointName = FName("None");
						FVector LocalLocation = GetCharacterOwner()->GetCapsuleComponent()->GetComponentLocation() - (0, 0, GetCharacterOwner()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * -0.4);

						if (AgressiveMoveMode.Contains(EAgressiveMoveMode::Run))
						{
							USoundBase* LocalSound = SoundRunStep;
							if (LocalSound)
							{
								UGameplayStatics::SpawnSoundAttached(LocalSound, LocalAttachToComponent, LocalAttachPointName, LocalLocation, EAttachLocation::KeepWorldPosition, false, 1.0, 1.0, 0.0, nullptr, nullptr);
							}
						}
						else
						{
							USoundBase* LocalSound = SoundRunStep;
							if (LocalSound)
							{
								UGameplayStatics::SpawnSoundAttached(LocalSound, LocalAttachToComponent, LocalAttachPointName, LocalLocation, EAttachLocation::KeepWorldPosition, false, 1.0, 1.0, 0.0, nullptr, nullptr);
							}
						}
					}

					TickTimePlayStep = 0.0;
				}
			}
			else
			{
				TickTimePlayStep = 0.0;
			}
		}
	}
}
void UAgressiveMovementComponent::TickTrick(float DeltaTime)
{
	if (ExecutedTrick)
	{
		ExecutedTrick->Tick(DeltaTime);
	}
	if (ExecutedTrick)
	{
		if (ExecutedTrick->TrickVisibilityObject)
		{
			ExecutedTrick->TrickVisibilityObject->Tick(DeltaTime);
		}
	}
	if (!ExecutedTrick)
	{
		UTrickObject* LTrick = GetEnableTrick();
		if (LTrick)
		{
			if (!LTrick->TrickVisibilityObject)
			{
				if (LTrick->WeakTrickVisibilityObject.Get())
				{
					LTrick->TrickVisibilityObject = LTrick->WeakTrickVisibilityObject.Get();
					LTrick->WeakTrickVisibilityObject = nullptr;
				}
				else
				{
					if (LTrick->TrickVisibilityClass.Get())
					{
						UTrickVisibilityObject* LNewTrick = NewObject<UTrickVisibilityObject>(this, LTrick->TrickVisibilityClass);
						LTrick->TrickVisibilityObject = LNewTrick;
						LNewTrick->MovementComponent = this;
						LNewTrick->TrickObject = LTrick;
					}
				}
			}
			ExecutedTrick = LTrick;
			ExecutedTrick->FinishTrickDelegate.AddDynamic(this, &UAgressiveMovementComponent::TrickEnd);
			LTrick->UseTrick();
			if (LTrick->TrickVisibilityObject)
			{
				LTrick->TrickVisibilityObject->StartTrick();
			}
		}
	}
}
void UAgressiveMovementComponent::JumpFromAllCruck(float Strength, FVector AddVector)
{
	if (CanTakeStamina(TakenJumpFromCruckStamina))
	{
		TakeStamina(TakenJumpFromCruckStamina);
		FVector LaunchVector = GetNormalizeCruckVector() + AddVector;
		Launch(LaunchVector * Strength);
		for (AMovementCableActor* CableActor : Cabels)
		{
			CableActor->Destroy();
		}
		Cabels.Empty();
		MaxWalkSpeed = DefaultMaxWalkSpeed;
	}
}
void UAgressiveMovementComponent::JumpFromCruckByIndex(float Strength, FVector AddVector, int Index)
{
	if (Cabels.IsValidIndex(Index))
	{
		AMovementCableActor* CableActor = Cabels[Index];
		if (CableActor)
		{
			FVector LaunchVector = CableActor->GetJumpUnitVector() + AddVector;
			Launch(CableActor->GetJumpUnitVector() * Strength);
			CableActor->Destroy();
		}
	}
	MaxWalkSpeed = DefaultMaxWalkSpeed;
}
void UAgressiveMovementComponent::TickCameraManagersUpdate(float DeltaTime)
{
	for (UDinamicCameraManager* LDinamicCameraManager : DynamicCameraManagers)
	{
		if (LDinamicCameraManager->CheckApplyCamera())
		{
			LDinamicCameraManager->ApplyCamera(DeltaTime);
		}
	}
}
void UAgressiveMovementComponent::TraceForWalkChannel()
{
	TArray<AActor*> IgnoredActors;
	TArray<FHitResult> HitResults;
	FVector TraceEnd = GetOwner()->GetActorLocation();
	TraceEnd = { TraceEnd.X, TraceEnd.Y,TraceEnd.Z - 10 };

	if (Debug)
	{
		UKismetSystemLibrary::SphereTraceMulti(this, GetOwner()->GetActorLocation(), TraceEnd, GetCharacterOwner()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 10, ETraceTypeQuery::TraceTypeQuery1, false, IgnoredActors, EDrawDebugTrace::ForOneFrame, HitResults, true);
	}
	else
	{
		UKismetSystemLibrary::SphereTraceMulti(this, GetOwner()->GetActorLocation(), TraceEnd, GetCharacterOwner()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 10, ETraceTypeQuery::TraceTypeQuery1, false, IgnoredActors, EDrawDebugTrace::None, HitResults, true);
	}

	WallTraceHitResults = HitResults;
	TraceSphereSucsess = !WallTraceHitResults.IsEmpty();
}
FVector UAgressiveMovementComponent::GetJumpFromWallVector()
{
	FVector LJumpVector = {0,0,0};
	if (WallTraceHitResults.Num() > 0)
	{
		for (FHitResult LHitResult : WallTraceHitResults)
		{
			if (UKismetMathLibrary::Dot_VectorVector(GetLastInputVector(), UKismetMathLibrary::GetDirectionUnitVector(GetCharacterOwner()->GetActorLocation(), LHitResult.ImpactPoint)) < DotToIgnoteJumpWalls)
			{
				LJumpVector = LJumpVector + UKismetMathLibrary::GetDirectionUnitVector(LHitResult.ImpactPoint, GetCharacterOwner()->GetActorLocation()) * (1 - (UKismetMathLibrary::Vector_Distance(LHitResult.ImpactPoint, GetCharacterOwner()->GetActorLocation()) / (GetCharacterOwner()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 10)));
			}
		}
  		LJumpVector = LJumpVector + AppendVectorJumpFromWall;
		UKismetMathLibrary::Vector_Normalize(LJumpVector);
	}
	return LJumpVector;
}
void UAgressiveMovementComponent::JumpFromWall()
{
	if (CanTakeStamina(TakenJumpFromWallStamina))
	{
		if (!ReloadJumpTimeHandle.IsValid())
		{
			FVector JumpVector = GetJumpFromWallVector() * StrengthJumpFromWall;
			if (!(JumpVector.Length() == 0))
			{
				TakeStamina(TakenJumpFromWallStamina);
				Launch(Velocity + JumpVector);
				GetWorld()->GetTimerManager().SetTimer(ReloadJumpTimeHandle, this, &UAgressiveMovementComponent::ReloadJump, TimeReloadJumpFromWall, false);
				RemoveAllMoveStatus();
			}
			else
			{
				if (CanDoubleJump && EnableDoubleJump)
				{
					JumpInSky();
				}
			}
		}
	}
}

void UAgressiveMovementComponent::ReloadJump()
{
	ReloadJumpTimeHandle.Invalidate();
}

FVector UAgressiveMovementComponent::GetNormalizeCruckVector()
{
	FVector LMovementDirection = { 0,0,0 };
	for (AMovementCableActor* LCableActor : Cabels)
	{
		FVector LCableVector = { 0,0,0 };
		LCableVector = UKismetMathLibrary::GetDirectionUnitVector(LCableActor->SceneComponentHand->GetComponentLocation(), LCableActor->GetLastPoint());
		LMovementDirection = LMovementDirection + LCableVector;
	}
	UKismetMathLibrary::Vector_Normalize(LMovementDirection);
	return LMovementDirection;
}

void UAgressiveMovementComponent::SetEnableCruck(bool NewEnableCruck)
{
	if (!EnableCruck == NewEnableCruck)
	{
		if (EnableCruck == true && NewEnableCruck == false)
		{
			for (AMovementCableActor* LCable : Cabels)
			{
				JumpFromAllCruck(0, { 0,0,0 });
			}
		}
		EnableCruck = NewEnableCruck;
	}
}

void UAgressiveMovementComponent::ReloadDoubleJump()
{
	CanDoubleJump = true;
}

void UAgressiveMovementComponent::JumpInSky()
{
	CanDoubleJump = false;
	TakeStamina(TakenJumpFromWallStamina);
	FVector LLaunchVector = { Velocity.X,Velocity.Y, StrengthJumpFromWall };
 	Launch(LLaunchVector);
	GetWorld()->GetTimerManager().SetTimer(ReloadJumpTimeHandle, this, &UAgressiveMovementComponent::ReloadJump, TimeReloadJumpFromWall, false);
	RemoveAllMoveStatus();
}

void UAgressiveMovementComponent::CruckFlyTick(float DeltaTime)
{
	if (GetEnableCruck()&&MovementMode==EMovementMode::MOVE_Falling)
	{
		FVector CruckVector = GetApplyCruck();
		if (!CruckVector.IsNearlyZero())
		{
			Launch(Velocity + CruckVector);
		}
	}
}

void UAgressiveMovementComponent::CruckWalkingTick(float Delta)
{
	if (GetEnableCruck())
	{
		FVector CruckVector = GetApplyCruck();
		if (!CruckVector.IsNearlyZero())
		{
			if (!SpeedStrengthPushModificator)
			{
				SpeedStrengthPushModificator = SpeedModificator->CreateNewModificator();
				SpeedStrengthPushModificator->SelfValue = 1;
				SpeedStrengthPushModificator->OperationType = EModificatorOperation::Multiply;
			}
			if (SpeedStrengthPushModificator)
			{
				CruckVector = { CruckVector.X,CruckVector.Y,0 };
				UKismetMathLibrary::Vector_Normalize(CruckVector);
				float InputVectorDot = GetLastInputVector().Dot(GetNormalizeCruckVector());
				float DotScale = FMath::Clamp(UKismetMathLibrary::Dot_VectorVector((CruckVector / CruckVector.Size()), (Velocity / Velocity.Size())), 0, 1) + 0.5;
				SpeedStrengthPushModificator->SelfValue = DotScale * FMath::Clamp(1 - (CruckVector.Size() / StrengthPushCruck) * InputVectorDot, 0, 1);
				MaxWalkSpeed = SpeedModificator->ApplyModificators(MaxWalkSpeed);
				return;
			}
		}
	}
	if (SpeedStrengthPushModificator)
	{
		SpeedStrengthPushModificator->SelfValue = 1.0;
	}
	return;
}
void UAgressiveMovementComponent::SetEnableClimb(bool NewEnableClimb)
{
	if (!EnableClimb == NewEnableClimb)
	{
		if (!NewEnableClimb)
		{
			RemoveMoveStatus(EAgressiveMoveMode::Climb);
		}
	}
}

bool UAgressiveMovementComponent::GetEnableClimb()
{
	return EnableClimb;
}

void UAgressiveMovementComponent::SetEnableMoveOnWall(bool NewEnableMoveOnWall)
{
	if (!NewEnableMoveOnWall == EnableMoveOnWall)
	{
		if (!NewEnableMoveOnWall)
		{
			RemoveMoveStatus(EAgressiveMoveMode::RunOnWall);
		}
		EnableMoveOnWall = NewEnableMoveOnWall;
	}
}

bool UAgressiveMovementComponent::GetEnableMoveOnWall()
{
	return EnableMoveOnWall;
}

FVector UAgressiveMovementComponent::GetMoveToWallVector()
{
	FHitResult OptimalWall;
	float MinDistance = 10000;
	if (WallTraceHitResults.Num() > 0)
	{
		for (FHitResult HitResult : WallTraceHitResults)
		{
			FVector ImpactNormal = HitResult.ImpactNormal;
			ImpactNormal.Normalize();
			double MaxFloorDot = 1.0 - UKismetMathLibrary::DegSin(GetWalkableFloorAngle());
			if (UKismetMathLibrary::InRange_FloatFloat(UKismetMathLibrary::Dot_VectorVector(ImpactNormal, { 0,0,1 }), MinDotAngleToRunOnWall, MaxFloorDot))
			{
				if (MinDistance > UKismetMathLibrary::Vector_Distance(GetCharacterOwner()->GetActorLocation(), HitResult.ImpactPoint))
				{
					OptimalWall = HitResult;
					MinDistance = UKismetMathLibrary::Vector_Distance(GetCharacterOwner()->GetActorLocation(), HitResult.ImpactPoint);
				}
			};
		}
	}
	FVector ForwardVector = GetCharacterOwner()->GetActorForwardVector();
	FRotator Rotator = UKismetMathLibrary::NormalizedDeltaRotator(UKismetMathLibrary::Conv_VectorToRotator(ForwardVector), UKismetMathLibrary::Conv_VectorToRotator(OptimalWall.ImpactNormal));
	FVector VectorToMove = UKismetMathLibrary::Cross_VectorVector(OptimalWall.ImpactNormal, { 0,0,1 });
	FVector AppendVector = UKismetMathLibrary::Cross_VectorVector(OptimalWall.ImpactNormal, VectorToMove).GetAbs()* 0.5;
	if (Rotator.Yaw < 0)
	{
		return VectorToMove + AppendVector;
	}
	else
	{
		return VectorToMove*-1 + AppendVector;
	}
}

void UAgressiveMovementComponent::MoveOnWallEvent()
{
	CheckVelocityMoveOnWall();
	if (!TraceSphereSucsess)
	{
		RemoveMoveStatus(EAgressiveMoveMode::RunOnWall);
	}
	if (AgressiveMoveMode.Contains(EAgressiveMoveMode::RunOnWall))
	{
		FVector LaunchVector = GetMoveToWallVector() * SpeedRunOnWall;
		Launch(LaunchVector);
		if (Debug)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.0, FColor::Black, LaunchVector.ToString());
		}
	}
}
void UAgressiveMovementComponent::StartRunOnWallInput()
{
	if (GetEnableMoveOnWall())
	{
		AddNewAgressiveModeInput(EAgressiveMoveMode::RunOnWall, 3);
	}
};

void UAgressiveMovementComponent::StartRunOnWall()
{
	ReloadDoubleJump();
	AddStaminaModificator(-10, "RunOnWall");
	EndStaminaDelegate.AddDynamic(this, &UAgressiveMovementComponent::LowStaminaEndRunOnWall);
}

void UAgressiveMovementComponent::EndRunOnWallInput()
{
	RemoveMoveStatus(EAgressiveMoveMode::RunOnWall);
};

void UAgressiveMovementComponent::EndRunOnWall()
{
	EndStaminaDelegate.RemoveDynamic(this, &UAgressiveMovementComponent::LowStaminaEndRunOnWall);
	RemoveStaminaModificator("RunOnWall");
}

void UAgressiveMovementComponent::LowStaminaEndRunOnWall()
{
	RemoveMoveStatus(EAgressiveMoveMode::RunOnWall);
}

void UAgressiveMovementComponent::CheckVelocityMoveOnWall()
{
	if (GetCharacterOwner()->GetVelocity().Length() < MinMoveOnWallVelocity || GetLastInputVector().Length()==0.0)
	{
		RemoveMoveStatus(EAgressiveMoveMode::RunOnWall);
	}
}

void UAgressiveMovementComponent::EndRunOnWallDirectly()
{
	if (Debug)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Black, "TimerWallEnd");
	}
	JumpFromWall();
	RemoveMoveStatus(EAgressiveMoveMode::RunOnWall);
}

void UAgressiveMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if (MovementMode == EMovementMode::MOVE_Falling)
	{
		RemoveMoveStatus(EAgressiveMoveMode::Slide);
		RemoveMoveStatus(EAgressiveMoveMode::Run);
	}
	else if(MovementMode == EMovementMode::MOVE_Walking || MovementMode == EMovementMode::MOVE_NavWalking)
	{
		RemoveMoveStatus(EAgressiveMoveMode::RunOnWall);
		ReloadDoubleJump();
	};
}

void UAgressiveMovementComponent::StartClimb()
{
	SetMovementMode(MOVE_Flying);
	Launch({ 0, 0, 0 });
	ReloadDoubleJump();
}

void UAgressiveMovementComponent::TickClimb()
{
	if (AgressiveMoveMode.Contains(EAgressiveMoveMode::Climb))
	{
		if (!ExecutedTrick || (ExecutedTrick && !ClimbTrickObjects.Contains(ExecutedTrick)))
		{
			if (!CheckInputClimb())
			{
				RemoveMoveStatus(EAgressiveMoveMode::Climb);
			}
		}
	}
	
}

void UAgressiveMovementComponent::EndClimb()
{
	SetMovementMode(MOVE_Falling);
}

void UAgressiveMovementComponent::StartClimbInput()
{
	AddNewAgressiveModeInput(EAgressiveMoveMode::Climb, 0);
}

void UAgressiveMovementComponent::EndClimbInput()
{
	RemoveMoveStatus(EAgressiveMoveMode::Climb);
}



void UAgressiveMovementComponent::PhysFalling(float deltaTime, int32 Iterations)
{
	Super::PhysFalling(deltaTime, Iterations);

	CruckFlyTick(deltaTime);
	MaxWalkSpeed = DefaultMaxWalkSpeed;
	if (AgressiveMoveMode.Contains(EAgressiveMoveMode::RunOnWall))
	{
		MoveOnWallEvent();
	}
	
}

void UAgressiveMovementComponent::PhysWalking(float deltaTime, int32 Iterations)
{
	Super::PhysWalking(deltaTime, Iterations);
	CruckWalkingTick(deltaTime);

	if (GetCharacterOwner()->GetVelocity().Length() < MinSpeedForSliding)
	{
		RemoveMoveStatus(EAgressiveMoveMode::Slide);
	};

	PlayStepTick(deltaTime);
}

bool UAgressiveMovementComponent::CheckInputClimb()
{
	FHitResult HitResult;
	auto DebugTrace = [&]() {if (Debug) { return EDrawDebugTrace::ForOneFrame; }; return EDrawDebugTrace::None; };
	const TArray<AActor*> ActorIgnore;
	UKismetSystemLibrary::LineTraceSingle(this, GetCharacterOwner()->GetActorLocation(), GetCharacterOwner()->GetActorLocation() + GetForwardClimbVector() * FindLeadgeTraceLength, TraceTypeQuery1, false, ActorIgnore, DebugTrace(), HitResult, true, FLinearColor::Blue);
	if (HitResult.bBlockingHit)
	{
		WallInFrontClimbCheck = HitResult;
		return FindClimbeLeadge();
	}
	return false;
}


bool UAgressiveMovementComponent::GetEnableCruck()
{
	return EnableCruck;
}


bool UAgressiveMovementComponent::FindClimbeLeadge()
{
	OptimalLeadge = FHitResult();
	TArray<FHitResult> HitResults;
	FindClimbByTrace(HitResults);
	if (!(HitResults.Num() > 0))
	{
		if (EnableTraceByMesh)
		{
			FindClimbByMeshPoligon(HitResults);
		}
	}
	if (HitResults.Num() > 0)
	{
		float LMinDot = 100000;
		FHitResult LAnswerHitResult;
		for (FHitResult HitResult : HitResults)
		{
			if (LMinDot < FVector::DotProduct(HitResult.ImpactNormal, GetOptimalClimbVectorDirection()))
			{
				LMinDot = FVector::DotProduct(HitResult.ImpactNormal, GetOptimalClimbVectorDirection());
			};
			LAnswerHitResult = HitResult;
		}
		OptimalLeadge = LAnswerHitResult;
		return true;
	}
	return false;
}


bool UAgressiveMovementComponent::FindClimbByTrace(TArray<FHitResult>& OptimalResults)
{
	auto DebugTrace = [&]() {if (Debug) { return EDrawDebugTrace::ForOneFrame; }; return EDrawDebugTrace::None; };
	const TArray<AActor*> ActorIgnore;
	for (int Index = 0; Index < TraceClimbNumber; Index++)
	{
		FVector LocalVector = GetForwardClimbVector();
		LocalVector = UKismetMathLibrary::RotateAngleAxis(LocalVector, (Index - TraceClimbNumber / 2) * TraceClimbAngle, { 0,0,1 });
		LocalVector = LocalVector;
		UKismetMathLibrary::Vector_Normalize(LocalVector);
		FHitResult LHitResult;
		FVector EndPoint = GetLeadgeTraceDirect() + (LocalVector) * FindLeadgeTraceLength;
		FVector BackTrace = { 0,0,100 };
		UKismetSystemLibrary::LineTraceSingle(this, GetLeadgeTraceDirect(), EndPoint, TraceTypeQuery1, false, ActorIgnore, DebugTrace(), LHitResult, true,FLinearColor::Blue);
		if (!LHitResult.bBlockingHit)
		{
			UKismetSystemLibrary::LineTraceSingle(this, EndPoint, EndPoint - BackTrace, TraceTypeQuery1, false, ActorIgnore, DebugTrace(), LHitResult, true, FLinearColor::Blue);
		}
		if (LHitResult.bBlockingHit)
		{
			float Length = FVector::Distance(LHitResult.ImpactPoint, WallInFrontClimbCheck.Location);
			if (MinDIstanceDepthToClimb < Length)
			{
				if (FVector::DotProduct(LHitResult.ImpactNormal, { 0,0,1 }) > MinDotAngleForClimb)
				{
					OptimalResults.Add(LHitResult);
				};
			}
		}
	}
	return false;
}


bool UAgressiveMovementComponent::FindClimbByMeshPoligon(TArray<FHitResult>& OptimalResults)
{
	TArray<FHitResult> HitResults;
	auto DebugTrace = [&]() {if (Debug) { return EDrawDebugTrace::ForOneFrame; }; return EDrawDebugTrace::None; };
	const TArray<AActor*> ActorIgnore;
	UStaticMeshComponent* StaticMeshComponent;
	UKismetSystemLibrary::SphereTraceMulti(this, GetLeadgeTraceDirect(), GetLeadgeTraceDirect() + GetCharacterOwner()->GetActorForwardVector() * FindLeadgeTraceLength, FindLeadgeTraceRadius, TraceTypeQuery1, false, ActorIgnore, DebugTrace(), HitResults, false);
	for (FHitResult LHitResult : HitResults)
	{
		AActor* Actor = LHitResult.GetActor();
		StaticMeshComponent = Cast<UStaticMeshComponent>(LHitResult.Component);
		int32 StartIndex = StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[0].Sections[0].FirstIndex;
		FRawStaticIndexBuffer& StaticIndexBuffer = StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[0].IndexBuffer;
		FPositionVertexBuffer& VertexBuffer = StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[0].VertexBuffers.PositionVertexBuffer;
		int32 NumTriangles = StaticIndexBuffer.GetNumIndices() / 3;
		TArray< TArray<FVector>> Triangles;
		for (int32 i = 0; i < NumTriangles; i++)
		{
			int32 Index0 = StaticIndexBuffer.GetIndex(StartIndex + i * 3);
			int32 Index1 = StaticIndexBuffer.GetIndex(StartIndex + i * 3 + 1);
			int32 Index2 = StaticIndexBuffer.GetIndex(StartIndex + i * 3 + 2);

			FVector3f Vertex0 = VertexBuffer.VertexPosition(Index0);
			FVector3f Vertex1 = VertexBuffer.VertexPosition(Index1);
			FVector3f Vertex2 = VertexBuffer.VertexPosition(Index2);

			FVector Vertex0V = { Vertex0.X,Vertex0.Y,Vertex0.Z };
			FVector Vertex1V = { Vertex1.X,Vertex1.Y,Vertex1.Z };
			FVector Vertex2V = { Vertex2.X,Vertex2.Y,Vertex2.Z };
			TArray<FVector> NewTriangle;
			NewTriangle.Add(Vertex0V * StaticMeshComponent->GetComponentScale());
			NewTriangle.Add(Vertex1V * StaticMeshComponent->GetComponentScale());
			NewTriangle.Add(Vertex2V * StaticMeshComponent->GetComponentScale());
			FVector LEdge1 = Vertex1V - Vertex0V;
			FVector LEdge2 = Vertex2V - Vertex0V;

			FVector Normal = FVector::CrossProduct(LEdge1, LEdge2).GetSafeNormal();
			NewTriangle.Add(Normal);
			Triangles.Add(NewTriangle);
			// Do something with the vertices...
		}
		if (Debug)
		{
			for (TArray<FVector> Triangle : Triangles)
			{
				UKismetSystemLibrary::DrawDebugPoint(this, Triangle[0] + StaticMeshComponent->GetComponentLocation(), 50.0, FLinearColor::Black, 5.0);
				UKismetSystemLibrary::DrawDebugPoint(this, Triangle[1] + StaticMeshComponent->GetComponentLocation(), 50.0, FLinearColor::Black, 5.0);
				UKismetSystemLibrary::DrawDebugPoint(this, Triangle[2] + StaticMeshComponent->GetComponentLocation(), 50.0, FLinearColor::Black, 5.0);
			}
		}
		for (TArray<FVector> Triangle : Triangles)
		{
			FVector LCheckVector = (Triangle[0] + Triangle[1] + Triangle[2] / 3);
			if (FVector::Distance(LCheckVector, GetLeadgeTraceDirect()) < FindLeadgeTraceRadius && FVector::DotProduct(Triangle[4], { 0,0,1 }) > MinDotAngleForClimb)
			{
				FHitResult LNewHitResult;
				LNewHitResult = LHitResult;
				LNewHitResult.ImpactPoint = LCheckVector;
				LNewHitResult.Location = LCheckVector;
				OptimalResults.Add(LNewHitResult);
			}
		}
	}
	return false;
}

FVector UAgressiveMovementComponent::GetLeadgeTraceDirect()
{
	if (LeadgeTraceSceneComponentDirect)
	{
		return LeadgeTraceSceneComponentDirect->GetComponentLocation();
	}
	return GetCharacterOwner()->GetActorLocation() + LeadgeTraceLocalVectorDirect;
}

FVector UAgressiveMovementComponent::GetForwardClimbVector()
{
	return GetCharacterOwner()->GetController()->GetControlRotation().Vector();
}

FVector UAgressiveMovementComponent::GetOptimalClimbVectorDirection()
{
	if(DirectionalClimbVector.Length() == 0)
	{
		return GetCharacterOwner()->GetViewRotation().Vector();
	}
	return DirectionalClimbVector;
}



