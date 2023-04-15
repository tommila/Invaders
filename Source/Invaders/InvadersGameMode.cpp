// Copyright Epic Games, Inc. All Rights Reserved.

#include "InvadersGameMode.h"
#include "Blueprint/UserWidget.h"
#include "CollisionShape.h"
#include "Components/ActorComponent.h"
#include "Components/Button.h"
#include "Components/InputComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextBlock.h"
#include "DataTypes.h"
#include "Engine/Blueprint.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/EngineTypes.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameUtils.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Text.h"
#include "Kismet/GameplayStatics.h"
#include "Layout/Geometry.h"
#include "Logging/LogMacros.h"
#include "Logging/LogVerbosity.h"
#include "Math/MathFwd.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/AssertionMacros.h"
#include "Misc/MemStack.h"
#include "Sound/SoundWave.h"
#include "Templates/Casts.h"
#include "UObject/UObjectGlobals.h"

const int MAX_BULLETS = 20;

void AInvadersGameMode::InitGame(const FString& MapName,
                                 const FString& Options,
                                 FString& ErrorMessage) {
    Super::InitGame(MapName, Options, ErrorMessage);
    const int EnemyNum = Rules.EnemiesInRow * Rules.EnemiesInColumn;
    InvadersGameState = {
        false, 0, 0, 0, 0, EnemyNum, EnemyNum, 0, 0, 0, PlayerDef.Lives, 0,
    };

    PlayerBullets.Reserve(MAX_BULLETS);
    EnemyBullets.Reserve(MAX_BULLETS);

    ShootingEnemies.Reserve(10);
    EnemyShips.Reserve(InvadersGameState.TotalEnemyNum);
    Asteroids.Reserve(4);
    AsteroidHP.Reserve(4);
}

void AInvadersGameMode::QuitGame() {
    UKismetSystemLibrary::QuitGame(GetWorld(),
                                   GetWorld()->GetFirstPlayerController(),
                                   EQuitPreference::Quit, false);
}

void AInvadersGameMode::StartPlay() {
    Super::StartPlay();

    // Map enemy scoring points
    for (FEnemyDef& Def : EnemyDefs) {
        EnemyPointMap.Add(Def.ShipClass, Def.Points);
    }

    LoadHiScore();

    InitInput();
    InitGameObjects();
    InitUIWidgets();

    InitSounds();

    ShowMainMenu();
}

/// INIT FUNCTIONS //

void AInvadersGameMode::InitInput() {
    // Input
    InputComponent = NewObject<UInputComponent>(this);
    InputComponent->RegisterComponent();
}

void AInvadersGameMode::InitSounds() {
    check(BgAudioMusic);
    check(ChatterAudioMusic);
    for (auto* Sfx : ExplosionSounds) {
        check(Sfx);
    }

    UGameplayStatics::PlaySound2D(GetWorld(), BgAudioMusic);
    UGameplayStatics::PlaySound2D(GetWorld(), ChatterAudioMusic);
}

void AInvadersGameMode::InitUIWidgets() {
    UWorld* World = GetWorld();

    check(MainMenuWidgetClass);
    check(GameOverWidgetClass);
    check(PauseMenuWidgetClass);
    check(TutorialMenuWidgetClass);

    MainMenuWidget = CreateWidget(World, MainMenuWidgetClass);
    GameOverWidget = CreateWidget(World, GameOverWidgetClass);
    PauseMenuWidget = CreateWidget(World, PauseMenuWidgetClass);
    TutorialMenuWidget = CreateWidget(World, TutorialMenuWidgetClass);

    UButton* StartGameButton =
        Cast<UButton>(MainMenuWidget->GetWidgetFromName("StartGameBtn"));
    UButton* ExitGameButton =
        Cast<UButton>(MainMenuWidget->GetWidgetFromName("ExitGameBtn"));

    UButton* GameOverRestartButton =
        Cast<UButton>(GameOverWidget->GetWidgetFromName("RestartBtn"));
    UButton* GameOverExitButton =
        Cast<UButton>(GameOverWidget->GetWidgetFromName("ExitBtn"));

    UButton* PauseRestartButton =
        Cast<UButton>(PauseMenuWidget->GetWidgetFromName("RestartBtn"));
    UButton* PauseExitGameButton =
        Cast<UButton>(PauseMenuWidget->GetWidgetFromName("ExitBtn"));

    check(StartGameButton);
    check(ExitGameButton);
    check(GameOverRestartButton);
    check(GameOverExitButton);
    check(PauseRestartButton);
    check(ExitGameButton);

    StartGameButton->OnClicked.AddDynamic(this,
                                          &AInvadersGameMode::ShowTutorialMenu);
    StartGameButton->OnPressed.AddDynamic(this,
                                          &AInvadersGameMode::ShowTutorialMenu);

    ExitGameButton->OnClicked.AddDynamic(this, &AInvadersGameMode::QuitGame);
    ExitGameButton->OnPressed.AddDynamic(this, &AInvadersGameMode::QuitGame);

    GameOverRestartButton->OnClicked.AddDynamic(
        this, &AInvadersGameMode::RestartInvadersGame);
    GameOverRestartButton->OnPressed.AddDynamic(
        this, &AInvadersGameMode::RestartInvadersGame);
    PauseRestartButton->OnClicked.AddDynamic(
        this, &AInvadersGameMode::RestartInvadersGame);
    PauseRestartButton->OnPressed.AddDynamic(
        this, &AInvadersGameMode::RestartInvadersGame);

    GameOverExitButton->OnClicked.AddDynamic(this,
                                             &AInvadersGameMode::ShowMainMenu);
    GameOverExitButton->OnPressed.AddDynamic(this,
                                             &AInvadersGameMode::ShowMainMenu);
    PauseExitGameButton->OnClicked.AddDynamic(this,
                                              &AInvadersGameMode::ShowMainMenu);
    PauseExitGameButton->OnPressed.AddDynamic(this,
                                              &AInvadersGameMode::ShowMainMenu);
}

void AInvadersGameMode::InitGameObjects() {
    UWorld* World = GetWorld();

    check(PlayerDef.ShipClass);

    check(PlayerBulletDef.BulletClass);
    check(EnemyBulletDef.BulletClass);
    check(AsteroidDef.AsteroidClass);

    check(EnemyDefs.Num() > 2);

    for (int Idx = 0; Idx < EnemyDefs.Num(); Idx++) {
        FEnemyDef& E = EnemyDefs[Idx];
        check(E.ShipClass);
        check(E.ShipRTClass);

        // Spawn render targets
        AActor* A = World->SpawnActor<AActor>(E.ShipRTClass);
        A->SetActorLocation(FVector(50000, Idx * 1000, -10000));
        A->SetActorRotation(FRotator(0, 0, 10));
        EnemyRTShips.Add(A);
    }

    check(PlayerDef.SpawnPoint);
    check(AsteroidDef.SpawnPoint);
    check(EnemySpawnPoint);
    check(UfoSpawnPoint);

    check(GameCamera);
    check(MainMenuCamera);

    // Player instancing
    FVector PlayerPos = PlayerDef.SpawnPoint->GetActorLocation();

    PlayerShip = World->SpawnActor<AActor>(PlayerDef.ShipClass);
    PlayerShip->Tags.Add("IsPlayer");

    // Enemy instancing
    EnemyShipGroup = World->SpawnActor(AActor::StaticClass());
    EnemyShipGroup->SetRootComponent(
        NewObject<USceneComponent>(EnemyShipGroup, TEXT("RootComponent")));
    int RowWidth = Rules.EnemySpread * Rules.EnemiesInRow;
    float Types = EnemyDefs.Num() - 1;
    for (float Idx = 0; Idx < InvadersGameState.TotalEnemyNum; Idx++) {
        float Col = 0;
        float Row = FMath::Modf(Idx / Rules.EnemiesInRow, &Col);
        FVector EnemyLocStart = FVector(RowWidth * Row - RowWidth * 0.5,
                                        -Rules.EnemySpread * Col, 0);

        FEnemyDef& EDef = EnemyDefs[int(Col) * (Types / Rules.EnemiesInColumn)];
        AActor* E = World->SpawnActor<AActor>(EDef.ShipClass);
        E->SetActorLocation(EnemyLocStart);
        E->AttachToActor(EnemyShipGroup,
                         FAttachmentTransformRules::KeepRelativeTransform);
        E->Tags.Add("IsEnemy");
        EnemyShips.Add(E);
    };

    // Last enemy definition reserved for ufo
    UfoShip = World->SpawnActor<AActor>(EnemyDefs.Top().ShipClass);
    UfoShip->Tags.Add("IsUfo");
    UfoShip->Tags.Add("IsEnemy");

    // Asteroid instancing
    FVector AsteroidPos = AsteroidDef.SpawnPoint->GetActorLocation();
    float Spread = Rules.SideMovementAmount * 4.f / 3.f;
    for (float Idx = 0; Idx < 4; Idx++) {
        AActor* E = World->SpawnActor<AActor>(AsteroidDef.AsteroidClass);
        FVector LocPos =
            FVector(Idx * Spread - Rules.SideMovementAmount * 2, 0, 0);
        E->SetActorLocation(AsteroidPos + LocPos);
        E->Tags.Add("IsAsteroid");
        E->Tags.Add("IsEnemy");
        Asteroids.Add(E);
    }

    // Bullet instace pool
    for (int Idx = 0; Idx < MAX_BULLETS; Idx++) {
        AActor* Bullet =
            GetWorld()->SpawnActor<AActor>(EnemyBulletDef.BulletClass);
        Bullet->SetActorEnableCollision(false);
        Bullet->SetActorHiddenInGame(true);
        EnemyBullets.Add(Bullet);
    }

    for (int Idx = 0; Idx < MAX_BULLETS; Idx++) {
        AActor* Bullet =
            GetWorld()->SpawnActor<AActor>(PlayerBulletDef.BulletClass);
        Bullet->SetActorEnableCollision(false);
        Bullet->SetActorHiddenInGame(true);
        PlayerBullets.Add(Bullet);
    }

    ResetUnits();
}

/// UI WIDGET FUNCTIONS ///

void AInvadersGameMode::ShowMainMenu() {
    InvadersGameState.LevelStarted = false;
    ResetUnits();

    APlayerController* Controller = GetWorld()->GetFirstPlayerController();

    SetActorTickEnabled(false);

    GameOverWidget->RemoveFromParent();
    PauseMenuWidget->RemoveFromParent();

    Controller->SetViewTarget(MainMenuCamera.Get());

    UButton* StartGameButton =
        Cast<UButton>(MainMenuWidget->GetWidgetFromName("StartGameBtn"));

    UTextBlock* HiScoreText =
        Cast<UTextBlock>(MainMenuWidget->GetWidgetFromName("HiScoreTxt"));
    InvadersGameState.PrevHiScore = InvadersGameState.HiScore;
    GameUtils::UpdateScoreTexts(InvadersGameState, HiScoreText);
    GameUtils::EnableUIMenu(Controller, MainMenuWidget, StartGameButton);
}

void AInvadersGameMode::ShowRestartMenu() {
    APlayerController* Controller = GetWorld()->GetFirstPlayerController();

    UButton* RestartGameButton =
        Cast<UButton>(GameOverWidget->GetWidgetFromName("ExitBtn"));

    UTextBlock* HiScoreText =
        Cast<UTextBlock>(GameOverWidget->GetWidgetFromName("HiScoreTxt"));

    UTextBlock* CurScoreText =
        Cast<UTextBlock>(GameOverWidget->GetWidgetFromName("ScoreTxt"));

    GameUtils::UpdateScoreTexts(InvadersGameState, HiScoreText, CurScoreText);
    GameUtils::EnableUIMenu(Controller, GameOverWidget, RestartGameButton);
}

void AInvadersGameMode::ShowPauseMenu() {
    APlayerController* Controller = GetWorld()->GetFirstPlayerController();
    InputComponent->ClearBindingValues();

    UButton* RestartGameButton =
        Cast<UButton>(PauseMenuWidget->GetWidgetFromName("RestartBtn"));

    UTextBlock* HiScoreText =
        Cast<UTextBlock>(PauseMenuWidget->GetWidgetFromName("HiScoreTxt"));

    UTextBlock* CurScoreText =
        Cast<UTextBlock>(PauseMenuWidget->GetWidgetFromName("ScoreTxt"));

    GameUtils::UpdateScoreTexts(InvadersGameState, HiScoreText, CurScoreText);
    GameUtils::EnableUIMenu(Controller, PauseMenuWidget, RestartGameButton);
}

void AInvadersGameMode::ShowTutorialMenu() {
    MainMenuWidget->RemoveFromParent();

    APlayerController* Controller = GetWorld()->GetFirstPlayerController();

    FInputModeGameOnly InputMode;
    Controller->SetInputMode(InputMode);
    Controller->SetShowMouseCursor(false);
    Controller->SetViewTarget(GameCamera.Get());

    TutorialMenuWidget->AddToViewport();

    InputComponent->ClearBindingValues();
    InputComponent->BindAction("Shoot", IE_Pressed, this,
                               &AInvadersGameMode::RestartInvadersGame);
    EnableInput(Controller);
}

/// GAME RESTART ///

void AInvadersGameMode::RestartInvadersGame() {
    GetWorldTimerManager().ClearTimer(EnemyAppearTHandle);
    GetWorldTimerManager().ClearTimer(PlayerAppearTHandle);
    GetWorldTimerManager().ClearTimer(EnemyShootTHandle);
    GetWorldTimerManager().ClearTimer(UfoAppearTHandle);
    GetWorldTimerManager().ClearTimer(PlayerShootTHandle);

    TutorialMenuWidget->RemoveFromParent();
    GameOverWidget->RemoveFromParent();
    PauseMenuWidget->RemoveFromParent();

    InvadersGameState.EnemyProgTime = 0;
    InvadersGameState.UfoProgTime = 0;
    InvadersGameState.EnemyAppearAnimTime = 5;
    InvadersGameState.PlayerAppearAnimTime = 1;

    InvadersGameState.CurrentLives = PlayerDef.Lives;
    InvadersGameState.LevelStarted = true;

    ResetUnits();

    PlayerShip->SetActorHiddenInGame(false);
    PlayerShip->SetActorEnableCollision(true);

    GetWorldTimerManager().SetTimer(
        PlayerAppearTHandle, this, &AInvadersGameMode::SpawnPlayer, 0.5, false);

    GetWorldTimerManager().SetTimer(
        EnemyAppearTHandle, this, &AInvadersGameMode::SpawnEnemies, 2.0, false);

    GetWorldTimerManager().SetTimer(
        UfoAppearTHandle, this, &AInvadersGameMode::UfoAppearCallback,
        FMath::RandRange(Rules.UfoAppearTimeMin, Rules.UfoAppearTimeMax),
        false);

    InputComponent->ClearActionBindings();
    InputComponent->BindAxis("MoveLeft");
    InputComponent->BindAxis("MoveRight");

    InputComponent->BindAction("Shoot", IE_Pressed, this,
                               &AInvadersGameMode::HandlePlayerShootPressed);
    InputComponent->BindAction("Shoot", IE_Released, this,
                               &AInvadersGameMode::HandlePlayerShootReleased);
    InputComponent->BindAction("Escape", IE_Pressed, this,
                               &AInvadersGameMode::HandleTogglePausePressed);

    APlayerController* Controller = GetWorld()->GetFirstPlayerController();
    FInputModeGameOnly InputMode;
    Controller->SetInputMode(InputMode);
    Controller->SetShowMouseCursor(false);
    Controller->SetViewTarget(GameCamera.Get());

    EnableInput(Controller);

    SetActorTickEnabled(true);
}

void AInvadersGameMode::SpawnEnemies() {
    UE_LOG(LogTemp, Warning, TEXT("Spawn enemies"));
    ShootingEnemies.Empty();
    InvadersGameState.EnemyProgTime = 0;
    InvadersGameState.EnemyAppearAnimTime = 5.f;
    InvadersGameState.ActiveEnemyNum = InvadersGameState.TotalEnemyNum;
    UpdateEnemyGroupMovement(0.0f);

    for (int Idx = 0; Idx < InvadersGameState.TotalEnemyNum; Idx++) {
        AActor* E = EnemyShips[Idx];
        E->SetActorHiddenInGame(false);
        E->SetActorEnableCollision(true);
        if (Idx < Rules.EnemiesInRow) {
            ShootingEnemies.Add(Idx);
        }
    };
    float NextEmitTime = Rules.EnemyShootFrequency;
    GetWorldTimerManager().SetTimer(EnemyShootTHandle, this,
                                    &AInvadersGameMode::EnemyShootTimerCallback,
                                    NextEmitTime, false);
}

void AInvadersGameMode::SpawnPlayer() {
    InvadersGameState.PlayerAppearAnimTime = 5.f;
    PlayerShip->SetActorHiddenInGame(false);
    PlayerShip->SetActorEnableCollision(true);
}

/// GAME UPDATE LOGIC ///

void AInvadersGameMode::Tick(float DeltaSeconds) {
    Super::Tick(DeltaSeconds);

    if (PauseMenuWidget->IsInViewport() || !InvadersGameState.LevelStarted) {
        return;
    }

    UpdateEnemyAppearAnimation(DeltaSeconds);
    UpdatePlayerAppearAnimation(DeltaSeconds);

    UpdatePlayerMovement(DeltaSeconds);
    UpdateEnemyGroupMovement(DeltaSeconds);
    UpdateUfoMovement(DeltaSeconds);

    UpdatePlayerBullets(DeltaSeconds);
    UpdateEnemyBullets(DeltaSeconds);
}

void AInvadersGameMode::UpdatePlayerMovement(float DeltaSeconds) {
    if (PlayerShip->IsHidden()) {
        return;
    }
    FVector ShipPos = PlayerShip->GetTargetLocation();

    float PlayerSideMovement = InputComponent->GetAxisValue("MoveRight") -
                               InputComponent->GetAxisValue("MoveLeft");

    // Position update
    PlayerMovement[0] =
        FMath::Lerp(PlayerMovement[0], PlayerSideMovement, DeltaSeconds * 5.f);
    ShipPos[0] += PlayerMovement[0] * PlayerDef.Speed * DeltaSeconds;
    ShipPos[0] = FMath::Clamp(ShipPos[0], -Rules.SideMovementAmount * 2,
                              Rules.SideMovementAmount * 2);
    PlayerShip->SetActorLocation(ShipPos);
    for (int Idx = 0; Idx < Asteroids.Num(); Idx++) {
        AActor* A = Asteroids[Idx];
        FRotator Rot = A->GetActorRotation();
        Rot.Add(0, 0, DeltaSeconds * 100);

        A->SetActorRotation(Rot);
    }

    // Camera update
    // FVector CamPos = FVector(0, 1000, 150);
    // FVector LookAt = ShipPos - CamPos;
    // LookAt.Normalize();

    // GameCamera->SetActorLocationAndRotation(CamPos, LookAt.Rotation());
}

void AInvadersGameMode::UpdateEnemyGroupMovement(float DeltaSeconds) {
    // Local side-to-side position is determined by the oscillation algorithm.
    // Top values are clamped between [-1, 1] so that we have delays before
    // resuming the side movement. Forward movement happens in-sync with the
    // side-to-side movement, not oscillated but linearly interpolated. In
    // addition we amplify and clamp the local forward position so that the
    // forward movement occurs only when side movement has paused.

    // TODO: Forward movement doesn't quite work yet with different
    //       OscXYRatio values
    //       Figure out why if theres time

    float SpeedN = FMath::Pow(
        1.0 -
            float(FMath::RoundToFloat(InvadersGameState.ActiveEnemyNum / 10.f) /
                  5.f),
        2);
    float SpeedFact = FMath::Lerp(Rules.MinSpeedFactor, 1.5, SpeedN);

    InvadersGameState.EnemyProgTime += DeltaSeconds * SpeedFact;
    float OscXYRatio = 2.f;
    float N = FMath::Fmod(InvadersGameState.EnemyProgTime, 4.0);

    // Oscillate between [-1, 1]
    float SideDirection = 1.0 - FMath::Abs(N - 2.0);
    // Offset x local position
    float RowPosition = (InvadersGameState.EnemyProgTime + 0.5) / 2.f;

    float RowInt = 0.f;
    float RowFract =
        FMath::Modf(FMath::Min(RowPosition, Rules.LastRow), &RowInt);

    RowInt = InvadersGameState.CurrentLevel + RowInt;

    FVector GroupPos = EnemySpawnPoint->GetActorLocation();

    GroupPos[0] += Rules.SideMovementAmount *
                   FMath::Clamp(SideDirection * OscXYRatio, -1.f, 1.f);
    GroupPos[1] += RowInt * Rules.ForwardMovementAmount +
                   Rules.ForwardMovementAmount *
                       FMath::Clamp(RowFract * OscXYRatio, 0.f, 1.f);
    EnemyShipGroup->SetActorLocation(GroupPos);
}

void AInvadersGameMode::UpdateUfoMovement(float DeltaSeconds) {
    if (!UfoShip->IsHidden()) {
        FVector from = UfoSpawnPoint->GetActorLocation();
        FVector to = from + FVector(-from[0] * 2, 0, 0);

        InvadersGameState.UfoProgTime += DeltaSeconds;
        float N = InvadersGameState.UfoProgTime / 5.f;
        UfoShip->SetActorLocation(FMath::Lerp(from, to, N));
        if (N >= 1.0) {
            InvadersGameState.UfoProgTime = 0.f;
            UfoShip->SetActorHiddenInGame(true);
            UfoShip->SetActorEnableCollision(false);

            GetWorldTimerManager().SetTimer(
                UfoAppearTHandle, this, &AInvadersGameMode::UfoAppearCallback,
                FMath::RandRange(Rules.UfoAppearTimeMin,
                                 Rules.UfoAppearTimeMax),
                false);
        }
    }
}

void AInvadersGameMode::UpdatePlayerBullets(float DeltaSeconds) {
    int BulletNum = InvadersGameState.ActivePlayerBullets;

    for (int BulletIdx = 0; BulletIdx < BulletNum; BulletIdx++) {
        int BulletRIdx = BulletNum - BulletIdx - 1;
        AActor* BulletActor = PlayerBullets[BulletRIdx];

        FVector Pos = BulletActor->GetActorLocation();
        Pos[1] -= PlayerBulletDef.Velocity * DeltaSeconds;

        BulletActor->SetActorLocation(Pos);

        float BulletDist = Pos[1];
        bool DeleteBullet = BulletDist < -500.0;
        bool PlaySound = false;

        AActor* UnitActor = nullptr;

        TArray<AActor*> OverlappedActors;
        BulletActor->GetOverlappingActors(OverlappedActors);

        if (OverlappedActors.Num() > 0 &&
            OverlappedActors[0]->ActorHasTag("IsEnemy")) {
            DeleteBullet = true;
            UnitActor = OverlappedActors[0];
        }

        if (UnitActor) {
            PlaySound = true;
            if (UnitActor->ActorHasTag("IsAsteroid")) {
                int HPIdx = Asteroids.Find(UnitActor);
                int HP = AsteroidHP[HPIdx];
                HP--;
                if (HP <= 0) {
                    UnitActor->SetActorHiddenInGame(true);
                    UnitActor->SetActorEnableCollision(false);
                }
                AsteroidHP[HPIdx] = HP;
                DeleteBullet = true;
                PlaySound = true;
            } else {
                UnitActor->SetActorHiddenInGame(true);
                UnitActor->SetActorEnableCollision(false);

                InvadersGameState.Score += EnemyPointMap[UnitActor->GetClass()];

                if (UnitActor->ActorHasTag("IsUfo")) {
                    // Reset ufo timer
                    InvadersGameState.UfoProgTime = 0.f;
                    GetWorldTimerManager().SetTimer(
                        UfoAppearTHandle, this,
                        &AInvadersGameMode::UfoAppearCallback,
                        FMath::RandRange(Rules.UfoAppearTimeMin,
                                         Rules.UfoAppearTimeMax),
                        false);
                } else {
                    // Update active enemy num and shooting enemies
                    InvadersGameState.ActiveEnemyNum--;

                    int EnemyIdx = EnemyShips.Find(UnitActor);
                    int ShootingIdx = ShootingEnemies.Find(EnemyIdx);

                    // Reorganize shootter enemies (first row)
                    if (ShootingIdx != INDEX_NONE) {
                        int NewIdx = ShootingEnemies[ShootingIdx];
                        ShootingEnemies.RemoveAt(ShootingIdx);

                        NewIdx += Rules.EnemiesInRow;
                        // Find next visible enemy row by row
                        while (NewIdx < InvadersGameState.TotalEnemyNum) {
                            if (!EnemyShips[NewIdx]->IsHidden()) {
                                ShootingEnemies.Add(NewIdx);
                                break;
                            }
                            NewIdx += Rules.EnemiesInRow;
                        }
                    }
                }
            }
        }
        if (PlaySound) {
            UGameplayStatics::PlaySoundAtLocation(
                GetWorld(),
                ExplosionSounds[FMath::RandRange(0, ExplosionSounds.Num() - 1)],
                BulletActor->GetActorLocation(), FMath::RandRange(0.2, 0.5));
        }
        if (DeleteBullet) {
            AActor* LastBullet =
                PlayerBullets[InvadersGameState.ActivePlayerBullets - 1];
            LastBullet->SetActorHiddenInGame(true);
            LastBullet->SetActorEnableCollision(false);
            BulletActor->SetActorLocation(LastBullet->GetActorLocation());

            InvadersGameState.ActivePlayerBullets--;
        }

        // All enemies killed
        if (!GetWorldTimerManager().IsTimerActive(EnemyAppearTHandle) &&
            InvadersGameState.ActiveEnemyNum == 0) {
            UE_LOG(LogTemp, Warning, TEXT("All enemies killed"));
            InvadersGameState.CurrentLevel++;
            InvadersGameState.EnemyProgTime = 0;

            GetWorldTimerManager().SetTimer(EnemyAppearTHandle, this,
                                            &AInvadersGameMode::SpawnEnemies,
                                            3.0, false);
        }
    }
}
void AInvadersGameMode::UpdateEnemyBullets(float DeltaSeconds) {
    int BulletNum = InvadersGameState.ActiveEnemyBullets;

    for (int BulletIdx = 0; BulletIdx < BulletNum; BulletIdx++) {
        int BulletRIdx = BulletNum - BulletIdx - 1;
        AActor* BulletActor = EnemyBullets[BulletRIdx];

        FVector Pos = BulletActor->GetActorLocation();
        Pos[1] += EnemyBulletDef.Velocity * DeltaSeconds;

        BulletActor->SetActorLocation(Pos);

        float BulletDist = Pos[1];
        bool DeleteBullet = BulletDist > 500.0;
        bool PlaySound = false;
        TArray<AActor*> OverlappedActors;
        BulletActor->GetOverlappingActors(OverlappedActors);

        if (OverlappedActors.Num() > 0) {
            AActor* Actor = OverlappedActors[0];

            // This could be handled by the collision channels but
            // I want to keep this as simple as possible.
            if (Actor->ActorHasTag("IsAsteroid")) {
                int HPIdx = Asteroids.Find(Actor);
                int HP = AsteroidHP[HPIdx];
                HP--;
                if (HP <= 0) {
                    Actor->SetActorHiddenInGame(true);
                    Actor->SetActorEnableCollision(false);
                }
                AsteroidHP[HPIdx] = HP;
                DeleteBullet = true;
                PlaySound = true;
            } else if (Actor->ActorHasTag("IsPlayer")) {
                HandlePlayerHit();
                DeleteBullet = true;
                PlaySound = true;
            } else {
                // Bullet to bullet contact
            }
        }

        if (PlaySound) {
            UGameplayStatics::PlaySoundAtLocation(
                GetWorld(),
                ExplosionSounds[FMath::RandRange(0, ExplosionSounds.Num() - 1)],
                BulletActor->GetActorLocation(), FMath::RandRange(0.2, 0.5));
        }

        if (DeleteBullet) {
            AActor* LastBullet =
                EnemyBullets[InvadersGameState.ActiveEnemyBullets - 1];
            LastBullet->SetActorHiddenInGame(true);
            LastBullet->SetActorEnableCollision(false);
            BulletActor->SetActorLocation(LastBullet->GetActorLocation());

            InvadersGameState.ActiveEnemyBullets--;
        }
    }
}

void AInvadersGameMode::UpdateEnemyAppearAnimation(float DeltaSeconds) {
    if (InvadersGameState.EnemyAppearAnimTime > 1) {
        for (AActor* E : EnemyShips) {
            if (!E->IsHidden()) {
                TArray<UActorComponent*> MeshComponents;
                E->GetComponents(UMeshComponent::StaticClass(), MeshComponents);
                const UMeshComponent* Mesh =
                    Cast<UMeshComponent>(MeshComponents[0]);
                UMaterialInstanceDynamic* Material =
                    Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0));
                if (Material) {
                    Material->SetScalarParameterValue(
                        "Gamma", InvadersGameState.EnemyAppearAnimTime);
                    float OpacityN =
                        (InvadersGameState.EnemyAppearAnimTime - 1.f) / 4;
                    Material->SetScalarParameterValue("Opacity",
                                                      1.0 - OpacityN);
                }
            }
        }
        InvadersGameState.EnemyAppearAnimTime =
            fmax(1, InvadersGameState.EnemyAppearAnimTime - DeltaSeconds * 8.f);
    }
}

void AInvadersGameMode::UpdatePlayerAppearAnimation(float DeltaSeconds) {
    if (InvadersGameState.PlayerAppearAnimTime > 1) {
        TArray<UActorComponent*> MeshComponents;
        PlayerShip->GetComponents(UMeshComponent::StaticClass(),
                                  MeshComponents);
        const UMeshComponent* Mesh = Cast<UMeshComponent>(MeshComponents[0]);
        UMaterialInstanceDynamic* Material =
            Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0));
        if (Material) {
            Material->SetScalarParameterValue(
                "Gamma", InvadersGameState.PlayerAppearAnimTime);
            float OpacityN = (InvadersGameState.PlayerAppearAnimTime - 1.f) / 4;
            Material->SetScalarParameterValue("Opacity", 1.0 - OpacityN);
        }
        InvadersGameState.PlayerAppearAnimTime = fmax(
            1, InvadersGameState.PlayerAppearAnimTime - DeltaSeconds * 8.f);
    }
}

/// HANDLE AND CALLBACK FUNCTIONS///

void AInvadersGameMode::HandlePlayerShootPressed() {
    if (!PlayerShip->IsHidden()) {
        this->IsPlayerShooting = true;
    }
    if (!GetWorldTimerManager().IsTimerActive(PlayerShootTHandle)) {
        GetWorldTimerManager().SetTimer(
            PlayerShootTHandle, this,
            &AInvadersGameMode::PlayerShootTimerCallback,
            PlayerDef.ShootFrequency, true);
        PlayerShootTimerCallback();
    }
}

void AInvadersGameMode::HandlePlayerShootReleased() {
    if (!PlayerShip->IsHidden()) {
        this->IsPlayerShooting = false;
    }
}

void AInvadersGameMode::HandlePlayerHit() {
    InvadersGameState.CurrentLives--;

    IsPlayerShooting = false;
    GetWorldTimerManager().ClearTimer(PlayerShootTHandle);
    PlayerShip->SetActorHiddenInGame(true);
    PlayerShip->SetActorEnableCollision(false);
    if (InvadersGameState.CurrentLives == 0) {
        SaveHiScore();
        DisableInput(GetWorld()->GetFirstPlayerController());
        ShowRestartMenu();

    } else {
        FVector PlayerPos = PlayerDef.SpawnPoint->GetActorLocation();
        PlayerShip->SetActorLocation(PlayerPos);

        GetWorldTimerManager().SetTimer(PlayerAppearTHandle, this,
                                        &AInvadersGameMode::SpawnPlayer, 1.0,
                                        false);
    }
}

void AInvadersGameMode::HandleTogglePausePressed() {
    if (!PauseMenuWidget->IsInViewport()) {
        PauseGame();
        ShowPauseMenu();
    } else {
        PauseMenuWidget->RemoveFromParent();
        UnpauseGame();
    }
}

void AInvadersGameMode::EnemyShootTimerCallback() {
    if (!PauseMenuWidget->IsInViewport()) {
        if (ShootingEnemies.Num() == 0) {
            return;
        }
        if (InvadersGameState.ActiveEnemyBullets < MAX_BULLETS) {
            AActor* EnemyActor = EnemyShips[ShootingEnemies[FMath::RandRange(
                0, ShootingEnemies.Num() - 1)]];
            FVector EnemyPos = EnemyActor->GetActorLocation();
            AActor* Bullet = EnemyBullets[InvadersGameState.ActiveEnemyBullets];
            Bullet->SetActorLocation(EnemyPos);
            Bullet->SetActorEnableCollision(true);
            Bullet->SetActorHiddenInGame(false);
            InvadersGameState.ActiveEnemyBullets++;
        }
        float NextEmitTime = Rules.EnemyShootFrequency;
        GetWorldTimerManager().SetTimer(
            EnemyShootTHandle, this,
            &AInvadersGameMode::EnemyShootTimerCallback, NextEmitTime, false);
    }
}

void AInvadersGameMode::PlayerShootTimerCallback() {
    if (!PauseMenuWidget->IsInViewport()) {
        if (!IsPlayerShooting) {
            GetWorldTimerManager().ClearTimer(PlayerShootTHandle);
        } else {
            if (InvadersGameState.ActivePlayerBullets < MAX_BULLETS) {
                AActor* Bullet =
                    PlayerBullets[InvadersGameState.ActivePlayerBullets];
                Bullet->SetActorLocation(PlayerShip->GetActorLocation());
                Bullet->SetActorEnableCollision(true);
                Bullet->SetActorHiddenInGame(false);
                InvadersGameState.ActivePlayerBullets++;
            }
        }
    }
}

void AInvadersGameMode::UfoAppearCallback() {
    UfoShip->SetActorHiddenInGame(false);
    UfoShip->SetActorEnableCollision(true);
    UpdateUfoMovement(0.f);
}

void AInvadersGameMode::StartLevelCallback() {
    TutorialMenuWidget->RemoveFromParent();

    RestartInvadersGame();
}

AActor* AInvadersGameMode::EmitBullet(TSubclassOf<AActor> BulletClass,
                                      FVector Pos) {
    AActor* Bullet = GetWorld()->SpawnActor<AActor>(BulletClass);
    Bullet->SetActorLocation(Pos);

    return Bullet;
}

void AInvadersGameMode::ResetUnits() {
    FVector PlayerPos = PlayerDef.SpawnPoint->GetActorLocation();

    TArray<UActorComponent*> MeshComponents;

    PlayerShip->SetActorLocation(PlayerPos);
    PlayerShip->SetActorHiddenInGame(true);
    PlayerShip->SetActorEnableCollision(false);

    PlayerShip->GetComponents(UMeshComponent::StaticClass(), MeshComponents);
    const UMeshComponent* PMesh = Cast<UMeshComponent>(MeshComponents[0]);
    UMaterialInstanceDynamic* PMaterial =
        Cast<UMaterialInstanceDynamic>(PMesh->GetMaterial(0));
    if (PMaterial) {
        PMaterial->SetScalarParameterValue("Gamma", 10);
        PMaterial->SetScalarParameterValue("Opacity", 0);
    }

    for (float Idx = 0; Idx < InvadersGameState.TotalEnemyNum; Idx++) {
        AActor* E = EnemyShips[Idx];
        E->SetActorHiddenInGame(true);
        E->SetActorEnableCollision(false);

        E->GetComponents(UMeshComponent::StaticClass(), MeshComponents);
        const UMeshComponent* EMesh = Cast<UMeshComponent>(MeshComponents[0]);
        UMaterialInstanceDynamic* EMaterial =
            Cast<UMaterialInstanceDynamic>(EMesh->GetMaterial(0));
        if (EMaterial) {
            EMaterial->SetScalarParameterValue("Gamma", 10);
            EMaterial->SetScalarParameterValue("Opacity", 0);
        }
    };

    UpdateEnemyGroupMovement(0.0);

    UfoShip->SetActorHiddenInGame(true);
    UfoShip->SetActorEnableCollision(false);
    UpdateUfoMovement(0.0);

    AsteroidHP.Empty();
    for (float Idx = 0; Idx < Asteroids.Num(); Idx++) {
        AActor* E = Asteroids[Idx];
        E->SetActorHiddenInGame(false);
        E->SetActorEnableCollision(true);

        AsteroidHP.Add(AsteroidDef.Health);
    }

    for (AActor* B : EnemyBullets) {
        B->SetActorEnableCollision(false);
        B->SetActorHiddenInGame(true);
    }

    for (AActor* B : PlayerBullets) {
        B->SetActorEnableCollision(false);
        B->SetActorHiddenInGame(true);
    }

    InvadersGameState.ActiveEnemyBullets = 0;
    InvadersGameState.ActivePlayerBullets = 0;
}

/// PAUSE FUNCTIONS ///

void AInvadersGameMode::PauseGame() {
    InputComponent->ClearBindingValues();

    InputComponent->RemoveAxisBinding("MoveLeft");
    InputComponent->RemoveAxisBinding("MoveRight");

    InputComponent->RemoveActionBinding("Shoot", IE_Pressed);
    InputComponent->RemoveActionBinding("Shoot", IE_Released);

    GetWorldTimerManager().PauseTimer(EnemyAppearTHandle);
    GetWorldTimerManager().PauseTimer(PlayerAppearTHandle);
    GetWorldTimerManager().PauseTimer(EnemyShootTHandle);
    GetWorldTimerManager().PauseTimer(PlayerShootTHandle);
    GetWorldTimerManager().PauseTimer(UfoAppearTHandle);
}

void AInvadersGameMode::UnpauseGame() {
    InputComponent->BindAxis("MoveLeft");
    InputComponent->BindAxis("MoveRight");

    InputComponent->BindAction("Shoot", IE_Pressed, this,
                               &AInvadersGameMode::HandlePlayerShootPressed);
    InputComponent->BindAction("Shoot", IE_Released, this,
                               &AInvadersGameMode::HandlePlayerShootReleased);

    APlayerController* Controller = GetWorld()->GetFirstPlayerController();
    FInputModeGameOnly InputMode;
    Controller->SetInputMode(InputMode);
    Controller->SetShowMouseCursor(false);
    Controller->SetViewTarget(GameCamera.Get());

    EnableInput(Controller);

    GetWorldTimerManager().UnPauseTimer(EnemyAppearTHandle);
    GetWorldTimerManager().UnPauseTimer(PlayerAppearTHandle);
    GetWorldTimerManager().UnPauseTimer(EnemyShootTHandle);
    GetWorldTimerManager().UnPauseTimer(PlayerShootTHandle);
    GetWorldTimerManager().UnPauseTimer(UfoAppearTHandle);
}

/// HISCORE SAVE/LOAD ///

void AInvadersGameMode::SaveHiScore() {
    if (InvadersGameState.Score > InvadersGameState.HiScore) {
        InvadersGameState.PrevHiScore = InvadersGameState.HiScore;
        InvadersGameState.HiScore = InvadersGameState.Score;
    }
    if (UInvadersSaveGame* SaveGameInstance =
            Cast<UInvadersSaveGame>(UGameplayStatics::CreateSaveGameObject(
                UInvadersSaveGame::StaticClass()))) {
        SaveGameInstance->HiScore = InvadersGameState.HiScore;
        UGameplayStatics::SaveGameToSlot(SaveGameInstance, "InvadersSaveSlot",
                                         0);
    }
}

void AInvadersGameMode::LoadHiScore() {
    if (UInvadersSaveGame* LoadedGame = Cast<UInvadersSaveGame>(
            UGameplayStatics::LoadGameFromSlot("InvadersSaveSlot", 0))) {
        InvadersGameState.HiScore = LoadedGame->HiScore;
        InvadersGameState.PrevHiScore = LoadedGame->HiScore;
    }
}
