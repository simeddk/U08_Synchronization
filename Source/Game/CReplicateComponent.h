#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CMovementComponent.h"
#include "CReplicateComponent.generated.h"

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

struct FCubicSpline
{
	FVector StartLocation, TargetLocation;
	FVector StartDerivative, TargetDerivative;

	FVector InterpolateLocation(float LerpRatio) const
	{
		return FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	}

	FVector InterpolateDerivative(float LerpRatio) const
	{
		return FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GAME_API UCReplicateComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCReplicateComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void ClearAcknowledgeMoves(FMoveState LastMove);
	void UpdateServerState(const FMoveState& Move);

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_SendMove(FMoveState Move);
		
	UPROPERTY(ReplicatedUsing = "OnRep_ServerState")
		FServerState ServerState;

	UFUNCTION()
		void OnRep_ServerState();

	void AutonomousProxy_OnRep_ServerState();
	void SimulatedProxy_OnRep_ServerState();

	void SimulateProxyTick(float DeltaTime);
	FCubicSpline CreateSpline();
	void InterpolateLocation(const FCubicSpline& Spline, float LerpRatio);
	void InterpolateVelocity(const FCubicSpline& Spline, float LerpRatio);
	void InterpolateRotation(float LerpRatio);
	float VelocityToDerivative();

	TArray<FMoveState> UnacknowledgeMoves;

	float ClientTimeSinceUpdate;
	float ClientTimeBetweenLastUpdate;
	FTransform ClientStartTransform;
	FVector ClientStartVelocity;

	float ClientSimulateTime;

	UCMovementComponent* MovementComponent;

	UPROPERTY()
		USceneComponent* MeshOffset;

	UFUNCTION(BlueprintCallable, meta = (CallableWithoutWorldContext))
		void SetMeshOffset(USceneComponent* InMeshOffset) {	MeshOffset = InMeshOffset; }
};
