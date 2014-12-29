// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Untitled.h"
#include "UntitledCharacter.h"

//////////////////////////////////////////////////////////////////////////
// AUntitledCharacter

AUntitledCharacter::AUntitledCharacter(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 450.f;
	GetCharacterMovement()->AirControl = 200.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = PCIP.CreateDefaultSubobject<USpringArmComponent>(this, TEXT("CameraBoom"));
	CameraBoom->AttachTo(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = PCIP.CreateDefaultSubobject<UCameraComponent>(this, TEXT("FollowCamera"));
	FollowCamera->AttachTo(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	TotalHealth = 3;
	Health = TotalHealth;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

void AUntitledCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (IsSliding)
	{
		const FRotator Rotation = GetActorRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		if (SlideSpeed < 0)
		{	
			IsSliding = false;
			SlideSpeed = 0;
		}
		CharacterMovement->MaxWalkSpeed = SlideSpeed;
		const FVector Direction = YawRotation.Vector();
		
		SlideSpeed -= SlideFriction;
		SlideFriction += .05;
		AddMovementInput(Direction, 1.0f);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AUntitledCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// Set up gameplay key bindings
	check(InputComponent);
	InputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	InputComponent->BindAction("Slide", IE_Pressed, this, &AUntitledCharacter::Slide);
	InputComponent->BindAction("Slide", IE_Released, this, &AUntitledCharacter::StopSliding);

	InputComponent->BindAxis("MoveForward", this, &AUntitledCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AUntitledCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	InputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &AUntitledCharacter::TurnAtRate);
	InputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	InputComponent->BindAxis("LookUpRate", this, &AUntitledCharacter::LookUpAtRate);

	// handle touch devices
	InputComponent->BindTouch(IE_Pressed, this, &AUntitledCharacter::TouchStarted);
	InputComponent->BindTouch(IE_Released, this, &AUntitledCharacter::TouchStopped);

}


void AUntitledCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	// jump, but only on the first touch
	if (FingerIndex == ETouchIndex::Touch1)
	{
		Jump();
	}
}

void AUntitledCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	if (FingerIndex == ETouchIndex::Touch1)
	{
		StopJumping();
	}
}

void AUntitledCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AUntitledCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AUntitledCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AUntitledCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void AUntitledCharacter::AddHealth(int32 HealthToAdd)
{
	if (CanAddHealth())
	{
		Health += HealthToAdd;
		if (Health > TotalHealth)
		{
			Health = TotalHealth;
		}
	}
}

bool AUntitledCharacter::CanAddHealth()
{
	if (Health >= TotalHealth)
	{
		return false;
	}
	return true;
}

void AUntitledCharacter::Slide()
{	
	float CurrentSpeed = CharacterMovement->Velocity.Size2D();
	if (CurrentSpeed > 10)
	{
		SlideFriction = 5;
		SlideSpeed = CurrentSpeed + 600;
	}
	IsSliding = true;

}

void AUntitledCharacter::StopSliding()
{
	CharacterMovement->MaxWalkSpeed = 600;
	IsSliding = false;

}