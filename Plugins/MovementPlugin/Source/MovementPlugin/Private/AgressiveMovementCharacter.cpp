// Fill out your copyright notice in the Description page of Project Settings.


#include "AgressiveMovementCharacter.h"
#include "AgressiveMovementComponent.h"
// Sets default values
AAgressiveMovementCharacter::AAgressiveMovementCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UAgressiveMovementComponent>(ACharacter::CharacterMovementComponentName))
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AAgressiveMovementCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAgressiveMovementCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AAgressiveMovementCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

