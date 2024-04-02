// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JeminiCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CustomCharacterMovementComponent.generated.h"

UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None			UMETA(Hidden),
	CMOVE_Climb			UMETA(DisplayName = "Climb"),
	CMOVE_WallRun		UMETA(DisplayName = "Wall Run"),
	CMOVE_MAX			UMETA(Hidden),
};

UCLASS()
class JEMINI_API UCustomCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	class FSavedMove_Jemini : public FSavedMove_Character
	{
	public:
		FSavedMove_Jemini();
		
		enum CompressedFlags
		{
			FLAG_Sprint			= 0x10,
			FLAG_Climb   		= 0x20,
			FLAG_Custom_1		= 0x40,
			FLAG_Custom_2		= 0x80,
		};
		
		uint8 Saved_bPressedJeminiJump:1;
		uint8 Saved_bWantsToSprint:1;
		uint8 Saved_bWantsToClimb:1;
		
		uint8 Saved_bTransitionFinished:1;
		uint8 Saved_bHadAnimRootMotion:1;
		uint8 Saved_bWallRunIsRight:1;

		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
		virtual void Clear() override;
		virtual uint8 GetCompressedFlags() const override;
		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
		virtual void PrepMoveFor(ACharacter* C) override;
	};

	class FNetworkPredictionData_Client_Jemini : public FNetworkPredictionData_Client_Character
	{
	public:
		FNetworkPredictionData_Client_Jemini(const UCharacterMovementComponent& ClientMovement);

		typedef FNetworkPredictionData_Client_Character Super;

		virtual FSavedMovePtr AllocateNewMove() override;
	};

	UPROPERTY(EditDefaultsOnly) float MinWallRunSpeed=200.f;
	UPROPERTY(EditDefaultsOnly) float MaxWallRunSpeed=800.f;
	UPROPERTY(EditDefaultsOnly) float MaxVerticalWallRunSpeed=200.f;
	UPROPERTY(EditDefaultsOnly) float WallRunPullAwayAngle=75;
	UPROPERTY(EditDefaultsOnly) float MinWallRunHeight=50.f;
	UPROPERTY(EditDefaultsOnly) UCurveFloat* WallRunGravityScaleCurve;

	UPROPERTY(EditDefaultsOnly) float WallAttractionForce = 200.f;
	UPROPERTY(EditDefaultsOnly) float WallJumpOffForce = 300.f;
	UPROPERTY(EditDefaultsOnly) float WallJumpForce = 400.f;
	UPROPERTY(EditDefaultsOnly) float MaxClimbSpeed = 300.f;
	UPROPERTY(EditDefaultsOnly) float BrakingDecelerationClimbing = 1000.f;
	UPROPERTY(EditDefaultsOnly) float ClimbReachDistance = 200.f;
	UPROPERTY(EditDefaultsOnly) float MaxSprintSpeed = 600.f;

	UPROPERTY(EditDefaultsOnly) float MantleMaxDistance = 200;
	UPROPERTY(EditDefaultsOnly) float MantleReachHeight = 50;
	UPROPERTY(EditDefaultsOnly) float MinMantleDepth = 30;
	UPROPERTY(EditDefaultsOnly) float MantleMinWallSteepnessAngle = 75;
	UPROPERTY(EditDefaultsOnly) float MantleMaxSurfaceAngle = 40;
	UPROPERTY(EditDefaultsOnly) float MantleMaxAlignmentAngle = 45;
	UPROPERTY(EditDefaultsOnly) UAnimMontage* TallMantleMontage;
	UPROPERTY(EditDefaultsOnly) UAnimMontage* TransitionTallMantleMontage;
	UPROPERTY(EditDefaultsOnly) UAnimMontage* ShortMantleMontage;
	UPROPERTY(EditDefaultsOnly) UAnimMontage* TransitionShortMantleMontage;
	UPROPERTY(EditDefaultsOnly) UAnimMontage* ProxyTallMantleMontage;
	UPROPERTY(EditDefaultsOnly) UAnimMontage* ProxyShortMantleMontage;
	UPROPERTY(EditDefaultsOnly) UAnimMontage* WallJumpMontage;

	
	UPROPERTY(Transient) AJeminiCharacter* MyCharacterOwner;
	
	TSharedPtr<FRootMotionSource_MoveToForce> TransitionRMS;
	FString TransitionName;
	UPROPERTY(Transient) UAnimMontage* TransitionQueuedMontage;
	float TransitionQueuedMontageSpeed;
	int TransitionRMS_ID;
	
	bool bWantsToSprint;
	bool bWantsToClimb;
	bool bTransitionFinished;
	bool bHadAnimRootMotion;
	bool Safe_bWallRunIsRight;

	int CorrectionCount=0;
	int TotalBitsSent=0;

	float AccumulatedClientLocationError=0.f;

	UPROPERTY(ReplicatedUsing=OnRep_ShortMantle) bool Proxy_bShortMantle;
	UPROPERTY(ReplicatedUsing=OnRep_TallMantle) bool Proxy_bTallMantle;

public:
	UCustomCharacterMovementComponent();

protected:
	virtual void InitializeComponent() override;
public:
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxBrakingDeceleration() const override;

	virtual bool CanAttemptJump() const override;
	virtual bool DoJump(bool bReplayingMoves) override;
	
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds) override;

protected:
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	FNetBitWriter JeminiServerMoveBitWriter;
	virtual bool ServerCheckClientError(float ClientTimeStamp, float DeltaTime, const FVector& Accel, const FVector& ClientWorldLocation, const FVector& RelativeClientLocation, UPrimitiveComponent* ClientMovementBase, FName ClientBaseBoneName, uint8 ClientMovementMode) override;
	virtual void CallServerMovePacked(const FSavedMove_Character* NewMove, const FSavedMove_Character* PendingMove, const FSavedMove_Character* OldMove) override;

private:
	bool IsServer() const;
	bool TryClimb();
	void PhysClimb(float deltaTime, int32 Iterations);

	bool TryMantle();
	FVector GetMantleStartLocation(FHitResult FrontHit, FHitResult SurfaceHit, bool bTallMantle) const;

	bool TryWallRun();
	void PhysWallRun(float deltaTime, int32 Iterations);

	float CapR() const;
	float CapHH() const;
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
	UFUNCTION(BlueprintPure)
	bool IsWallRunning() const { return IsCustomMovementMode(CMOVE_WallRun); }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
	UFUNCTION() void OnRep_ShortMantle() const;
	UFUNCTION() void OnRep_TallMantle() const;
};
