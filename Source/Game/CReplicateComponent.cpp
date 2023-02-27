#include "CReplicateComponent.h"
#include "Net/UnrealNetwork.h"

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
	ClientTimeBetweenLastUpdate = ClientTimeSinceUpdate;
	ClientTimeSinceUpdate = 0;

	ClientStartLocation = GetOwner()->GetActorLocation();
}

void UCReplicateComponent::SimulateProxyTick(float DeltaTime)
{
	ClientTimeSinceUpdate += DeltaTime;

	if (ClientTimeBetweenLastUpdate < KINDA_SMALL_NUMBER) return;

	FVector startLocation = ClientStartLocation;
	FVector targetLocation = ServerState.Transform.GetLocation();
	float lerpRatio = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdate;

	FVector newLocation = FMath::LerpStable(startLocation, targetLocation, lerpRatio);

	GetOwner()->SetActorLocation(newLocation);
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
	return true;
}

