#include "DreamCar.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"

ADreamCar::ADreamCar()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
}

void ADreamCar::BeginPlay()
{
	Super::BeginPlay();
	
}

void ADreamCar::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADreamCar, ReplicatedLocation);
	DOREPLIFETIME(ADreamCar, ReplicatedRotation);
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

	FVector force = GetActorForwardVector() * MaxForce * Throttle;
	force += GetAirResistance();
	force += GetRollingResistance();

	FVector acceleration = force / Mass;
	Velocity = Velocity + acceleration * DeltaTime;

	UpdateRotation(DeltaTime);
	UpdateLocation(DeltaTime);

	if (HasAuthority())
	{
		ReplicatedLocation = GetActorLocation();
		ReplicatedRotation = GetActorRotation();
	}
	else
	{
		SetActorLocation(ReplicatedLocation);
		SetActorRotation(ReplicatedRotation);
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

void ADreamCar::UpdateRotation(float DeltaTime)
{
	float direction = (GetActorForwardVector() | Velocity) * DeltaTime;

	float roationAngle = direction / TurningRadius * Steering;
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
	Server_MoveForward(Value);
}

void ADreamCar::MoveRight(float Value)
{
	Steering = Value;
	Server_MoveRight(Value);
}

void ADreamCar::Server_MoveForward_Implementation(float Value)
{
	Throttle = Value;
}

void ADreamCar::Server_MoveRight_Implementation(float Value)
{
	Steering = Value;
}

bool ADreamCar::Server_MoveForward_Validate(float Value)
{
	return FMath::Abs(Value) <= 1;
}

bool ADreamCar::Server_MoveRight_Validate(float Value)
{
	return FMath::Abs(Value) <= 1;
}
