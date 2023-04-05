// Fill out your copyright notice in the Description page of Project Settings.


#include "SpaceShipActor.h"

// Sets default values
ASpaceShipActor::ASpaceShipActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

        // Our root component will be a sphere that reacts to physics
        CollisionShape =
            CreateDefaultSubobject<USphereComponent>(TEXT("RootComponent"));
        RootComponent = CollisionShape;
        CollisionShape->InitSphereRadius(40.0f);
        CollisionShape->SetCollisionProfileName(TEXT("Player"));

        Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
        Mesh->SetupAttachment(RootComponent);

        static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeVisualAsset(
            TEXT("/Engine/BasicShapes/Cube"));

        if (CubeVisualAsset.Succeeded()) {
          Mesh->SetStaticMesh(CubeVisualAsset.Object);
          Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
          Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, -40.0f));
          Mesh->SetWorldScale3D(FVector(0.8f));
        }

}

// Called when the game starts or when spawned
void ASpaceShipActor::BeginPlay()
{
	Super::BeginPlay();

}
