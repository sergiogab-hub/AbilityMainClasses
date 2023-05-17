// Copyright Epic Games, Inc. All Rights Reserved.


#include "SergioTestContentClasses/RPGTranscendenceHammer.h"
#include "RPGCharacterBase.h"
#include "Kismet/KismetMathLibrary.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/RPGGameplayAbility.h"

// Sets default values
ARPGTranscendenceHammer::ARPGTranscendenceHammer()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	RotationAngleAxis = 0.f;
	RotationDirection = -1.f;
	RotateAxisVector = FVector(0.f, 0.f, 1.0f);
	MinRotationSpeedValue = 180.f;
	MaxRotationSpeedValue = 350.f;
	MaxRotationRadiusValue = 200.f;
	MinRotationRadiusValue = 70.f;
	RotationSpeed = MinRotationSpeedValue;
	RotationRadius = MinRotationRadiusValue;
	LerpMoveHammertoEnemyValue = 0.F;
	MoveHammerToEnemySmoothValueRange = 2.0f;

	CurrentHamexIndex = 0;

	bIsInSpinningMode = false;
	bWasHammerUsed = false;
	bIsHammerPreparingToUse = false;
	bHasToHammerControl = false;


	PlayerCharacterRef = nullptr;
	EnemyNPCRef = nullptr;

	PreviewForwardVectorToCompare = FVector::ZeroVector;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ARPGTranscendenceHammer::BeginPlay()
{
	Super::BeginPlay();

	TrySetupReferences();

	ActivatedHammer();	
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ARPGTranscendenceHammer::TrySetupReferences()
{
	PlayerCharacterRef = Cast<ARPGCharacterBase>(GetOwner());
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ARPGTranscendenceHammer::ActivatedHammer()
{
    bIsHammerActive = true;
	GetWorldTimerManager().SetTimer(OrbitAroundHandle, this, &ARPGTranscendenceHammer::HammersOrbitMovement, GetWorld()->GetDeltaSeconds(), true);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ARPGTranscendenceHammer::DeactivatedHammer(AActor* DeactivatedByRef)
{
    SetActorHiddenInGame(true);
	
	SetActorEnableCollision(false);

	const bool bValidOrbitHandleTurnOff = OrbitAroundHandle.IsValid() && GetWorldTimerManager().IsTimerActive(OrbitAroundHandle);
	if (bValidOrbitHandleTurnOff)
	{
		GetWorldTimerManager().ClearTimer(OrbitAroundHandle);
	}

	const bool bValidMoveHandleTurnOff = MoveHammerToEnemyHandle.IsValid() && GetWorldTimerManager().IsTimerActive(MoveHammerToEnemyHandle);
	if (bValidMoveHandleTurnOff)
	{
		GetWorldTimerManager().ClearTimer(MoveHammerToEnemyHandle);
	}

	const bool bValidSpinningHandleTurnOff = SpinningModeHandle.IsValid() && GetWorldTimerManager().IsTimerActive(SpinningModeHandle);
	if (bValidMoveHandleTurnOff)
	{
		GetWorldTimerManager().ClearTimer(SpinningModeHandle);
	}

	bIsHammerActive = false;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ARPGTranscendenceHammer::HammersOrbitMovement()
{
 	
	if (!IsValid(PlayerCharacterRef))
	{
		TrySetupReferences();
		return;
	}
	
	if (GetRotationValidVariance() || bIsInSpinningMode)
	{
		ContractedRotation();
	}
	else
	{
		ExpandedRotation();
	}

	/*Calculate the new AngleAxis*/
	PreviewForwardVectorToCompare = PlayerCharacterRef->GetActorForwardVector();
	RotationAngleAxis = ((RotationSpeed *  GetWorld()->GetDeltaSeconds()) * RotationDirection) + RotationAngleAxis;

	/*Refreshes Axis value don't exceed 360 degrees */
	const bool HasToResetAngleAxis = RotationAngleAxis >= 360.f || RotationAngleAxis <= -360.f;
	if (HasToResetAngleAxis)
	{
		RotationAngleAxis = 0.0f;
	}

	/*Calculate the new position based on the angle axis about the vector */
	const FVector VectorRadius = FVector(RotationRadius, 0.f, 0.f);
	const FVector RotateNewLocation = VectorRadius.RotateAngleAxis(RotationAngleAxis, RotateAxisVector);
	const FVector HammerNewLocation = PlayerCharacterRef->GetActorLocation() + RotateNewLocation;
	SetActorLocation(HammerNewLocation);	
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool ARPGTranscendenceHammer::GetRotationValidVariance()
{  
    // Is the player is in a sufficiently accurate and valid rotation to update direccion?
	CurrentDotAngleVariance = FVector::DotProduct(PreviewForwardVectorToCompare , PlayerCharacterRef->GetActorRightVector());
	const bool bIsNearly = FMath::IsNearlyEqual(CurrentDotAngleVariance, 0.f, 0.016f);
	
	return bIsNearly;	
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ARPGTranscendenceHammer::ExpandedRotation()
{    
	
	RotationSpeed = FMath::Clamp(RotationSpeed + 3.f, MinRotationSpeedValue, MaxRotationSpeedValue);
	RotationRadius = FMath::Clamp(RotationRadius + 1, MinRotationRadiusValue, MaxRotationRadiusValue);
	if (CurrentDotAngleVariance > 0.f)
	{
		RotationDirection = -1.0f;
	}
	else
	{
		RotationDirection = 1.0f;
	}
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ARPGTranscendenceHammer::ContractedRotation()
{
	RotationDirection = RotationDirection;
	RotationSpeed = FMath::Clamp(RotationSpeed - 1.f, MinRotationSpeedValue, MaxRotationSpeedValue);
	RotationRadius = FMath::Clamp(RotationRadius - 1, MinRotationRadiusValue, MaxRotationRadiusValue);	
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ARPGTranscendenceHammer::StartSpinningMode(const bool bHasToUse ,const bool bIsInControlMode, const float NewAngleAxis)
{
  bIsHammerPreparingToUse = bHasToUse; 
  bHasToHammerControl = bIsInControlMode;
  RotationAngleAxis = RotationAngleAxis + NewAngleAxis;
  bIsInSpinningMode = true;

  //Adjust the speed and radius to create the spinning status correctly
  MinRotationSpeedValue = MinRotationSpeedValue * 6.F;
  MinRotationRadiusValue = MinRotationRadiusValue / 2.F;

  GetWorldTimerManager().SetTimer(SpinningModeHandle, this, &ARPGTranscendenceHammer::CheckSpinningModeState, GetWorld()->GetDeltaSeconds(), true , 0.20f);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ARPGTranscendenceHammer::CheckSpinningModeState()
{
	if (!bIsHammerPreparingToUse)
	{
	   StopSpinningMode();
	   return;
	}

	//Has to check the world stay in the 20 degrees front actor zone to allow use the hammer
	const float AbsoluteAngleAxis = FMath::Abs(RotationAngleAxis);
	const bool bValidRotationAngle = FMath::IsNearlyEqual(AbsoluteAngleAxis , 120.f, 5.f);
	if (!bValidRotationAngle)
	{
		return;
	}

	if (bHasToHammerControl)
	{		
		 StartMoveToEnemyCase();		 	 	
	}
	else
	{
		ProjectileHammerCase();
	}	

	bIsHammerPreparingToUse = false;
	StopSpinningMode();
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ARPGTranscendenceHammer::StopSpinningMode()
{
	if (SpinningModeHandle.IsValid() && bIsInSpinningMode)
	{
		GetWorldTimerManager().ClearTimer(SpinningModeHandle);

		bIsInSpinningMode = false;

		//Adjust back the speed and radius to revert the spinning status correctly
		MinRotationSpeedValue = MinRotationSpeedValue / 6.F;
		MinRotationRadiusValue = MinRotationRadiusValue * 2;		
	}	
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ARPGTranscendenceHammer::ProjectileHammerCase()
{
      DeactivatedHammer();
	  
	  bWasHammerUsed = true;

	  /*BP Event Use it to Spawn "BP Projectile Hammer Class" Because the base RPG Projectile inheritance system and classes was made only in BP */
	  BP_ProjectileHammerCase();
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ARPGTranscendenceHammer::StartMoveToEnemyCase()
{  
	if (!IsValid(EnemyNPCRef))
	{
        return;
	}

	GetWorldTimerManager().ClearTimer(OrbitAroundHandle);
	bIsHammerActive = false;
	bWasHammerUsed = true;

	GetWorldTimerManager().SetTimer(MoveHammerToEnemyHandle, this, &ARPGTranscendenceHammer::MoveToEnemy, GetWorld()->GetDeltaSeconds(), true);
	
	if (!EnemyNPCRef->IsPendingKill())
	{
		EnemyNPCRef->OnDestroyed.AddDynamic(this, &ARPGTranscendenceHammer::DeactivatedHammer);
	}		
	else
	{
		DeactivatedHammer();
	}	
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ARPGTranscendenceHammer::MoveToEnemy()
{
	if (!IsValid(EnemyNPCRef))
	{
		DeactivatedHammer();
		return;
	}

	//Calculation of new world location between the hammer and the enemy
	LerpMoveHammertoEnemyValue = LerpMoveHammertoEnemyValue + GetWorld()->GetDeltaSeconds();
	const float LerpAlpha = UKismetMathLibrary::MapRangeClamped(LerpMoveHammertoEnemyValue , 0.f , MoveHammerToEnemySmoothValueRange, 0.f, 1.f);
	const FVector NewLocationHammer = FMath::Lerp(GetActorLocation(), EnemyNPCRef->GetActorLocation(), LerpAlpha);
	SetActorLocation(NewLocationHammer);

	const bool bCloseEnough = LerpAlpha >= (0.3 / MoveHammerToEnemySmoothValueRange);
	if (bCloseEnough)
	{	    
		StopMoveToEnemy();
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ARPGTranscendenceHammer::StopMoveToEnemy()
{
    //The NPC becomes "Ally", in case the project has actors with more tag, a better method of search and replacement should be made.
	EnemyNPCRef->Tags[0] = FName(TEXT("Player"));

	AttachToActor(EnemyNPCRef , FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	GetWorldTimerManager().ClearTimer(MoveHammerToEnemyHandle);
	//Small Adjustment that allow Fit Hammer(Create a socket is the right)
	AddActorLocalOffset(FVector(0.f , 0.f , 80.f), false);
}
