// Copyright Epic Games, Inc. All Rights Reserved.

#include "PersianGameMode.h"
#include "PersianHUD.h"
#include "PersianCharacter.h"
#include "UObject/ConstructorHelpers.h"

APersianGameMode::APersianGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = APersianHUD::StaticClass();
}
