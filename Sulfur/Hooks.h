#pragma once

#include "SDK.hpp"

#include "minhook/MinHook.h"

#pragma comment(lib, "minhook/minhook.lib")

using namespace SDK;

void (*SetMatchState)(AFortGameModeAthena* a1, FName NewState);

void ChangeState(const FString& NewStateStr)
{
	auto StringLib = (UKismetStringLibrary*)UKismetStringLibrary::StaticClass();
	auto NewState = StringLib->STATIC_Conv_StringToName(NewStateStr);

	auto GameMode = static_cast<AFortGameModeAthena*>(Globals::World->AuthorityGameMode);

	// Remake of AGameMode::SetMatchState

	if (GameMode->MatchState.ToString() != NewState.ToString())
	{
		SULFUR_LOG("Changed MatchState to " + NewState.ToString() + '\n');

		GameMode->MatchState = NewState;

		// OnMatchStateSet();

		auto FullGameState = (AAthena_GameState_C*)Globals::World->GameState;

		if (FullGameState)
		{
			// FullGameState->SetMatchState(NewState);

			FullGameState->MatchState = NewState;
			FullGameState->OnRep_MatchState();
		}

		GameMode->K2_OnSetMatchState(NewState);
	};

	// SetMatchState(GameMode, NewState);

	// ((AAthena_GameState_C*)Globals::World->GameState)->PreviousMatchState = ((AAthena_GameState_C*)Globals::World->GameState)->MatchState;
	// ((AAthena_GameState_C*)Globals::World->GameState)->MatchState = Name;
	// ((AAthena_GameState_C*)Globals::World->GameState)->OnRep_MatchState();

}

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
				Globals::Pawn->bIsDBNO = !Globals::Pawn->bIsDBNO;
				Globals::Pawn->OnRep_IsDBNO();
			}

			if (GetAsyncKeyState(VK_F5) & 0x1 && Globals::Pawn && Globals::PC)
			{
				auto GameMode = static_cast<AFortGameModeAthena*>(Globals::World->AuthorityGameMode);
				auto GameState = (AFortGameStateAthena*)Globals::World->GameState;

				// GameMode->StartPlay();

				Globals::PC->bClientPawnIsLoaded = true;
				Globals::PC->bReadyToStartMatch = true;
				Globals::PC->bAssignedStartSpawn = true;
				Globals::PC->bHasInitiallySpawned = true;
				Globals::PC->bHasClientFinishedLoading = true;
				Globals::PC->bHasServerFinishedLoading = true;
				Globals::PC->OnRep_bHasServerFinishedLoading();
				
				if (NetHooks::BeaconHost)
				{
					auto ClientConnections = NetHooks::BeaconHost->GetNetDriver()->ClientConnections;

					for (int i = 0; i < ClientConnections.Num(); i++)
					{
						auto PlayerController = (AFortPlayerControllerAthena*)ClientConnections[i]->PlayerController;
						auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;
						auto PawnPlayerState = (AFortPlayerStateAthena*)PlayerController->Pawn->PlayerState;

						if (PlayerState != PawnPlayerState)
							PlayerController->Pawn->PlayerState = PlayerController->PlayerState;;

						PlayerState->bHasStartedPlaying = true;
						PlayerState->bHasFinishedLoading = true;
						PlayerState->bIsReadyToContinue = true;
						PlayerState->OnRep_bHasStartedPlaying();

						PlayerController->bClientPawnIsLoaded = true;
						PlayerController->bReadyToStartMatch = true;
						PlayerController->bAssignedStartSpawn = true;
						PlayerController->bHasInitiallySpawned = true;
						PlayerController->bHasClientFinishedLoading = true;
						PlayerController->bHasServerFinishedLoading = true;
						PlayerController->OnRep_bHasServerFinishedLoading();
					}
				}

				((AAthena_GameState_C*)Globals::World->GameState)->GameplayState = EFortGameplayState::NormalGameplay;
				((AAthena_GameState_C*)Globals::World->GameState)->OnRep_GameplayState();
			}

			if (GetAsyncKeyState(VK_F6) & 0x1)
			{
				auto StringLib = (UKismetStringLibrary*)UKismetStringLibrary::StaticClass();
				auto Name = StringLib->STATIC_Conv_StringToName(L"InProgress");
				std::cout << Name.ComparisonIndex << '\n';

				auto GameMode = static_cast<AFortGameModeAthena*>(Globals::World->AuthorityGameMode);

				std::cout << GameMode->GetMatchState().ToString() << '\n';

				std::cout << GameMode->GetMatchState().ComparisonIndex << '\n';

				// GameMode->StartMatch();

				GameMode->MatchState = Name;
				((AAthena_GameState_C*)Globals::World->GameState)->MatchState = Name;
				((AAthena_GameState_C*)Globals::World->GameState)->OnRep_MatchState();

				SULFUR_LOG("Changed MatchState to InProgress!");

				std::cout << GameMode->GetMatchState().ToString() << '\n';

				std::cout << GameMode->GetMatchState().ComparisonIndex << '\n';
			}

			if (GetAsyncKeyState(VK_F7) & 1)
			{
				auto StringLib = (UKismetStringLibrary*)UKismetStringLibrary::StaticClass();
				auto Name = StringLib->STATIC_Conv_StringToName(L"InProgress");

				((AAthena_GameState_C*)Globals::World->GameState)->MatchState = Name;
				((AAthena_GameState_C*)Globals::World->GameState)->OnRep_MatchState();
			}
			
			else if (GetAsyncKeyState(VK_F8) & 1)
			{
				/* Replicated Everything
				
				[3-30-2022 15:19] Function /Script/Engine.GameStateBase.OnRep_ReplicatedHasBegunPlay Athena_GameState_C /Game/Athena/Maps/Athena_Faceoff.Athena_Faceoff.PersistentLevel.Athena_GameState_C_0
				[3-30-2022 15:19] Function /Script/Engine.GameState.OnRep_MatchState Athena_GameState_C /Game/Athena/Maps/Athena_Faceoff.Athena_Faceoff.PersistentLevel.Athena_GameState_C_0
				[3-30-2022 15:19] Function /Script/FortniteGame.FortGameStateZone.OnRep_ReplicatedWorldTimeSeconds Athena_GameState_C /Game/Athena/Maps/Athena_Faceoff.Athena_Faceoff.PersistentLevel.Athena_GameState_C_0
				[3-30-2022 15:19] Function /Script/FortniteGame.FortGameStateZone.OnRep_ReplicatedWorldTimeSeconds Athena_GameState_C /Game/Athena/Maps/Athena_Faceoff.Athena_Faceoff.PersistentLevel.Athena_GameState_C_0
				[3-30-2022 15:19] Function /Script/FortniteGame.FortPlayerController.OnRep_bHasServerFinishedLoading Athena_PlayerController_C /Game/Athena/Maps/Athena_Faceoff.Athena_Faceoff.PersistentLevel.Athena_PlayerController_C_0
				[3-30-2022 15:19] Function /Script/FortniteGame.FortGameState.OnRep_GameplayState Athena_GameState_C /Game/Athena/Maps/Athena_Faceoff.Athena_Faceoff.PersistentLevel.Athena_GameState_C_0
				[3-30-2022 15:19] Function /Script/FortniteGame.FortPlayerState.OnRep_bHasStartedPlaying FortPlayerStateAthena /Game/Athena/Maps/Athena_Faceoff.Athena_Faceoff.PersistentLevel.FortPlayerStateAthena_1
				
				*/

				auto GameMode = static_cast<AFortGameModeAthena*>(Globals::World->AuthorityGameMode);

				ChangeState(L"InProgress");

				GameMode->StartMatch();
				((AAthena_GameState_C*)Globals::World->GameState)->bReplicatedHasBegunPlay = true;
				((AAthena_GameState_C*)Globals::World->GameState)->OnRep_ReplicatedHasBegunPlay();
				GameMode->StartPlay();

				// ((AAthena_GameState_C*)Globals::World->GameState)->OnRep_MatchState();

				Globals::PC->bClientPawnIsLoaded = true;
				Globals::PC->bReadyToStartMatch = true;
				Globals::PC->bAssignedStartSpawn = true;
				Globals::PC->bHasInitiallySpawned = true;
				Globals::PC->bHasClientFinishedLoading = true;
				Globals::PC->bHasServerFinishedLoading = true;
				Globals::PC->OnRep_bHasServerFinishedLoading();

				((AAthena_GameState_C*)Globals::World->GameState)->GameplayState = EFortGameplayState::NormalGameplay;
				((AAthena_GameState_C*)Globals::World->GameState)->OnRep_GameplayState();

				if (NetHooks::BeaconHost)
				{
					auto ClientConnections = NetHooks::BeaconHost->GetNetDriver()->ClientConnections;

					for (int i = 0; i < ClientConnections.Num(); i++)
					{
						auto PlayerController = (AFortPlayerControllerAthena*)ClientConnections[i]->PlayerController;
						auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;
						auto PawnPlayerState = (AFortPlayerStateAthena*)PlayerController->Pawn->PlayerState;

						if (PlayerState != PawnPlayerState)
							PlayerController->Pawn->PlayerState = PlayerController->PlayerState;

						PlayerController->bClientPawnIsLoaded = true;
						PlayerController->bReadyToStartMatch = true;
						PlayerController->bAssignedStartSpawn = true;
						PlayerController->bHasInitiallySpawned = true;
						PlayerController->bHasClientFinishedLoading = true;
						PlayerController->bHasServerFinishedLoading = true;
						PlayerController->OnRep_bHasServerFinishedLoading();

						PlayerState->bHasStartedPlaying = true;
						PlayerState->bHasFinishedLoading = true;
						PlayerState->bIsReadyToContinue = true;
						PlayerState->OnRep_bHasStartedPlaying();
					}
				}

				// ((AAthena_GameState_C*)Globals::World->GameState)->GameplayState = EFortGameplayState::NormalGameplay;
				// ((AAthena_GameState_C*)Globals::World->GameState)->OnRep_GameplayState();
			}

			else if (GetAsyncKeyState(VK_F9) & 1)
			{
				auto TextActor = (ATextRenderActor*)Util::SpawnActor(ATextRenderActor::StaticClass(), Globals::PC->K2_GetActorLocation(), Globals::PC->K2_GetActorRotation());

				TextActor->TextRender->SetText(L"Hello");

				TextActor->SetReplicates(true);
				TextActor->bNetStartup = true;
				TextActor->SetNetDormancy(ENetDormancy::DORM_Awake);

				NetHooks::NetReplicator->Replicate(TextActor);
			}
		}

		if (pFunction->GetName().find("ReadyToStartMatch") != std::string::npos && bIsReady)
		{
			std::cout << "ReadyToStartMatch!\n";

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
				Globals::PC->QuickBars->OnRep_Owner();

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