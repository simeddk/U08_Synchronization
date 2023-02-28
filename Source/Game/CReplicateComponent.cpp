#include "CReplicateComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameState.h"

UCReplicateComponent::UCReplicateComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicated(true);
}


void UCReplicateComponent::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent = GetOwner()->FindComponentByClass<UCMovementComponent>();
}

void UCReplicateComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCReplicateComponent, ServerState);
}


void UCReplicateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (MovementComponent == nullptr) return;

	FMoveState lastMove = MovementComponent->GetLastMove();

	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		UnacknowledgeMoves.Add(lastMove);
		Server_SendMove(lastMove);
	}

	if (GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		UpdateServerState(lastMove);
	}

	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		SimulateProxyTick(DeltaTime);
	}
}

void UCReplicateComponent::Server_SendMove_Implementation(FMoveState Move)
{
	if (MovementComponent == nullptr) return;

	ClientSimulateTime += Move.DeltaTime;

	MovementComponent->SimulateMove(Move);
	UpdateServerState(Move);
}

void UCReplicateComponent::OnRep_ServerState()
{
	switch (GetOwnerRole())
	{
		case ROLE_AutonomousProxy: 	AutonomousProxy_OnRep_ServerState();	break;
		case ROLE_SimulatedProxy:	SimulatedProxy_OnRep_ServerState();		break;
	}

	
}

void UCReplicateComponent::AutonomousProxy_OnRep_ServerState()
{
	if (MovementComponent == nullptr) return;

	GetOwner()->SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.Velocity);

	ClearAcknowledgeMoves(ServerState.LastMove);

	for (const FMoveState& move : UnacknowledgeMoves)
	{
		MovementComponent->SimulateMove(move);
	}
}

void UCReplicateComponent::SimulatedProxy_OnRep_ServerState()
{
	if (MovementComponent == nullptr) return;

	ClientTimeBetweenLastUpdate = ClientTimeSinceUpdate;
	ClientTimeSinceUpdate = 0;

	if (!!MeshOffset)
		ClientStartTransform = MeshOffset->GetComponentTransform();
	
	ClientStartVelocity = MovementComponent->GetVelocity();

	GetOwner()->SetActorTransform(ServerState.Transform);
}

void UCReplicateComponent::SimulateProxyTick(float DeltaTime)
{
	ClientTimeSinceUpdate += DeltaTime;

	if (ClientTimeBetweenLastUpdate < .8f) return;
	if (MovementComponent == nullptr) return;
	
	float lerpRatio = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdate;

	FCubicSpline spline = CreateSpline();
	InterpolateLocation(spline, lerpRatio);
	InterpolateVelocity(spline, lerpRatio);
	InterpolateRotation(lerpRatio);
}

FCubicSpline UCReplicateComponent::CreateSpline()
{
	FCubicSpline spline;

	spline.StartLocation = ClientStartTransform.GetLocation();
	spline.TargetLocation = ServerState.Transform.GetLocation();

	spline.StartDerivative = ClientStartVelocity * VelocityToDerivative();
	spline.TargetDerivative = ServerState.Velocity * VelocityToDerivative();

	return spline;
}

void UCReplicateComponent::InterpolateLocation(const FCubicSpline& Spline, float LerpRatio)
{
	FVector newLocation = Spline.InterpolateLocation(LerpRatio);

	if (!!MeshOffset)
		MeshOffset->SetWorldLocation(newLocation);
}

void UCReplicateComponent::InterpolateVelocity(const FCubicSpline& Spline, float LerpRatio)
{
	FVector newDerivative = Spline.InterpolateDerivative(LerpRatio);
	FVector newVelocity = newDerivative / VelocityToDerivative();
	MovementComponent->SetVelocity(newVelocity);
}

void UCReplicateComponent::InterpolateRotation(float LerpRatio)
{
	FQuat startRotation = ClientStartTransform.GetRotation();
	FQuat targetRotation = ServerState.Transform.GetRotation();
	FQuat newRotation = FQuat::Slerp(startRotation, targetRotation, LerpRatio);
	
	if (!!MeshOffset)
		MeshOffset->SetWorldRotation(newRotation);
}

float UCReplicateComponent::VelocityToDerivative()
{
	return ClientTimeBetweenLastUpdate * 100;
}

void UCReplicateComponent::ClearAcknowledgeMoves(FMoveState LastMove)
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

void UCReplicateComponent::UpdateServerState(const FMoveState& Move)
{
	ServerState.LastMove = Move;
	ServerState.Transform = GetOwner()->GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
}

bool UCReplicateComponent::Server_SendMove_Validate(FMoveState Move)
{
	bool noTimeCheat = ClientSimulateTime <= GetWorld()->GetGameState()->GetServerWorldTimeSeconds();

	if (noTimeCheat == false)
	{
		UE_LOG(LogTemp, Error, TEXT("TOO FAST TIME!!!!"));
		return false;
	}

	if (Move.IsValid() == false)
	{
		UE_LOG(LogTemp, Error, TEXT("INVALID MOVE!!!!"));
		return false;
	}

	return true;
}

