// Fill out your copyright notice in the Description page of Project Settings.


#include "GE_UseMana.h"

namespace JeminiGE
{
	const FName JeminiTag = TEXT("State.JeminiTag");
}

void UGE_UseMana::PostInitProperties()
{
	Super::PostInitProperties();
	InheritableOwnedTagsContainer.AddTag(FGameplayTag::RequestGameplayTag(JeminiGE::JeminiTag));
	GetGrantedTags();
	DurationPolicy = EGameplayEffectDurationType::Instant;
	
}
