// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Abilities/GameplayAbility.h"
#include "RPGTranscendenceHammer.generated.h"

class ARPGCharacterBase;
class USceneComponent;
class UStaticMesh;

UCLASS()
class ACTIONRPG_API ARPGTranscendenceHammer : public AActor
{
	GENERATED_BODY()
public:

	// Sets default values for this actor's properties
	ARPGTranscendenceHammer();

protected:

    /**Angle which the hammer rotates based on the player */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly , Category = "Properties|HammerRotation")
	float RotationAngleAxis;

	/**Direction in which it rotates, -1 left or 1 right*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Properties|HammerRotation")
	float RotationDirection;

	/**Which Axis will rotate in this case Z*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Properties|HammerRotation")
	FVector RotateAxisVector;

	/**The speed at which the hammer rotates*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Properties|HammerRotation")
	float RotationSpeed;

	/**The radius at which the hammer rotates */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Properties|HammerRotation")
	float RotationRadius;

	/**Compare vector of the player's past forward position vs the current one*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Properties|HammerRotation")
	FVector PreviewForwardVectorToCompare;

	/**The minimum rotation speed that the hammers can have on the axis*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Properties|HammerRotation")
	float MinRotationSpeedValue;

	/**The Maximum rotation speed that the hammers can have on the axis*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Properties|HammerRotation")
	float MaxRotationSpeedValue;

	/**The minimum radius that the hammers can have around axis*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Properties|HammerRotation")
	float MinRotationRadiusValue;

	/**The maximum radius that the hammers can have around axis*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Properties|HammerRotation")
	float MaxRotationRadiusValue;

	UPROPERTY()
	FTimerHandle OrbitAroundHandle;

	UPROPERTY()
	FTimerHandle MoveHammerToEnemyHandle;

	UPROPERTY()
	FTimerHandle  SpinningModeHandle;

	/**Player Ref*/
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite , Category = "Properties| References")
	ARPGCharacterBase* PlayerCharacterRef;

	/**Enemy Ref*/
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Properties| References")
	ARPGCharacterBase* EnemyNPCRef;
	
	/**If it is true the hammer is rotanting in spinning mode and preparing to use it*/
	UPROPERTY(BlueprintReadWrite)
	uint8 bIsInSpinningMode : 1;

	/**If it is true the hammer has already been used in fire o control case */
	UPROPERTY(BlueprintReadOnly )
	uint8 bWasHammerUsed : 1;

	/**If it is true the hammer is preparing to use */
	UPROPERTY(BlueprintReadOnly)
	uint8 bIsHammerPreparingToUse : 1;

	/**Is the Hammer active?*/
	UPROPERTY(BlueprintReadOnly)
	uint8 bIsHammerActive : 1;

	/**Is the Hammer to control enemys?*/
	UPROPERTY(BlueprintReadOnly)
	uint8 bHasToHammerControl : 1;

	/**The current index of the hammer in the main Array on the ability "GA_Transcendence*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly , Category = "Properties")
	int32 CurrentHamexIndex;

	/**This value allows to adjust the speed]/smoothness of the movement with which hammer is moving toenemy*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Properties|HammerRotation")
	float MoveHammerToEnemySmoothValueRange;

	/** Save the current value of the Lerp*/
	float LerpMoveHammertoEnemyValue;
    
	/**Variance Player Angle */
	float CurrentDotAngleVariance;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Try to SetUp the necessary references */
	void TrySetupReferences();

	/**Function that activates the Hammer and its main orbit functionality*/
	void ActivatedHammer();

	/**Function that deactivates the Hammer it does not destroy it, it only remains inactive.*/
	/**@Param :"DeactivatedByRef "Necessary reference when it is called from the on destroy delegate. Nullptr by default */
	UFUNCTION()
	void DeactivatedHammer(AActor* DeactivatedByRef = nullptr);

	/**Adjust and updates the correct positioning of the hammers around the player*/
	void HammersOrbitMovement();

	/**The Valid Variance distance lets me know when a player is rolling or walking in a sufficient direction to update the orbit direction of the hammers*/
	bool GetRotationValidVariance();

	/** Hammers Expanding on their own orbit*/
	void ExpandedRotation();

    /** Hammers Contrated on their own orbit*/
	void ContractedRotation();

	/**Check when the hammer is an acceptable angle to shoot smoothly forward case*/
	void CheckSpinningModeState();

	/**Stop Spinning the hammer and return to its orbital state*/
	void StopSpinningMode();

	/**Fire as a projectile case (Click Left event)*/
	void ProjectileHammerCase();
	
	/**Support event to help spawm the projectile class (no c++ inheritance by project default)*/
	UFUNCTION(BlueprintImplementableEvent)
	void BP_ProjectileHammerCase();

	/**Prepare the hammer for the enemy control case */
	void StartMoveToEnemyCase();

	/**This function moves the hammer in the direction of the enemy in a fluid way to "control" it.*/
	void MoveToEnemy();

	/**Stop the move enemy update and adjust the attach hammer*/
	void StopMoveToEnemy();

public:

	/**Start Spinning the hammer and prepare to posible use it case*/
	UFUNCTION(BlueprintCallable)
	void StartSpinningMode(const bool bHasToUse, const bool bIsInControlMode, const float NewAngleAxis);

	/**RotationAngleAxis Setter Function */
	UFUNCTION(BlueprintCallable)
	void SetRotationAnglesAxis(const float NewRotationAxis) { RotationAngleAxis = NewRotationAxis;}

	/**NewPreviewVector Setter Function */
	UFUNCTION(BlueprintCallable)
	void SetPreviewForwardVector(const FVector& NewPreviewVector) { PreviewForwardVectorToCompare = NewPreviewVector ; }

	/**NewIndex Setter Function */
	UFUNCTION(BlueprintCallable)
	void SetCurrentHamerIndex(const int32 NewIndex) { CurrentHamexIndex = NewIndex; }

	/**NewEnemyRef Setter Function */
	UFUNCTION(BlueprintCallable)
	void SetEnemyNPCRef(ARPGCharacterBase* NewEnemyRef) {EnemyNPCRef = NewEnemyRef;}

	UFUNCTION(BlueprintCallable)
	float GetRotationDirection() const { return RotationDirection;}

	UFUNCTION(BlueprintCallable)
	bool GetWasHammerUsed() const { return bWasHammerUsed; }

	UFUNCTION(BlueprintCallable)
	bool GetIsPreparingToUse() const { return bIsHammerPreparingToUse; }

	UFUNCTION(BlueprintImplementableEvent , BlueprintCallable)
	void BP_ToggleHammerVFX(const bool bHasToFireVFX);
};