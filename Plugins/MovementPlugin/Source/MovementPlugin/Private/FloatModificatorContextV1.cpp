// Fill out your copyright notice in the Description page of Project Settings.


#include "FloatModificatorContextV1.h"

float UFloatModificator::ApplyModificator(float Value)
{
	
		switch (OperationType)
		{
		case (EModificatorOperation::Add):
			Value = SelfValue + Value;
			break;

		case (EModificatorOperation::Multiply):
			Value = Value * SelfValue;
			break;
		}

		return Value;
}

UFloatModificator* UFloatModificatorContext::CreateNewModificator()
{
	UFloatModificator* Object = NewObject<UFloatModificator>(this);
	Modificators.Add(Object);
	return Object;
}

float UFloatModificatorContext::ApplyModificators(float Value)
{
	for (UFloatModificator* Modificator : Modificators)
	{
		Value = Modificator->ApplyModificator(Value);
	}
	return Value;
}

void UFloatModificatorContext::RemoveModificatorByName(FString Name)
{
	for (UFloatModificator* Modificator : Modificators)
	{
		if (Name == Modificator->Name)
		{
			Modificators.Remove(Modificator);
			break;
		}
	}
}
