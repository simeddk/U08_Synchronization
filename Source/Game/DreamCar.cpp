#include "DreamCar.h"
#include "DrawDebugHelpers.h"

ADreamCar::ADreamCar()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(false);

	MovementComponent = CreateDefaultSubobject<UCMovementComponent>(TEXT("MovementComponent"));
	ReplicateComponent = CreateDefaultSubobject<UCReplicateComponent>(TEXT("ReplicateComponent"));
}

void ADreamCar::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		NetUpdateFrequency = 1;
	}
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

	DrawDebugString(GetWorld(), FVector(0, 0, 240), "Local : " + GetRoleText(GetLocalRole()), this, FColor::Black, DeltaTime, false, 1.2f);
	DrawDebugString(GetWorld(), FVector(0, 0, 200), "Remote : " + GetRoleText(GetRemoteRole()), this, FColor::White, DeltaTime, false);
}

void ADreamCar::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ADreamCar::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ADreamCar::MoveRight);
}

void ADreamCar::MoveForward(float Value)
{
	if (MovementComponent == nullptr) return;

	MovementComponent->SetThrottle(Value);
}

void ADreamCar::MoveRight(float Value)
{
	if (MovementComponent == nullptr) return;

	MovementComponent->SetSteering(Value);
}
