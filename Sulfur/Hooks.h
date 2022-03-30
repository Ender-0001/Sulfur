#pragma once

#include "SDK.hpp"

#include "minhook/MinHook.h"

#pragma comment(lib, "minhook/minhook.lib")

using namespace SDK;

namespace Hooks
{
	bool bIsReady = false;
	bool bHasSpawned = false;
	bool bIsInGame = false;
	bool bHasInitedTheBeacon = false;

	LPVOID(*ProcessEvent)(void*, void*, void*);
	LPVOID ProcessEventHook(UObject* pObject, UFunction* pFunction, LPVOID pParams)
	{
		if (pFunction->GetName().find("BP_PlayButton") != std::string::npos)
		{
			Globals::PC->SwitchLevel(TEXT("Athena_Faceoff?game=athena"));
			bIsReady = true;
		}

		/*if (pFunction->GetName().find("ReceiveTick") != std::string::npos && pObject == Globals::PC && NetHooks::BeaconHost != NULL)
		{
			if (NetHooks::BeaconHost->IsBeaconValid())
			{
				if (NetHooks::BeaconHost->GetNetDriver())
				{
					if (NetHooks::BeaconHost->GetNetDriver()->ClientConnections.Num() != 0)
					{
						auto DeltaSeconds = ((AActor_ReceiveTick_Params*)pParams)->DeltaSeconds;
						NetHooks::NetReplicator->Tick(DeltaSeconds);
					}
				}
			}
		}*/

		if (pFunction->GetName().find("Tick") != std::string::npos)
		{
			if (GetAsyncKeyState(VK_F1) & 0x1)
			{
				if (!bHasInitedTheBeacon) {
					Replicator::InitOffsets();
					Beacon::InitOffsets();

					NetHooks::Init();
					bHasInitedTheBeacon = true;
				}
			}

			if (GetAsyncKeyState(VK_F2) & 0x1 && Globals::PC)
			{
				if (Globals::PC->Pawn)
				{
					auto Location = Globals::PC->Pawn->K2_GetActorLocation();
					auto NewFortPickup = reinterpret_cast<AFortPickup*>(Util::SpawnActor(AFortPickup::StaticClass(), Location, FRotator()));

					NewFortPickup->PrimaryPickupItemEntry.Count = 1;
					NewFortPickup->PrimaryPickupItemEntry.ItemDefinition = UObject::FindObject<UFortWeaponItemDefinition>("WID_Assault_AutoHigh_Athena_SR_Ore_T03.WID_Assault_AutoHigh_Athena_SR_Ore_T03");
					NewFortPickup->OnRep_PrimaryPickupItemEntry();

					NewFortPickup->TossPickup(Location, nullptr, 1, true);

					NetHooks::NetReplicator->Replicate(NewFortPickup); //Replicates the pickup
				}
			}

			if (GetAsyncKeyState(VK_SPACE) & 0x1 && Globals::Pawn && bHasSpawned)
			{
				if (!Globals::Pawn->IsJumpProvidingForce())
				{
					Globals::Pawn->Jump();
				}
			}

			if (GetAsyncKeyState(VK_F3) & 0x1 && Globals::Pawn && Globals::PC && bHasSpawned)
			{
				if (!Globals::Pawn->bIsDBNO)
				{
					Globals::Pawn->bIsDBNO = true;
					Globals::Pawn->OnRep_IsDBNO();
				}
				else
				{
					Globals::Pawn->bIsDBNO = false;
					Globals::Pawn->OnRep_IsDBNO();
				}
			}

			if (GetAsyncKeyState(VK_F5) & 0x1 && Globals::Pawn)
			{
				auto GameMode = static_cast<AFortGameModeAthena*>(Globals::World->AuthorityGameMode);
				GameMode->StartMatch();
				GameMode->StartPlay();

				Globals::PC->bClientPawnIsLoaded = true;
				Globals::PC->bReadyToStartMatch = true;
				Globals::PC->bAssignedStartSpawn = true;
				Globals::PC->bHasInitiallySpawned = true;
				Globals::PC->bHasClientFinishedLoading = true;
				Globals::PC->bHasServerFinishedLoading = true;
				Globals::PC->OnRep_bHasServerFinishedLoading();

				auto GameState = (AFortGameStateAthena*)Globals::World->GameState;
				auto StringLib = (UKismetStringLibrary*)UKismetStringLibrary::StaticClass();
				auto Name = StringLib->STATIC_Conv_StringToName(L"InProgress");
				GameState->MatchState = Name;
				GameState->OnRep_MatchState();
				((AAthena_GameState_C*)Globals::World->GameState)->GameplayState = EFortGameplayState::NormalGameplay;
				((AAthena_GameState_C*)Globals::World->GameState)->OnRep_GameplayState();
			}
		}

		if (pFunction->GetName().find("ReadyToStartMatch") != std::string::npos && bIsReady)
		{
			Globals::FortEngine = UObject::FindObject<UFortEngine>("FortEngine_");
			Globals::World = Globals::FortEngine->GameViewport->World;
			Globals::PC = reinterpret_cast<AFortPlayerController*>(Globals::FortEngine->GameInstance->LocalPlayers[0]->PlayerController);

			if (!bHasSpawned) {
				Globals::Pawn = reinterpret_cast<PLAYER_CLASS*>(Util::SpawnActor(PLAYER_CLASS::StaticClass(), FVector(0, 0, 5000), FRotator()));

				Globals::PC->Possess(Globals::Pawn);

				auto PlayerState = reinterpret_cast<PLAYER_STATE_CLASS*>(Globals::PC->PlayerState);
				Globals::Pawn->ServerChoosePart(EFortCustomPartType::Head, UObject::FindObject<UCustomCharacterPart>("CustomCharacterPart F_Med_Head1.F_Med_Head1"));
				Globals::Pawn->ServerChoosePart(EFortCustomPartType::Body, UObject::FindObject<UCustomCharacterPart>("CustomCharacterPart F_Med_Soldier_01.F_Med_Soldier_01"));
				PlayerState->OnRep_CharacterParts();

				PlayerState->bHasStartedPlaying = true;
				PlayerState->bHasFinishedLoading = true;
				PlayerState->bIsReadyToContinue = true;
				PlayerState->OnRep_bHasStartedPlaying();

				Globals::PC->QuickBars = (AFortQuickBars*)Util::SpawnActor(AFortQuickBars::StaticClass(), {}, {});
				Globals::PC->QuickBars->SetOwner(Globals::PC);

				auto Def = UObject::FindObject<UFortWeaponItemDefinition>("WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");

				auto TempItemInstance = Def->CreateTemporaryItemInstanceBP(1, 1);
				TempItemInstance->SetOwningControllerForTemporaryItem(Globals::PC);

				((UFortWorldItem*)TempItemInstance)->ItemEntry.Count = 1;

				auto ItemEntry = ((UFortWorldItem*)TempItemInstance)->ItemEntry;
				Globals::PC->WorldInventory->Inventory.ReplicatedEntries.Add(ItemEntry);
				Globals::PC->QuickBars->ServerAddItemInternal(ItemEntry.ItemGuid, EFortQuickBars::Primary, 0);

				Globals::PC->WorldInventory->HandleInventoryLocalUpdate();
				Globals::PC->HandleWorldInventoryLocalUpdate();
				Globals::PC->OnRep_QuickBar();
				Globals::PC->QuickBars->OnRep_PrimaryQuickBar();
				Globals::PC->QuickBars->OnRep_SecondaryQuickBar();

				Globals::Pawn->EquipWeaponDefinition(UObject::FindObject<UFortWeaponItemDefinition>("WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01"), ItemEntry.ItemGuid);

				bIsReady = false;
				bIsInGame = true;
				bHasSpawned = true;
			}
		}

		if (pFunction->GetName().find("ServerReadyToStartMatch") != std::string::npos)
		{
			std::cout << pObject->GetFullName() << " " << pFunction->GetFullName() << std::endl;
		}

		if (pFunction->GetName().find("ServerShortTimeout") != std::string::npos) {
			return NULL;
		}

		return ProcessEvent(pObject, pFunction, pParams);
	}

	static void Init()
	{
		auto FEVFT = *reinterpret_cast<void***>(Globals::FortEngine);
		auto PEAddr = FEVFT[64];

		MH_CreateHook(reinterpret_cast<LPVOID>(PEAddr), ProcessEventHook, reinterpret_cast<LPVOID*>(&ProcessEvent));
		MH_EnableHook(reinterpret_cast<LPVOID>(PEAddr));
	}
}