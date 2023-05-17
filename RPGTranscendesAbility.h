// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/RPGGameplayAbility.h"
#include "Abilities/RPGAbilitySystemComponent.h"
#include "RPGTranscendesAbility.generated.h"

class ARPGCharacterBase;
class UAnimMontage;
class URPGGameplayAbility;
class URPGAbilityTask_PlayMontageAndWaitForEvent;
class ARPGTranscendenceHammer;

UCLASS()
class ACTIONRPG_API URPGTranscendesAbility : public URPGGameplayAbility
{
	GENERATED_BODY()

	URPGTranscendesAbility();

protected:
   
   /** Player Character Ref*/
   UPROPERTY(BlueprintReadOnly)
   ARPGCharacterBase* PlayerCharacterReference;

   /** Player Character Ability System Ref*/
   UAbilitySystemComponent* PlayerAbilitySystemRef;

   /**The Hammer Class to Use by Skill*/
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Properties")
   TSubclassOf<ARPGTranscendenceHammer> HammerClassToSpawn;
   
   /**This Objtec type determines the collision channel filter during the ability to control and determine who enemy collides.*/
   UPROPERTY(EditDefaultsOnly, Category = "Properties")
   TArray<TEnumAsByte<EObjectTypeQuery>> ControlCollisionObjectTypes;

   /**Array that it saves the current hammers reference*/
   UPROPERTY(BlueprintReadOnly)
   TArray<ARPGTranscendenceHammer*> AbilityCurrentHammersRefs;

   /**Array that it saves the current control enemys reference*/
   UPROPERTY(BlueprintReadOnly)
	TArray<ARPGCharacterBase*> AbilityCurrentEnemyRefs;

   /** Montage Task */
   UPROPERTY()
   URPGAbilityTask_PlayMontageAndWaitForEvent* CurrentMontageTask;

   /** Fire Hammer case montage reference */
   UPROPERTY(EditDefaultsOnly, Category = "Properties|Animation")
   UAnimMontage* TranscendenceAttackFireMontage;

   /** Control Hammer case montage reference */
   UPROPERTY(EditDefaultsOnly, Category = "Properties|Animation")
   UAnimMontage* TranscendenceAttackControlMontage;

   /**Effect apply during the ability that determines is in transcendence state*/
   UPROPERTY(EditDefaultsOnly, Category = "Properties|GAS")
   TSubclassOf<UGameplayEffect> TranscendenceEffectSubclass;

   /**Transcendence effect handle*/
   FActiveGameplayEffectHandle TranscendenceEffectHandle;

   /**Tag to identify if has to cancel the ability*/
   UPROPERTY(EditDefaultsOnly, Category = "Properties|GAS")
   FGameplayTag TranscendenceCancelTag;

   /**Tag to identify if has to use hammer as a Projectile*/
   UPROPERTY(EditDefaultsOnly, Category = "Properties|GAS")
   FGameplayTag StartFireProjectileHammerTag;

   /**Tag to identify if has to use hammer as a Control*/
   UPROPERTY(EditDefaultsOnly, Category = "Properties|GAS")
   FGameplayTag StartControlEnemyHammerTag;

   /**Transcendence effect identification tag*/
   UPROPERTY(EditDefaultsOnly, Category = "Properties|GAS")
   FGameplayTag TranscendenceTag;

   /**Draining mana cost effect identifcation tag*/
   UPROPERTY(EditDefaultsOnly, Category = "Properties|GAS")
   FGameplayTag DrainingManaTag;

   /**Tag received by the montage to start the fire projectle process*/
   UPROPERTY(EditDefaultsOnly, Category = "Properties|GAS")
   FGameplayTag TranscendenceAnimationEventFireTag;

   /**Tag received by the montage to start the control enemy process*/
   UPROPERTY(EditDefaultsOnly, Category = "Properties|GAS")
   FGameplayTag TranscendenceAnimationEventControlTag;

   /**Current number of hammers remaining to be used*/
   UPROPERTY( BlueprintReadOnly, Category = "Properties")
   int32 CurrentNumberOfHammers;

   /**Current Index begin using*/
   UPROPERTY( BlueprintReadOnly, Category = "Properties")
   int32 CurrentIndexHammerToUse;
   
   /**During the use process this variable gets what should be the next index of use it*/
   UPROPERTY(BlueprintReadOnly, Category = "Properties")
   int32 NextUseHammerIndexToUse;

   /**The input press event is for projectile hamemer?*/
   UPROPERTY(BlueprintReadOnly, Category = "Properties")
   uint8 bHasToSendHammerFire : 1;

   /**The radius of reach to control an enemy*/
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Properties")
   float ControlEnemiesRadius;

protected:

    /**Generic custom function to receive events and identify them with the tag*/
	UFUNCTION()
	void OnGameplayEventReceived(FGameplayEventData Payload);

	/** Listener function that will trigger when the transcendence is removed from the Player */
	UFUNCTION()
	void OnTranscendenceEffectRemoved(const FGameplayEffectRemovalInfo& GameplayEffectRemovalInfo);

	/** Called when current montage sent a event*/
	UFUNCTION()
	void OnMontageEventReceived(FGameplayTag EventTag, FGameplayEventData EventData);

	/** Called every time the mana atributte change and take control of the process */
	void OnManaChanged(const FOnAttributeChangeData& AttributeData);

	/**Activate Ability Function*/
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/**End Ability Function*/
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Play Ability Any Montage and bind the respectic Params*/
	bool PlayAbilityMontage(UAnimMontage* Montage, const float PlayRate = 1.f, const FName& StartSection = NAME_None, const bool bStopWhenAbilityEnds = true);

	/**Generic function to add a new gameplay event listener*/
	void AddWaitGameplayEvent(const FGameplayTag& Tag);

	/** Prepare the system to use any hammer*/
	void PreparetoUse();

	/**Start the process of hammer enemy control*/
	void SendHammerToControl();

	/** Use the hammer*/
	void UseHammer(const bool bHasToControl , ARPGCharacterBase* EnemyRef);

public:

	UFUNCTION(BlueprintImplementableEvent)
	void BP_EndAbility();

	UFUNCTION(BlueprintImplementableEvent)
	void BP_PreparePlayerToUseEvent(const bool bIsFire);

	UFUNCTION(BlueprintImplementableEvent)
	void BP_UseHammerEvent();

};
