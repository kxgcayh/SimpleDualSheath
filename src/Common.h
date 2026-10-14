#pragma once

namespace SDS
{
	using namespace RE;

	namespace Common
	{
		bool                CanEquipEitherHand(const TESObjectWEAP* a_weapon);
		bool                IsShieldEquipped(const Actor* a_actor);
		const BGSEquipSlot* GetLeftHandSlot();
	}
}
