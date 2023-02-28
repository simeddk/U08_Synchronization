#include "CMovementComponent.h"
#include "GameFramework/GameState.h"

UCMovementComponent::UCMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UCMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	
}

void UCMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwnerRole() == ROLE_AutonomousProxy || GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		LastMove = CreateMove(DeltaTime);
		SimulateMove(LastMove);
	}
}

void UCMovementComponent::SimulateMove(const FMoveState& Move)
{
	FVector force = GetOwner()->GetActorForwardVector() * MaxForce * Move.Throttle;
	force += GetAirResistance();
	force += GetRollingResistance();

	FVector acceleration = force / Mass;
	Velocity = Velocity + acceleration * Move.DeltaTime;

	UpdateRotation(Move.DeltaTime, Move.Steering);
	UpdateLocation(Move.DeltaTime);
}

FMoveState UCMovementComponent::CreateMove(float DeltaTime)
{
	FMoveState move;
	move.DeltaTime = DeltaTime * TimeCheatValue;
	move.Throttle = Throttle * MoveCheatValue;
	move.Steering = Steering;
	move.Time = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();

	return move;
}

FVector UCMovementComponent::GetAirResistance()
{
	return -Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
}

FVector UCMovementComponent::GetRollingResistance()
{
	float minusGravity = -GetWorld()->GetGravityZ() / 100;
	float normal = minusGravity * Mass;
	return -Velocity.GetSafeNormal() * RollingCoefficient * normal;
}

void UCMovementComponent::UpdateLocation(float DeltaTime)
{
	FVector translation = Velocity * 100 * DeltaTime;

	FHitResult hit;
	GetOwner()->AddActorWorldOffset(translation, true, &hit);

	if (hit.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;
	}
}

void UCMovementComponent::UpdateRotation(float DeltaTime, float InSteering)
{
	float direction = (GetOwner()->GetActorForwardVector() | Velocity) * DeltaTime;

	float roationAngle = direction / TurningRadius * InSteering;
	FQuat roationYaw(GetOwner()->GetActorUpVector(), roationAngle);

	Velocity = roationYaw.RotateVector(Velocity);

	GetOwner()->AddActorWorldRotation(roationYaw);
}

