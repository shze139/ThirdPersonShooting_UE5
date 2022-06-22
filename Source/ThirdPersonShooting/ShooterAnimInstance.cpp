// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterAnimInstance.h"
#include "ShooterCharacter.h"
#include "GameFramework\CharacterMovementComponent.h"
#include "Kismet\KismetMathLibrary.h"

void UShooterAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
    if(!ShooterCharacter) Cast<AShooterCharacter>(TryGetPawnOwner());
    if (ShooterCharacter)
    {
        // Get the lateral speed of the character from velocity
        FVector Velocity = ShooterCharacter->GetVelocity();
        Velocity.Z = 0.f;
        Speed = Velocity.Size();

        // Is the character in the air
        bIsInAir = ShooterCharacter->GetCharacterMovement()->IsFalling();

        // Is the character accelerating?
        if (ShooterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f) // find the magnitude of the acceleration
            bIsAccelerating = true;
        else 
            bIsAccelerating = false;

        FRotator AimRotation = ShooterCharacter->GetBaseAimRotation();
        FRotator MovementRotation = 
            UKismetMathLibrary::MakeRotFromX(ShooterCharacter->GetVelocity());

        MovementOffsetYaw = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;

        if ( ShooterCharacter->GetVelocity().Size()) LastMovementOffsetYaw = MovementOffsetYaw;

        // Debug message for moving
        /*FString RotationMessage = FString::Printf(
            TEXT("Base Aim Rotation: %f"),
            AimRotation.Yaw);
        FString MovementRotationMessage = FString::Printf(
            TEXT("Movement Rotation: %f"),
            MovementRotation.Yaw);*/
        FString MovementOffsetYawMessage = FString::Printf(
            TEXT("Movement Offset Yaw: %f"),
            MovementOffsetYaw);

        if (GEngine)
        {
            //GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::White, RotationMessage);
            //GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::White, MovementRotationMessage);
            GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::White, MovementOffsetYawMessage);
        }

        bAiming = ShooterCharacter->GetAiming();
    }
   
}

void UShooterAnimInstance::NativeInitializeAnimation()
{
    ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
}
