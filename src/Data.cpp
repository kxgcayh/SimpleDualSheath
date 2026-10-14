#include "Data.h"
#include "NodeUtils.h"

namespace SDS
{
	namespace Data
	{
		using namespace RE;

		Weapon::Weapon(
			const char*                a_nodeName,
			const char*                a_nodeNameLeft,
			const Config::ConfigEntry& a_config) :
			m_nodeName(a_nodeName),
			m_nodeNameLeft(
				a_config.m_sheathNode.empty() ?
					a_nodeNameLeft :
					a_config.m_sheathNode.c_str()),
			m_flags(a_config.m_flags)
		{
		}

		const BSFixedString& Weapon::GetNodeName(bool a_left) const
		{
			bool swap = m_flags.test(Flags::kSwap);

			if (a_left) {
				return swap ? m_nodeName : m_nodeNameLeft;
			} else {
				return swap ? m_nodeNameLeft : m_nodeName;
			}
		}

		NiNode* Weapon::GetNode(NiNode* a_root, bool a_left) const
		{
			const auto obj = NodeUtils::GetNodeByName(a_root, GetNodeName(a_left));
			return obj ? obj->AsNode() : nullptr;
		}

		void WeaponData::SetStrings(
			std::uint32_t a_type,
			const char*   a_nodeName,
			const char*   a_nodeNameLeft)
		{
			if (a_type < std::size(m_entries)) {
				if (auto& entry = m_entries[a_type]) {
					if (a_nodeName) {
						entry->m_nodeName = a_nodeName;
					}

					if (a_nodeNameLeft) {
						entry->m_nodeNameLeft = a_nodeNameLeft;
					}
				}
			}
		}

		auto WeaponData::Get(
			Actor*               a_actor,
			const TESObjectWEAP* a_weapon,
			bool                 a_left) const
			-> const Weapon*
		{
			auto type = a_weapon->GetWeaponType();

			if (std::to_underlying(type) < std::size(m_entries)) {
				if (auto& entry = m_entries[std::to_underlying(type)]) {
					if (a_actor == RE::PlayerCharacter::GetSingleton()) {
						if (!entry->m_flags.test(Flags::kPlayer)) {
							return nullptr;
						}
					} else {
						if (!entry->m_flags.test(Flags::kNPC)) {
							return nullptr;
						}
					}

					if (!a_left && !entry->m_flags.test(Flags::kRight)) {
						return nullptr;
					}

					return entry.get();
				}
			}

			return nullptr;
		}

		const BSFixedString* WeaponData::GetNodeName(
			const TESObjectWEAP* a_weapon,
			bool                 a_left) const
		{
			auto type = a_weapon->GetWeaponType();

			if (std::to_underlying(type) < std::size(m_entries)) {
				if (auto& entry = m_entries[std::to_underlying(type)]) {
					return std::addressof(entry->GetNodeName(a_left));
				}
			}

			return nullptr;
		}
	}
}
