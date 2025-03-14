// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_UseMana.generated.h"

/**
 * 
 */
UCLASS()
class JEMINI_API UGE_UseMana : public UGameplayEffect
{
	GENERATED_BODY()

	virtual void PostInitProperties() override;
	
};
