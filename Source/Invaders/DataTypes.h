#pragma once

#include "CoreMinimal.h"
#include "Engine/TargetPoint.h"
#include "GameFramework/SaveGame.h"

#include "DataTypes.generated.h"

USTRUCT(BlueprintType)
struct FGameRules {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int EnemiesInRow = 10;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int EnemiesInColumn = 5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float EnemySpread = 25.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1"))
    float EnemyShootFrequency = 1.f;
    UPROPERTY(EditAnywhere,
              BlueprintReadWrite,
              meta = (ClampMin = "0.0", ClmpMax = "1"))
    float EnemyShootFrequencyRand = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ForwardMovementAmount = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SideMovementAmount = 200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MinSpeedFactor = 0.25f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxSpeedFactor = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float UfoAppearTimeMin = 6.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float UfoAppearTimeMax = 12.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int LastRow = 8;
};

USTRUCT(BlueprintType)
struct FEnemyDef {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Game)
    TSubclassOf<AActor> ShipClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Game)
    TSubclassOf<AActor> ShipRTClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int Points = 100.f;
};

USTRUCT(BlueprintType)
struct FPlayerDef {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Game)
    TSubclassOf<AActor> ShipClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<ATargetPoint> SpawnPoint;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Speed = 250.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int Lives = 3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ShootFrequency = 0.75f;
};

USTRUCT(BlueprintType)
struct FBulletDef {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSubclassOf<AActor> BulletClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Velocity = 100.f;
};

USTRUCT(BlueprintType)
struct FAsteroidDef {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSubclassOf<AActor> AsteroidClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<ATargetPoint> SpawnPoint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Spread = 200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int Health = 5;
};

UCLASS()
class INVADERS_API UInvadersSaveGame : public USaveGame {
    GENERATED_BODY()
   public:
    UPROPERTY(VisibleAnywhere, Category = Basic)
    uint32 HiScore;
};

// Lightweight game state;
struct FInvadersGameState {
    bool LevelStarted;
    float EnemyProgTime;
    float EnemyAppearAnimTime;
    float PlayerAppearAnimTime;
    float UfoProgTime;
    int TotalEnemyNum;
    int ActiveEnemyNum;
    int ActiveEnemyBullets;
    int ActivePlayerBullets;
    int CurrentLevel;
    int CurrentLives;

    int32 Score;
    int32 PrevHiScore;
    int32 HiScore;
};
