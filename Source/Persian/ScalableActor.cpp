// Fill out your copyright notice in the Description page of Project Settings.


#include "ScalableActor.h"

// Sets default values
AScalableActor::AScalableActor() : bPossessed{ false } {
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	this->PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AScalableActor::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AScalableActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (this->bPossessed) {
	} else {
	}
}

