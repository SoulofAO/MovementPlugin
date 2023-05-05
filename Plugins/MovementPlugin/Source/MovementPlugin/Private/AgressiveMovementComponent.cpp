// Fill out your copyright notice in the Description page of Project Settings.


#include "AgressiveMovementComponent.h"
#include "MovementCableActor.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "FloatModificatorContextV1.h"
#include "Camera/CameraShakeBase.h"

UAgressiveMovementComponent::UAgressiveMovementComponent()
{
	DefaultMaxWalkSpeed = MaxWalkSpeed;
	SpeedModificator = NewObject<UFloatModificatorContext>(this, "SpeedModificator");
	StaminaModificator = NewObject<UFloatModificatorContext>(this, "StaminaModificator");
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
	TArray<EAgressiveMoveMode> OldAgressiveMode = AgressiveMoveMode;
	AgressiveMoveMode.Add(NewAgressiveMoveMode);
	if (!AgressiveMoveMode.Contains(NewAgressiveMoveMode))
	{
		switch (NewAgressiveMoveMode)
		{
		case EAgressiveMoveMode::None:
			break;
		case EAgressiveMoveMode::Slide:
			if (AgressiveMoveMode.Contains(EAgressiveMoveMode::RunOnWall))
			{
				EndRunOnWall();
			}
			StartSlide();
			break;
		case EAgressiveMoveMode::RunOnWall:

			if (MovementMode != EMovementMode::MOVE_Falling)
			{
				AgressiveMoveMode.Remove(NewAgressiveMoveMode);
			}
			else
			{
				StartRunOnWall();
			}
			break;
		case EAgressiveMoveMode::Run:
			break;
		default:
			break;
		}
	}
}

void UAgressiveMovementComponent::RemoveMoveStatus(EAgressiveMoveMode NewAgressiveMoveMode)
{
	TArray<EAgressiveMoveMode> OldAgressiveMode = AgressiveMoveMode;
	AgressiveMoveMode.Remove(NewAgressiveMoveMode);

	if (!AgressiveMoveMode.Contains(NewAgressiveMoveMode))
	{
		switch (NewAgressiveMoveMode)
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
			break;
		default:
			break;
		}
	}
}

void UAgressiveMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	AddStaminaModificator(BaseAddStaminaValue, "BaseAddModificator");
}

void UAgressiveMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TickCalculateStamina(DeltaTime);
	TraceForWalkChannel();
}

void UAgressiveMovementComponent::BeginDestroy()
{
	Super::BeginDestroy();
	StaminaModificator->Modificators.Empty();
	SpeedModificator->Modificators.Empty();
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
		SpeedRunModificator->SelfValue = 2.0;
	}
	MaxWalkSpeed = SpeedModificator->ApplyModificators(MaxWalkSpeed);
	AddStaminaModificator(-10,"Run");
}

void UAgressiveMovementComponent::EndRun()
{
	if (SpeedRunModificator)
	{
		SpeedRunModificator->SelfValue = 1.0;
	}
	MaxWalkSpeed = SpeedModificator->ApplyModificators(MaxWalkSpeed);
	RemoveStaminaModificator("Run");
}

void UAgressiveMovementComponent::AddStaminaModificator(float Value, FString Name)
{
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
}

void UAgressiveMovementComponent::RemoveStaminaModificator(FString Name)
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
				AddStaminaModificator(BaseAddStaminaValue, "BaseAddModificator");
			}
		}
	}
}


void UAgressiveMovementComponent::StartSlideInput()
{
	AddMoveStatus(EAgressiveMoveMode::Slide);
}

void UAgressiveMovementComponent::StartSlide()
{
	DefaultGroundFriction = GroundFriction;
	GroundFriction = 0.01;
}

void UAgressiveMovementComponent::EndSlideInput()
{
	RemoveMoveStatus(EAgressiveMoveMode::Slide);
}

void UAgressiveMovementComponent::EndSlide()
{
	GroundFriction = DefaultGroundFriction;
}


AMovementCableActor* UAgressiveMovementComponent::SpawnCruck(FVector Location, TSubclassOf<AMovementCableActor> CableActorClass, bool CheckLocationDistance)
{
	if (CheckLocationDistance)
	{
		if (UKismetMathLibrary::Vector_Distance(Location, HandComponent->GetComponentLocation())< LengthCruck)
		{
			return BaseSpawnCruck(Location, CableActorClass);
		}
	}
	else
	{
		return BaseSpawnCruck(Location, CableActorClass);
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
void UAgressiveMovementComponent::JumpFromAllCruck(float Strength, FVector AddVector)
{
	if (CanTakeStamina(TakenJumpFromCruckStamina))
	{
		TakeStamina(TakenJumpFromCruckStamina);
		for (AMovementCableActor* CableActor : Cabels)
		{
			FVector LaunchVector = CableActor->GetJumpUnitVector() + AddVector;
			Launch(CableActor->GetJumpUnitVector() * Strength);
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
}
FVector UAgressiveMovementComponent::GetJumpFromWallVector()
{
	FVector JumpVector = {0,0,0};
	if (WallTraceHitResults.Num() > 0)
	{
		for (FHitResult HitResult : WallTraceHitResults)
		{
			JumpVector = JumpVector + UKismetMathLibrary::GetDirectionUnitVector(HitResult.ImpactPoint,GetCharacterOwner()->GetActorLocation()) * (1-(UKismetMathLibrary::Vector_Distance(HitResult.ImpactPoint, GetCharacterOwner()->GetActorLocation())/ (GetCharacterOwner()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 10)));
		}
  		JumpVector = JumpVector + AppendVectorJumpFromWall;
		UKismetMathLibrary::Vector_Normalize(JumpVector);
	}
	return JumpVector;
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
			}
		}
	}
}

void UAgressiveMovementComponent::ReloadJump()
{
	ReloadJumpTimeHandle.Invalidate();
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
			if (UKismetMathLibrary::Dot_VectorVector(ImpactNormal, { 0,0,1 }) > MinDotAngleToRunOnWall)
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
	if (Rotator.Yaw > 0)
	{
		return UKismetMathLibrary::Cross_VectorVector(OptimalWall.ImpactNormal, { 0,0,1 });
	}
	else
	{
		return UKismetMathLibrary::Cross_VectorVector(OptimalWall.ImpactNormal, { 0,0,1 })*-1;
	}
}

void UAgressiveMovementComponent::MoveOnWallEvent()
{
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
	AddMoveStatus(EAgressiveMoveMode::RunOnWall);
};

void UAgressiveMovementComponent::StartRunOnWall()
{
	AddStaminaModificator(-10, "RunOnWall");
	EndStaminaDelegate.AddDynamic(this, &UAgressiveMovementComponent::EndRunOnWall);
}

void UAgressiveMovementComponent::EndRunOnWallInput()
{
	RemoveMoveStatus(EAgressiveMoveMode::RunOnWall);
};

void UAgressiveMovementComponent::EndRunOnWall()
{
	RemoveStaminaModificator("RunOnWall");
}

void UAgressiveMovementComponent::CheckVelocityMoveOnWall()
{
	if (GetCharacterOwner()->GetVelocity().Length() < MinMoveOnWallVelocity)
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
	EndRunOnWall();
}

void UAgressiveMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if (MovementMode == EMovementMode::MOVE_Falling)
	{
		SpeedModificator->Modificators.Remove(SpeedStrengthPushModificator);
		RemoveMoveStatus(EAgressiveMoveMode::Slide);
		RemoveMoveStatus(EAgressiveMoveMode::Run);
	}
	else if(MovementMode == EMovementMode::MOVE_Walking || MovementMode == EMovementMode::MOVE_NavWalking)
	{
		SpeedStrengthPushModificator = SpeedModificator->CreateNewModificator();
		SpeedStrengthPushModificator->SelfValue = 1;
		SpeedStrengthPushModificator->OperationType = EModificatorOperation::Multiply;
		EndRunOnWall();
	};
}


void UAgressiveMovementComponent::PhysFalling(float deltaTime, int32 Iterations)
{
	Super::PhysFalling(deltaTime, Iterations);
	FVector CruckVector = GetApplyCruck();
	if (!CruckVector.IsNearlyZero())
	{
		Launch(Velocity + CruckVector);
	}
	MaxWalkSpeed = DefaultMaxWalkSpeed;
	MoveOnWallEvent();
	
}

void UAgressiveMovementComponent::PhysWalking(float deltaTime, int32 Iterations)
{
	Super::PhysWalking(deltaTime, Iterations);
	FVector CruckVector = GetApplyCruck();
	if (!CruckVector.IsNearlyZero())
	{
		CruckVector = { CruckVector.X,CruckVector.Y,0 };
		UKismetMathLibrary::Vector_Normalize(CruckVector);
		FHitResult HitResult;
		float DotScale = FMath::Clamp(UKismetMathLibrary::Dot_VectorVector((CruckVector / CruckVector.Size()), (Velocity / Velocity.Size())),0,1)+0.5;
		SpeedStrengthPushModificator->SelfValue = DotScale* FMath::Clamp(1 - CruckVector.Size() / StrengthPushCruck, 0, 1);
		MaxWalkSpeed = SpeedModificator->ApplyModificators(MaxWalkSpeed);
	}
}

