// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"

#include "SpaceShipActor.generated.h"

UCLASS()
class INVADERS_API ASpaceShipActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASpaceShipActor();

        UPROPERTY(EditAnywhere)
        UStaticMeshComponent* Mesh;

        UPROPERTY(EditAnywhere)
        USphereComponent* CollisionShape;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

};
