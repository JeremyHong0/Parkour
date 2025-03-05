// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "AbilitySystemInterface.h"

#include "JeminiCharacter.generated.h"


UCLASS(config=Game)
class AJeminiCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* AttackAction;
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Movement)
	class UCustomCharacterMovementComponent* CustomCharacterMovementComponent;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override
	{
		return AblilitySystemComponent;
	}
	
public:
	AJeminiCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void Jump() override;
	virtual void StopJumping() override;

	void AttackMelee();
protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, category="GAS", meta = (AllowPrivateAccess = "true"))
	class UAbilitySystemComponent* AblilitySystemComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, category="GAS", meta = (AllowPrivateAccess = "true"))
	const class UBaseAttributeSet* BaseAttributeSet; 

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
			

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay();

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	bool bPressedCustomJump;
	
	FCollisionQueryParams GetIgnoreCharacterParams() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Pawn)
	UAnimMontage* AttackMeleeAnim;
};

