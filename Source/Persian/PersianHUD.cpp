// Copyright Epic Games, Inc. All Rights Reserved.

#include "PersianHUD.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "CanvasItem.h"
#include "UObject/ConstructorHelpers.h"

APersianHUD::APersianHUD()
{
	// Set the crosshair texture
	static ConstructorHelpers::FObjectFinder<UTexture2D>
		CrosshairTexObj(TEXT("/Game/FirstPerson/Textures/FirstPersonCrosshair"));
	// Set the thumbs up texture
	static ConstructorHelpers::FObjectFinder<UTexture2D>
		ThumbsUpObj(TEXT("/Game/FirstPerson/Textures/thumbsup"));
	this->CrosshairTex = CrosshairTexObj.Object;
	this->ThumbsUpTex = ThumbsUpObj.Object;
}


void APersianHUD::DrawHUD()
{
	Super::DrawHUD();

	// Draw very simple crosshair

	// find center of the Canvas
	const FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);

	// offset by half the texture's dimensions so that the center of the texture aligns with the center of the Canvas
	const FVector2D CrosshairDrawPosition(Center.X - this->CrosshairTex->GetSizeX() * 0.5,
										  Center.Y - this->CrosshairTex->GetSizeY() * 0.5);

	// draw the crosshair
	FCanvasTileItem TileItem(CrosshairDrawPosition, CrosshairTex->Resource, FLinearColor::White);
	TileItem.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(TileItem);
}

void APersianHUD::MissionComplete() {
	this->CrosshairTex = this->ThumbsUpTex;
}
