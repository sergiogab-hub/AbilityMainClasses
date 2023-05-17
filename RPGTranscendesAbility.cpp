// Copyright Epic Games, Inc. All Rights Reserved.


#include "SergioTestContentClasses/RPGTranscendesAbility.h"
#include "SergioTestContentClasses/RPGTranscendenceHammer.h"
#include "Abilities/RPGAbilityTask_PlayMontageAndWaitForEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEffectRemoved.h"
#include "RPGCharacterBase.h"


URPGTranscendesAbility::URPGTranscendesAbility()
{
	PlayerCharacterReference = nullptr;
	CurrentNumberOfHammers = 0;
	AbilityCurrentHammersRefs.Empty();
	ControlEnemiesRadius = 5000.f;
	bHasToSendHammerFire = false;
	CurrentIndexHammerToUse = 0;
	NextUseHammerIndexToUse = 0;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void URPGTranscendesAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{  
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const bool bValidCommint = CommitAbility(Handle, ActorInfo, ActivationInfo);
	if (!bValidCommint)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	PlayerCharacterReference = Cast<ARPGCharacterBase>(GetAvatarActorFromActorInfo());
	PlayerAbilitySystemRef = PlayerCharacterReference->GetAbilitySystemComponent();
	if (!IsValid(PlayerCharacterReference) || !IsValid(PlayerAbilitySystemRef))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	PlayerAbilitySystemRef = PlayerCharacterReference->GetAbilitySystemComponent();
	if (!IsValid(TranscendenceEffectSubclass))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}


	//Apply the transcendence effect and bind the end effect by atribute duration finish
	const FGameplayEffectContextHandle& EffectContext = PlayerAbilitySystemRef->MakeEffectContext();
	const FGameplayEffectSpecHandle& TranscendenceModeSpecHandle = PlayerAbilitySystemRef->MakeOutgoingSpec(TranscendenceEffectSubclass, 1.f, EffectContext);
	TranscendenceEffectHandle = PlayerAbilitySystemRef->ApplyGameplayEffectSpecToSelf(*TranscendenceModeSpecHandle.Data.Get());

	UAbilityTask_WaitGameplayEffectRemoved* TranscendenceRemove = UAbilityTask_WaitGameplayEffectRemoved::WaitForGameplayEffectRemoved(this, TranscendenceEffectHandle);
	if (IsValid(TranscendenceRemove))
	{
		TranscendenceRemove->OnRemoved.AddDynamic(this, &URPGTranscendesAbility::OnTranscendenceEffectRemoved);
		TranscendenceRemove->ReadyForActivation();
	}
	
	//Event to determine if the player is mana Out
	FOnGameplayAttributeValueChange& OnManaChangedEvent = PlayerAbilitySystemRef->GetGameplayAttributeValueChangeDelegate(PlayerCharacterReference->GetAttributeSet()->GetManaAttribute());
	OnManaChangedEvent.AddUObject(this, &URPGTranscendesAbility::OnManaChanged);

	//Ability Communication events FIRE from the BP_CharacterPlayer
	AddWaitGameplayEvent(TranscendenceCancelTag);
	AddWaitGameplayEvent(StartFireProjectileHammerTag);
	AddWaitGameplayEvent(StartControlEnemyHammerTag);
	
	CurrentNumberOfHammers = PlayerCharacterReference->GetAttributeSet()->GetNumberOfHammers();
	if (CurrentNumberOfHammers > 0 && IsValid(HammerClassToSpawn))
	{
		for (int i = 0; i <= CurrentNumberOfHammers - 1; i++)
		{		    
			ARPGTranscendenceHammer* CurrentHammerToSpawn = GetWorld()->SpawnActorDeferred<ARPGTranscendenceHammer>(HammerClassToSpawn, PlayerCharacterReference->GetActorTransform(),PlayerCharacterReference, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
			if(IsValid(CurrentHammerToSpawn))
			{
			    CurrentHammerToSpawn->SetRotationAnglesAxis(i * (360 / CurrentNumberOfHammers));
				CurrentHammerToSpawn->SetPreviewForwardVector(PlayerCharacterReference->GetActorForwardVector());
				CurrentHammerToSpawn->SetCurrentHamerIndex(i);
				CurrentHammerToSpawn->FinishSpawning(PlayerCharacterReference->GetActorTransform());
				AbilityCurrentHammersRefs.Add(CurrentHammerToSpawn);
			}
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void URPGTranscendesAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!IsValid(PlayerCharacterReference) || !IsValid(PlayerAbilitySystemRef))
	{
		return;
	}

	UAnimMontage* CurrentTranscendenceMontage = PlayerCharacterReference->GetCurrentMontage();
	if (IsValid(CurrentTranscendenceMontage))
	{
		PlayerCharacterReference->StopAnimMontage(CurrentTranscendenceMontage);
	}

	// Was the ability cancel by drain mana or cancel manually
	const bool PlayerStillEffect = TranscendenceTag.IsValid() && PlayerAbilitySystemRef->HasMatchingGameplayTag(TranscendenceTag);
	if (PlayerStillEffect)
	{
		PlayerAbilitySystemRef->RemoveActiveGameplayEffect(TranscendenceEffectHandle);	
	}

	//Stop Drain Mana
	if (DrainingManaTag.IsValid())
	{
		FGameplayTagContainer TagContainer;
		TagContainer.AddTag(DrainingManaTag);
		PlayerAbilitySystemRef->RemoveActiveEffectsWithGrantedTags(TagContainer);
	}

	
	//Set defaults controls enemies
	for (ARPGCharacterBase* CurrentEnemyRef : AbilityCurrentEnemyRefs)
	{
		if (IsValid(CurrentEnemyRef))
		{
			//The NPC becomes again "Enemy", in case the project has actors with more tag, a better method of search and replacement should be made.
			CurrentEnemyRef->Tags[0] = FName(TEXT("Enemy"));
		}
	}

	//Destroy Hammers
	for (ARPGTranscendenceHammer* CurrentHammerRef : AbilityCurrentHammersRefs)
	{
		if (IsValid(CurrentHammerRef))
		{
			CurrentHammerRef->Destroy();
		}
	}

	//Sanity Defaults
	AbilityCurrentEnemyRefs.Empty();
	AbilityCurrentHammersRefs.Empty();
	bHasToSendHammerFire = false;
	CurrentIndexHammerToUse = 0;
	NextUseHammerIndexToUse = 0;

   BP_EndAbility();

   Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void URPGTranscendesAbility::OnManaChanged(const FOnAttributeChangeData& AttributeData)
{
	if (AttributeData.GEModData == nullptr)
	{
		return;
	}

	const float ManaValue = AttributeData.NewValue;

	if (ManaValue <= 0.f)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void URPGTranscendesAbility::AddWaitGameplayEvent(const FGameplayTag& Tag)
{
	if (!Tag.IsValid())
	{
		return;
	}

	UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, Tag);
	if (IsValid(WaitEventTask))
	{
		WaitEventTask->EventReceived.AddDynamic(this, &URPGTranscendesAbility::OnGameplayEventReceived);
		WaitEventTask->ReadyForActivation();
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void URPGTranscendesAbility::OnGameplayEventReceived(FGameplayEventData Payload)
{
    FGameplayTag CurrentEventTag = Payload.EventTag;
	if (CurrentEventTag.IsValid())
	{		
		if (CurrentEventTag == TranscendenceCancelTag)
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		}	

		if (CurrentEventTag == StartFireProjectileHammerTag)
		{
		    bHasToSendHammerFire = true;
			PreparetoUse();
		}

		if (CurrentEventTag == StartControlEnemyHammerTag)
		{
			PreparetoUse();
		}
    }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void URPGTranscendesAbility::SendHammerToControl()
{
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(PlayerCharacterReference);
	ActorsToIgnore.Append(AbilityCurrentEnemyRefs);

	TArray<AActor*> OverlappedActors;
	UKismetSystemLibrary::SphereOverlapActors(GetWorld() , PlayerCharacterReference->GetActorLocation() , ControlEnemiesRadius, ControlCollisionObjectTypes, ARPGCharacterBase::StaticClass() , ActorsToIgnore , OverlappedActors);
	for (AActor* OverlappedActor : OverlappedActors)
	{
		ARPGCharacterBase* OverlappedEnemy = Cast<ARPGCharacterBase>(OverlappedActor);
		const bool bValidOverlappedEnemy = IsValid(OverlappedEnemy) && OverlappedEnemy->ActorHasTag(FName(TEXT("Enemy"))) && !AbilityCurrentEnemyRefs.Contains(OverlappedEnemy);
		if (bValidOverlappedEnemy)
		{
			AbilityCurrentEnemyRefs.Add(OverlappedEnemy);
			UseHammer(true , OverlappedEnemy);
			break;
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void URPGTranscendesAbility::PreparetoUse()
{
   ARPGTranscendenceHammer* CurrentHammerRef = AbilityCurrentHammersRefs[CurrentIndexHammerToUse];
   if (!IsValid(CurrentHammerRef))
   {
      return;
   }
   
   //Is Player is valid state to use the next hammer
   UAnimMontage* MyCurrentMontage = PlayerCharacterReference->GetCurrentMontage();
   const bool bIsNotPerfomingMontage = MyCurrentMontage != TranscendenceAttackFireMontage && MyCurrentMontage != TranscendenceAttackControlMontage;
   const bool bIsValidState= (!CurrentHammerRef->GetIsPreparingToUse()) && !(CurrentIndexHammerToUse + 1 >= PlayerCharacterReference->GetAttributeSet()->GetNumberOfHammers());
   const bool bIsValidUse = bIsNotPerfomingMontage && bIsValidState;
   if (!bIsValidUse)
   {
      bHasToSendHammerFire = false;
	  return;
   }

   if (bHasToSendHammerFire)
   {
       PlayAbilityMontage(TranscendenceAttackFireMontage);
   }
   else
   {
       PlayAbilityMontage(TranscendenceAttackControlMontage);
   }

   //BP Event to switch VFX And visuals
   BP_PreparePlayerToUseEvent(bHasToSendHammerFire);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void URPGTranscendesAbility::UseHammer(const bool bHasToControl, ARPGCharacterBase* EnemyRef)
{
	if (bHasToControl && !EnemyRef)
	{
		return;
	}

    CurrentNumberOfHammers--;
	int32 AuxAngleCalculateCounter = 1;
	for (int i = 0 ; i <= AbilityCurrentHammersRefs.Num() -1  ; i++)
	{
	    ARPGTranscendenceHammer* HammerRef = AbilityCurrentHammersRefs[i];
		if (IsValid(HammerRef))
		{
			if (NextUseHammerIndexToUse == i)
			{
				HammerRef->SetEnemyNPCRef(EnemyRef);
				HammerRef->StartSpinningMode(true, bHasToControl, 0.f);
				CurrentIndexHammerToUse = i;
			}
			else if (!HammerRef->GetWasHammerUsed())
			{
			    //Calculate the new hammer angle axis based on his current rotation direction
				const float AngleAxisToCompare = ((360 / CurrentNumberOfHammers) - (360 / (CurrentNumberOfHammers + 1))) * AuxAngleCalculateCounter;
				const float AnglesVariance = HammerRef->GetRotationDirection() > 0.f ? 360.f : 0;
				const float NewAngleAxis = AngleAxisToCompare - AnglesVariance;
				HammerRef->StartSpinningMode(false, false, NewAngleAxis);
				AuxAngleCalculateCounter++;
			}
		}	
	}

	NextUseHammerIndexToUse++;

	BP_UseHammerEvent();
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool URPGTranscendesAbility::PlayAbilityMontage(UAnimMontage* Montage, const float PlayRate /*= 1.f*/, const FName& StartSection /*= NAME_None*/, const bool bStopWhenAbilityEnds /*= true*/)
{
	if (!IsValid(Montage))
	{
		return false;
	}
	CurrentMontageTask = URPGAbilityTask_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
		this,
		NAME_None,
		Montage,
		FGameplayTagContainer(),
		PlayRate,
		StartSection,
		bStopWhenAbilityEnds,
		1.f
	);

	CurrentMontageTask->EventReceived.AddDynamic(this, &URPGTranscendesAbility::OnMontageEventReceived);
	CurrentMontageTask->ReadyForActivation();

	return true;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void URPGTranscendesAbility::OnTranscendenceEffectRemoved(const FGameplayEffectRemovalInfo& GameplayEffectRemovalInfo)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void URPGTranscendesAbility::OnMontageEventReceived(FGameplayTag EventTag, FGameplayEventData EventData)
{
	if (EventTag.IsValid())
	{
		if (EventTag == TranscendenceAnimationEventFireTag)
		{
		    UseHammer(false, nullptr);
			bHasToSendHammerFire = false;
		}

		if (EventTag == TranscendenceAnimationEventControlTag)
		{
		    SendHammerToControl();
		}
	}
}