#include "DataTypes.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

namespace GameUtils {

inline void UpdateScoreTexts(FInvadersGameState& State,
                             UTextBlock* HiScoreText,
                             UTextBlock* CurScoreText = nullptr) {
    FString HiScoreStr = "hiscore: " + FString::FromInt(State.PrevHiScore);
    HiScoreText->SetText(FText::FromString(HiScoreStr));

    if (CurScoreText) {
        FString CurScoreStr = "score: " + FString::FromInt(State.Score);
        if (State.Score > State.PrevHiScore) {
            CurScoreStr.Append(" // new hiscore //");
        }
        CurScoreText->SetText(FText::FromString(CurScoreStr));
    }
}

inline void EnableUIMenu(APlayerController* Controller,
                         UUserWidget* MenuWidget,
                         UButton* FocusedButton = nullptr) {
    FInputModeGameAndUI InputMode;
    if (FocusedButton) {
        InputMode.SetWidgetToFocus(FocusedButton->GetCachedWidget());
    }
    Controller->SetInputMode(InputMode);
    Controller->SetShowMouseCursor(true);

    MenuWidget->SetIsEnabled(true);
    MenuWidget->AddToViewport();
}
}  // namespace GameUtils
