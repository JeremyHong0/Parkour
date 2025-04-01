// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JeminiCharacter.h"
#include "LushCharacter.generated.h"

/**
 * 
 */
UCLASS()
class JEMINI_API ALushCharacter : public AJeminiCharacter
{
	GENERATED_BODY()

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* InputMappingContext;
	
protected:
	virtual void BeginPlay() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
public:
	virtual void ResetCombo() override;
	virtual void AttackMelee(bool& bDoesAttackSucceed) override;
	virtual void SaveComboAttack(bool& bDoesAttackSucceed) override;
	
	
};

inline void ALushCharacter::ResetCombo()
{
	Super::ResetCombo();
}

inline void ALushCharacter::AttackMelee(bool& bDoesAttackSucceed)
{
	Super::AttackMelee(bDoesAttackSucceed);
}

inline void ALushCharacter::SaveComboAttack(bool& bDoesAttackSucceed)
{
	Super::SaveComboAttack(bDoesAttackSucceed);
}
