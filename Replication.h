#pragma once

using namespace SDK;

namespace Replication
{
	namespace Functions
	{
		bool(*ReplicateActor)(UActorChannel*);
		void(*CallPreReplication)(AActor*, UNetDriver*);
		bool(*ForceNetUpdate)(AActor*);
		__int64(*SetChannelActor)(UActorChannel*, AActor*);
		UChannel* (*CreateChannel)(UNetConnection*, int, bool, int32_t);
		void (*CloseActorChannel)(UActorChannel*, uint8_t);
		bool (*IsWithinNetRelevancyDistance)(AActor*, FVector& SrcLocation);
	}

	namespace Enums
	{
		enum EChannelType
		{
			CHTYPE_None = 0,
			CHTYPE_Control = 1,
			CHTYPE_Actor = 2,
			CHTYPE_File = 3,
			CHTYPE_Voice = 4,
			CHTYPE_MAX = 8
		};

		enum class EChannelCloseReason : uint8_t
		{
			Destroyed,
			Dormancy,
			LevelUnloaded,
			Relevancy,
			TearOff,
			MAX = 15
		};
	}

	namespace Movement
	{
		namespace Functions
		{
			void (*ClientSendAdjustment)(APlayerController*);
		}
	}
}

class Replicator
{
private:
	UNetDriver* NetDriver;
	std::vector<AActor*> DormantActors;

public:
	Replicator(UNetDriver* InNetDriver)
	{
		NetDriver = InNetDriver;
	}

	bool IsNetDriverValid() const
	{
		return (NetDriver != nullptr);
	}

	UNetDriver* GetNetDriver()
	{
		return NetDriver;
	}

	bool Replicate(AActor* InActor)
	{
		int32_t ClientsThatReplicatedIt = 0;

		if (IsNetDriverValid()) {
			for (int i = 0; i < NetDriver->ClientConnections.Num(); i++)
			{
				auto Connection = NetDriver->ClientConnections[i];

				if (Connection)
				{
					auto Channel = (UActorChannel*)(Replication::Functions::CreateChannel(Connection, Replication::Enums::EChannelType::CHTYPE_Actor, true, -1));
					
					if (Channel) {
						Replication::Functions::SetChannelActor(Channel, InActor);
						Channel->Connection = Connection;

						Replication::Functions::ReplicateActor(Channel);

						SULFUR_LOG("ReplicatingActor: " << InActor->GetName() << " To: " << Connection->GetName());

						ClientsThatReplicatedIt++;
					}
				}
			}
		}

		if (IsNetDriverValid()) {
			auto Connections = NetDriver->ClientConnections.Num();

			if (Connections == ClientsThatReplicatedIt)
			{
				return true;
			}
		}

		return false;
	}

	UActorChannel* ReplicateToClient(AActor* InActor, UNetConnection* InConnection)
	{
		if (InActor->IsA(APlayerController::StaticClass()) && InActor != InConnection->PlayerController)
			return nullptr;

		auto Channel = (UActorChannel*)(Replication::Functions::CreateChannel(InConnection, Replication::Enums::EChannelType::CHTYPE_Actor, true, -1));

		if (Channel) {
			Replication::Functions::SetChannelActor(Channel, InActor);
			Channel->Connection = InConnection;

			Replication::Functions::CallPreReplication(InActor, InConnection->Driver);

			if (Replication::Functions::ReplicateActor(Channel)) {
				SULFUR_LOG("ReplicatingActor: " << InActor->GetName() << " To: " << InConnection->GetName());
				return Channel;
			}

			//Replication::Functions::ForceNetUpdate(InActor);
		}

		return NULL;
	}

	void InitalizeConnection(UNetConnection* InConnection)
	{
		TArray<AActor*> Actors;
		Globals::GPS->STATIC_GetAllActorsOfClass(Globals::World, AActor::StaticClass(), &Actors);

		for (int i = 0; i < Actors.Num(); i++)
		{
			auto Actor = Actors[i];
			
			if (Actor)
			{
				if (Actor->bReplicates && Actor->NetDormancy != ENetDormancy::DORM_Initial && !Actor->bNetStartup)
					this->ReplicateToClient(Actor, InConnection);
			}
		}
	}

	UActorChannel* FindChannel(AActor* Actor, UNetConnection* Connection)
	{
		for (int i = 0; i < Connection->OpenChannels.Num(); i++)
		{
			auto Channel = Connection->OpenChannels[i];
			
			if (Channel && Channel->Class)
			{
				if (Channel->Class->GetFullName() == ACTOR_CHANNEL_CLASS)
				{
					if (((UActorChannel*)Channel)->Actor == Actor)
						return ((UActorChannel*)Channel);
				}
			}
		}

		return NULL;
	}

	int PrepConnections(float DeltaSeconds)
	{
		int bFoundReadyConnection = FALSE;

		for (int ConnIdx = 0; ConnIdx < NetDriver->ClientConnections.Num(); ConnIdx++)
		{
			UNetConnection* Connection = NetDriver->ClientConnections[ConnIdx];
			if (!Connection) continue;

			AActor* OwningActor = Connection->OwningActor;

			if (OwningActor)
			{
				bFoundReadyConnection = TRUE;
				AActor* DesiredViewTarget = OwningActor;

				if (Connection->PlayerController)
				{
					if (AActor* ViewTarget = Connection->PlayerController->GetViewTarget())
					{
						DesiredViewTarget = ViewTarget;
					}
				}

				Connection->ViewTarget = DesiredViewTarget;

				for (int ChildIdx = 0; ChildIdx < Connection->Children.Num(); ++ChildIdx)
				{
					UNetConnection* ChildConnection = Connection->Children[ChildIdx];
					if (ChildConnection && ChildConnection->PlayerController && ChildConnection->ViewTarget)
					{
						ChildConnection->ViewTarget = DesiredViewTarget;
					}
				}
			}
			else
			{
				Connection->ViewTarget = nullptr;

				for (int ChildIdx = 0; ChildIdx < Connection->Children.Num(); ++ChildIdx)
				{
					UNetConnection* ChildConnection = Connection->Children[ChildIdx];
					if (ChildConnection && ChildConnection->PlayerController && ChildConnection->ViewTarget)
					{
						ChildConnection->ViewTarget = nullptr;
					}
				}
			}
		}

		return bFoundReadyConnection;
	}

	void Tick(float DeltaSeconds)
	{
		++*(DWORD*)(__int64(NetDriver) + 0x2C8);

		auto NumClientsToTick = PrepConnections(DeltaSeconds);

		if (NumClientsToTick == 0)
			return;

		for (int i = 0; i < NetDriver->ClientConnections.Num(); i++)
		{
			UNetConnection* const NetConnection = NetDriver->ClientConnections[i];
			APlayerController* const PC = NetConnection->PlayerController;

			for (int i = 0; i < NetConnection->OpenChannels.Num(); i++)
			{
				auto Channel = NetConnection->OpenChannels[i];

				if (Channel && Channel->Class)
				{
					if (Channel->Class->GetFullName() == ACTOR_CHANNEL_CLASS)
					{
						if (!((UActorChannel*)Channel)->Actor)
						{
							Replication::Functions::CloseActorChannel((UActorChannel*)Channel, (uint8_t)Replication::Enums::EChannelCloseReason::Destroyed);
						}
					}
				}
			}

			if (PC && NetConnection->ViewTarget)
			{
				auto Channel = this->FindChannel(PC, NetConnection);

				if (Channel != NULL)
				{
					Replication::Functions::CallPreReplication(PC, NetConnection->Driver);
					Replication::Functions::ReplicateActor(Channel);
				}

				if (PC->Pawn)
				{
					Channel = this->FindChannel(PC->Pawn, NetConnection);

					if (Channel != NULL)
					{
						Replication::Functions::CallPreReplication(PC->Pawn, NetConnection->Driver);
						Replication::Functions::ReplicateActor(Channel);
					}
				}

				Replication::Movement::Functions::ClientSendAdjustment(PC);

				for (int ChildIdx = 0; ChildIdx < NetConnection->Children.Num(); ++ChildIdx)
				{
					UNetConnection* ChildConnection = NetConnection->Children[ChildIdx];
					if (ChildConnection && ChildConnection->PlayerController && ChildConnection->ViewTarget)
					{
						Replication::Movement::Functions::ClientSendAdjustment(ChildConnection->PlayerController);
					}
				}
			}
			else
				continue;

			TArray<AActor*> Actors;
			Globals::GPS->STATIC_GetAllActorsOfClass(Globals::World, AActor::StaticClass(), &Actors);

			for (int j = 0; j < Actors.Num(); j++)
			{
				auto Actor = Actors[j];

				/*

				if (!Actor->IsA(APlayerState::StaticClass()) && !Actor->IsA(APawn::StaticClass()) && !Actor->IsA(AFortWorldManager::StaticClass()))
					if (!Actor->bReplicates || !Actor->bNetStartup || (Actor->NetDormancy == ENetDormancy::DORM_Initial && Actor->bNetStartup))
						continue;

				*/

				auto Channel = this->FindChannel(Actor, NetConnection);

				if (Channel == NULL)
					Channel = this->ReplicateToClient(Actor, NetConnection);

				else
				{
					Replication::Functions::CallPreReplication(Actor, NetConnection->Driver);
					Replication::Functions::ReplicateActor(Channel);
				}

				if (Actor->bReplicateMovement)
					Actor->OnRep_AttachmentReplication();
			}
		}
	}

	static void InitOffsets()
	{
		SULFUR_LOG("Setting up Replication Offsets!");

		auto BaseAddr = Util::BaseAddress();

		Replication::Functions::CloseActorChannel = decltype(Replication::Functions::CloseActorChannel)(Util::FindPattern("48 89 5C 24 ? 48 89 7C 24 ? 41 56 48 83 EC 60 33 FF"));
		Replication::Functions::CallPreReplication = decltype(Replication::Functions::CallPreReplication)(Util::FindPattern("48 85 D2 0F 84 ? ? ? ? 48 8B C4 55 57 41 54 48 8D 68 A1 48 81 EC ? ? ? ? 48 89 58 08 4C 8B E2 48 89"));
		Replication::Functions::ForceNetUpdate = decltype(Replication::Functions::ForceNetUpdate)(Util::FindPattern("48 85 C0 74 23 48 8B 48 38"));
		Replication::Functions::IsWithinNetRelevancyDistance = decltype(Replication::Functions::IsWithinNetRelevancyDistance)(Util::FindPattern("48 83 EC 28 48 8B 81 ? ? ? ? 48 85 C0 74 26 0F 28 88 ? ? ? ? 0F 28 C1 F3 0F 11 4C 24 ? 0F C6 C1 55 0F C6 C9 AA F3 0F 11 4C 24 ? F3 0F 11 44 24 ? EB 18 F2 0F 10 05 ? ? ? ? 8B 05 ? ? ? ? F2 0F 11 44 24 ? 89 44 24 14 48 8D 44 24 ? F2 0F 10 00 8B 40 08 F2 0F 11 04 24 F3 0F 10 0C 24 F3 0F 10 54 24"));
		Replication::Functions::CreateChannel = decltype(Replication::Functions::CreateChannel)(BaseAddr + CREATE_CHANNEL_OFFSET);
		Replication::Functions::SetChannelActor = decltype(Replication::Functions::SetChannelActor)(BaseAddr + SET_CHANNEL_ACTOR_OFFSET);
		Replication::Functions::ReplicateActor = decltype(Replication::Functions::ReplicateActor)(BaseAddr + REPLICATE_ACTOR_OFFSET);
		Replication::Movement::Functions::ClientSendAdjustment = decltype(Replication::Movement::Functions::ClientSendAdjustment)(BaseAddr + CLIENT_SEND_AUDJUSTMENTS_OFFSET);
	}
};