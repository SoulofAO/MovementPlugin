// Fill out your copyright notice in the Description page of Project Settings.


#include "Tricks/TrickObjects.h"
#include "AgressiveMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "VisibilityTricks/VisibilityTrickObjects.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "Curves/CurveVector.h"
#include "Components/CapsuleComponent.h"

UWorld* UTrickObject::GetWorld() const
{
	if (GIsEditor && !GIsPlayInEditorWorld) return nullptr;
	else if (GetOuter()) return GetOuter()->GetWorld();
	else if (GWorld) return GWorld;
	else return nullptr;
}

bool UTrickObject::CheckTrickEnable_Implementation()
{
	return false;
}

void UTrickObject::UseTrick_Implementation()
{
}

void UTrickObject::Tick_Implementation(float DeltaTick)
{
}

void UTrickObject::StartReloadTimer_Implementation()
{
	if (ReloadTime > 0)
	{
		GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &UTrickObject::EndReloadTimer, ReloadTime, false);
	}
}

void UTrickObject::EndReloadTimer()
{
	ReloadTimer.Invalidate();
}


bool UClimbTrickObject::CheckTrickEnable_Implementation()
{
	if (MovementComponent->AgressiveMoveMode.Contains(EAgressiveMoveMode::Climb))
	{
		return true;
	}
	return false;
}

void UClimbTrickObject::UseTrick_Implementation()
{
}

bool UCrossbarJumpTrickObject::CheckTrickEnable_Implementation()
{
	if (Super::CheckTrickEnable_Implementation())
	{
		TArray<FHitResult> HitResults;
		auto DebugTrace = [&]() {if (MovementComponent->Debug) { return EDrawDebugTrace::ForOneFrame; }; return EDrawDebugTrace::None; };
		const TArray<AActor*> ActorIgnore;
		UKismetSystemLibrary::SphereTraceMulti(MovementComponent->GetCharacterOwner(), MovementComponent->GetCharacterOwner()->GetActorLocation(), MovementComponent->GetCharacterOwner()->GetActorLocation() + MovementComponent->GetCharacterOwner()->GetActorForwardVector() * TraceRadiusForward, TraceRadiusForward, TraceTypeQuery1, false, ActorIgnore, DebugTrace(), HitResults, false);
		if (HitResults.Num() > 0)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.0, FColor::Blue, HitResults[0].GetActor()->GetName());
			return true;
		}

	}
	return false;
}

void UCrossbarJumpTrickObject::UseTrick_Implementation()
{
	ACharacter* LCharacter = MovementComponent->GetCharacterOwner();
	float LStrength = 0.0;
	if (ApplyStrengthAsVelocity)
	{
		LStrength = MovementComponent->Velocity.Length();
	}
	else
	{
		LStrength = DirectStrength;
	}
	MovementComponent->Launch(LCharacter->GetActorForwardVector() * LStrength);
	FinishTrickDelegate.Broadcast(this);
}

bool UClimbingTopEndTrickObject::CheckTrickEnable_Implementation()
{
	if (Super::CheckTrickEnable_Implementation())
	{
		FVector LTopVector = { 0,0,1 };
		if (MovementComponent->OptimalLeadge.ImpactNormal.Dot(LTopVector) > MinDotForEndTop)
		{
			return true;
		}
	}
	return false;
}

void UClimbingTopEndTrickObject::UseTrick_Implementation()
{
	float HalfHead = MovementComponent->GetCharacterOwner()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() ;
	FVector AddLocation = { 0,0, HalfHead};
	MoveToPoint(MovementComponent->OptimalLeadge.ImpactPoint + AddLocation);
}


bool UClimbingUpTrickObject::CheckTrickEnable_Implementation()
{
	return MovementComponent->AgressiveMoveMode.Contains(EAgressiveMoveMode::Climb);
}

void UClimbingUpTrickObject::UseTrick_Implementation()
{
	MoveToPoint(MovementComponent->OptimalLeadge.ImpactPoint);
}

void UClimbingUpTrickObject::MoveToPoint_Implementation(FVector Point)
{
	EnableMove = true;
	TickTime = 0;
	StartLocation = MovementComponent->GetCharacterOwner()->GetActorLocation();
	EndLocation = Point;
}

void UClimbingUpTrickObject::EndMovePoint_Implementation()
{
	FinishTrickDelegate.Broadcast(this);
}

void UClimbingUpTrickObject::Tick_Implementation(float DeltaTime)
{
	TickTime = UKismetMathLibrary::FClamp(DeltaTime + TickTime,0,1);
	FVector LNewLocation = UKismetMathLibrary::VLerp(StartLocation, EndLocation, TickTime / DirectTime);
	if (CurveAddLocation)
	{
		LNewLocation = LNewLocation + CurveAddLocation->GetVectorValue(TickTime / DirectTime);
	}
	MovementComponent->GetCharacterOwner()->SetActorLocation(LNewLocation);
	if (TickTime >= 1)
	{
		EndMovePoint();
	}
}
