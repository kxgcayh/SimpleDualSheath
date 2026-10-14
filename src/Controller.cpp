#include "Controller.h"
#include "Common.h"
#include "Tasks.h"

namespace SDS
{
	using namespace Common;
	using namespace Data;

	Controller::Controller(
		const Config& a_conf) :
		m_conf(a_conf),
		m_shieldOnBackSwitch(1)
	{
	}

	void Controller::InitializeData()
	{
		m_strings = std::make_shared<StringHolder>();

		m_data = std::make_unique<WeaponData>();

		m_data->Create(WEAPON_TYPE::kOneHandSword, StringHolder::NINODE_SWORD, StringHolder::NINODE_SWORD_LEFT, m_conf.m_sword);
		m_data->Create(WEAPON_TYPE::kOneHandAxe, StringHolder::NINODE_AXE, StringHolder::NINODE_AXE_LEFT, m_conf.m_axe);
		m_data->Create(WEAPON_TYPE::kOneHandMace, StringHolder::NINODE_MACE, StringHolder::NINODE_MACE_LEFT, m_conf.m_mace);
		m_data->Create(WEAPON_TYPE::kOneHandDagger, StringHolder::NINODE_DAGGER, StringHolder::NINODE_DAGGER_LEFT, m_conf.m_dagger);
		m_data->Create(WEAPON_TYPE::kStaff, StringHolder::NINODE_STAFF, StringHolder::NINODE_STAFF_LEFT, m_conf.m_staff);
		m_data->Create(WEAPON_TYPE::kTwoHandSword, StringHolder::NINODE_WEAPON_BACK, StringHolder::NINODE_SWORD_ON_BACK_LEFT, m_conf.m_2hSword);
		m_data->Create(WEAPON_TYPE::kTwoHandAxe, StringHolder::NINODE_WEAPON_BACK, StringHolder::NINODE_AXE_ON_BACK_LEFT, m_conf.m_2hAxe);

		if (!m_conf.m_shield.m_sheathNode.empty()) {
			m_strings->m_shieldSheathNode = m_conf.m_shield.m_sheathNode.c_str();
		}
	}

	bool Controller::GetParentNodes(
		const Data::Weapon* a_entry,
		NiNode*             a_root,
		bool                a_left,
		NiNode*&            a_sheathedNode,
		NiNode*&            a_drawnNode) const
	{
		const auto nodea = a_entry->GetNode(a_root, a_left);
		if (!nodea) {
			return false;
		}

		const auto nodeb = NodeUtils::GetNodeByName(
			a_root,
			a_left ?
				m_strings->m_shield :
				m_strings->m_weapon);

		if (!nodeb) {
			return false;
		}

		const auto sheathedNiNode = nodea->AsNode();
		const auto drawnNiNode    = nodeb->AsNode();

		if (!sheathedNiNode || !drawnNiNode) {
			return false;
		}

		a_sheathedNode = sheathedNiNode;
		a_drawnNode    = drawnNiNode;

		return true;
	}

	bool Controller::GetIsDrawn(
		Actor*     a_actor,
		DrawnState a_state)
	{
		switch (a_state) {
		case DrawnState::Drawn:
			return true;
		case DrawnState::Sheathed:
			return false;
		default:
			return a_actor->IsWeaponDrawn();
		};
	}

	BIPED_OBJECT Controller::GetShieldBipedObject(
		Actor* a_actor)
	{
		if (const auto race = a_actor->GetRace()) {
			return race->data.shieldObject.get();
		}

		return BIPED_OBJECT::kNone;
	}

	void Controller::ProcessEquippedWeapon(
		Actor*               a_actor,
		const NiRootNodes&   a_roots,
		const TESObjectWEAP* a_weapon,
		bool                 a_drawn,
		bool                 a_left) const
	{
		const auto entry = m_data->Get(a_actor, a_weapon, a_left);
		if (!entry) {
			return;
		}

		char buf[1024];
		a_weapon->GetNodeName(buf);

		const BSFixedString weaponNodeName(buf);

		for (std::uint32_t i = 0; i < std::size(a_roots.m_nodes); i++) {
			auto& root = a_roots.m_nodes[i];

			if (!root) {
				continue;
			}

			if (i == 1 && !entry->FirstPerson()) {
				continue;
			}

			NiNode *sheathedNode, *drawnNode;
			if (!GetParentNodes(entry, root, a_left, sheathedNode, drawnNode)) {
				continue;
			}

			auto sourceNode = a_drawn ? sheathedNode : drawnNode;
			auto targetNode = a_drawn ? drawnNode : sheathedNode;

			if (auto w1 = NodeUtils::FindChildObject(sourceNode, weaponNodeName)) {
				NodeUtils::AttachToNode(w1, targetNode);
				NodeUtils::SetVisible(w1, true);
			} else if (auto w2 = NodeUtils::FindChildObject(targetNode, weaponNodeName)) {
				NodeUtils::SetVisible(w2, true);
			}
		}
	}

	void Controller::ProcessWeaponDrawnChange(
		Actor* a_actor,
		bool   a_drawn) const
	{
		const auto pm = a_actor->GetActorRuntimeData().currentProcess;
		if (!pm) {
			return;
		}

		NiRootNodes roots(a_actor);
		roots.GetNPCRoots(m_strings->m_npcroot);

		if (const auto form = pm->GetEquippedLeftHand()) {
			if (form->IsWeapon()) {
				ProcessEquippedWeapon(a_actor, roots, static_cast<const TESObjectWEAP*>(form), a_drawn, true);
			} else if (form->IsArmor()) {
				const auto armor = static_cast<const TESObjectARMO*>(form);
				if (armor->IsShield() && m_conf.m_shield.IsEnabled()) {
					ProcessEquippedShield(a_actor, roots, a_drawn, GetShieldOnBackSwitch(a_actor));
				}
			}
		}

		if (const auto weapon = pm->GetEquippedRightHand()->As<TESObjectWEAP>()) {
			ProcessEquippedWeapon(a_actor, roots, weapon, a_drawn, false);
		}
	}

	void Controller::QueueProcessWeaponDrawnChange(
		TESObjectREFR* a_actor,
		DrawnState     a_drawnState) const
	{
		Tasks::QueueLoadedActorTask(
			a_actor,
			[this, a_drawnState](Actor* a_actor) {
				ProcessWeaponDrawnChange(
					a_actor,
					GetIsDrawn(a_actor, a_drawnState));
			});
	}

	bool Controller::IsShieldEnabled(Actor* a_actor) const
	{
		return a_actor == RE::PlayerCharacter::GetSingleton() ?
		           m_conf.m_shield.m_flags.test(Flags::kPlayer) :
		           m_conf.m_shield.m_flags.test(Flags::kNPC);
	}

	bool Controller::IsShieldEnabled(bool a_player) const
	{
		return a_player ?
		           m_conf.m_shield.m_flags.test(Flags::kPlayer) :
		           m_conf.m_shield.m_flags.test(Flags::kNPC);
	}

	bool Controller::GetShieldOnBackSwitch(Actor* a_actor) const
	{
		return a_actor != RE::PlayerCharacter::GetSingleton() || m_shieldOnBackSwitch.load(std::memory_order_acquire) != 0;
	}

	bool Controller::GetShieldOnBackSwitch() const
	{
		return m_shieldOnBackSwitch.load(std::memory_order_acquire) != 0;
	}

	bool Controller::ShouldBlockShieldHide(Actor* a_actor) const
	{
		if (a_actor == RE::PlayerCharacter::GetSingleton()) {
			if (!m_conf.m_shieldHideFlags.test(Flags::kPlayer)) {
				return false;
			}
		} else {
			if (!m_conf.m_shieldHideFlags.test(Flags::kNPC)) {
				return false;
			}
		}

		if (m_conf.m_shieldHideFlags.test(Flags::kMountOnly)) {
			return a_actor->GetActorRuntimeData().boolFlags.all(Actor::BOOL_FLAGS::kGettingOnOffMount) ||
			       a_actor->IsOnMount();
		}

		return true;
	}

	void Controller::ProcessEquippedShield(
		Actor*             a_actor,
		const NiRootNodes& a_roots,
		bool               a_drawn,
		bool               a_switch) const
	{
		if (!IsShieldEnabled(a_actor)) {
			return;
		}

		const auto bipedObject = GetShieldBipedObject(a_actor);
		if (bipedObject >= BIPED_OBJECT::kTotal) {
			return;
		}

		for (std::size_t i = 0; i < std::size(a_roots.m_nodes); i++) {
			auto& root = a_roots.m_nodes[i];

			if (!root) {
				continue;
			}

			bool firstPerson = (i == 1);

			if (firstPerson && !m_conf.m_shield.FirstPerson()) {
				continue;
			}

			auto& bipedModel = a_actor->GetBiped1(firstPerson);
			if (!bipedModel) {
				continue;
			}

			auto& data = bipedModel->objects[std::to_underlying(bipedObject)];

			const auto form = data.item;
			if (!form) {
				continue;
			}

			const auto armor = form->As<TESObjectARMO>();
			if (!armor) {
				continue;
			}

			if (!armor->IsShield()) {
				continue;
			}

			auto& armorNode = data.partClone;
			if (!armorNode) {
				continue;
			}

			const auto& targetNodeName = (a_drawn || !a_switch) ?
			                                 m_strings->m_shield :
			                                 m_strings->m_shieldSheathNode;

			auto targetNode = NodeUtils::GetNodeByName(root, targetNodeName);
			if (!targetNode) {
				continue;
			}

			const auto targetNiNode = targetNode->AsNode();
			if (!targetNiNode) {
				continue;
			}

			NodeUtils::AttachToNode(armorNode.get(), targetNiNode);
		}
	}

	NiNode* Controller::GetScbAttachmentNode(
		Actor*         a_actor,
		TESObjectWEAP* a_weapon,
		NiNode*        a_root,
		bool           a_is1p) const
	{
		auto entry = m_data->Get(a_actor, a_weapon, true);
		if (!entry) {
			return nullptr;
		}

		if (a_is1p && !entry->FirstPerson()) {
			return nullptr;
		}

		auto root = a_root;

		if (auto rootobj = root->GetObjectByName(m_strings->m_npcroot)) {
			if (auto rootnode = rootobj->AsNode()) {
				root = rootnode;
			}
		}

		return entry->GetNode(root, true);
	}

	const BSFixedString* Controller::GetScbAttachmentNodeName(NiNode*, TESObjectWEAP* a_form) const
	{
		return m_data->GetNodeName(a_form, true);
	}

	const BSFixedString* Controller::GetWeaponAttachmentNodeName(
		Actor*         a_actor,
		TESObjectWEAP* a_weapon,
		bool           a_is1p,
		bool           a_left) const
	{
		auto entry = m_data->Get(a_actor, a_weapon, a_left);
		if (!entry) {
			return nullptr;
		}

		if (a_is1p && !entry->FirstPerson()) {
			return nullptr;
		}

		return std::addressof(entry->GetNodeName(a_left));
	}

	const BSFixedString* Controller::GetShieldAttachmentNodeName(
		Actor*         a_actor,
		TESObjectARMO* a_armor,
		bool           a_is1p) const
	{
		if (!a_armor->IsShield()) {
			return nullptr;
		}

		if (!IsShieldEnabled(a_actor)) {
			return nullptr;
		}

		if (!GetShieldOnBackSwitch(a_actor)) {
			return nullptr;
		}

		if (a_actor->IsWeaponDrawn()) {
			return nullptr;
		}

		if (a_is1p && !m_conf.m_shield.FirstPerson()) {
			return nullptr;
		}

		return std::addressof(m_strings->m_shieldSheathNode);
	}

	void Controller::OnActorLoad(TESObjectREFR* a_actor) const
	{
		Tasks::QueueLoadedActorTask(a_actor, [this](Actor* a_actor) {
			ProcessWeaponDrawnChange(a_actor, a_actor->IsWeaponDrawn());

			if (m_conf.m_npcEquipLeft && ActorQualifiesForEquip(a_actor)) {
				EvaluateEquip(a_actor);
			}
		});
	}

	BSEventNotifyControl Controller::ProcessEvent(
		const TESObjectLoadedEvent* a_evn,
		BSTEventSource<TESObjectLoadedEvent>*)
	{
		if (a_evn && a_evn->loaded) {
			if (auto actor = TESForm::LookupByID<Actor>(a_evn->formID)) {
				OnActorLoad(actor);
			}
		}

		return BSEventNotifyControl::kContinue;
	}

	BSEventNotifyControl Controller::ProcessEvent(
		const TESInitScriptEvent* a_evn,
		BSTEventSource<TESInitScriptEvent>*)
	{
		if (a_evn && a_evn->objectInitialized) {
			OnActorLoad(a_evn->objectInitialized.get());
		}

		return BSEventNotifyControl::kContinue;
	}

	void Controller::OnWeaponEquip(Actor* a_actor, const TESObjectWEAP* a_weapon)
	{
		if (!m_conf.m_npcEquipLeft) {
			return;
		}

		if (!ActorQualifiesForEquip(a_actor)) {
			return;
		}

		const auto pm = a_actor->GetActorRuntimeData().currentProcess;
		if (!pm) {
			return;
		}

		if (a_weapon != pm->GetEquippedRightHand()) {
			return;
		}

		if (CanEquipEitherHand(a_weapon)) {
			QueueEvaluateEquip(a_actor);
		}
	}

	BSEventNotifyControl Controller::ProcessEvent(
		const TESEquipEvent* a_evn,
		BSTEventSource<TESEquipEvent>*)
	{
		if (a_evn && a_evn->equipped && a_evn->actor) {
			if (const auto actor = a_evn->actor->As<Actor>()) {
				if (const auto weapon = TESForm::LookupByID<TESObjectWEAP>(a_evn->baseObject)) {
					OnWeaponEquip(actor, weapon);
				}
			}
		}

		return BSEventNotifyControl::kContinue;
	}

	BSEventNotifyControl Controller::ProcessEvent(
		const TESSwitchRaceCompleteEvent* a_evn,
		BSTEventSource<TESSwitchRaceCompleteEvent>*)
	{
		if (a_evn && a_evn->subject) {
			QueueProcessWeaponDrawnChange(
				a_evn->subject.get(),
				DrawnState::Determine);
		}

		return BSEventNotifyControl::kContinue;
	}

	BSEventNotifyControl Controller::ProcessEvent(
		const SKSE::ActionEvent* a_evn,
		BSTEventSource<SKSE::ActionEvent>*)
	{
		if (a_evn) {
			switch (*a_evn->type) {
			case SKSE::ActionEvent::Type::kEndDraw:
				QueueProcessWeaponDrawnChange(a_evn->actor, DrawnState::Drawn);
				break;
			case SKSE::ActionEvent::Type::kEndSheathe:
				QueueProcessWeaponDrawnChange(a_evn->actor, DrawnState::Sheathed);
				break;
			default:
				break;
			}
		}

		return BSEventNotifyControl::kContinue;
	}

	void Controller::EvaluateDrawnStateOnNearbyActors()
	{
		Tasks::AddTask([this] {
			if (auto player = RE::PlayerCharacter::GetSingleton();
			    Tasks::IsREFRValid(player)) {
				ProcessWeaponDrawnChange(
					player,
					player->IsWeaponDrawn());
			}

			const auto pl = RE::ProcessLists::GetSingleton();
			if (!pl) {
				return;
			}

			for (const auto& handle : pl->highActorHandles) {
				if (!handle) {
					continue;
				}

				const auto actor = handle.get();
				if (!actor) {
					continue;
				}

				if (!Tasks::IsREFRValid(actor.get())) {
					continue;
				}

				ProcessWeaponDrawnChange(
					actor.get(),
					actor->IsWeaponDrawn());
			}
		});
	}

	void Controller::Receive(const Events::OnSetEquipSlot&)
	{
		const auto player = RE::PlayerCharacter::GetSingleton();
		if (!player || !player->Is3DLoaded()) {
			return;
		}

		EvaluateDrawnStateOnNearbyActors();
	}

	void Controller::SaveGameHandler(SKSE::SerializationInterface* a_intfc)
	{
		a_intfc->OpenRecord('DSDS', std::to_underlying(SerializationVersion::kDataVersion1));

		SerializedData data{
			m_shieldOnBackSwitch.load(std::memory_order_acquire)
		};

		a_intfc->WriteRecordData(&data, sizeof(data));
	}

	void Controller::LoadGameHandler(SKSE::SerializationInterface* a_intfc)
	{
		std::uint32_t type, length, version;

		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			switch (type) {
			case 'DSDS':
				{
					SerializedData data;

					if (a_intfc->ReadRecordData(std::addressof(data), sizeof(data)) == sizeof(data)) {
						m_shieldOnBackSwitch.store(data.shieldOnBackSwitch, std::memory_order_acquire);
					}
				}
				break;
			default:
				break;
			}
		}
	}

	void Controller::OnKeyPressed()
	{
		const auto n = m_shieldOnBackSwitch.fetch_xor(1, std::memory_order_acq_rel);

		const SDSPlayerShieldOnBackSwitchEvent evn{ !static_cast<bool>(n) };
		SendEvent(evn);

		Tasks::QueueLoadedActorTask(
			RE::PlayerCharacter::GetSingleton(),
			[this](Actor* a_actor) {
				NiRootNodes roots(a_actor);
				roots.GetNPCRoots(m_strings->m_npcroot);

				const bool drawn = a_actor->IsWeaponDrawn();
				const bool sw    = GetShieldOnBackSwitch(a_actor);

				ProcessEquippedShield(a_actor, roots, drawn, sw);

				if (m_conf.m_shieldHandWorkaround &&
			        !drawn &&
			        IsShieldEquipped(a_actor)) {
					const std::int32_t value = sw ? 0 : 10;

					a_actor->SetGraphVariableInt(
						m_strings->m_iLeftHandType,
						value);
				}
			});
	}
}
