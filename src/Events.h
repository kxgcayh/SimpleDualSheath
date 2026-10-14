#pragma once

// Tiny replacement for the old framework's ::Events::EventSink /
// ThreadSafeEventDispatcher used for internal plugin events.
namespace SDS
{
	namespace Events
	{
		struct OnSetEquipSlot
		{
		};

		template <class T>
		class EventSink
		{
		public:
			virtual ~EventSink() = default;

			virtual void Receive(const T& a_evn) = 0;
		};

		template <class T>
		class EventDispatcher
		{
		public:
			void AddSink(EventSink<T>* a_sink)
			{
				const std::lock_guard lock(m_lock);
				m_sinks.push_back(a_sink);
			}

			void SendEvent(const T& a_evn)
			{
				const std::lock_guard lock(m_lock);
				for (const auto sink : m_sinks) {
					if (sink) {
						sink->Receive(a_evn);
					}
				}
			}

		private:
			std::mutex                m_lock;
			std::vector<EventSink<T>*> m_sinks;
		};
	}

	struct SDSPlayerShieldOnBackSwitchEvent
	{
		bool shieldOnBack;
	};
}
