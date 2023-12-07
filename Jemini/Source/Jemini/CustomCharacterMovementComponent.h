// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JeminiCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CustomCharacterMovementComponent.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None			UMETA(Hidden),
	CMOVE_Climb			UMETA(DisplayName = "Climb"),
	CMOVE_MAX			UMETA(Hidden),
};

UCLASS()
class JEMINI_API UCustomCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	float WallAttractionForce = 200.f;
	UPROPERTY(EditDefaultsOnly)
	float WallJumpOffForce = 300.f;
	UPROPERTY(EditDefaultsOnly)
	float WallJumpForce = 400.f;
	UPROPERTY(EditDefaultsOnly)
	float MaxClimbSpeed = 300.f;
	UPROPERTY(EditDefaultsOnly)
	float BrakingDecelerationClimbing = 1000.f;
	UPROPERTY(EditDefaultsOnly)
	float ClimbReachDistance = 200.f;
	UPROPERTY(EditDefaultsOnly)
	float MaxSprintSpeed = 600.f;

	UPROPERTY(Transient)
	AJeminiCharacter* MyCharacterOwner;

	bool bWantsToClimb;
	bool bWantsToSprint;

public:
	UCustomCharacterMovementComponent();

protected:
	virtual void InitializeComponent() override;
public:
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxBrakingDeceleration() const override;

	virtual bool CanAttemptJump() const override;
	virtual bool DoJump(bool bReplayingMoves) override;
	
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
protected:
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

private:
	bool TryClimb();
	void PhysClimb(float deltaTime, int32 Iterations);
	
public:
	UFUNCTION(BlueprintCallable)
	void ClimbPressed();
	UFUNCTION(BlueprintCallable)
	void ClimbReleased();

	UFUNCTION(BlueprintCallable)
	void SprintPressed();
	UFUNCTION(BlueprintCallable)
	void SprintReleased();

	UFUNCTION(BlueprintPure)
	bool IsCustomMovementMode(ECustomMovementMode InCustomMovementMode) const;
	UFUNCTION(BlueprintPure)
	bool IsMovementMode(EMovementMode InMovementMode) const;

	UFUNCTION(BlueprintPure)
	bool IsClimbing() const { return IsCustomMovementMode(CMOVE_Climb); }
};
