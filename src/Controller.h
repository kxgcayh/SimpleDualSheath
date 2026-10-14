#pragma once

#include "Config.h"
#include "Data.h"
#include "EquipManager.h"
#include "Events.h"
#include "InputHandler.h"
#include "NodeUtils.h"
#include "StringHolder.h"

namespace SDS
{
	using namespace RE;

	class Controller :
		public EquipExtensions,
		public ComboKeyPressHandler,
		public BSTEventSink<TESObjectLoadedEvent>,
		public BSTEventSink<TESInitScriptEvent>,
		public BSTEventSink<TESEquipEvent>,
		public BSTEventSink<TESSwitchRaceCompleteEvent>,
		public BSTEventSink<SKSE::ActionEvent>,
		public Events::EventSink<Events::OnSetEquipSlot>,
		public Events::EventDispatcher<SDSPlayerShieldOnBackSwitchEvent>
	{
		enum class SerializationVersion : std::uint32_t
		{
			kDataVersion1 = 1
		};

#pragma pack(push, 1)

		struct SerializedData
		{
			unsigned char shieldOnBackSwitch;
		};

#pragma pack(pop)

	public:
		enum class DrawnState : std::uint8_t
		{
			Determine,
			Sheathed,
			Drawn
		};

		Controller(const Config& a_conf);

		Controller(const Controller&)            = delete;
		Controller(Controller&&)                 = delete;
		Controller& operator=(const Controller&) = delete;
		Controller& operator=(Controller&&)      = delete;

		void                               InitializeData();
		[[nodiscard]] NiNode*              GetScbAttachmentNode(Actor* a_actor, TESObjectWEAP* a_form, NiNode* a_root, bool a_is1p) const;
		[[nodiscard]] const BSFixedString* GetScbAttachmentNodeName(NiNode* a_root, TESObjectWEAP* a_form) const;
		[[nodiscard]] const BSFixedString* GetWeaponAttachmentNodeName(Actor* a_actor, TESObjectWEAP* a_form, bool a_is1p, bool a_left) const;
		[[nodiscard]] const BSFixedString* GetShieldAttachmentNodeName(Actor* a_actor, TESObjectARMO* a_form, bool a_is1p) const;

		[[nodiscard]] inline constexpr const auto& GetConfig() const
		{
			return m_conf;
		}

		[[nodiscard]] inline const StringHolder* GetStringHolder() const
		{
			return m_strings.get();
		}

		[[nodiscard]] bool                IsShieldEnabled(Actor* a_actor) const;
		[[nodiscard]] bool                IsShieldEnabled(bool a_player) const;
		[[nodiscard]] bool                GetShieldOnBackSwitch(Actor* a_actor) const;
		[[nodiscard]] bool                GetShieldOnBackSwitch() const;
		[[nodiscard]] bool                ShouldBlockShieldHide(Actor* a_actor) const;
		[[nodiscard]] static BIPED_OBJECT GetShieldBipedObject(Actor* a_actor);

		void EvaluateDrawnStateOnNearbyActors();

		// Serialization
		void SaveGameHandler(SKSE::SerializationInterface* a_intfc);
		void LoadGameHandler(SKSE::SerializationInterface* a_intfc);

		void QueueProcessWeaponDrawnChange(TESObjectREFR* a_actor, DrawnState a_drawnState) const;

	private:
		[[nodiscard]] bool GetParentNodes(
			const Data::Weapon* a_entry,
			NiNode*             a_root,
			bool                a_left,
			NiNode*&            a_sheathedNode,
			NiNode*&            a_drawnNode) const;

		void ProcessEquippedWeapon(Actor* a_actor, const NiRootNodes& a_roots, const TESObjectWEAP* a_weapon, bool a_drawn, bool a_left) const;
		void ProcessWeaponDrawnChange(Actor* a_actor, bool a_drawn) const;

		void ProcessEquippedShield(Actor* a_actor, const NiRootNodes& a_roots, bool a_drawn, bool a_switch) const;

		[[nodiscard]] static bool GetIsDrawn(Actor* a_actor, DrawnState a_state);

		void OnActorLoad(TESObjectREFR* a_actor) const;
		void OnWeaponEquip(Actor* a_actor, const TESObjectWEAP* a_weapon);

		// Beth
		BSEventNotifyControl ProcessEvent(const TESObjectLoadedEvent* a_evn, BSTEventSource<TESObjectLoadedEvent>*) override;
		BSEventNotifyControl ProcessEvent(const TESInitScriptEvent* a_evn, BSTEventSource<TESInitScriptEvent>*) override;
		BSEventNotifyControl ProcessEvent(const TESEquipEvent* evn, BSTEventSource<TESEquipEvent>*) override;  // shield only
		BSEventNotifyControl ProcessEvent(const TESSwitchRaceCompleteEvent* evn, BSTEventSource<TESSwitchRaceCompleteEvent>*) override;

		// SKSE
		BSEventNotifyControl ProcessEvent(const SKSE::ActionEvent* a_evn, BSTEventSource<SKSE::ActionEvent>*) override;

		// EngineExtensions
		void Receive(const Events::OnSetEquipSlot& a_evn) override;

		void OnKeyPressed() override;

		const Config m_conf;

		std::shared_ptr<StringHolder>     m_strings;
		std::unique_ptr<Data::WeaponData> m_data;

		std::atomic<std::uint8_t> m_shieldOnBackSwitch;
	};
}
