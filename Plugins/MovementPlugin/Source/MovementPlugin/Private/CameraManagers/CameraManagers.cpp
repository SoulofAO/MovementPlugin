// Fill out your copyright notice in the Description page of Project Settings.


#include "CameraManagers/CameraManagers.h"
#include "AgressiveMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/Character.h"


UWorld* UDinamicCameraManager::GetWorld() const
{
	if (GIsEditor && !GIsPlayInEditorWorld) return nullptr;
	else if (GetOuter()) return GetOuter()->GetWorld();
	else if (GWorld) return GWorld;
	else return nullptr;
}

void UDinamicCameraManager::ApplyCamera_Implementation(float DeltaTime)
{
}

FRotator UDinamicCameraManager::CalculateApplyRotator_Implementation(float DeltaTime)
{
	return FRotator(0, 0, 1) * DeltaTime;
}

bool UDinamicCameraManager::CheckApplyCamera_Implementation()
{
	return true;
}

UBaseDynamicCameraManager::UBaseDynamicCameraManager()
{
	VelocityToCameraRotation = LoadObject<UCurveFloat>(nullptr, TEXT("/Game/Plugins/MovementPlugin/Content/Other/BaseVelocityToCameraRotation.uasset"));
	DotToCameraRotation = LoadObject<UCurveFloat>(nullptr, TEXT("/Game/Plugins/MovementPlugin/Content/Other/BaseRotationToCameraRotation.uasset"));
}

void UBaseDynamicCameraManager::ApplyCamera_Implementation(float DeltaTime)
{
	APlayerController* PlayerController = Cast<APlayerController>(MovementComponent->GetCharacterOwner()->GetController());
	FRotator Rotation = PlayerController->GetControlRotation() + CalculateApplyRotator(DeltaTime);
	PlayerController->SetControlRotation(Rotation);
}

FRotator UBaseDynamicCameraManager::CalculateApplyRotator_Implementation(float DeltaTime)
{
	FRotator LRotator = { 0,0,0 };
	if (CheckApplyCamera())
	{
		FVector LLocalTargetVector = MovementComponent->GetNormalizeCruckVector();
		float LVelocityRotateValue = VelocityToCameraRotation->GetFloatValue(MovementComponent->Velocity.Length());
		float LDotToTargetRotateValue = DotToCameraRotation->GetFloatValue(MovementComponent->GetCharacterOwner()->GetActorForwardVector().Dot(LLocalTargetVector));
		LRotator = UKismetMathLibrary::NormalizedDeltaRotator(MovementComponent->GetCharacterOwner()->GetControlRotation(), LLocalTargetVector.Rotation()) * LVelocityRotateValue * LDotToTargetRotateValue;
	}
	return LRotator;
}

bool UBaseDynamicCameraManager::CheckApplyCamera_Implementation()
{
	return (MovementComponent->GetEnableCruck() && MovementComponent->MovementMode == EMovementMode::MOVE_Falling && MovementComponent->Cabels.IsValidIndex(0));
}