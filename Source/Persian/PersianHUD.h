// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PersianHUD.generated.h"

UCLASS()
class APersianHUD : public AHUD
{
	GENERATED_BODY()

public:
	APersianHUD();

	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

	UFUNCTION(BlueprintCallable, Category = Persian)
		void MissionComplete();

private:
	/** Crosshair asset pointer */
	class UTexture2D* CrosshairTex;

	/** Thumbs up asset pointer */
	class UTexture2D* ThumbsUpTex;

};

