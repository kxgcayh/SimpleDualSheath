#include "Config.h"
#include "StringHolder.h"

namespace SDS
{
	using namespace Data;

	namespace
	{
		std::string ToLower(std::string_view a_in)
		{
			std::string out{ a_in };
			std::transform(out.begin(), out.end(), out.begin(),
				[](unsigned char a_c) { return static_cast<char>(std::tolower(a_c)); });
			return out;
		}

		void Trim(std::string& a_str)
		{
			const auto notSpace = [](unsigned char a_c) { return !std::isspace(a_c); };
			a_str.erase(a_str.begin(), std::find_if(a_str.begin(), a_str.end(), notSpace));
			a_str.erase(std::find_if(a_str.rbegin(), a_str.rend(), notSpace).base(), a_str.end());
		}

		std::vector<std::string> Split(std::string_view a_in, char a_delim)
		{
			std::vector<std::string> out;
			std::string              cur;
			for (const auto c : a_in) {
				if (c == a_delim) {
					Trim(cur);
					if (!cur.empty()) {
						out.emplace_back(std::move(cur));
						cur.clear();
					}
				} else {
					cur += c;
				}
			}
			Trim(cur);
			if (!cur.empty()) {
				out.emplace_back(std::move(cur));
			}
			return out;
		}

		class MiniINI
		{
		public:
			explicit MiniINI(const std::string& a_path)
			{
				std::ifstream file(a_path);
				if (!file.is_open()) {
					return;
				}
				_loaded = true;

				std::string        section;
				std::string        line;
				while (std::getline(file, line)) {
					if (!line.empty() && line.back() == '\r') {
						line.pop_back();
					}
					Trim(line);
					if (line.empty() || line[0] == ';' || line[0] == '#') {
						continue;
					}
					if (line.front() == '[' && line.back() == ']') {
						line = line.substr(1, line.size() - 2);
						Trim(line);
						section = ToLower(line);
						continue;
					}
					const auto pos = line.find('=');
					if (pos == std::string::npos) {
						continue;
					}
					auto key = line.substr(0, pos);
					auto val = line.substr(pos + 1);
					// strip inline comments
					if (const auto cpos = val.find_first_of(";#"); cpos != std::string::npos) {
						val = val.substr(0, cpos);
					}
					Trim(key);
					Trim(val);
					_data[ToLower(section) + '\x1F' + ToLower(key)] = val;
				}
			}

			[[nodiscard]] bool IsLoaded() const noexcept { return _loaded; }

			[[nodiscard]] std::string GetValue(std::string_view a_section, std::string_view a_key, std::string_view a_default) const
			{
				const auto it = _data.find(ToLower(a_section) + '\x1F' + ToLower(std::string{ a_key }));
				return it != _data.end() ? it->second : std::string{ a_default };
			}

			[[nodiscard]] bool GetBoolValue(std::string_view a_section, std::string_view a_key, bool a_default) const
			{
				const auto val = ToLower(GetValue(a_section, a_key, a_default ? "true" : "false"));
				if (val == "true" || val == "1" || val == "yes" || val == "on") {
					return true;
				}
				if (val == "false" || val == "0" || val == "no" || val == "off") {
					return false;
				}
				return a_default;
			}

		private:
			bool _loaded{ false };
			std::unordered_map<std::string, std::string> _data;
		};

		struct FlagDef
		{
			const char* name;
			Data::Flags flag;
			bool        internal;
		};

		constexpr FlagDef s_flagData[] = {
			{ "npc", Data::Flags::kNPC, false },
			{ "player", Data::Flags::kPlayer, false },
			{ "firstperson", Data::Flags::kFirstPerson, false },
			{ "mountonly", Data::Flags::kMountOnly, false },
			{ "right", Data::Flags::kRight, true },
			{ "swap", Data::Flags::kSwap, true },
		};
	}

	auto FlagParser::Parse(
		const std::string& a_in,
		bool               a_internal)
		-> Flags
	{
		auto out = Flags::kNone;

		for (const auto& e : Split(a_in, '|')) {
			const auto lname = ToLower(e);
			for (const auto& [name, flg, internal] : s_flagData) {
				if (lname == name) {
					if (!internal || a_internal) {
						out |= flg;
					}
					break;
				}
			}
		}

		return out;
	}

	void ConfigKeyCombo::Parse(
		const std::string& a_input)
	{
		m_comboKey = 0;
		m_key      = 0;

		std::vector<std::uint32_t> e;
		for (const auto& t : Split(a_input, '+')) {
			try {
				e.push_back(static_cast<std::uint32_t>(std::stoul(t, nullptr, 0)));
			} catch (...) {
			}
		}

		if (e.size() > 1) {
			m_comboKey = e[0];
			m_key      = e[1];
		} else if (e.size() == 1) {
			m_key = e[0];
		}
	}

	Config::Config(const std::string& a_path)
	{
		Load(a_path);
	}

	bool Config::Load(const std::string& a_path)
	{
		MiniINI reader(a_path);

		m_disableScabbards       = reader.GetBoolValue(SECT_GENERAL, "DisableAllScabbards", false);
		m_disableWeapNodeSharing = reader.GetBoolValue(SECT_GENERAL, "DisableWeaponNodeSharing", false);

		m_sword = {
			FlagParser::Parse(reader.GetValue(SECT_SWORD, KW_FLAGS, "Player|NPC")),
			reader.GetValue(SECT_SWORD, KW_SHEATHNODE, StringHolder::NINODE_SWORD_LEFT)
		};

		m_axe = {
			FlagParser::Parse(reader.GetValue(SECT_AXE, KW_FLAGS, "Player|NPC")),
			reader.GetValue(SECT_AXE, KW_SHEATHNODE, StringHolder::NINODE_AXE_LEFT)
		};

		m_mace = {
			FlagParser::Parse(reader.GetValue(SECT_MACE, KW_FLAGS, "Player|NPC")),
			reader.GetValue(SECT_MACE, KW_SHEATHNODE, StringHolder::NINODE_MACE_LEFT)
		};

		m_dagger = {
			FlagParser::Parse(reader.GetValue(SECT_DAGGER, KW_FLAGS, "Player|NPC")),
			reader.GetValue(SECT_DAGGER, KW_SHEATHNODE, StringHolder::NINODE_DAGGER_LEFT)
		};

		m_2hSword = {
			FlagParser::Parse(reader.GetValue(SECT_2HSWORD, KW_FLAGS, "")),
			reader.GetValue(SECT_2HSWORD, KW_SHEATHNODE, StringHolder::NINODE_SWORD_ON_BACK_LEFT)
		};

		m_2hAxe = {
			FlagParser::Parse(reader.GetValue(SECT_2HAXE, KW_FLAGS, "")),
			reader.GetValue(SECT_2HAXE, KW_SHEATHNODE, StringHolder::NINODE_AXE_ON_BACK_LEFT)
		};

		m_staff = {
			FlagParser::Parse(reader.GetValue(SECT_STAFF, KW_FLAGS, "Player|NPC|Right"), true),
			reader.GetValue(SECT_STAFF, KW_SHEATHNODE, StringHolder::NINODE_STAFF_LEFT)
		};

		m_shield = {
			FlagParser::Parse(reader.GetValue(SECT_SHIELD, KW_FLAGS, "")),
			reader.GetValue(SECT_SHIELD, KW_SHEATHNODE, StringHolder::NINODE_SHIELD_BACK)
		};

		m_shieldHandWorkaround = reader.GetBoolValue(SECT_SHIELD, "ClenchedHandWorkaround", false);
		m_shwForceIfDrawn      = reader.GetBoolValue(SECT_SHIELD, "ClenchedHandWorkaroundForceIfDrawn", false);
		m_shieldHideFlags      = FlagParser::Parse(reader.GetValue(SECT_SHIELD, "DisableHideOnSit", ""));
		m_shieldToggleKeys.Parse(reader.GetValue(SECT_SHIELD, "ToggleKeys", ""));

		m_npcEquipLeft = reader.GetBoolValue(SECT_NPC, "EquipLeft", false);

		return (m_loaded = reader.IsLoaded());
	}
}
