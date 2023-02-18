// Fill out your copyright notice in the Description page of Project Settings.


#include "AgressiveMovementComponent.h"
#include "MovementCableActor.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

UAgressiveMovementComponent::UAgressiveMovementComponent()
{
	DefaultMaxWalkSpeed = MaxWalkSpeed;
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
FVector UAgressiveMovementComponent::GetMoveToWallVector()
{
	TArray<AActor*> IgnoredActors;
	TArray<FHitResult> HitResults;
	FVector TraceEnd = GetOwner()->GetActorLocation();
	TraceEnd = { TraceEnd.X, TraceEnd.Y,TraceEnd.Z - 10 };
	UKismetSystemLibrary::SphereTraceMulti(this, GetOwner()->GetActorLocation(), TraceEnd, GetCharacterOwner()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), ETraceTypeQuery::TraceTypeQuery1, false, IgnoredActors,EDrawDebugTrace::None, HitResults, true);
	for (FHitResult HitResult : HitResults)
	{
	}
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
		MaxWalkSpeed = DotScale * FMath::Clamp(1 - CruckVector.Size()/StrengthPushCruck,0,1) * DefaultMaxWalkSpeed;
	}
}
