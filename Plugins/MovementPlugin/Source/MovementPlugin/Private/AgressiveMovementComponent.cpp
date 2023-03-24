// Fill out your copyright notice in the Description page of Project Settings.


#include "AgressiveMovementComponent.h"
#include "MovementCableActor.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "FloatModificatorContextV1.h"

UAgressiveMovementComponent::UAgressiveMovementComponent()
{
	DefaultMaxWalkSpeed = MaxWalkSpeed;
	DefaultWalkFriction = GroundFriction;
	SpeedModificator = NewObject<UFloatModificatorContext>(this, "SpeedModificator");
	EndTimeMoveOnWallDelegate.BindUFunction(this, "TimerWallEnd");
	RestoreMoveOnWallDelegate.BindUFunction(this, "TimerWallRestore");
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
}

void UAgressiveMovementComponent::EndRun()
{
	if (SpeedRunModificator)
	{
		SpeedRunModificator->SelfValue = 1.0;
	}
	MaxWalkSpeed = SpeedModificator->ApplyModificators(MaxWalkSpeed);
}

void UAgressiveMovementComponent::StartRolling()
{
	GroundFriction = RollingMultiplyFriction*DefaultWalkFriction;
}

void UAgressiveMovementComponent::EndRolling()
{
	GroundFriction = DefaultWalkFriction;
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
	for (AMovementCableActor* CableActor : Cabels)
	{
		FVector LaunchVector = CableActor->GetJumpUnitVector() + AddVector;
		Launch(CableActor->GetJumpUnitVector() * Strength);
		CableActor->Destroy();
	}
	Cabels.Empty();
	MaxWalkSpeed = DefaultMaxWalkSpeed;
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
	UKismetSystemLibrary::SphereTraceMulti(this, GetOwner()->GetActorLocation(), TraceEnd, GetCharacterOwner()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+10, ETraceTypeQuery::TraceTypeQuery1, false, IgnoredActors, EDrawDebugTrace::None, HitResults, true);
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
void UAgressiveMovementComponent::ReloadJump()
{
	ReloadJumpTimeHandle.Invalidate();
}
void UAgressiveMovementComponent::JumpFromWall()
{
	if (!ReloadJumpTimeHandle.IsValid())
	{
 		FVector JumpVector = GetJumpFromWallVector() * StrengthJumpFromWall;
		Launch(Velocity + JumpVector);
		GetWorld()->GetTimerManager().SetTimer(ReloadJumpTimeHandle, this, &UAgressiveMovementComponent::ReloadJump, TimeReloadJumpFromWall, false);
	}
}
FVector UAgressiveMovementComponent::GetMoveToWallVector()
{
	if (WallTraceHitResults.Num() > 0)
	{
		
	}
	return {0,0,0};
}

void UAgressiveMovementComponent::StartRunOnWall()
{
	RunOnWall = true;
	if (MoveOnWallTimeHandle.IsValid())
	{
		float TimeRemaining = GetWorld()->GetTimerManager().GetTimerRemaining(MoveOnWallTimeHandle);
		GetWorld()->GetTimerManager().SetTimer(MoveOnWallTimeHandle, EndTimeMoveOnWallDelegate, DefaultTimeRunOnWall - TimeRemaining, false);
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(MoveOnWallTimeHandle, EndTimeMoveOnWallDelegate, DefaultTimeRunOnWall, false);
	}
}

void UAgressiveMovementComponent::EndRunOnWall()
{
	RunOnWall = false;
	if (MoveOnWallTimeHandle.IsValid())
	{
		float TimeElasped = GetWorld()->GetTimerManager().GetTimerElapsed(MoveOnWallTimeHandle);
		GetWorld()->GetTimerManager().SetTimer(MoveOnWallTimeHandle, RestoreMoveOnWallDelegate, TimeElasped, false);
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(MoveOnWallTimeHandle, RestoreMoveOnWallDelegate, DefaultTimeRunOnWall, false);
	}
}

void UAgressiveMovementComponent::TimerWallEnd()
{
	EndRunOnWall();
}

void UAgressiveMovementComponent::TimerWallRestore()
{
	StartRunOnWall();
}

void UAgressiveMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if (MovementMode == EMovementMode::MOVE_Falling)
	{
		SpeedModificator->Modificators.Remove(SpeedStrengthPushModificator);
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
	TraceForWalkChannel();
	
}

void UAgressiveMovementComponent::PhysWalking(float deltaTime, int32 Iterations)
{
	Super::PhysWalking(deltaTime, Iterations);
	TraceForWalkChannel();
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
