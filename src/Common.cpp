#include "Common.h"

namespace SDS
{
	bool Common::CanEquipEitherHand(const TESObjectWEAP* a_weapon)
	{
		const auto equipSlot = a_weapon->GetEquipSlot();
		if (!equipSlot) {
			return false;
		}

		const auto dom = BGSDefaultObjectManager::GetSingleton();
		if (!dom) {
			return false;
		}

		return (
			equipSlot == dom->GetObject<BGSEquipSlot>(DEFAULT_OBJECT::kRightHandEquip) ||
			equipSlot == dom->GetObject<BGSEquipSlot>(DEFAULT_OBJECT::kEitherHandEquip) ||
			equipSlot == dom->GetObject<BGSEquipSlot>(DEFAULT_OBJECT::kLeftHandEquip));
	}

	bool Common::IsShieldEquipped(const Actor* a_actor)
	{
		if (!a_actor) {
			return false;
		}

		const auto pm = a_actor->GetActorRuntimeData().currentProcess;
		if (!pm) {
			return false;
		}

		const auto form = pm->GetEquippedLeftHand();
		if (!form) {
			return false;
		}

		const auto armor = form->As<TESObjectARMO>();
		if (!armor) {
			return false;
		}

		return armor->IsShield();
	}

	const BGSEquipSlot* Common::GetLeftHandSlot()
	{
		if (const auto dom = BGSDefaultObjectManager::GetSingleton()) {
			return dom->GetObject<BGSEquipSlot>(DEFAULT_OBJECT::kLeftHandEquip);
		}
		return nullptr;
	}
}
