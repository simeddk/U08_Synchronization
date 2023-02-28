#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "CMovementComponent.h"
#include "CReplicateComponent.h"
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
	void MoveForward(float Value);
	void MoveRight(float Value);

public:
	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly)
		UCMovementComponent* MovementComponent;

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly)
		UCReplicateComponent* ReplicateComponent;
};
