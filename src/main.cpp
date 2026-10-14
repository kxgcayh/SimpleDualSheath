#include "Controller.h"
#include "EngineExtensions.h"
#include "PluginInterface.h"
#include "Tasks.h"

namespace SDS
{
	namespace
	{
		std::shared_ptr<Controller>       s_controller;
		std::unique_ptr<PluginInterface>  s_pluginInterface;
		bool                              s_loaded = false;

		void MessageHandler(SKSE::MessagingInterface::Message* a_message)
		{
			switch (a_message->type) {
			case SKSE::MessagingInterface::kInputLoaded:
				{
					auto& config = s_controller->GetConfig();

					if (config.m_shieldToggleKeys.Has() &&
					    config.m_shield.IsPlayerEnabled()) {
						if (const auto inputMgr = RE::BSInputDeviceManager::GetSingleton()) {
							s_controller->SetKeys(
								config.m_shieldToggleKeys.GetComboKey(),
								config.m_shieldToggleKeys.GetKey());

							inputMgr->AddEventSink(s_controller.get());
						} else {
							SKSE::log::error("Couldn't get input device manager");
						}
					}
				}
				break;
			case SKSE::MessagingInterface::kDataLoaded:
				{
					s_controller->InitializeData();

					if (const auto edl = RE::ScriptEventSourceHolder::GetSingleton()) {
						edl->AddEventSink<RE::TESObjectLoadedEvent>(s_controller.get());
						edl->AddEventSink<RE::TESInitScriptEvent>(s_controller.get());

						auto& config = s_controller->GetConfig();

						if (config.m_npcEquipLeft) {
							edl->AddEventSink<RE::TESEquipEvent>(s_controller.get());
							edl->AddEventSink<RE::TESContainerChangedEvent>(s_controller.get());
						}
					} else {
						SKSE::log::error("Couldn't get event dispatcher list");
					}
				}
				break;
			case SKSE::MessagingInterface::kPostLoadGame:
				// skip first, evaluate on subsequent loads
				if (s_loaded) {
					s_controller->EvaluateDrawnStateOnNearbyActors();
				}
				[[fallthrough]];
			case SKSE::MessagingInterface::kNewGame:
				s_loaded = true;
				break;
			case SKSE::MessagingInterface::kPostLoad:
				if (s_controller) {
					s_pluginInterface = std::make_unique<PluginInterface>(s_controller);
				}
				break;
			default:
				break;
			}
		}

		void SaveGameHandler(SKSE::SerializationInterface* a_intfc)
		{
			s_controller->SaveGameHandler(a_intfc);
		}

		void LoadGameHandler(SKSE::SerializationInterface* a_intfc)
		{
			s_controller->LoadGameHandler(a_intfc);
		}

		std::string MVResultToString(flag<EngineExtensions::MemoryValidationFlags> a_flags)
		{
			using flag_t = EngineExtensions::MemoryValidationFlags;

			std::string result;

			if (a_flags.test(flag_t::kWeaponLeftAttach)) {
				result += "WeaponLeftAttach, ";
			}

			if (a_flags.test(flag_t::kStaffAttach)) {
				result += "StaffAttach, ";
			}

			if (a_flags.test(flag_t::kShieldAttach)) {
				result += "ShieldAttach, ";
			}

			if (a_flags.test(flag_t::kDisableShieldHideOnSit)) {
				result += "DisableShieldHideOnSit, ";
			}

			if (a_flags.test(flag_t::kScabbardAttach)) {
				result += "ScabbardAttach, ";
			}

			if (a_flags.test(flag_t::kScabbardDetach)) {
				result += "ScabbardDetach, ";
			}

			if (a_flags.test(flag_t::kScabbardGet)) {
				result += "ScabbardGet, ";
			}

			if (a_flags.test(flag_t::kRemoveWeaponScabbard)) {
				result += "RemoveWeaponScabbard";
			}

			if (result.ends_with(", ")) {
				result.resize(result.size() - 2);
			}

			return result;
		}

		bool Initialize()
		{
			Config config("Data/SKSE/Plugins/SimpleDualSheath.ini");
			if (!config.IsLoaded()) {
				SKSE::log::warn("Unable to load the configuration file, using defaults");
			}

			auto mvResult = EngineExtensions::ValidateMemory(config);

			if (mvResult != EngineExtensions::MemoryValidationFlags::kNone) {
				auto desc = MVResultToString(mvResult);
				SKSE::log::critical("Memory validation failed ({}), aborting", desc);
				return false;
			}

			auto controller = std::make_shared<Controller>(config);

			const auto messaging = SKSE::GetMessagingInterface();
			if (!messaging) {
				SKSE::log::critical("Could not get messaging interface");
				return false;
			}

			const auto actionSource = SKSE::GetActionEventSource();
			if (!actionSource) {
				SKSE::log::critical("Could not get SKSEActionEvent dispatcher");
				return false;
			}

			if (!messaging->RegisterListener(MessageHandler)) {
				SKSE::log::critical("Could not register message listener");
				return false;
			}

			const auto serialization = SKSE::GetSerializationInterface();
			if (!serialization) {
				SKSE::log::critical("Could not get serialization interface");
				return false;
			}

			serialization->SetUniqueID('ASDS');
			serialization->SetSaveCallback(SaveGameHandler);
			serialization->SetLoadCallback(LoadGameHandler);

			EngineExtensions::Initialize(controller);

			actionSource->AddEventSink(controller.get());

			s_controller = controller;

			return true;
		}
	}

	PluginInterface* GetPluginInterface()
	{
		return s_pluginInterface.get();
	}
}

extern "C" __declspec(dllexport) void* SKMP_GetPluginInterface()
{
	return SDS::GetPluginInterface();
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse, { .logName = "SimpleDualSheath", .trampoline = true, .trampolineSize = 0x800 });

	SKSE::log::info("SimpleDualSheath 1.6.0 loading (runtime {})", REL::Module::get().version().string());

	if (!SDS::Initialize()) {
		SKSE::log::critical("Plugin initialization failed");
		return false;
	}

	SKSE::log::info("SimpleDualSheath loaded");
	return true;
}
