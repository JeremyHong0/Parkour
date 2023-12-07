// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomCharacterMovementComponent.h"

UCustomCharacterMovementComponent::UCustomCharacterMovementComponent()
{
}

void UCustomCharacterMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();

	MyCharacterOwner = Cast<AJeminiCharacter>(GetOwner());
}

float UCustomCharacterMovementComponent::GetMaxSpeed() const
{
	if(IsMovementMode(MOVE_Walking) && bWantsToSprint)
		return MaxSprintSpeed;
	
	if(MovementMode != MOVE_Custom)
		return Super::GetMaxSpeed();

	if(CustomMovementMode == CMOVE_Climb)
		return MaxClimbSpeed;

	UE_LOG(LogTemp, Fatal, TEXT("Invalid Movement Mode"))
	return -1.f;
}

float UCustomCharacterMovementComponent::GetMaxBrakingDeceleration() const
{
	if(MovementMode != MOVE_Custom)
		return Super::GetMaxBrakingDeceleration();

	return BrakingDecelerationClimbing;
}

bool UCustomCharacterMovementComponent::CanAttemptJump() const
{
	return Super::CanAttemptJump() || IsClimbing();
}

bool UCustomCharacterMovementComponent::DoJump(bool bReplayingMoves)
{
	const bool bWasOnWall = IsClimbing();
	if(Super::DoJump(bReplayingMoves))
	{
		if(bWasOnWall)
		{
			Velocity += FVector::UpVector * WallJumpForce * .5f;
			Velocity += Acceleration.GetSafeNormal2D() * WallJumpForce * .5f;
		}
		return true;
	}
	return false;
}

void UCustomCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	if(IsFalling() && bWantsToClimb)
	{
		if(TryClimb())
			bWantsToClimb = false;
	}
	else if(IsClimbing() && bWantsToClimb)
	{
		const FRotator OldRotation = CharacterOwner->GetActorRotation();
		CharacterOwner->SetActorRotation(FRotator(0.f, OldRotation.Yaw, 0.f));
		SetMovementMode(MOVE_Falling);
		bWantsToClimb = false;
	}
	
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

void UCustomCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);
	
	if(CustomMovementMode == CMOVE_Climb)
		PhysClimb(deltaTime, Iterations);
}

void UCustomCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode,
	uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if(IsFalling())
		bOrientRotationToMovement = true;
}

bool UCustomCharacterMovementComponent::TryClimb()
{
	if(!IsFalling())
		return false;

	FHitResult SurfHit;
	const FVector CapLoc = UpdatedComponent->GetComponentLocation();
	GetWorld()->LineTraceSingleByProfile(SurfHit, CapLoc, CapLoc + UpdatedComponent->GetForwardVector() * ClimbReachDistance, "BlockAll", MyCharacterOwner->GetIgnoreCharacterParams());

	if(!SurfHit.IsValidBlockingHit())
		return false;

	const FQuat NewRotation = FRotationMatrix::MakeFromXZ(-SurfHit.Normal, FVector::UpVector).ToQuat();
	SafeMoveUpdatedComponent(FVector::ZeroVector, NewRotation, false, SurfHit);

	SetMovementMode(MOVE_Custom, CMOVE_Climb);

	bOrientRotationToMovement = false;
	
	return true;
}

void UCustomCharacterMovementComponent::PhysClimb(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}
	if (!CharacterOwner || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}
	
	// Perform the move
	bJustTeleported = false;
	Iterations++;
	const FVector OldLocation = UpdatedComponent->GetComponentLocation();
	FHitResult SurfHit, FloorHit;
	GetWorld()->LineTraceSingleByProfile(SurfHit, OldLocation, OldLocation + UpdatedComponent->GetForwardVector() * ClimbReachDistance, "BlockAll", MyCharacterOwner->GetIgnoreCharacterParams());
	GetWorld()->LineTraceSingleByProfile(FloorHit, OldLocation, OldLocation + FVector::DownVector * 1.2f, "BlockAll", MyCharacterOwner->GetIgnoreCharacterParams());
	if (!SurfHit.IsValidBlockingHit() || FloorHit.IsValidBlockingHit())
	{
		const FRotator OldRotation = CharacterOwner->GetActorRotation();
		CharacterOwner->SetActorRotation(FRotator(0.0f, OldRotation.Yaw, 0.0f));
		SetMovementMode(MOVE_Falling);
		StartNewPhysics(deltaTime, Iterations);
		return;
	}

	// Transform Acceleration
	Acceleration.Z = 0.f;
	Acceleration = Acceleration.RotateAngleAxis(90.f, -UpdatedComponent->GetRightVector());

	// Apply acceleration
	CalcVelocity(deltaTime, 0.f, false, GetMaxBrakingDeceleration());
	Velocity = FVector::VectorPlaneProject(Velocity, SurfHit.Normal);

	// Compute move parameters
	const FVector Delta = deltaTime * Velocity; 
	if (!Delta.IsNearlyZero())
	{
		FHitResult Hit;
		SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);
		FVector WallAttractionDelta = -SurfHit.Normal * WallAttractionForce * deltaTime;
		SafeMoveUpdatedComponent(WallAttractionDelta, UpdatedComponent->GetComponentQuat(), true, Hit);
	}

	Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime; 
}

void UCustomCharacterMovementComponent::ClimbPressed()
{
	if(IsFalling() || IsClimbing())
		bWantsToClimb = true;
}

void UCustomCharacterMovementComponent::ClimbReleased()
{
	bWantsToClimb = false;
}

void UCustomCharacterMovementComponent::SprintPressed()
{
	bWantsToSprint = true;
}

void UCustomCharacterMovementComponent::SprintReleased()
{
	bWantsToSprint = false;
}

bool UCustomCharacterMovementComponent::IsCustomMovementMode(ECustomMovementMode InCustomMovementMode) const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == InCustomMovementMode;
}

bool UCustomCharacterMovementComponent::IsMovementMode(EMovementMode InMovementMode) const
{
	return InMovementMode == MovementMode;
}
