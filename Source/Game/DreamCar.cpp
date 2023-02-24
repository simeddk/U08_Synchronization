#include "DreamCar.h"

ADreamCar::ADreamCar()
{
	PrimaryActorTick.bCanEverTick = true;

}

void ADreamCar::BeginPlay()
{
	Super::BeginPlay();
	
}

void ADreamCar::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector translation = Velocity * 100 * DeltaTime;
	AddActorWorldOffset(translation);

}

void ADreamCar::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ADreamCar::MoveForward);
}

void ADreamCar::MoveForward(float Value)
{
	Velocity = GetActorForwardVector() * 20 * Value;

}

