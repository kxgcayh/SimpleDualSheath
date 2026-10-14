#pragma once

#include "Flags.h"

namespace SDS
{
	using namespace RE;

	class FlagParser
	{
	public:
		static Data::Flags Parse(const std::string& a_in, bool a_internal = false);
	};

	class ConfigKeyCombo
	{
	public:
		ConfigKeyCombo() = default;
		void Parse(const std::string& a_input);

		[[nodiscard]] bool Has() const
		{
			return m_key != 0;
		}

		[[nodiscard]] auto GetKey() const
		{
			return m_key;
		}

		[[nodiscard]] auto GetComboKey() const
		{
			return m_comboKey;
		}

	private:
		std::uint32_t m_key{ 0 };
		std::uint32_t m_comboKey{ 0 };
	};

	struct Config
	{
		inline static constexpr auto SECT_GENERAL = "General"sv;
		inline static constexpr auto SECT_NPC     = "NPC"sv;
		inline static constexpr auto SECT_SWORD   = "Sword"sv;
		inline static constexpr auto SECT_AXE     = "Axe"sv;
		inline static constexpr auto SECT_MACE    = "Mace"sv;
		inline static constexpr auto SECT_DAGGER  = "Dagger"sv;
		inline static constexpr auto SECT_STAFF   = "Staff"sv;
		inline static constexpr auto SECT_SHIELD  = "ShieldOnBack"sv;
		inline static constexpr auto SECT_2HSWORD = "2HSword"sv;
		inline static constexpr auto SECT_2HAXE   = "2HAxe"sv;

		inline static constexpr auto KW_FLAGS      = "Flags"sv;
		inline static constexpr auto KW_SHEATHNODE = "SheathNode"sv;

	public:
		struct ConfigEntry
		{
			flag<Data::Flags> m_flags{ Data::Flags::kNone };
			std::string       m_sheathNode;

			[[nodiscard]] inline constexpr bool IsEnabled() const noexcept
			{
				return m_flags.test_any(Data::Flags::kEnabled);
			}

			[[nodiscard]] inline constexpr bool IsPlayerEnabled() const noexcept
			{
				return m_flags.test(Data::Flags::kPlayer);
			}

			[[nodiscard]] inline constexpr bool FirstPerson() const noexcept
			{
				return m_flags.test(Data::Flags::kFirstPerson);
			}
		};

		Config() = default;
		Config(const std::string& a_path);

		bool Load(const std::string& a_path);

		[[nodiscard]] inline constexpr bool IsLoaded() const noexcept
		{
			return m_loaded;
		}

		[[nodiscard]] inline constexpr bool HasEnabled2HEntries() const noexcept
		{
			return m_2hSword.IsEnabled() ||
			       m_2hAxe.IsEnabled();
		}

		ConfigEntry m_sword;
		ConfigEntry m_axe;
		ConfigEntry m_mace;
		ConfigEntry m_dagger;
		ConfigEntry m_staff;
		ConfigEntry m_2hSword;
		ConfigEntry m_2hAxe;
		ConfigEntry m_shield;

		bool m_disableScabbards{ false };
		bool m_npcEquipLeft{ false };
		bool m_shieldHandWorkaround{ false };
		bool m_shwForceIfDrawn{ false };
		bool m_disableWeapNodeSharing{ false };

		ConfigKeyCombo m_shieldToggleKeys;

		flag<Data::Flags> m_shieldHideFlags{ Data::Flags::kNone };

	private:
		bool m_loaded{ false };
	};
}
