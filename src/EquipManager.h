#pragma once

#include "Common.h"

namespace SDS
{
	using namespace RE;

	struct EquipCandidateCollector
	{
	public:
		EquipCandidateCollector(TESObjectWEAP* a_ignore);

		void Accept(const ContainerObject& a_entry);
		void Accept(InventoryEntryData* a_entryData);

		std::map<TESObjectWEAP*, std::int32_t> m_results;

	private:
		TESObjectWEAP* m_ignore;

		template <class T>
		void Process(TESBoundObject* a_item, T a_count);
	};

	class EquipExtensions :
		public BSTEventSink<TESContainerChangedEvent>
	{
	public:
	protected:
		static bool CheckDualWield(Actor* a_actor);
		static bool ActorQualifiesForEquip(Actor* a_actor);

		void QueueEvaluateEquip(TESObjectREFR* a_actor) const;

		void EvaluateEquip(Actor* a_actor) const;

		BSEventNotifyControl ProcessEvent(
			const TESContainerChangedEvent*           a_evn,
			BSTEventSource<TESContainerChangedEvent>* a_dispatcher) override;
	};
}
