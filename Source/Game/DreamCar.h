#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "DreamCar.generated.h"

UCLASS()
class GAME_API ADreamCar : public APawn
{
	GENERATED_BODY()

public:
	ADreamCar();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	FVector GetAirResistance();
	FVector GetRollingResistance();
	void UpdateLocation(float DeltaTime);
	void UpdateRotation(float DeltaTime);

private:
	void MoveForward(float Value);
	void MoveRight(float Value);

	UFUNCTION(Reliable, Server, WithValidation)
		void Server_MoveForward(float Value);

	UFUNCTION(Reliable, Server, WithValidation)
		void Server_MoveRight(float Value);

private:
	UPROPERTY(EditAnywhere)
		float Mass = 1000;

	UPROPERTY(EditAnywhere)
		float MaxForce = 10000;

	UPROPERTY(EditAnywhere)
		float TurningRadius = 10;

	UPROPERTY(EditAnywhere)
		float DragCoefficient = 16.f;

	UPROPERTY(EditAnywhere)
		float RollingCoefficient = 0.015f;

private:
	FVector Velocity;

	UPROPERTY(Replicated)
		FVector ReplicatedLocation;

	UPROPERTY(Replicated)
		FRotator ReplicatedRotation;

	float Throttle;
	float Steering;
};
