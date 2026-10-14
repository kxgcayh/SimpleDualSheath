#include "Tasks.h"

namespace SDS
{
	bool Tasks::IsREFRValid(const TESObjectREFR* a_refr)
	{
		if (!a_refr || a_refr->formID == 0 || !a_refr->Is3DLoaded() || a_refr->IsDeleted()) {
			return false;
		}
		return true;
	}

	void Tasks::AddTask(std::function<void()> a_task)
	{
		if (const auto taskIntfc = SKSE::GetTaskInterface()) {
			taskIntfc->AddTask(std::move(a_task));
		} else {
			SKSE::log::error("Task interface unavailable, running inline");
			a_task();
		}
	}

	void Tasks::QueueActorTask(TESObjectREFR* a_refr, std::function<void(Actor*)> a_task)
	{
		const auto actor = a_refr ? a_refr->As<Actor>() : nullptr;
		if (!actor) {
			return;
		}
		AddTask([actor, task = std::move(a_task)] {
			if (IsREFRValid(actor)) {
				task(actor);
			}
		});
	}

	void Tasks::QueueLoadedActorTask(TESObjectREFR* a_refr, std::function<void(Actor*)> a_task)
	{
		const auto actor = a_refr ? a_refr->As<Actor>() : nullptr;
		if (!actor || !IsREFRValid(actor)) {
			return;
		}
		AddTask([actor, task = std::move(a_task)] {
			if (IsREFRValid(actor)) {
				task(actor);
			}
		});
	}
}
