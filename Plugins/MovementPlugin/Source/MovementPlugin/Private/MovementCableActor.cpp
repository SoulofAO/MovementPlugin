// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementCableActor.h"
#include "CableComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values
AMovementCableActor::AMovementCableActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SceneComponentHand = CreateDefaultSubobject<USceneComponent>("SceneComponentHand");
	SceneComponentHook = CreateDefaultSubobject<USceneComponent>("SceneComponentHook");
	RootComponent = CreateDefaultSubobject<USceneComponent>("RootComponent");
	SceneComponentHand->SetupAttachment(RootComponent);
	SceneComponentHook->SetupAttachment(RootComponent);
	CableComponents.Empty();

	UCableComponent* Cable = CreateDefaultSubobject<UCableComponent>("Cable1");
	Cable->EndLocation = { 0,0,0 };
	CableComponents.Add(Cable);
	Cable->SetupAttachment(SceneComponentHook);
	Cable->SetAttachEndToComponent(SceneComponentHand);
}

// Called when the game starts or when spawned
void AMovementCableActor::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AMovementCableActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	HookTick();
}

void AMovementCableActor::InitilizeHook_Implementation()
{

}

void AMovementCableActor::HookTick()
{
	FHitResult HitResult;

	GetWorld()->LineTraceSingleByChannel(HitResult, SceneComponentHand->GetComponentLocation(), GetLastPoint(), ECollisionChannel::ECC_Visibility);
	if (HitResult.bBlockingHit)
	{
		ArrayPoints.Add(HitResult.ImpactPoint + HitResult.ImpactNormal * 10);
		NewCableComponent(HitResult.ImpactPoint + HitResult.ImpactNormal * 10);
		UKismetSystemLibrary::DrawDebugLine(GetWorld(), HitResult.TraceStart, HitResult.TraceEnd, FLinearColor::Red, 0.1, 10);
	}

	if (ArrayPoints.Num() > 0)
	{
		GetWorld()->LineTraceSingleByChannel(HitResult, SceneComponentHand->GetComponentLocation(), GetSecondLastPoint(), ECollisionChannel::ECC_Visibility);
		if (!HitResult.bBlockingHit)
		{
			FVector Point = SceneComponentHand->GetComponentLocation();
			FVector FirstDirection = UKismetMathLibrary::GetDirectionUnitVector(Point,GetLastPoint());
			FVector SecondDirection = UKismetMathLibrary::GetDirectionUnitVector(GetLastPoint(),GetSecondLastPoint());
			float optimaldot = 0.9;
			float Dot = UKismetMathLibrary::Dot_VectorVector(FirstDirection, SecondDirection);
			if (Dot > optimaldot)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Black, FString::SanitizeFloat(Dot));
				ArrayPoints.RemoveAt(ArrayPoints.Num() - 1);
				RemoveCableComponent(false);
				UKismetSystemLibrary::DrawDebugLine(GetWorld(), HitResult.TraceStart, HitResult.TraceEnd, FLinearColor::Green, 0.1, 10);

			}
		}
	}
}

FVector AMovementCableActor::GetLastPoint()
{
	if(ArrayPoints.Num() == 0)
	{
		return SceneComponentHook->GetComponentLocation();
	}
	return ArrayPoints[ArrayPoints.Num()-1];
}

FVector AMovementCableActor::GetSecondLastPoint()
{
	if (ArrayPoints.Num() <= 1)
	{
		return SceneComponentHook->GetComponentLocation();
	}

	return ArrayPoints[ArrayPoints.Num() - 2];
}



void AMovementCableActor::NewCableComponent(FVector HitLocation)
{
	RemoveCableComponent(true);
	UCableComponent* Cable = NewObject<UCableComponent>(this, UCableComponent::StaticClass());
	Cable->SetupAttachment(RootComponent);
	Cable->RegisterComponent();

	CableComponents.Add(Cable);
	Cable->SetWorldLocation(GetSecondLastPoint(), false, nullptr, ETeleportType::None);
	Cable->EndLocation = HitLocation - GetActorLocation();
	FVector LastPoint = GetSecondLastPoint();
	Cable->CableLength = FVector::Dist(LastPoint, HitLocation);
	//Cable->NumSegments = 2;

	float LastCableLength = 0;

	for (UCableComponent* LocalCable : CableComponents)
	{
		LastCableLength = LastCableLength + LocalCable->CableLength;
	}
	LastCableLength = MaxCableLength - LastCableLength;

	Cable = NewObject<UCableComponent>(this, UCableComponent::StaticClass());
	Cable->SetupAttachment(RootComponent);
	Cable->RegisterComponent();

	CableComponents.Add(Cable);
	Cable->EndLocation = { 0,0,0 };
	Cable->SetWorldLocation(HitLocation, false, nullptr, ETeleportType::None);
	Cable->SetAttachEndToComponent(SceneComponentHand);
	Cable->CableLength = LastCableLength;
}

float AMovementCableActor::GetRealCableLength()
{
	float CableLength = 0.0;
	CableLength = UKismetMathLibrary::Vector_Distance(SceneComponentHand->GetComponentLocation(), GetLastPoint());

	if (ArrayPoints.Num()> 0)
	{
		for (int x = ArrayPoints.Num() - 1; x > 0; x--)
		{
			CableLength = CableLength + UKismetMathLibrary::Vector_Distance(ArrayPoints[x], ArrayPoints[x - 1]);
		}
		CableLength = CableLength + UKismetMathLibrary::Vector_Distance(ArrayPoints[0], SceneComponentHook->GetComponentLocation());
	}
	return CableLength;
}

void AMovementCableActor::RemoveCableComponent(bool RemoveBeforeAdd)
{
	if (CableComponents.Num()>=1)
	{
		CableComponents[CableComponents.Num() - 1]->DestroyComponent();
		CableComponents.RemoveAt(CableComponents.Num() - 1);
	}
	if (!RemoveBeforeAdd)
	{
		UCableComponent* Cable;
		if (CableComponents.Num() >= 1)
		{
			Cable = CableComponents[CableComponents.Num() - 1];
			if (Cable)
			{
				Cable->EndLocation = { 0,0,0 };
				Cable->SetAttachEndToComponent(SceneComponentHand);
			}
		}
	}
}

FVector AMovementCableActor::GetJumpUnitVector()
{
	FVector MainVector = UKismetMathLibrary::GetDirectionUnitVector(SceneComponentHand->GetComponentLocation(),GetLastPoint());
	float MinDistance = 1000;
	float AngularJumpScalar = 1 - UKismetMathLibrary::Clamp(UKismetMathLibrary::Vector_Distance(SceneComponentHand->GetComponentLocation(), GetLastPoint()) / MinDistance, 0, 1);
	FRotator RotatorBetweenCables = UKismetMathLibrary::FindLookAtRotation(MainVector, UKismetMathLibrary::GetDirectionUnitVector(GetLastPoint(), GetSecondLastPoint())) * -1;
	FVector AngluarVector = RotatorBetweenCables.RotateVector(MainVector);

	return MainVector + AngluarVector*AngularJumpScalar;
}

