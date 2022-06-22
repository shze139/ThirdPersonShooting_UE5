// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera\CameraComponent.h"
#include "GameFramework\CharacterMovementComponent.h"
#include "Kismet\GameplayStatics.h"
#include "Sound\SoundCue.h"
#include "Engine\SkeletalMeshSocket.h"
#include "Animation\AnimMontage.h"
#include "DrawDebugHelpers.h"
#include "Particles\ParticleSystemComponent.h"
#include "Item.h"
#include "Components\WidgetComponent.h"

// Sets default values
AShooterCharacter::AShooterCharacter() :
	// Base rates for turning/looking up
	BaseTurnRate(45.f),
	BaseLookUpRate(45.f),
	// Turn rates for aiming / not aiming
	HipTurnRate(90.f),
	HipLookUpRate(90.f),
	AimingTurnRate(15.f),
	AimingLookUpRate(15.f),
	// Mouse look sensitivity scale factor
	MouseHipTurnRate(1.f),
	MouseHipLookUpTurnRate(1.f),
	MouseAimingTurnRate(0.3f),
	MouseAimgLookUpRate(0.3f),
	// True when aiming the weapon
	bAiming(false),
	// Camera field of view values
	CameraDefaultFOV(0.f), // will set its value in BeginPlay
	CameraZoomedFOV(50.0),
	CameraCurrentFOV(0.f),
	ZoomInterpSpeed(10.f),
	// Crosshair spread factors
	CrosshairSpreadMutiplier(0.f),
	CrosshairVelocityFactor(0.f),
	CrosshairInAirFactor(0.f),
	CrosshairAimFactor(0.f),
	CrosshairShootingFactor(0.f),
	// Bullet fire timer varibles
	ShootingTimeDuration(0.05f),
	bFiringBullet(false),
	//
	AutomaicFireRate(0.1f),
	bFireButtonPressed(false),
	bShouldFire(true),
	// Item trace variable
	bShouldTraceForItems(false)

	// Constructor Body Begin
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create a camera boom (pulls in towards the character if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 250.f; // The camera follows at the distance behind the character
	CameraBoom->bUsePawnControlRotation = true; // Use contronller rotation
	CameraBoom->SocketOffset = FVector(0.f, 85.f, 70.f);

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of camera boom;
	FollowCamera->bUsePawnControlRotation = false; // Do not use the controller rotation;

	// Don't rotate when the controller rotates. Let the controller only affect the camera
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // Character moves in the direction of input...
	GetCharacterMovement()->RotationRate = FRotator{ 0.f, 540.f, 0.f };// ... at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;
}

// Called when the game starts or when spawned
void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();


	//Print the name of this instance;
	UE_LOG(LogTemp, Warning, TEXT("Name of instance: %s"), *GetName());

	if (FollowCamera)
	{
		CameraDefaultFOV = GetFollowCamera()->FieldOfView;
		CameraCurrentFOV = CameraDefaultFOV;
	}
}

void AShooterCharacter::MoveForward(float Value)
{
	if (Controller && Value != 0.0f)
	{
		// find out which way os forward
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation{ 0, Rotation.Yaw, 0 };

		FVector Direction{ FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X) };
		AddMovementInput(Direction, Value);
	}
}

void AShooterCharacter::MoveRightward(float Value)
{
	if (Controller && Value != 0.0f)
	{
		const FRotator Rotation{ GetControlRotation() };
		const FRotator YawRotation{ 0, Rotation.Yaw, 0 };

		FVector Direction{ FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y) };
		AddMovementInput(Direction, Value);
	}
}

void AShooterCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame frome the rate information.
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());// deg/sec * sec/frame
}

void AShooterCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate infomation.
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds()); // deg/sec * sec/frame
}

void AShooterCharacter::Turn(float Value)
{
	float TurnScaleFactor{ 0.f };

	if (bAiming)
		TurnScaleFactor = MouseAimingTurnRate;
	else
		TurnScaleFactor = MouseHipTurnRate;

	AddControllerYawInput(Value * TurnScaleFactor);
}

void AShooterCharacter::LookUp(float Value)
{
	float LookUpScaleFactor{ 0.f };

	if (bAiming)
		LookUpScaleFactor = MouseAimgLookUpRate;
	else
		LookUpScaleFactor = MouseHipLookUpTurnRate;

	AddControllerPitchInput(Value * LookUpScaleFactor);
}

void AShooterCharacter::FireWeapon()
{
	//UE_LOG(LogTemp, Warning, TEXT("FireWeapon()"));
	if (FireSound) UGameplayStatics::PlaySound2D(this, FireSound); // play fire sound
	const USkeletalMeshSocket* BarrelSocket = GetMesh()->GetSocketByName("BarrelSocket");
	if (BarrelSocket)
	{
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(GetMesh());
		if (MuzzleFlash)
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, SocketTransform);
		

		FVector BeamEnd;
		bool bBeamEnd = GetBeamEndLocation( SocketTransform.GetLocation(), BeamEnd);

		if (bBeamEnd)
		{
			// Spawn impact particle
			if (ImpactParticles)
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, BeamEnd);
			

			// Spawn the beam particle 
			if (BeamParticles)
			{
				UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(), 
					BeamParticles, 
					SocketTransform.GetLocation()); // Spawn beam particle at location of barrel socket to...
				if (Beam) Beam->SetVectorParameter(FName("Target"), BeamEnd); // ...this point
			}
		}

	}
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HipFireMontage)
	{
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}
	// Start bullet fire timer for bullet 
	StartCrosshairBulletFire();
}

bool AShooterCharacter::GetBeamEndLocation(
	const FVector& MuzzleSocketLocation, 
	FVector& OutBeamLocation)
{
	// Check for crosshair trace hit
	FHitResult CrosshairHitResult;
	bool bCrosshairHit = TraceUnderCrosshairs(CrosshairHitResult, OutBeamLocation);

	if (bCrosshairHit)
	{
		// Tentative beam location - still need to trace from gun
		OutBeamLocation = CrosshairHitResult.Location;
	}
	else // No crosshair trace hit
	{
		// OutBeamLocation is the end location for the line trace
	}

	// Perfrom the second line trace from gun barrel
	FHitResult WeaponTraceHit;
	const FVector WeaponTraceStart{ MuzzleSocketLocation };
	const FVector StartToEnd{ OutBeamLocation - MuzzleSocketLocation };
	const FVector WeaponTraceEnd{ MuzzleSocketLocation + StartToEnd * 1.25f };
	GetWorld()->LineTraceSingleByChannel(
		WeaponTraceHit,
		WeaponTraceStart,
		WeaponTraceEnd,
		ECollisionChannel::ECC_Visibility);

	if (WeaponTraceHit.bBlockingHit) // object between barrel and BeamEndPoint;
	{
		OutBeamLocation = WeaponTraceHit.Location;
		return true;
	}

	return false;
}

void AShooterCharacter::AimingButtonPressed()
{
	bAiming = true;
}

void AShooterCharacter::AimingButtonReleased()
{
	bAiming = false;
}

void AShooterCharacter::InterpolateCameraFOV(float DeltaTime)
{
	if (FollowCamera)
	{
		if (bAiming)
			CameraCurrentFOV = FMath::FInterpTo(
				CameraCurrentFOV, 
				CameraZoomedFOV, 
				DeltaTime, 
				ZoomInterpSpeed);
		else
			CameraCurrentFOV = FMath::FInterpTo(
				CameraCurrentFOV, 
				CameraDefaultFOV, 
				DeltaTime, 
				ZoomInterpSpeed);
	
		GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
	}
}

void AShooterCharacter::SetLookRates()
{
	if (bAiming)
	{
		BaseTurnRate = AimingTurnRate;
		BaseLookUpRate = AimingLookUpRate;
	} else
	{
		BaseTurnRate = HipTurnRate;
		BaseLookUpRate = HipLookUpRate;
	}
}

void AShooterCharacter::CalculateCrosshairSpread(float DeltaTime)
{
	FVector2D WalkSpeedRange{ 0.f, 600.f };
	FVector2D VelocityMutiplierRange{ 0.f, 1.f };
	FVector Velocity{GetVelocity()};
	Velocity.Z = 0.f;

	//Calculate the crosshair velocity factor
	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(
		WalkSpeedRange, 
		VelocityMutiplierRange, 
		Velocity.Size());

	// Calculate the crosshair in air factor
	if (GetCharacterMovement()->IsFalling()) // is in air
		// Spread the crosshair slowly while in air
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
	else // is on the ground
		// Spread the crosshair faster while on the ground
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 5.f);

	// Calculate the crosshair aim factor
	if (bAiming)
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.5, DeltaTime, 5.f);
	else
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0, DeltaTime, 5.f);

	// True 0.05 secs after firing
	if (bFiringBullet)
	{
		CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.5f, DeltaTime, 60.f);
	} 
	else
	{
		CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 60.f);
	}

	CrosshairSpreadMutiplier =
		CrosshairVelocityFactor +
		CrosshairInAirFactor -
		CrosshairAimFactor +
		CrosshairShootingFactor +
		0.5f;
}

void AShooterCharacter::StartCrosshairBulletFire()
{
	bFiringBullet = true;

	GetWorldTimerManager().SetTimer(
		CrosshairShootTimer,
		this,
		&AShooterCharacter::FinishCrosshairBulletFire,
		ShootingTimeDuration);
}

void AShooterCharacter::FinishCrosshairBulletFire()
{
	bFiringBullet = false;
}

void AShooterCharacter::FireButtonPressed()
{
	bFireButtonPressed = true;
	StartFireTimer();
}

void AShooterCharacter::FireButtonReleased()
{
	bFireButtonPressed = false;
}

void AShooterCharacter::StartFireTimer()
{
	if (bShouldFire)
	{
		FireWeapon();
		bShouldFire = false;
		GetWorldTimerManager().SetTimer(
			AutomaticFireTimer,
			this,
			&AShooterCharacter::AutoFireReset,
			AutomaicFireRate);
	}
}

void AShooterCharacter::AutoFireReset()
{
	bShouldFire = true;
	if (bFireButtonPressed)
	{
		StartFireTimer();
	}
}

bool AShooterCharacter::TraceUnderCrosshairs(FHitResult& OutHitResult, FVector& OutHitLocation)
{
	// Get viewport size
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation{ ViewportSize.X / 2.f, ViewportSize.Y / 2.f };
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0), 
		CrosshairLocation,
		CrosshairWorldPosition, 
		CrosshairWorldDirection);

	if (bScreenToWorld)
	{
		// Trace from crosshair world position outward
		const FVector Start{ CrosshairWorldPosition };
		const FVector End{ Start + CrosshairWorldDirection * 50'000.f };
		OutHitLocation = End;
		GetWorld()->LineTraceSingleByChannel(
			OutHitResult, 
			Start, 
			End, 
			ECollisionChannel::ECC_Visibility);

		if (OutHitResult.bBlockingHit)
		{
			OutHitLocation = OutHitResult.Location;
			return true;
		}
	}

	return false;
}

void AShooterCharacter::TraceForItem()
{
	if (bShouldTraceForItems)
	{
		FHitResult ItemTraceResult;
		FVector HitLocation;
		TraceUnderCrosshairs(ItemTraceResult, HitLocation);
		if (ItemTraceResult.bBlockingHit)
		{
			AItem* HitItem = Cast<AItem>(ItemTraceResult.GetActor());
			if (HitItem && HitItem->GetPickupWidget())
			{
				// Show item's pickup widget 
				HitItem->GetPickupWidget()->SetVisibility(true);
			}
			// We hit an Aitem last frame
			if (TraceHitItemLastFrame)
			{
				// We are hitting a differnt Aitem this frame from last frame...
				// ...or Aitem is null
				if (HitItem != TraceHitItemLastFrame) 
					TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
			}
			// Store a reference to Hititem for next frame
			TraceHitItemLastFrame = HitItem;
		}
	}
	else if(TraceHitItemLastFrame)
	{
		// No longer overlapping any item, AItem last frame should not show widget
		TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
	}
}

// Called every frame
void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//Handle interplation for zoom when aiming
	InterpolateCameraFOV(DeltaTime);
	// Set different turn rate/look up rate base on bAiming for contronller users
	SetLookRates();
	// Calculate crosshair spread multiplier
	CalculateCrosshairSpread(DeltaTime);
	// Check OverlappedItemCount, then trace for items
	TraceForItem();
}

// Called to bind functionality to input
void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	// Bind axis input.
	PlayerInputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRightward);
	PlayerInputComponent->BindAxis("TurnRate", this, &AShooterCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("lookUpRate", this, &AShooterCharacter::LookUpAtRate);
	PlayerInputComponent->BindAxis("Turn", this, &AShooterCharacter::Turn);
	PlayerInputComponent->BindAxis("lookUp", this, &AShooterCharacter::LookUp);

	// Bind action input.
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &AShooterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("FireButton", IE_Released, this, &AShooterCharacter::FireButtonReleased);


	PlayerInputComponent->BindAction("Aiming", IE_Pressed, this, &AShooterCharacter::AimingButtonPressed);
	PlayerInputComponent->BindAction("Aiming", IE_Released, this, &AShooterCharacter::AimingButtonReleased);
}

float AShooterCharacter::GetCrosshairSpreadMultiplier() const
{
	return CrosshairSpreadMutiplier;
}

void AShooterCharacter::IncrementOverlappedItemCount(int8 Amount)
{
	if (OverlappedItemCount + Amount <= 0)
	{
		OverlappedItemCount = 0;
		bShouldTraceForItems = false;
	}
	else
	{
		OverlappedItemCount += Amount;
		bShouldTraceForItems = true;
	}
}

