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

// BlueprintGeneratedClass PBWA_S1_ArchwayLargeSupport.PBWA_S1_ArchwayLargeSupport_C
// 0x0000 (0x0EF0 - 0x0EF0)
class APBWA_S1_ArchwayLargeSupport_C : public ABuildingWall
{
public:

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("BlueprintGeneratedClass PBWA_S1_ArchwayLargeSupport.PBWA_S1_ArchwayLargeSupport_C");
		return ptr;
	}

};


}

#ifdef _MSC_VER
	#pragma pack(pop)
#endif
