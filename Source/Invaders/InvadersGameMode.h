// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/AudioComponent.h"
#include "Components/InputComponent.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/GameMode.h"
#include "Sound/SoundCue.h"

#include "Camera/CameraActor.h"
#include "Engine/TargetPoint.h"

#include "DataTypes.h"
#include "InvadersGameMode.generated.h"

class AGroupActor;
class ACameraActor;

UCLASS()
class INVADERS_API AInvadersGameMode : public AGameMode {
    GENERATED_BODY()

    AActor* PlayerShip;
    AActor* UfoShip;
    TArray<AActor*> EnemyShips;
    TArray<AActor*> EnemyRTShips;
    AActor* EnemyShipGroup;

    FVector2D PlayerMovement;

    bool IsPlayerShooting;

    TArray<int> ShootingEnemies;

    TArray<AActor*> PlayerBullets;
    TArray<AActor*> EnemyBullets;

    TArray<AActor*> Asteroids;
    TArray<int> AsteroidHP;

    TMap<UClass*, int> EnemyPointMap;

    FTimerHandle EnemyShootTHandle;
    FTimerHandle PlayerShootTHandle;
    FTimerHandle UfoAppearTHandle;
    FTimerHandle EnemyAppearTHandle;
    FTimerHandle PlayerAppearTHandle;

    UPROPERTY()
    UUserWidget* GameOverWidget;
    UPROPERTY()
    UUserWidget* MainMenuWidget;
    UPROPERTY()
    UUserWidget* PauseMenuWidget;
    UPROPERTY()
    UUserWidget* TutorialMenuWidget;

   public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Game)
    FGameRules Rules;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Game)
    FPlayerDef PlayerDef;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Game)
    TArray<FEnemyDef> EnemyDefs;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Game)
    TSoftObjectPtr<ACameraActor> GameCamera;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Game)
    TSoftObjectPtr<ACameraActor> MainMenuCamera;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Game)
    TSoftObjectPtr<ATargetPoint> EnemySpawnPoint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Game)
    TSoftObjectPtr<ATargetPoint> UfoSpawnPoint;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Game)
    FAsteroidDef AsteroidDef;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Game)
    FBulletDef PlayerBulletDef;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Game)
    FBulletDef EnemyBulletDef;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI)
    TSubclassOf<UUserWidget> GameOverWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI)
    TSubclassOf<UUserWidget> MainMenuWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI)
    TSubclassOf<UUserWidget> PauseMenuWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI)
    TSubclassOf<UUserWidget> TutorialMenuWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
    USoundWave* BgAudioMusic;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
    USoundWave* ChatterAudioMusic;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
    TArray<USoundWave*> ExplosionSounds;

   protected:
    FInvadersGameState InvadersGameState;

    virtual void InitGame(const FString& MapName,
                          const FString& Options,
                          FString& ErrorMessage) override;

    virtual void StartPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION()
    void QuitGame();

    void InitUIWidgets();

    UFUNCTION()
    void InitGameObjects();

    void InitInput();
    void InitSounds();

    UFUNCTION()
    void ShowMainMenu();

    void ShowRestartMenu();
    void ShowPauseMenu();

    UFUNCTION()
    void ShowTutorialMenu();

    UFUNCTION()
    void RestartInvadersGame();

    UFUNCTION()
    void SpawnEnemies();

    UFUNCTION()
    void SpawnPlayer();

    // Game logic functions
    void UpdatePlayerMovement(float DeltaSeconds);
    void UpdateEnemyGroupMovement(float DeltaSeconds);
    void UpdateUfoMovement(float DeltaSeconds);

    void UpdatePlayerBullets(float DeltaSeconds);
    void UpdateEnemyBullets(float DeltaSeconds);

    void UpdateEnemyAppearAnimation(float DeltaSeconds);
    void UpdatePlayerAppearAnimation(float DeltaSeconds);

    void HandlePlayerShootPressed();
    void HandlePlayerShootReleased();
    void HandlePlayerHit();
    void HandleTogglePausePressed();

    void EnemyShootTimerCallback();
    void PlayerShootTimerCallback();
    void UfoAppearCallback();

    void StartLevelCallback();

    AActor* EmitBullet(TSubclassOf<AActor> PlayerShipClass, FVector Pos);

    void ResetUnits();
    void PauseGame();
    void UnpauseGame();
    void SaveHiScore();
    void LoadHiScore();
};
