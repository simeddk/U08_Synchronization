#include "DreamCar.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameState.h"

ADreamCar::ADreamCar()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
}

void ADreamCar::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		NetUpdateFrequency = 1;
	}
}

void ADreamCar::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADreamCar, ServerState);
}

FString GetRoleText(ENetRole InRole)
{
	switch (InRole)
	{
		case ROLE_None:				return "None";
		case ROLE_SimulatedProxy:	return "Simulated";
		case ROLE_AutonomousProxy:	return "Autonomous";
		case ROLE_Authority:		return "Authority";
		default:					return "Error";
	}
}

void ADreamCar::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		FMoveState move = CreateMove(DeltaTime);

		SimulateMove(move);
		UnacknowledgeMoves.Add(move);

		UE_LOG(LogTemp, Warning, TEXT("%d"), UnacknowledgeMoves.Num());

		Server_SendMove(move);
	}

	DrawDebugString(GetWorld(), FVector(0, 0, 240), "Local : " + GetRoleText(GetLocalRole()), this, FColor::Black, DeltaTime, false, 1.2f);
	DrawDebugString(GetWorld(), FVector(0, 0, 200), "Remote : " + GetRoleText(GetRemoteRole()), this, FColor::White, DeltaTime, false);
}


FVector ADreamCar::GetAirResistance()
{
	return -Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
}

FVector ADreamCar::GetRollingResistance()
{
	float minusGravity = -GetWorld()->GetGravityZ() / 100;
	float normal = minusGravity * Mass;
	return -Velocity.GetSafeNormal() * RollingCoefficient * normal;
}

void ADreamCar::UpdateLocation(float DeltaTime)
{
	FVector translation = Velocity * 100 * DeltaTime;

	FHitResult hit;
	AddActorWorldOffset(translation, true, &hit);

	if (hit.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;
	}
}

void ADreamCar::UpdateRotation(float DeltaTime, float InSteering)
{
	float direction = (GetActorForwardVector() | Velocity) * DeltaTime;

	float roationAngle = direction / TurningRadius * InSteering;
	FQuat roationYaw(GetActorUpVector(), roationAngle);

	Velocity = roationYaw.RotateVector(Velocity);

	AddActorWorldRotation(roationYaw);
}

void ADreamCar::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ADreamCar::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ADreamCar::MoveRight);
}

void ADreamCar::MoveForward(float Value)
{
	Throttle = Value;
}

void ADreamCar::MoveRight(float Value)
{
	Steering = Value;
}

void ADreamCar::SimulateMove(const FMoveState& Move)
{
	FVector force = GetActorForwardVector() * MaxForce * Move.Throttle;
	force += GetAirResistance();
	force += GetRollingResistance();

	FVector acceleration = force / Mass;
	Velocity = Velocity + acceleration * Move.DeltaTime;

	UpdateRotation(Move.DeltaTime, Move.Steering);
	UpdateLocation(Move.DeltaTime);
}

FMoveState ADreamCar::CreateMove(float DeltaTime)
{
	FMoveState move;
	move.DeltaTime = DeltaTime;
	move.Throttle = Throttle;
	move.Steering = Steering;
	move.Time = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();

	return move;
}

void ADreamCar::ClearAcknowledgeMoves(FMoveState LastMove)
{
	TArray<FMoveState> nakMoves;

	for (const FMoveState& move : UnacknowledgeMoves)
	{
		if (move.Time > LastMove.Time)
		{
			nakMoves.Add(move);
		}
	}

	UnacknowledgeMoves = nakMoves;
}

void ADreamCar::Server_SendMove_Implementation(FMoveState Move)
{
	Throttle = Move.Throttle;
	Steering = Move.Steering;

	SimulateMove(Move);
	ServerState.LastMove = Move;
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = Velocity;
}

void ADreamCar::OnRep_ServerState()
{
	SetActorTransform(ServerState.Transform);
	Velocity = ServerState.Velocity;

	ClearAcknowledgeMoves(ServerState.LastMove);

	for (const FMoveState& move : UnacknowledgeMoves)
	{
		SimulateMove(move);
	}
}

bool ADreamCar::Server_SendMove_Validate(FMoveState Move)
{
	return true;
}
