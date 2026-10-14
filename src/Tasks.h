#pragma once

// Replacement for the old framework's ITaskPool: ActorRef validity checks
// plus main-thread marshaling through SKSE's task interface.
namespace SDS
{
	using namespace RE;

	namespace Tasks
	{
		bool IsREFRValid(const TESObjectREFR* a_refr);

		void AddTask(std::function<void()> a_task);

		void QueueActorTask(TESObjectREFR* a_refr, std::function<void(Actor*)> a_task);
		void QueueLoadedActorTask(TESObjectREFR* a_refr, std::function<void(Actor*)> a_task);
	}
}
