#include "EquipManager.h"
#include "Tasks.h"

namespace SDS
{
	bool EquipExtensions::CheckDualWield(Actor* a_actor)
	{
		auto race = a_actor->GetRace();

		if (!race) {
			return false;
		}

		if (!race->data.flags.any(RACE_DATA::Flag::kCanDualWield)) {
			return false;
		}

		if (auto extraCombatStyle = a_actor->extraList.GetByType<ExtraCombatStyle>()) {
			if (auto cs = extraCombatStyle->combatStyle) {
				return cs->flags.any(TESCombatStyle::FLAG::kAllowDualWielding);
			}
		}

		if (auto npc = a_actor->GetActorBase()) {
			if (auto cs = npc->combatStyle) {
				return cs->flags.any(TESCombatStyle::FLAG::kAllowDualWielding);
			}
		}

		return false;
	}

	bool EquipExtensions::ActorQualifiesForEquip(Actor* a_actor)
	{
		return a_actor != RE::PlayerCharacter::GetSingleton() && CheckDualWield(a_actor) && !a_actor->IsDead();
	}

	EquipCandidateCollector::EquipCandidateCollector(
		TESObjectWEAP* a_ignore) :
		m_ignore(a_ignore)
	{
	}

	void EquipCandidateCollector::Accept(const ContainerObject& a_entry)
	{
		if (a_entry.obj) {
			Process(a_entry.obj, a_entry.count);
		}
	}

	void EquipCandidateCollector::Accept(InventoryEntryData* a_entryData)
	{
		if (a_entryData && a_entryData->object) {
			Process(a_entryData->object, a_entryData->countDelta);
		}
	}

	template <class T>
	void EquipCandidateCollector::Process(
		TESBoundObject* a_item,
		T               a_count)
	{
		if (a_item == m_ignore) {
			return;
		}

		auto weapon = a_item->As<TESObjectWEAP>();
		if (!weapon) {
			return;
		}

		if (!Common::CanEquipEitherHand(weapon)) {
			return;
		}

		m_results.try_emplace(weapon, 0).first->second += a_count;
	}

	enum class EquipItemResult
	{
		kEquipped,
		kFailed,
		kSlotInUse,
		kNotEnoughItems
	};

	static EquipItemResult EquipItem(
		Actor*         a_actor,
		TESObjectWEAP* a_item)
	{
		const auto targetEquipSlot = Common::GetLeftHandSlot();
		if (!targetEquipSlot) {
			return EquipItemResult::kFailed;
		}

		const auto equipManager = ActorEquipManager::GetSingleton();
		if (!equipManager) {
			return EquipItemResult::kFailed;
		}

		const auto changes = a_actor->extraList.GetByType<ExtraContainerChanges>();
		if (!changes || !changes->changes || !changes->changes->entryList) {
			return EquipItemResult::kFailed;
		}

		for (const auto entry : *changes->changes->entryList) {
			if (!entry || entry->object != a_item) {
				continue;
			}

			// Skip worn copies (including the one in the right hand); an
			// unworn stack entry means a spare copy is available.
			if (entry->IsWorn() || entry->countDelta <= 0) {
				continue;
			}

			ExtraDataList* xList = nullptr;
			if (entry->extraLists && entry->extraLists->begin() != entry->extraLists->end()) {
				xList = *entry->extraLists->begin();
			}

			equipManager->EquipObject(a_actor, a_item, xList, 1, targetEquipSlot, false, false, false, false);

			return EquipItemResult::kEquipped;
		}

		return EquipItemResult::kNotEnoughItems;
	}

	void EquipExtensions::QueueEvaluateEquip(TESObjectREFR* a_actor) const
	{
		Tasks::QueueLoadedActorTask(
			a_actor,
			[this](Actor* a_actor) {
				EvaluateEquip(a_actor);
			});
	}

	void EquipExtensions::EvaluateEquip(Actor* a_actor) const
	{
		const auto pm = a_actor->GetActorRuntimeData().currentProcess;
		if (!pm) {
			return;
		}

		if (pm->GetEquippedLeftHand()) {
			return;
		}

		const auto right = pm->GetEquippedRightHand();
		if (!right) {
			return;
		}

		auto weaponRight = right->As<TESObjectWEAP>();
		if (!weaponRight) {
			return;
		}

		if (!Common::CanEquipEitherHand(weaponRight)) {
			return;
		}

		auto res = EquipItem(a_actor, weaponRight);
		switch (res) {
		case EquipItemResult::kSlotInUse:
		case EquipItemResult::kEquipped:
			return;
		default:
			break;
		}

		EquipCandidateCollector collector(weaponRight);

		if (auto npc = a_actor->GetActorBase()) {
			npc->ForEachContainerObject([&](ContainerObject& a_obj) {
				collector.Accept(a_obj);
				return BSContainer::ForEachResult::kContinue;
			});
		}

		const auto changes = a_actor->extraList.GetByType<ExtraContainerChanges>();
		if (changes && changes->changes && changes->changes->entryList) {
			for (const auto entry : *changes->changes->entryList) {
				if (entry) {
					collector.Accept(entry);
				}
			}
		}

		if (collector.m_results.empty()) {
			return;
		}

		std::vector<std::pair<std::uint32_t, TESObjectWEAP*>> sortedWeapons;
		sortedWeapons.reserve(collector.m_results.size());

		for (const auto& e : collector.m_results) {
			if (e.second <= 0) {
				continue;
			}

			auto damage = static_cast<std::uint32_t>(e.first->attackDamage);

			auto it = std::lower_bound(
				sortedWeapons.cbegin(),
				sortedWeapons.cend(),
				damage,
				[](auto& a_data, auto a_value) {
					return a_data.first > a_value;
				});

			sortedWeapons.emplace(it, damage, e.first);
		}

		for (const auto& e : sortedWeapons) {
			auto r = EquipItem(a_actor, e.second);
			switch (r) {
			case EquipItemResult::kSlotInUse:
			case EquipItemResult::kEquipped:
				return;
			default:
				break;
			}
		}
	}

	BSEventNotifyControl EquipExtensions::ProcessEvent(
		const TESContainerChangedEvent* a_evn,
		BSTEventSource<TESContainerChangedEvent>*)
	{
		if (a_evn) {
			if (auto actor = TESForm::LookupByID<Actor>(a_evn->newContainer)) {
				if (ActorQualifiesForEquip(actor)) {
					if (auto weapon = TESForm::LookupByID<TESObjectWEAP>(a_evn->baseObj)) {
						if (Common::CanEquipEitherHand(weapon)) {
							QueueEvaluateEquip(actor);
						}
					}
				}
			}
		}

		return BSEventNotifyControl::kContinue;
	}
}
