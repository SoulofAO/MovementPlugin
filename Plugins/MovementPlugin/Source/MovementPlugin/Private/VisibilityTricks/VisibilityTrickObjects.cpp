// Fill out your copyright notice in the Description page of Project Settings.


#include "VisibilityTricks/VisibilityTrickObjects.h"
#include "AgressiveMovementComponent.h"
#include "GameFramework/Character.h"
#include "Tricks/TrickObjects.h"

UWorld* UTrickVisibilityObject::GetWorld() const
{
	if (GIsEditor && !GIsPlayInEditorWorld) return nullptr;
	else if (GetOuter()) return GetOuter()->GetWorld();
	else if (GWorld) return GWorld;
	else return nullptr;
}

void UTrickVisibilityObject::StartTrick_Implementation()
{
}

void UTrickVisibilityObject::EndTrick_Implementation()
{
}

void UTrickVisibilityObject::Tick_Implementation(float DeltaTime)
{
}

void UMontageTrickVisibility::StartTrick_Implementation()
{
	Super::StartTrick_Implementation();
	float MinResult = -1;
	float LDistance = MovementComponent->OptimalLeadge.ImpactPoint.Z - MovementComponent->GetCharacterOwner()->GetActorLocation().Z;
	FAnimClimbingStruct LOptimalClimbingMontage;
	for (FAnimClimbingStruct LAnimClimbingEndMontage : AnimClimbingEndMontage)
	{
		if (LAnimClimbingEndMontage.AnimMontage)
		{
			float LDistanceOptimal = (1 - (LAnimClimbingEndMontage.DistanceClimbStart - LDistance) / LAnimClimbingEndMontage.NormalizeDistance);
			if (LDistanceOptimal > MinResult)
			{
				LOptimalClimbingMontage = LAnimClimbingEndMontage;
				MinResult = LDistanceOptimal;
			}
		}
	}
	OptimalAnimClimbingEndMontage = LOptimalClimbingMontage;
	MovementComponent->GetCharacterOwner()->PlayAnimMontage(LOptimalClimbingMontage.AnimMontage);
	MovementComponent->GetCharacterOwner()->GetMesh()->GetAnimInstance()->OnMontageEnded.AddDynamic(this, &UMontageTrickVisibility::MontagePlayEndBind);
}

void UMontageTrickVisibility::EndTrick_Implementation()
{
}

void UMontageTrickVisibility::MontagePlayEndBind(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == OptimalAnimClimbingEndMontage.AnimMontage)
	{
		MovementComponent->GetCharacterOwner()->GetMesh()->GetAnimInstance()->OnMontageEnded.RemoveDynamic(this, &UMontageTrickVisibility::MontagePlayEndBind);
	}
}
