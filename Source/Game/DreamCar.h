#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "DreamCar.generated.h"

//AutonoMouse -> Authority
USTRUCT()
struct FMoveState 
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
		float Throttle;

	UPROPERTY()
		float Steering;

	UPROPERTY()
		float DeltaTime;

	UPROPERTY()
		float Time;
};

//Authority -> Simulate
USTRUCT()
struct FServerState
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
		FTransform Transform;

	UPROPERTY()
		FVector Velocity;

	UPROPERTY()
		FMoveState LastMove;
};

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
	void UpdateRotation(float DeltaTime, float InSteering);

private:
	void MoveForward(float Value);
	void MoveRight(float Value);

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_SendMove(FMoveState Move);

	void SimulateMove(const FMoveState& Move);

	FMoveState CreateMove(float DeltaTime);
	void ClearAcknowledgeMoves(FMoveState LastMove);

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
	UPROPERTY(ReplicatedUsing = "OnRep_ServerState")
		FServerState ServerState;

private:
	FVector Velocity;

	UFUNCTION()
		void OnRep_ServerState();

	float Throttle;
	float Steering;

	TArray<FMoveState> UnacknowledgeMoves;
};
