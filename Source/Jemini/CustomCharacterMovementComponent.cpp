// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomCharacterMovementComponent.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

// Helper Macros
#if 0
float MacroDuration = 2.f;
#define SLOG(x) GEngine->AddOnScreenDebugMessage(-1, MacroDuration ? MacroDuration : -1.f, FColor::Yellow, x);
#define POINT(x, c) DrawDebugPoint(GetWorld(), x, 10, c, !MacroDuration, MacroDuration);
#define LINE(x1, x2, c) DrawDebugLine(GetWorld(), x1, x2, c, !MacroDuration, MacroDuration);
#define CAPSULE(x, c) DrawDebugCapsule(GetWorld(), x, CapHH(), CapR(), FQuat::Identity, c, !MacroDuration, MacroDuration);
#else
#define SLOG(x)
#define POINT(x, c)
#define LINE(x1, x2, c)
#define CAPSULE(x, c)
#endif

UCustomCharacterMovementComponent::FSavedMove_Jemini::FSavedMove_Jemini()
{
	Saved_bWantsToSprint = 0;
}

bool UCustomCharacterMovementComponent::FSavedMove_Jemini::CanCombineWith(const FSavedMovePtr& NewMove,
	ACharacter* InCharacter, float MaxDelta) const
{
	const FSavedMove_Jemini* NewJeminiMove = static_cast<FSavedMove_Jemini*>(NewMove.Get());

	if(Saved_bWantsToSprint != NewJeminiMove->Saved_bWantsToSprint)
	{
		return false;
	}
	if(Saved_bWantsToClimb != NewJeminiMove->Saved_bWantsToClimb)
	{
		return false;
	}
	return FSavedMove_Character::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void UCustomCharacterMovementComponent::FSavedMove_Jemini::Clear()
{
	FSavedMove_Character::Clear();

	Saved_bPressedJeminiJump = 0;
	Saved_bWantsToSprint = 0;
	Saved_bWantsToClimb = 0;

	Saved_bHadAnimRootMotion = 0;
	Saved_bTransitionFinished = 0;
}

uint8 UCustomCharacterMovementComponent::FSavedMove_Jemini::GetCompressedFlags() const
{
	uint8 Result = FSavedMove_Character::GetCompressedFlags();

	if (Saved_bWantsToSprint)
		Result |= FLAG_Sprint;
	if(Saved_bPressedJeminiJump)
		Result |= FLAG_JumpPressed;
	if(Saved_bWantsToClimb)
		Result |= FLAG_Climb;

	return Result;
} 

void UCustomCharacterMovementComponent::FSavedMove_Jemini::SetMoveFor(ACharacter* C, float InDeltaTime,
	FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	FSavedMove_Character::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	const UCustomCharacterMovementComponent* CharacterMovement = Cast<UCustomCharacterMovementComponent>(C->GetCharacterMovement());

	Saved_bWantsToSprint = CharacterMovement->bWantsToSprint;
	Saved_bWantsToClimb = CharacterMovement->bWantsToClimb;
	Saved_bPressedJeminiJump = CharacterMovement->MyCharacterOwner->bPressedCustomJump;

	Saved_bHadAnimRootMotion = CharacterMovement->bHadAnimRootMotion;
	Saved_bTransitionFinished = CharacterMovement->bTransitionFinished;
}

void UCustomCharacterMovementComponent::FSavedMove_Jemini::PrepMoveFor(ACharacter* C)
{
	FSavedMove_Character::PrepMoveFor(C);

	UCustomCharacterMovementComponent* CharacterMovement = Cast<UCustomCharacterMovementComponent>(C->GetCharacterMovement());

	CharacterMovement->bWantsToSprint = Saved_bWantsToSprint;
	CharacterMovement->bWantsToClimb = Saved_bWantsToClimb;
	CharacterMovement->MyCharacterOwner->bPressedCustomJump = Saved_bPressedJeminiJump;

	CharacterMovement->bHadAnimRootMotion = Saved_bHadAnimRootMotion;
	CharacterMovement->bTransitionFinished = Saved_bTransitionFinished;
}

UCustomCharacterMovementComponent::FNetworkPredictionData_Client_Jemini::FNetworkPredictionData_Client_Jemini(const UCharacterMovementComponent& ClientMovement)
		:Super(ClientMovement)
{
}

FSavedMovePtr UCustomCharacterMovementComponent::FNetworkPredictionData_Client_Jemini::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_Jemini());
}

UCustomCharacterMovementComponent::UCustomCharacterMovementComponent()
{
	NavAgentProps.bCanCrouch = true;
	JeminiServerMoveBitWriter.SetAllowResize(true);
}

void UCustomCharacterMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();

	MyCharacterOwner = Cast<AJeminiCharacter>(GetOwner());
}

FNetworkPredictionData_Client* UCustomCharacterMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr)

	if (ClientPredictionData == nullptr)
	{
		UCustomCharacterMovementComponent* MutableThis = const_cast<UCustomCharacterMovementComponent*>(this);

		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Jemini(*this);
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f; 
	}
	return ClientPredictionData;
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

bool UCustomCharacterMovementComponent::DoJump(bool bReplayingMoves, float DeltaTime)
{
	const bool bWasOnWall = IsClimbing();
	if(Super::DoJump(bReplayingMoves, DeltaTime))
	{
		if(bWasOnWall)
		{
			if (!bReplayingMoves)
			{
				CharacterOwner->PlayAnimMontage(WallJumpMontage);
			}
			const FRotator OldRotation = CharacterOwner->GetActorRotation();
			CharacterOwner->SetActorRotation(FRotator(0.f, OldRotation.Yaw, 0.f));	
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
	
	if (MyCharacterOwner->bPressedCustomJump)
	{
		if (TryMantle())
		{
			MyCharacterOwner->StopJumping();		
		}
		else
		{
			MyCharacterOwner->bPressedCustomJump = false;
			CharacterOwner->bPressedJump = true;
			CharacterOwner->CheckJumpInput(DeltaSeconds);
			bOrientRotationToMovement = true;
		}
	}
	
	if (bTransitionFinished)
	{
		UE_LOG(LogTemp, Warning, TEXT("FINISHED RM"))

		if (TransitionName == "Mantle")
		{
			if (IsValid(TransitionQueuedMontage))
			{
				UE_LOG(LogTemp, Warning, TEXT("Queued Mantle"))

				SetMovementMode(MOVE_Flying);
				CharacterOwner->PlayAnimMontage(TransitionQueuedMontage, TransitionQueuedMontageSpeed);
				TransitionQueuedMontageSpeed = 0.f;
				TransitionQueuedMontage = nullptr;
			}
			else
			{
				SetMovementMode(MOVE_Walking);
			}
		}
		TransitionName = "";
		bTransitionFinished = false;
	}
	
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

void UCustomCharacterMovementComponent::UpdateCharacterStateAfterMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateAfterMovement(DeltaSeconds);
	
	if (!HasAnimRootMotion() && bHadAnimRootMotion && IsMovementMode(MOVE_Flying))
		SetMovementMode(MOVE_Walking);

	if (GetRootMotionSourceByID(TransitionRMS_ID) && GetRootMotionSourceByID(TransitionRMS_ID)->Status.HasFlag(ERootMotionSourceStatusFlags::Finished))
	{
		RemoveRootMotionSourceByID(TransitionRMS_ID);
		bTransitionFinished = true;
	}
	
	bHadAnimRootMotion = HasAnimRootMotion();
}

void UCustomCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	bWantsToSprint = (Flags & FSavedMove_Jemini::FLAG_Sprint) != 0;
	bWantsToClimb  = (Flags & FSavedMove_Jemini::FLAG_Climb) != 0;
}

void UCustomCharacterMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation,
	const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
}

void UCustomCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);
	
	if(CustomMovementMode == CMOVE_Climb)
	{
		UE_LOG(LogTemp, Warning, TEXT("Climb Started"));
		PhysClimb(deltaTime, Iterations);
	}
}

void UCustomCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode,
	uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if(IsFalling())
		bOrientRotationToMovement = true;
}

bool UCustomCharacterMovementComponent::ServerCheckClientError(float ClientTimeStamp, float DeltaTime,
	const FVector& Accel, const FVector& ClientWorldLocation, const FVector& RelativeClientLocation,
	UPrimitiveComponent* ClientMovementBase, FName ClientBaseBoneName, uint8 ClientMovementMode)
{
	if (GetCurrentNetworkMoveData()->NetworkMoveType == FCharacterNetworkMoveData::ENetworkMoveType::NewMove)
	{
		float LocationError = FVector::Dist(UpdatedComponent->GetComponentLocation(), ClientWorldLocation);
		GEngine->AddOnScreenDebugMessage(6, 100.f, FColor::Yellow, FString::Printf(TEXT("Loc: %s"), *ClientWorldLocation.ToString()));
		AccumulatedClientLocationError += LocationError * DeltaTime;
	}
	
	return Super::ServerCheckClientError(ClientTimeStamp, DeltaTime, Accel, ClientWorldLocation, RelativeClientLocation,
	                                     ClientMovementBase,
	                                     ClientBaseBoneName, ClientMovementMode);
}

void UCustomCharacterMovementComponent::CallServerMovePacked(const FSavedMove_Character* NewMove,
	const FSavedMove_Character* PendingMove, const FSavedMove_Character* OldMove)
{
	// Get storage container we'll be using and fill it with movement data
	FCharacterNetworkMoveDataContainer& MoveDataContainer = GetNetworkMoveDataContainer();
	MoveDataContainer.ClientFillNetworkMoveData(NewMove, PendingMove, OldMove);

	// Reset bit writer without affecting allocations
	FBitWriterMark BitWriterReset;
	BitWriterReset.Pop(JeminiServerMoveBitWriter);

	// 'static' to avoid reallocation each invocation
	static FCharacterServerMovePackedBits PackedBits;
	UNetConnection* NetConnection = CharacterOwner->GetNetConnection();	

	{
		// Extract the net package map used for serializing object references.
		JeminiServerMoveBitWriter.PackageMap = NetConnection ? ToRawPtr(NetConnection->PackageMap) : nullptr;
	}

	if (JeminiServerMoveBitWriter.PackageMap == nullptr)
	{
		UE_LOG(LogNetPlayerMovement, Error, TEXT("CallServerMovePacked: Failed to find a NetConnection/PackageMap for data serialization!"));
		return;
	}

	// Serialize move struct into a bit stream
	if (!MoveDataContainer.Serialize(*this, JeminiServerMoveBitWriter, JeminiServerMoveBitWriter.PackageMap) || JeminiServerMoveBitWriter.IsError())
	{
		UE_LOG(LogNetPlayerMovement, Error, TEXT("CallServerMovePacked: Failed to serialize out movement data!"));
		return;
	}

	// Copy bits to our struct that we can NetSerialize to the server.
	PackedBits.DataBits.SetNumUninitialized(JeminiServerMoveBitWriter.GetNumBits());
	
	check(PackedBits.DataBits.Num() >= JeminiServerMoveBitWriter.GetNumBits());
	FMemory::Memcpy(PackedBits.DataBits.GetData(), JeminiServerMoveBitWriter.GetData(), JeminiServerMoveBitWriter.GetNumBytes());

	TotalBitsSent += PackedBits.DataBits.Num();

	// Send bits to server!
	ServerMovePacked_ClientSend(PackedBits);

	MarkForClientCameraUpdate();
}

bool UCustomCharacterMovementComponent::IsServer() const
{
	return CharacterOwner->HasAuthority();
}

bool UCustomCharacterMovementComponent::TryClimb()
{
	if(!IsFalling())
	{
		return false;
	}

	FHitResult SurfHit;
	const FVector CapLoc = UpdatedComponent->GetComponentLocation();
	GetWorld()->LineTraceSingleByProfile(SurfHit, CapLoc, CapLoc + UpdatedComponent->GetForwardVector() * ClimbReachDistance, "BlockAll", MyCharacterOwner->GetIgnoreCharacterParams());

	if(!SurfHit.IsValidBlockingHit())
	{
		return false;
	}

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
	if (!CharacterOwner || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion()
		&& !CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)))
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
	GetWorld()->LineTraceSingleByProfile(FloorHit, OldLocation, OldLocation + -UpdatedComponent->GetUpVector() * CapHH() * 1.3f, "BlockAll", MyCharacterOwner->GetIgnoreCharacterParams());
	LINE(OldLocation, OldLocation + (-UpdatedComponent->GetUpVector()) * CapHH() * 1.2f, FColor::Red)

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

bool UCustomCharacterMovementComponent::TryMantle()
{
	if (!(IsMovementMode(MOVE_Walking) && !IsCrouching()) && !IsMovementMode(MOVE_Falling))
		return false;

	// Helper Variables
	FVector BaseLoc = UpdatedComponent->GetComponentLocation() + FVector::DownVector * CapHH();
	FVector Fwd = UpdatedComponent->GetForwardVector().GetSafeNormal2D();
	auto Params = MyCharacterOwner->GetIgnoreCharacterParams();
	float MaxHeight = CapHH() * 2+ MantleReachHeight;
	float CosMMWSA = FMath::Cos(FMath::DegreesToRadians(MantleMinWallSteepnessAngle));
	float CosMMSA = FMath::Cos(FMath::DegreesToRadians(MantleMaxSurfaceAngle));
	float CosMMAA = FMath::Cos(FMath::DegreesToRadians(MantleMaxAlignmentAngle));

	
	// Check Front Face
	FHitResult FrontHit;
	float CheckDistance = FMath::Clamp(Velocity | Fwd, CapR() + 30, MantleMaxDistance);
	FVector FrontStart = BaseLoc + FVector::UpVector * (MaxStepHeight - 1);
	for (int i = 0; i < 6; i++)
	{
		if (GetWorld()->LineTraceSingleByProfile(FrontHit, FrontStart, FrontStart + Fwd * CheckDistance, "BlockAll", Params))
			break;
		FrontStart += FVector::UpVector * (2.f * CapHH() - (MaxStepHeight - 1)) / 5;
	}
	if (!FrontHit.IsValidBlockingHit()) return false;
	float CosWallSteepnessAngle = FrontHit.Normal | FVector::UpVector;
	if (FMath::Abs(CosWallSteepnessAngle) > CosMMWSA || (Fwd | -FrontHit.Normal) < CosMMAA)
		return false;

	// Check Height
	TArray<FHitResult> HeightHits;
	FHitResult SurfaceHit;
	FVector WallUp = FVector::VectorPlaneProject(FVector::UpVector, FrontHit.Normal).GetSafeNormal();
	float WallCos = FVector::UpVector | FrontHit.Normal;
	float WallSin = FMath::Sqrt(1 - WallCos * WallCos);
	FVector TraceStart = FrontHit.Location + Fwd + WallUp * (MaxHeight - (MaxStepHeight - 1)) / WallSin;
	if (!GetWorld()->LineTraceMultiByProfile(HeightHits, TraceStart, FrontHit.Location + Fwd, "BlockAll", Params))
		return false;
	for (const FHitResult& Hit : HeightHits)
	{
		if (Hit.IsValidBlockingHit())
		{
			SurfaceHit = Hit;
			break;
		}
	}
	if (!SurfaceHit.IsValidBlockingHit() || (SurfaceHit.Normal | FVector::UpVector) < CosMMSA)
		return false;
	float Height = (SurfaceHit.Location - BaseLoc) | FVector::UpVector;

	if (Height > MaxHeight)
		return false;
	
	// Check Clearance
	float SurfaceCos = FVector::UpVector | SurfaceHit.Normal;
	float SurfaceSin = FMath::Sqrt(1 - SurfaceCos * SurfaceCos);
	FVector ClearCapLoc = SurfaceHit.Location + Fwd * CapR() + FVector::UpVector * (CapHH() + 1 + CapR() * 2 * SurfaceSin);
	FCollisionShape CapShape = FCollisionShape::MakeCapsule(CapR(), CapHH());
	if (GetWorld()->OverlapAnyTestByProfile(ClearCapLoc, FQuat::Identity, "BlockAll", CapShape, Params))
	{
		return false;
	}
	// Mantle Selection
	FVector ShortMantleTarget = GetMantleStartLocation(FrontHit, SurfaceHit, false);
	FVector TallMantleTarget = GetMantleStartLocation(FrontHit, SurfaceHit, true);
	
	bool bTallMantle = false;
	if (IsMovementMode(MOVE_Walking) && Height > CapHH() * 2)
		bTallMantle = true;
	else if (IsMovementMode(MOVE_Falling) && (Velocity | FVector::UpVector) < 0)
	{
		if (!GetWorld()->OverlapAnyTestByProfile(TallMantleTarget, FQuat::Identity, "BlockAll", CapShape, Params))
			bTallMantle = true;
	}
	FVector TransitionTarget = bTallMantle ? TallMantleTarget : ShortMantleTarget;

	// Perform Transition to Mantle
	float UpSpeed = Velocity | FVector::UpVector;
	float TransDistance = FVector::Dist(TransitionTarget, UpdatedComponent->GetComponentLocation());

	TransitionQueuedMontageSpeed = FMath::GetMappedRangeValueClamped(FVector2D(-500, 750), FVector2D(.9f, 1.2f), UpSpeed);
	TransitionRMS.Reset();
	TransitionRMS = MakeShared<FRootMotionSource_MoveToForce>();
	TransitionRMS->AccumulateMode = ERootMotionAccumulateMode::Override;
	
	TransitionRMS->Duration = FMath::Clamp(TransDistance / 500.f, .1f, .25f);
	TransitionRMS->StartLocation = UpdatedComponent->GetComponentLocation();
	TransitionRMS->TargetLocation = TransitionTarget;

	// Apply Transition Root Motion Source
	Velocity = FVector::ZeroVector;
	SetMovementMode(MOVE_Flying);
	TransitionRMS_ID = ApplyRootMotionSource(TransitionRMS);
	TransitionName = "Mantle";

	// Animations
	if (bTallMantle)
	{
		TransitionQueuedMontage = TallMantleMontage;
		CharacterOwner->PlayAnimMontage(TransitionTallMantleMontage, 1 / TransitionRMS->Duration);
		if (IsServer())
			Proxy_bTallMantle = !Proxy_bTallMantle;
	}
	else
	{
		TransitionQueuedMontage = ShortMantleMontage;
		CharacterOwner->PlayAnimMontage(TransitionShortMantleMontage, 1 / TransitionRMS->Duration);
		if (IsServer())
			Proxy_bShortMantle = !Proxy_bShortMantle;
	}

	return true;

}

FVector UCustomCharacterMovementComponent::GetMantleStartLocation(FHitResult FrontHit, FHitResult SurfaceHit,
	bool bTallMantle) const
{
	float CosWallSteepnessAngle = FrontHit.Normal | FVector::UpVector;
	float DownDistance = bTallMantle ? CapHH() * 2.f : MaxStepHeight - 1;
	FVector EdgeTangent = FVector::CrossProduct(SurfaceHit.Normal, FrontHit.Normal).GetSafeNormal();

	FVector MantleStart = SurfaceHit.Location;
	MantleStart += FrontHit.Normal.GetSafeNormal2D() * (2.f + CapR());
	MantleStart += UpdatedComponent->GetForwardVector().GetSafeNormal2D().ProjectOnTo(EdgeTangent) * CapR() * .3f;
	MantleStart += FVector::UpVector * CapHH();
	MantleStart += FVector::DownVector * DownDistance;
	MantleStart += FrontHit.Normal.GetSafeNormal2D() * CosWallSteepnessAngle * DownDistance;

	return MantleStart;
}

float UCustomCharacterMovementComponent::CapR() const
{
	return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();
}

float UCustomCharacterMovementComponent::CapHH() const
{
	return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
}

void UCustomCharacterMovementComponent::ClimbPressed()
{
	//Todo: Fix problem with climbing doesn't work on client.
	if(IsFalling() || IsClimbing())
	{
		bWantsToClimb = true;
	}
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

void UCustomCharacterMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UCustomCharacterMovementComponent, Proxy_bShortMantle, COND_SkipOwner)
	DOREPLIFETIME_CONDITION(UCustomCharacterMovementComponent, Proxy_bTallMantle, COND_SkipOwner)
}

void UCustomCharacterMovementComponent::OnRep_ShortMantle() const
{
	CharacterOwner->PlayAnimMontage(ProxyShortMantleMontage);
}

void UCustomCharacterMovementComponent::OnRep_TallMantle() const
{
	CharacterOwner->PlayAnimMontage(ProxyTallMantleMontage);
}
