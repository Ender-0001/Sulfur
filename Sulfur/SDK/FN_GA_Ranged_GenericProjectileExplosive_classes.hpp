#pragma once

// Fortnite (2.4.2) SDK

#ifdef _MSC_VER
	#pragma pack(push, 0x8)
#endif

#include "../SDK.hpp"

namespace SDK
{
//---------------------------------------------------------------------------
//Classes
//---------------------------------------------------------------------------

// BlueprintGeneratedClass GA_Ranged_GenericProjectileExplosive.GA_Ranged_GenericProjectileExplosive_C
// 0x0000 (0x0B00 - 0x0B00)
class UGA_Ranged_GenericProjectileExplosive_C : public UGA_Ranged_GenericDamage_C
{
public:

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("BlueprintGeneratedClass GA_Ranged_GenericProjectileExplosive.GA_Ranged_GenericProjectileExplosive_C");
		return ptr;
	}

};


}

#ifdef _MSC_VER
	#pragma pack(pop)
#endif
