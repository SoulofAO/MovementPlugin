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
	MaxAcceleration = 600;
	FallingLateralFriction = 0.1;
}

void UAgressiveMovementComponent::CheckActivateMoveMode(float DeltaTime)
{
	for (int count = 0; count < NotActiveAgressiveMoveMode.Num(); count++)
	{
		FAgressiveMoveModeInput* LocalAgressiveMoveMode = &NotActiveAgressiveMoveMode[count];
		if(LocalAgressiveMoveMode->Time>0)
		{
			LocalAgressiveMoveMode->Time = LocalAgressiveMoveMode->Time - DeltaTime;
		}
		else
		{
			switch (LocalAgressiveMoveMode->AgressiveMoveMode)
			{
			case EAgressiveMoveMode::Run:
				if (CheckRun(true))
				{
					AddMoveStatus(*LocalAgressiveMoveMode);
					count--;
				}
				break;

			case EAgressiveMoveMode::Slide:
			{
				if(CheckSlide(true))
				{
					AddMoveStatus(*LocalAgressiveMoveMode);
					count--;
				}
				break;
			}
			case EAgressiveMoveMode::RunOnWall:
			{
				if (CheckRunOnWall(true))
				{
					AddMoveStatus(*LocalAgressiveMoveMode);
					count--;
				}
				break;
			}

			case EAgressiveMoveMode::Climb:
			{
				if (CheckInputClimb() && (MovementMode == EMovementMode::MOVE_Falling) && TraceSphereSucsess)
				{
					AddMoveStatus(*LocalAgressiveMoveMode);
					count--;
				}
				break;
			};
			break;
			}
		}
	}
}

void UAgressiveMovementComponent::AddNotActiveAgressiveModeInput(FAgressiveMoveModeInput NewAgressiveMoveModeInput, float DirectTimeToActive)
{
	if (!ContainsNotActiveMoveModeInput(NewAgressiveMoveModeInput.AgressiveMoveMode) && !ContainsActiveMoveModeInput(NewAgressiveMoveModeInput.AgressiveMoveMode))
	{
		if (DirectTimeToActive>=0.0)
		{
			NewAgressiveMoveModeInput.Time = DirectTimeToActive;
		}
		else
		{
			NewAgressiveMoveModeInput.InitializeTimeAsDefaultTime();
		}
		int count = 0;
		for (FAgressiveMoveModeInput LocalMoveMode : NotActiveAgressiveMoveMode)
		{
			if (LocalMoveMode.Priority > NewAgressiveMoveModeInput.Priority)
			{
				break;
			}
			count = count + 1;
		}
		NotActiveAgressiveMoveMode.Insert(NewAgressiveMoveModeInput, count);
	}
}

FAgressiveMoveModeInput UAgressiveMovementComponent::FindActiveMoveModeInput(EAgressiveMoveMode MoveMode, bool& Sucsess)
{
	for (FAgressiveMoveModeInput AgressiveMoveModeInput : ActiveAgressiveMoveMode)
	{
		if (AgressiveMoveModeInput.AgressiveMoveMode == MoveMode)
		{
			Sucsess = true;
			return AgressiveMoveModeInput;
		}
	}
	Sucsess = false;
	return FAgressiveMoveModeInput();
}

FAgressiveMoveModeInput UAgressiveMovementComponent::FindNotActiveMoveModeInput(EAgressiveMoveMode MoveMode, bool& Sucsess)
{
	for (FAgressiveMoveModeInput AgressiveMoveModeInput : NotActiveAgressiveMoveMode)
	{
		if (AgressiveMoveModeInput.AgressiveMoveMode == MoveMode)
		{
			Sucsess = true;
			return AgressiveMoveModeInput;
		}
	}
	Sucsess = false;
	return FAgressiveMoveModeInput();
}

bool UAgressiveMovementComponent::ContainsActiveMoveModeInput(EAgressiveMoveMode MoveMode)
{
	bool Sucsess = false;
	FindActiveMoveModeInput(MoveMode, Sucsess);
	return Sucsess;
}

bool UAgressiveMovementComponent::ContainsNotActiveMoveModeInput(EAgressiveMoveMode MoveMode)
{
	bool Sucsess = false;
	FindNotActiveMoveModeInput(MoveMode, Sucsess);
	return Sucsess;
}

void UAgressiveMovementComponent::RemoveActiveAgressiveModeInputByMode(EAgressiveMoveMode RemoveMoveMode)
{
	int count = 0;
	for (FAgressiveMoveModeInput LocalAgressiveMoveMode : ActiveAgressiveMoveMode)
	{
		if (LocalAgressiveMoveMode.AgressiveMoveMode == RemoveMoveMode)
		{
			ActiveAgressiveMoveMode.RemoveAt(count);
			break;
		}
		count = count + 1;
	}
}

void UAgressiveMovementComponent::RemoveNotActiveAgressiveModeInputByMode(EAgressiveMoveMode RemoveMoveMode)
{
	int count = 0;
	for (FAgressiveMoveModeInput LocalAgressiveMoveMode : NotActiveAgressiveMoveMode)
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

void UAgressiveMovementComponent::AddMoveStatus(FAgressiveMoveModeInput NewAgressiveMoveMode)
{
	RemoveNotActiveAgressiveModeInputByMode(NewAgressiveMoveMode.AgressiveMoveMode);
	TArray<FAgressiveMoveModeInput> OldAgressiveMode = ActiveAgressiveMoveMode;
	if (true)
	{
		ActiveAgressiveMoveMode.Add(NewAgressiveMoveMode);

		switch (NewAgressiveMoveMode.AgressiveMoveMode)
		{
		case EAgressiveMoveMode::None:
			break;
		case EAgressiveMoveMode::Slide:
			RemoveMoveStatusByMode(EAgressiveMoveMode::Run);
			StartSlide();
			break;
		case EAgressiveMoveMode::RunOnWall:
			StartRunOnWall();
			break;
		case EAgressiveMoveMode::Run:
			RemoveMoveStatusByMode(EAgressiveMoveMode::Slide);
			StartRun();
			break;
		case EAgressiveMoveMode::Climb:
			RemoveAllMoveStatus();
			ActiveAgressiveMoveMode.Add(NewAgressiveMoveMode);
			StartClimb();
			break;
		default:
			break;
		}
	}
};

void UAgressiveMovementComponent::RemoveMoveStatusByMode(EAgressiveMoveMode RemoveAgressiveMoveMode, bool SendToInput, float DirectReloadTime)
{
	if (ContainsActiveMoveModeInput(RemoveAgressiveMoveMode))
	{
		SwitchRemoveModeStatus(RemoveAgressiveMoveMode);
		if (SendToInput)
		{
			bool LSucsess = true;
			FAgressiveMoveModeInput RemoveAgressiveAgressive = FindActiveMoveModeInput(RemoveAgressiveMoveMode, LSucsess);
			RemoveActiveAgressiveModeInputByMode(RemoveAgressiveMoveMode);
			if (LSucsess)
			{
				AddNotActiveAgressiveModeInput(RemoveAgressiveAgressive, DirectReloadTime);
			}
		}
		else
		{
			RemoveActiveAgressiveModeInputByMode(RemoveAgressiveMoveMode);
		}
	}
}

void UAgressiveMovementComponent::RemoveMoveStatusByInput(FAgressiveMoveModeInput RemoveAgressiveMoveModeInput, bool SendToInput, float DirectReloadTime)
{
	if (ContainsActiveMoveModeInput(RemoveAgressiveMoveModeInput.AgressiveMoveMode))
	{
		SwitchRemoveModeStatus(RemoveAgressiveMoveModeInput.AgressiveMoveMode);
		if (SendToInput)
		{
			RemoveActiveAgressiveModeInputByMode(RemoveAgressiveMoveModeInput.AgressiveMoveMode);
			AddNotActiveAgressiveModeInput(RemoveAgressiveMoveModeInput, DirectReloadTime);
		}
		else
		{
			RemoveActiveAgressiveModeInputByMode(RemoveAgressiveMoveModeInput.AgressiveMoveMode);
		}
	}
}

void UAgressiveMovementComponent::ForceRemoveMoveStatusAndInputByMode(EAgressiveMoveMode RemoveAgressiveMoveMode)
{
	RemoveNotActiveAgressiveModeInputByMode(RemoveAgressiveMoveMode);
	RemoveMoveStatusByMode(RemoveAgressiveMoveMode, false);
}

void UAgressiveMovementComponent::SwitchRemoveModeStatus(EAgressiveMoveMode RemoveAgressiveMoveMode)
{
	TArray<FAgressiveMoveModeInput> OldAgressiveMode = ActiveAgressiveMoveMode;

	if (ContainsActiveMoveModeInput(RemoveAgressiveMoveMode))
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
}

void UAgressiveMovementComponent::RemoveAllMoveStatus(bool SendToInput)
{
	TArray<FAgressiveMoveModeInput> LOldAgressiveMode = ActiveAgressiveMoveMode;
	for (FAgressiveMoveModeInput LMoveMode : LOldAgressiveMode)
	{
		RemoveMoveStatusByInput(LMoveMode, SendToInput);
	}
};

void UAgressiveMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	AccelerationModificator = NewObject<UFloatModificatorContext>(this, "AccelerationModificator");
	StaminaModificator = NewObject<UFloatModificatorContext>(this, "StaminaModificator");
	MaxWalkSpeedModificator = NewObject<UFloatModificatorContext>(this, "MaxWalkSpeedModificator");

	AddStaminaModificator(BaseAddStaminaValue, "BaseAddModificator");

	DefaultCharacterSize = GetCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	DefaultMaxWalkSpeed = MaxWalkSpeed;
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
	CheckActivateMoveMode(DeltaTime);
	TickTrick(DeltaTime);
	TickRun();
	TickClimb();

	//Debug
	if (Debug)
	{
		if (DebugActiveMoveMode)
		{
			for (FAgressiveMoveModeInput LMoveMode : ActiveAgressiveMoveMode)
			{
				FString LocalString = GetCharacterOwner()->GetName() + " " + UEnum::GetValueAsString(LMoveMode.AgressiveMoveMode);
				GEngine->AddOnScreenDebugMessage(-1, 0.0, FColor::Green, LocalString);
			}
		}
		if (DebugNotActiveMoveMode)
		{
			for (FAgressiveMoveModeInput LMoveMode : NotActiveAgressiveMoveMode)
			{
				FString LocalString = GetCharacterOwner()->GetName() + " " + UEnum::GetValueAsString(LMoveMode.AgressiveMoveMode);
				GEngine->AddOnScreenDebugMessage(-1, 0.0, FColor::Emerald, LocalString);
			}
		}
		if (ExecutedTrick)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.0, FColor::Blue, ExecutedTrick->GetName());
		}
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
	if (AccelerationModificator)
	{
		AccelerationModificator->Modificators.Empty();
	}
}
void UAgressiveMovementComponent::SetEnableRun(bool NewEnableRun)
{
	if (!(NewEnableRun == EnableRun))
	{
		if (!NewEnableRun)
		{
			RemoveMoveStatusByMode(EAgressiveMoveMode::Run);
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
	if (!RunMaxSpeedModification)
	{
		RunMaxSpeedModification = MaxWalkSpeedModificator->CreateNewModificator();
		RunMaxSpeedModification->OperationType = EModificatorOperation::Multiply;

	}
	if (RunMaxSpeedModification)
	{
		RunMaxSpeedModification->SelfValue = SpeedRunMultiply;
	}
	MaxWalkSpeed = MaxWalkSpeedModificator->ApplyModificators(DefaultMaxWalkSpeed);


	if (!RunAccelerationModification)
	{
		RunAccelerationModification = AccelerationModificator->CreateNewModificator();
		RunAccelerationModification->OperationType = EModificatorOperation::Multiply;
	}
	if (RunAccelerationModification)
	{
		RunAccelerationModification->SelfValue = AccelerationRunMultiply;
	}


	MaxAcceleration = AccelerationModificator->ApplyModificators(DefaultMaxAcceleration);
	RunStaminaModificator = AddStaminaModificator(LowStaminaRunValue,"Run");
	EndStaminaDelegate.AddDynamic(this, &UAgressiveMovementComponent::LowStaminaEndRun);
}

void UAgressiveMovementComponent::EndRun()
{
	EndStaminaDelegate.RemoveDynamic(this, &UAgressiveMovementComponent::LowStaminaEndRun);
	if (RunAccelerationModification)
	{
		RunAccelerationModification->SelfValue = 1.0;
	}
	if (RunMaxSpeedModification)
	{
		RunMaxSpeedModification->SelfValue = 1.0;
	}
	MaxAcceleration = AccelerationModificator->ApplyModificators(DefaultMaxAcceleration);
	RemoveStaminaModificator("Run");
}

bool UAgressiveMovementComponent::CheckRun(bool ToEnableStatus)
{
	if (ToEnableStatus)
	{
		return !ContainsActiveMoveModeInput(EAgressiveMoveMode::Slide) && MovementMode == EMovementMode::MOVE_Walking;
	}
	else
	{
		return false;
	}
}

void UAgressiveMovementComponent::StartRunInput()
{
	if (GetEnableRun())
	{
		AddNotActiveAgressiveModeInput({ EAgressiveMoveMode::Run,2 });
	}
}

void UAgressiveMovementComponent::EndRunInput()
{
	ForceRemoveMoveStatusAndInputByMode(EAgressiveMoveMode::Run);
}

void UAgressiveMovementComponent::LowStaminaEndRun()
{
	RemoveMoveStatusByMode(EAgressiveMoveMode::Run,true,0.5);
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
			RemoveMoveStatusByMode(EAgressiveMoveMode::Slide);
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
	if (ContainsActiveMoveModeInput(EAgressiveMoveMode::Slide))
	{
		GetCharacterOwner()->GetCapsuleComponent()->SetCapsuleSize(GetCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), Size);
	}
}

void UAgressiveMovementComponent::StartSlideInput()
{
	if (GetEnableSlide())
	{
		AddNotActiveAgressiveModeInput({ EAgressiveMoveMode::Slide, 0 });
	}
}

void UAgressiveMovementComponent::StartSlide()
{
	DefaultGroundFriction = GroundFriction;
	DefailtGroundBraking = BrakingDecelerationWalking;
	GroundFriction = GroundFrictionWhenSliding;
	BrakingDecelerationWalking = GroundBrakingWhenSliding;
	GetCharacterOwner()->GetCapsuleComponent()->SetCapsuleSize(GetCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), SlideCharacterSize);
	UFloatModificator* FloatModificator = AccelerationModificator->CreateNewModificator();
	FloatModificator->SelfValue = 0;
	FloatModificator->Name = "Slide";
	FloatModificator->OperationType = EModificatorOperation::Multiply;
	MaxAcceleration = AccelerationModificator->ApplyModificators(DefaultMaxAcceleration);
}

bool UAgressiveMovementComponent::CheckSlide(bool ToEnableStatus)
{
	FVector NormilizeMultiplier = { 1.0,1.0,0 };
	FVector LocalNormaliaizeVelocity = GetCharacterOwner()->GetVelocity() * NormilizeMultiplier;
	if (ToEnableStatus)
	{
		if (ContainsActiveMoveModeInput(EAgressiveMoveMode::RunOnWall) && GetCustomControlVector().Dot({0,0,1}) < 0)
		{
			if ((LocalNormaliaizeVelocity.Length() > MinSpeedForSliding) && MovementMode == EMovementMode::MOVE_Falling)
			{
				return true;
			}
		}
		else
		{
			if ((LocalNormaliaizeVelocity.Length() > MinSpeedForSliding) && MovementMode == EMovementMode::MOVE_Falling && !TraceSphereSucsess)
			{
				return true;
			}
		}
		return false;
	}
	else
	{
		if ((LocalNormaliaizeVelocity.Length() > MinSpeedForSliding))
		{
			return true;
		}
		return false;
	}
	return false;
}


void UAgressiveMovementComponent::EndSlideInput()
{
	ForceRemoveMoveStatusAndInputByMode(EAgressiveMoveMode::Slide);
}

void UAgressiveMovementComponent::TickRun()
{
	if (ContainsActiveMoveModeInput(EAgressiveMoveMode::Run))
	{
		float DotScale = UKismetMathLibrary::Dot_VectorVector(GetLastInputVector(), GetCharacterOwner()->GetActorForwardVector());
		if(DotScale<0)
		{
			RunStaminaModificator->SelfValue = 0;
			RunAccelerationModification->SelfValue = 1.0;
			RunMaxSpeedModification->SelfValue = 1.0;
		}
		else
		{
			DotScale = DotScale + 1;
			RunStaminaModificator->SelfValue = LowStaminaRunValue * DotScale * GetLastInputVector().Length()/sqrt(3);
			RunAccelerationModification->SelfValue = AccelerationRunMultiply * DotScale;
			RunMaxSpeedModification->SelfValue = SpeedRunMultiply * DotScale;
		}

	}
}

void UAgressiveMovementComponent::EndSlide()
{
	GroundFriction = DefaultGroundFriction;
	BrakingDecelerationWalking = DefailtGroundBraking;
	GetCharacterOwner()->GetCapsuleComponent()->SetCapsuleSize(GetCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), DefaultCharacterSize);
	AccelerationModificator->RemoveModificatorByName("Slide");
	MaxAcceleration = AccelerationModificator->ApplyModificators(DefaultMaxAcceleration);
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
			MovementDirection = MovementDirection + CableVector * CableActor->CableStiffness * (CableActor->GetRealCableLength()- CableActor->MaxCableLength);
		}
	}
	return MovementDirection;
}
void UAgressiveMovementComponent::PlayStepTick(float DeltaTime)
{
	if (PlaySensorEvents && Cast<APlayerController>(GetCharacterOwner()->GetController()))
	{
		if (VelocityToDelayPerStep)
		{
			if ((MovementMode != EMovementMode::MOVE_Falling) && (!ContainsActiveMoveModeInput(EAgressiveMoveMode::Slide)))
			{
				TickTimePlayStep = DeltaTime + TickTimePlayStep;
				float GetTimeToStep = VelocityToDelayPerStep->GetFloatValue(GetCharacterOwner()->GetVelocity().Length());
				if (TickTimePlayStep > GetTimeToStep)
				{
					if (PlayCameraShakes)
					{
						if (ContainsActiveMoveModeInput(EAgressiveMoveMode::Run))
						{
							if (RunCameraShake)
							{
								Cast<APlayerController>(GetCharacterOwner()->GetController())->PlayerCameraManager->StartCameraShake(RunCameraShake);
							}
						}
						else
						{
							if (WalkCameraShake)
							{
								Cast<APlayerController>(GetCharacterOwner()->GetController())->PlayerCameraManager->StartCameraShake(WalkCameraShake);
							}
						}
					}
					if (PlayMoveSounds)
					{
						USceneComponent* LocalAttachToComponent = GetCharacterOwner()->GetCapsuleComponent();
						FName LocalAttachPointName = FName("None");
						FVector LocalLocation = GetCharacterOwner()->GetCapsuleComponent()->GetComponentLocation() - (0, 0, GetCharacterOwner()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * -0.4);

						if (ContainsActiveMoveModeInput(EAgressiveMoveMode::Run))
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
		AddImpulse(LaunchVector * Strength, true);
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
			AddImpulse(CableActor->GetJumpUnitVector() * Strength, true);
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
				AddImpulse(JumpVector,true);
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
 	AddImpulse(LLaunchVector,true);
	GetWorld()->GetTimerManager().SetTimer(ReloadJumpTimeHandle, this, &UAgressiveMovementComponent::ReloadJump, TimeReloadJumpFromWall, false);
	RemoveAllMoveStatus();
}

void UAgressiveMovementComponent::CruckFlyTick(float DeltaTime)
{
	if (GetEnableCruck()&&MovementMode==EMovementMode::MOVE_Falling&&!Cabels.IsEmpty())
	{
		FVector CruckVector = GetApplyCruck();
		if (!CruckVector.IsNearlyZero())
		{
			AddForce(CruckVector * Mass);
		}
	}
}

void UAgressiveMovementComponent::CruckWalkingTick(float Delta)
{
	if (GetEnableCruck())
	{
		if (Cabels.IsValidIndex(0))
		{
			FVector CruckVector = GetApplyCruck();
			float LStrengthCruck = CruckVector.Length();
			if (!CruckVector.IsNearlyZero())
			{
				if (!SpeedStrengthPushModificator)
				{
					SpeedStrengthPushModificator = MaxWalkSpeedModificator->CreateNewModificator();
					SpeedStrengthPushModificator->Name = "SpeedStrengthPushModificator";
					SpeedStrengthPushModificator->SelfValue = 1;
					SpeedStrengthPushModificator->OperationType = EModificatorOperation::Multiply;
				}
				if (SpeedStrengthPushModificator)
				{
					CruckVector = { CruckVector.X,CruckVector.Y,0 };
					UKismetMathLibrary::Vector_Normalize(CruckVector);
					float InputVectorDot = GetLastInputVector().Dot(GetNormalizeCruckVector());
					float LValueSpeedModificator = ((InputVectorDot + 1.0) / 2 + 0.5) * (FMath::Clamp(1 - LStrengthCruck / StrengthPushCruck, 0.1, 1));
					SpeedStrengthPushModificator->SelfValue = LValueSpeedModificator;
					if (Debug)
					{
						GEngine->AddOnScreenDebugMessage(-1, 0.0, FColor::Red, FString::SanitizeFloat(LValueSpeedModificator));
					}
					MaxWalkSpeed = MaxWalkSpeedModificator->ApplyModificators(DefaultMaxWalkSpeed);
					return;
				}
			}
		}
		else
		{
			if (SpeedStrengthPushModificator)
			{
				SpeedStrengthPushModificator->SelfValue = 1.0;
				MaxWalkSpeed = MaxWalkSpeedModificator->ApplyModificators(DefaultMaxWalkSpeed);
			}
		}
	}
	return;
}
void UAgressiveMovementComponent::SetEnableClimb(bool NewEnableClimb)
{
	if (!EnableClimb == NewEnableClimb)
	{
		if (!NewEnableClimb)
		{
			RemoveMoveStatusByMode(EAgressiveMoveMode::Climb);
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
			RemoveMoveStatusByMode(EAgressiveMoveMode::RunOnWall);
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
	GetCustomControlVector().ProjectOnToNormal(OptimalWall.ImpactNormal);
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

void UAgressiveMovementComponent::TickRunOnWall(float DeltaTime)
{
	if (ContainsActiveMoveModeInput(EAgressiveMoveMode::RunOnWall))
	{
		if (!CheckRunOnWall(false))
		{
			RemoveMoveStatusByMode(EAgressiveMoveMode::RunOnWall);
			return;
		}
		FVector LaunchVector = GetMoveToWallVector() * SpeedRunOnWall;
		Launch(LaunchVector);
	}
}
void UAgressiveMovementComponent::StartRunOnWallInput()
{
	if (GetEnableMoveOnWall())
	{
		AddNotActiveAgressiveModeInput({ EAgressiveMoveMode::RunOnWall, 3 });
	}
};

void UAgressiveMovementComponent::StartRunOnWall()
{
	ReloadDoubleJump();
	AddStaminaModificator(LowStaminaRunOnWall, "RunOnWall");
	EndStaminaDelegate.AddDynamic(this, &UAgressiveMovementComponent::LowStaminaEndRunOnWall);
}

FVector UAgressiveMovementComponent::GetCustomControlVector_Implementation()
{
	return GetCharacterOwner()->GetController()->GetControlRotation().Vector();
}

void UAgressiveMovementComponent::EndRunOnWallInput()
{
	ForceRemoveMoveStatusAndInputByMode(EAgressiveMoveMode::RunOnWall);
};

void UAgressiveMovementComponent::EndRunOnWall()
{
	EndStaminaDelegate.RemoveDynamic(this, &UAgressiveMovementComponent::LowStaminaEndRunOnWall);
	RemoveStaminaModificator("RunOnWall");
}

void UAgressiveMovementComponent::LowStaminaEndRunOnWall()
{
	RemoveMoveStatusByMode(EAgressiveMoveMode::RunOnWall);
}


bool UAgressiveMovementComponent::CheckRunOnWall(bool ToEnableStatus)
{
	bool LHaveGoodAngle = false;
	for (FHitResult HitResult : WallTraceHitResults)
	{
		if (HitResult.ImpactNormal.Dot({ 0,0,1 }) > 0.7)
		{
			LHaveGoodAngle = true;
		}
	}
	if (ToEnableStatus)
	{
		return ((MovementMode == EMovementMode::MOVE_Falling) && TraceSphereSucsess && GetCharacterOwner()->GetVelocity().Length() > MinMoveOnWallVelocity && GetLastInputVector().Length() != 0.0 && !LHaveGoodAngle);
	}
	else
	{
		return (GetCharacterOwner()->GetVelocity().Length() > MinMoveOnWallVelocity || GetLastInputVector().Length() != 0.0 && TraceSphereSucsess && !LHaveGoodAngle);
		return false;
	}
	return false;
}

void UAgressiveMovementComponent::TickMoveOnWall(float DeltaTime)
{
	if (ContainsActiveMoveModeInput(EAgressiveMoveMode::RunOnWall))
	{
		if (ContainsActiveMoveModeInput(EAgressiveMoveMode::Slide))
		{
			TickSlideOnWall(DeltaTime);
		}
		else
		{
			TickRunOnWall(DeltaTime);
		}
	}
}

void UAgressiveMovementComponent::TickSlideOnWall(float DeltaTime)
{

}

void UAgressiveMovementComponent::EndRunOnWallDirectly()
{
	if (Debug)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Black, "TimerWallEnd");
	}
	JumpFromWall();
	RemoveMoveStatusByMode(EAgressiveMoveMode::RunOnWall);
}

void UAgressiveMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if (MovementMode == EMovementMode::MOVE_Falling)
	{
		RemoveMoveStatusByMode(EAgressiveMoveMode::Slide);
		RemoveMoveStatusByMode(EAgressiveMoveMode::Run);
	}
	else if(MovementMode == EMovementMode::MOVE_Walking || MovementMode == EMovementMode::MOVE_NavWalking)
	{
		RemoveMoveStatusByMode(EAgressiveMoveMode::RunOnWall);
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
	if (ContainsActiveMoveModeInput(EAgressiveMoveMode::Climb))
	{
		if (!ExecutedTrick || (ExecutedTrick && !ClimbTrickObjects.Contains(ExecutedTrick)))
		{
			if (!CheckInputClimb())
			{
				RemoveMoveStatusByMode(EAgressiveMoveMode::Climb);
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
	AddNotActiveAgressiveModeInput({ EAgressiveMoveMode::Climb, 0 });
}

void UAgressiveMovementComponent::EndClimbInput()
{
	ForceRemoveMoveStatusAndInputByMode(EAgressiveMoveMode::Climb);
}



void UAgressiveMovementComponent::PhysFalling(float deltaTime, int32 Iterations)
{
	Super::PhysFalling(deltaTime, Iterations);

	CruckFlyTick(deltaTime);
	MaxWalkSpeed = DefaultMaxWalkSpeed;
	TickMoveOnWall(deltaTime);

	FVector VelocityInCSystem = Velocity / 100;
	FVector AirFrenselVelocity = VelocityInCSystem.Length() * VelocityInCSystem.Length() * AirCableFrenselCoifficient*-1 * 100 * Velocity.GetSafeNormal();
	if (Debug)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0, FColor::Blue, AirFrenselVelocity.ToString());
	}
	AddForce(AirFrenselVelocity*Mass);
}

void UAgressiveMovementComponent::PhysWalking(float deltaTime, int32 Iterations)
{
	Super::PhysWalking(deltaTime, Iterations);
	CruckWalkingTick(deltaTime);

	if (!CheckSlide(false))
	{
		RemoveMoveStatusByMode(EAgressiveMoveMode::Slide);
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



