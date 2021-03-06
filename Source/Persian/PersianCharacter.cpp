// Copyright Epic Games, Inc. All Rights Reserved.

#include "PersianCharacter.h"
#include "PersianProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId

#include <limits>

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// FObjectState
FObjectState::FObjectState() {}
	FObjectState::FObjectState(double const& dist, FRotator const& cam_rotation,
		FRotator const &object_rotation, FVector const &offset, FVector const &scale,
		EComponentMobility::Type const &mobility)
		: Dist{ dist }, CamRotation{ cam_rotation }, ObjectRotation{ object_rotation },
		  Offset{ offset }, Scale{ scale }, Mobility{ mobility } {}

//////////////////////////////////////////////////////////////////////////
// APersianCharacter

APersianCharacter::APersianCharacter()
{
	this->AttachedObject = nullptr;
	this->State = FObjectState {
		std::numeric_limits<double>::lowest(),
		FRotator{0, 0, 0},
		FRotator{0, 0, 0},
		FVector{0, 0, 0},
		FVector{1},
		EComponentMobility::Movable,
	};

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(false);			// otherwise won't be visible in the multiplayer
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	// FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.

	// Create VR Controllers.
	R_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("R_MotionController"));
	R_MotionController->MotionSource = FXRMotionControllerBase::RightHandSourceId;
	R_MotionController->SetupAttachment(RootComponent);
	L_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("L_MotionController"));
	L_MotionController->SetupAttachment(RootComponent);

	// Create a gun and attach it to the right-hand VR controller.
	// Create a gun mesh component
	VR_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VR_Gun"));
	VR_Gun->SetOnlyOwnerSee(false);			// otherwise won't be visible in the multiplayer
	VR_Gun->bCastDynamicShadow = false;
	VR_Gun->CastShadow = false;
	VR_Gun->SetupAttachment(R_MotionController);
	VR_Gun->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	VR_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("VR_MuzzleLocation"));
	VR_MuzzleLocation->SetupAttachment(VR_Gun);
	VR_MuzzleLocation->SetRelativeLocation(FVector(0.000004, 53.999992, 10.000000));
	VR_MuzzleLocation->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));		// Counteract the rotation of the VR gun model.

	// Uncomment the following line to turn motion controllers on by default:
	//bUsingMotionControllers = true;
}

void APersianCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));

	// Show or hide the two versions of the gun based on whether or not we're using motion controllers.
	if (bUsingMotionControllers)
	{
		VR_Gun->SetHiddenInGame(false, true);
		Mesh1P->SetHiddenInGame(true, true);
	}
	else
	{
		VR_Gun->SetHiddenInGame(true, true);
		Mesh1P->SetHiddenInGame(false, true);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void APersianCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &APersianCharacter::OnFire);

	// Enable touchscreen input
	EnableTouchscreenMovement(PlayerInputComponent);

	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &APersianCharacter::OnResetVR);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &APersianCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &APersianCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &APersianCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &APersianCharacter::LookUpAtRate);
}

void APersianCharacter::OnFire()
{
	// Play an animation if possible.
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
	if (this->AttachedObject == nullptr) {
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green,
				TEXT("Attempting to attach object .."));
		}
		FHitResult res = this->VisionHit(1500);
		if (res.Actor.Get() != nullptr && this->Attach(res.Actor.Get(), res.Location)) {
			this->AttachedObject->SetActorEnableCollision(false);
			this->ScaleAttachedObject(30.0 / this->State.Dist);
		}
	} else {
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green,
				TEXT("Attempting to detach object .."));
		}
		this->MoveAttachedObject();
		this->AttachedObject->SetActorEnableCollision(true);
		this->Detach();
	}
}

void APersianCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void APersianCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == true)
	{
		return;
	}
	if ((FingerIndex == TouchItem.FingerIndex) && (TouchItem.bMoved == false))
	{
		OnFire();
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void APersianCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	TouchItem.bIsPressed = false;
}

//Commenting this section out to be consistent with FPS BP template.
//This allows the user to turn without using the right virtual joystick

//void APersianCharacter::TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location)
//{
//	if ((TouchItem.bIsPressed == true) && (TouchItem.FingerIndex == FingerIndex))
//	{
//		if (TouchItem.bIsPressed)
//		{
//			if (GetWorld() != nullptr)
//			{
//				UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport();
//				if (ViewportClient != nullptr)
//				{
//					FVector MoveDelta = Location - TouchItem.Location;
//					FVector2D ScreenSize;
//					ViewportClient->GetViewportSize(ScreenSize);
//					FVector2D ScaledDelta = FVector2D(MoveDelta.X, MoveDelta.Y) / ScreenSize;
//					if (FMath::Abs(ScaledDelta.X) >= 4.0 / ScreenSize.X)
//					{
//						TouchItem.bMoved = true;
//						float Value = ScaledDelta.X * BaseTurnRate;
//						AddControllerYawInput(Value);
//					}
//					if (FMath::Abs(ScaledDelta.Y) >= 4.0 / ScreenSize.Y)
//					{
//						TouchItem.bMoved = true;
//						float Value = ScaledDelta.Y * BaseTurnRate;
//						AddControllerPitchInput(Value);
//					}
//					TouchItem.Location = Location;
//				}
//				TouchItem.Location = Location;
//			}
//		}
//	}
//}

void APersianCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void APersianCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void APersianCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void APersianCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

bool APersianCharacter::EnableTouchscreenMovement(class UInputComponent* PlayerInputComponent)
{
	if (FPlatformMisc::SupportsTouchInput() || GetDefault<UInputSettings>()->bUseMouseForTouch)
	{
		PlayerInputComponent->BindTouch(EInputEvent::IE_Pressed, this, &APersianCharacter::BeginTouch);
		PlayerInputComponent->BindTouch(EInputEvent::IE_Released, this, &APersianCharacter::EndTouch);

		//Commenting this out to be more consistent with FPS BP template.
		//PlayerInputComponent->BindTouch(EInputEvent::IE_Repeat, this, &APersianCharacter::TouchUpdate);
		return true;
	}
	
	return false;
}

FHitResult APersianCharacter::VisionHit(double const &Far) const {
	FHitResult ret;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = true;
	FVector CamLocation = this->GetFirstPersonCameraComponent()->GetComponentLocation();
	FVector Forward = this->GetFirstPersonCameraComponent()->GetForwardVector();
	this->GetWorld()->LineTraceSingleByChannel(ret,
		CamLocation, CamLocation + Forward * Far,
		ECollisionChannel::ECC_Visibility,
		QueryParams
	);
	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Black,
			FString::Printf(TEXT("Hit distance is %f"), ret.Distance));
	}
	return ret;
}

void APersianCharacter::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	this->ScaleAttachedObject(30.0 / this->State.Dist);
}

void APersianCharacter::ScaleAttachedObject(double const &RelativeScale) {
	if (this->AttachedObject != nullptr) {
		/* Update object scale */
		this->AttachedObject->SetActorScale3D(this->State.Scale * RelativeScale);
		FVector CamLocation = this->GetFirstPersonCameraComponent()->GetComponentLocation();
		FVector CamForward = this->GetFirstPersonCameraComponent()->GetForwardVector();
		FVector RotatedOffset = FRotator(
			FQuat(this->GetFirstPersonCameraComponent()->GetComponentRotation())
			* FQuat(this->State.CamRotation.GetInverse())
		).RotateVector(this->State.Offset);
		FVector TargetLocation;
		TargetLocation = CamLocation
			+ (CamForward * this->State.Dist - RotatedOffset) * RelativeScale
			// + CamForward * RelativeScale * this->State.Dist
			// - this->State.Offset * RelativeScale
			;
		FRotator TargetRotation = FRotator(
			FQuat(this->GetFirstPersonCameraComponent()->GetComponentRotation())
			* FQuat(this->State.CamRotation.GetInverse())
			* FQuat(this->State.ObjectRotation)
		);

		/* Update object position and orientation */
		this->AttachedObject->TeleportTo(TargetLocation, TargetRotation);
	}
}

void APersianCharacter::MoveAttachedObject(double const &Far) {
	if (this->AttachedObject != nullptr) {
		FVector CamLocation = this->GetFirstPersonCameraComponent()->GetComponentLocation();
		FVector CamForward = this->GetFirstPersonCameraComponent()->GetForwardVector();
		FRotator CamRotation = this->GetFirstPersonCameraComponent()->GetComponentRotation();
		FHitResult hitres, optimhit;
		optimhit.bBlockingHit = false;
		optimhit.bStartPenetrating = false;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		QueryParams.AddIgnoredActor(this->AttachedObject);
		QueryParams.bTraceComplex = true;

		double minScale = std::numeric_limits<double>::max();

		for (FVector const& d : this->Directions) {
			FVector dir = CamRotation.RotateVector(d).GetSafeNormal();
			this->GetWorld()->LineTraceSingleByChannel(
				hitres, CamLocation, CamLocation + dir * Far,
				ECollisionChannel::ECC_Visibility,
				QueryParams
			);
			// DrawDebugLine(this->GetWorld(), CamLocation, hitres.Location, FColor::Yellow, false, 5);
			if (hitres.bBlockingHit && !hitres.bStartPenetrating) {
				optimhit = hitres;
				minScale = FMath::Min<double>(minScale, (hitres.Distance - 1) / d.Size());
			}
			minScale = FMath::Min<double>(minScale, Far / d.Size());
		}

		this->ScaleAttachedObject(minScale);
	}
}

bool APersianCharacter::Attach(AActor* Object, FVector const &HitLocation) {
	if (Object == nullptr || Object->GetRootComponent()->Mobility == EComponentMobility::Static) {
		return false;
	}
	this->AttachedObject = Object;
	/* Disable physics simulation */
	this->AttachedObject->DisableComponentsSimulatePhysics();
	FVector centroid, _;
	this->AttachedObject->GetActorBounds(true, centroid, _);
	FVector CamLocation = this->GetFirstPersonCameraComponent()->GetComponentLocation();
	FRotator InvCamRotation = this->GetFirstPersonCameraComponent()->GetComponentRotation().GetInverse();
	this->State = FObjectState{
		(HitLocation - CamLocation).Size(),
		// this->GetActorRotation().GetInverse() + this->AttachedObject->GetActorRotation(),
		this->GetFirstPersonCameraComponent()->GetComponentRotation(),
		this->AttachedObject->GetActorRotation(),
		HitLocation - centroid,
		this->AttachedObject->GetActorScale3D(),
		this->AttachedObject->GetRootComponent()->Mobility,
	};
	/* Enable movement */
	this->AttachedObject->GetRootComponent()->SetMobility(EComponentMobility::Movable);
	TArray<UStaticMeshComponent *> meshes;
	this->AttachedObject->GetComponents<UStaticMeshComponent>(meshes, true);
	for (auto meshcomp : meshes) {
		auto mesh = meshcomp->GetStaticMesh();
		if (mesh->GetNumVertices(0) > 0) {
			FPositionVertexBuffer const* verts =
				&mesh->RenderData->LODResources[0].VertexBuffers.PositionVertexBuffer;
			for (uint32_t i = 0; i < verts->GetNumVertices(); ++i) {
				FVector const vert = this->AttachedObject->GetActorLocation()
					+ this->AttachedObject->GetTransform().TransformVector(verts->VertexPosition(i));
				this->Directions.Push(
					InvCamRotation.RotateVector((vert - CamLocation))
				);
			}
		}
	}
	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Yellow,
			FString::Printf(TEXT("%d directions"), this->Directions.Num()));
	}
	/* Do not attach an object with no body */
	if (this->Directions.Num() == 0) {
		this->Detach();
		return false;
	}
	return true;
}
void APersianCharacter::Detach() {
	if (this->AttachedObject == nullptr) {
		return;
	}
	/* Re-enable physics simulation */
	Cast<UPrimitiveComponent>(this->AttachedObject->GetRootComponent())->SetSimulatePhysics(true);
	/* Disable movement */
	this->AttachedObject->GetRootComponent()->SetMobility(this->State.Mobility);
	this->AttachedObject = nullptr;
	this->State = FObjectState {
		std::numeric_limits<double>::lowest(),
		FRotator{0, 0, 0},
		FRotator{0, 0, 0},
		FVector{0, 0, 0},
		FVector{1},
		EComponentMobility::Movable,
	};
	this->Directions.Empty();
}
AActor* const APersianCharacter::Attaching() const {
	return this->AttachedObject;
}
