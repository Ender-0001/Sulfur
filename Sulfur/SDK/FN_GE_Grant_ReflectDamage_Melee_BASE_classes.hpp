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

// BlueprintGeneratedClass GE_Grant_ReflectDamage_Melee_BASE.GE_Grant_ReflectDamage_Melee_BASE_C
// 0x0000 (0x0670 - 0x0670)
class UGE_Grant_ReflectDamage_Melee_BASE_C : public UGameplayEffect
{
public:

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("BlueprintGeneratedClass GE_Grant_ReflectDamage_Melee_BASE.GE_Grant_ReflectDamage_Melee_BASE_C");
		return ptr;
	}

};


}

#ifdef _MSC_VER
	#pragma pack(pop)
#endif
