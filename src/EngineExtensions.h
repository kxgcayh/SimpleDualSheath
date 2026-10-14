#pragma once

#include "Controller.h"
#include "Events.h"

namespace SDS
{
	using namespace RE;

	class BShkbAnimationGraph;

	class EngineExtensions
	{
	public:
		EngineExtensions(const std::shared_ptr<Controller>& a_controller);

		EngineExtensions(const EngineExtensions&)            = delete;
		EngineExtensions& operator=(const EngineExtensions&) = delete;

		enum class MemoryValidationFlags : std::uint8_t
		{
			kNone = 0,

			kWeaponLeftAttach       = 1ui8 << 0,
			kStaffAttach            = 1ui8 << 1,
			kShieldAttach           = 1ui8 << 2,
			kDisableShieldHideOnSit = 1ui8 << 3,
			kScabbardAttach         = 1ui8 << 4,
			kScabbardDetach         = 1ui8 << 5,
			kScabbardGet            = 1ui8 << 6,
			kRemoveWeaponScabbard   = 1ui8 << 7,
		};

		static void                  Initialize(const std::shared_ptr<Controller>& a_controller);
		static MemoryValidationFlags ValidateMemory(const Config& a_config);

		inline static auto GetSingleton() noexcept
		{
			return m_Instance.get();
		}

		inline constexpr auto& GetEventDispatchers() noexcept
		{
			return m_dispatchers;
		}

	private:
		void Patch_SCB_Attach();
		void Patch_SCB_Detach();
		void Patch_SCB_Get();
		void Patch_RemoveWeaponScabbard();
		void Patch_WeaponObjects_Attach();
		void Patch_ObjectAttachDefault();
		void Patch_DisableShieldHideOnSit();
		bool Hook_TESObjectWEAP_SetEquipSlot();
		bool Patch_ShieldHandWorkaround();
		void Patch_WeapTypeToNodeArrayInit();

		static_assert(std::is_same_v<std::underlying_type_t<BIPED_OBJECT>, std::uint32_t>);

		static NiNode*     GetScbAttachmentNode_Hook(BipedAnim* a_biped, BIPED_OBJECT a_bipedSlot, NiNode* a_root);
		static NiNode*     GetScbAttachmentNode_Cleanup_Hook(TESForm* a_form, BipedAnim* a_biped);
		static NiAVObject* GetWeaponShieldSlotNode_Hook(NiNode* a_root, const BSFixedString& a_nodeName, BipedAnim* a_biped, BIPED_OBJECT a_bipedSlot, bool a_is1p, bool& a_skipHide);
		static NiAVObject* GetWeaponStaffSlotNode_Hook(NiNode* a_root, const BSFixedString& a_nodeName, BipedAnim* a_biped, BIPED_OBJECT a_bipedSlot, bool a_is1p, bool& a_skipHide);
		static NiAVObject* GetSlotNodeDefault_Hook(NiNode* a_root, const BSFixedString& a_nodeName, BipedAnim* a_biped, BIPED_OBJECT a_bipedSlot, bool a_is1p);
		static NiAVObject* GetScabbardNode_Hook(NiNode* a_object, const BSFixedString& a_nodeName, bool a_left, bool a_is1p);
		static void        TESObjectWEAP_SetEquipSlot_Hook(BGSEquipType* a_this, BGSEquipSlot* a_slot);

		static bool Unk140609D50_BShkbAnimationGraph_SetGraphVariableInt_Hook(BShkbAnimationGraph* a_graph, const BSFixedString& a_name, std::int32_t a_value, Actor* a_actor);
		static bool Unk1406097C0_IAnimationGraphManagerHolder_SetVariableOnGraphsInt_Hook(RE::IAnimationGraphManagerHolder* a_holder, const BSFixedString& a_name, std::int32_t a_value, Actor* a_actor);
		static bool Unk140634D20_IAnimationGraphManagerHolder_SetVariableOnGraphsInt_Hook(RE::IAnimationGraphManagerHolder* a_holder, const BSFixedString& a_name, std::int32_t a_value, Actor* a_actor);

		static const BSFixedString& WeapTypeToNodeInit1_Hook();
		static const BSFixedString& WeapTypeToNodeInit2_Hook();

		static bool RemoveWeaponScabbard_Rpl(NiNode* a_node);

		using BShkbAnimationGraph_SetGraphVariableInt_t             = bool (*)(BShkbAnimationGraph* a_graph, const BSFixedString& a_name, std::int32_t a_value);
		using IAnimationGraphManagerHolder_SetVariableOnGraphsInt_t = bool (*)(RE::IAnimationGraphManagerHolder* a_holder, const BSFixedString& a_name, std::int32_t a_value);

		static bool          ShouldBlockShieldHide(Actor* a_actor);
		const BSFixedString* GetWeaponAttachmentNodeName(BipedAnim* a_biped, BIPED_OBJECT a_bipedSlot, bool a_is1p, bool a_left);

		decltype(&TESObjectWEAP_SetEquipSlot_Hook)            m_TESObjectWEAP_SetEquipSlot_o{ nullptr };
		BShkbAnimationGraph_SetGraphVariableInt_t             m_BShkbAnimationGraph_SetGraphVariableInt_o{ nullptr };
		IAnimationGraphManagerHolder_SetVariableOnGraphsInt_t m_unk1406097C0_IAnimationGraphManagerHolder_SetVariableOnGraphsInt_o{ nullptr };
		IAnimationGraphManagerHolder_SetVariableOnGraphsInt_t m_unk140634D20_IAnimationGraphManagerHolder_SetVariableOnGraphsInt_o{ nullptr };

		struct
		{
			Events::EventDispatcher<Events::OnSetEquipSlot> m_setEquipSlot;
		} m_dispatchers;

		std::shared_ptr<Controller> m_controller;

		// SE ID, AE ID, SE offset, AE offset. AE offsets verified against
		// SkyrimSE.exe 1.7.104 (see README); SE offsets preserved from 1.5.97.
		inline static REL::Relocation<std::uintptr_t> m_scbAttach_a{ REL::RelocationID(15569, 15746), REL::VariantOffset(0x3A3, 0x3BA, 0x3A3) };
		inline static REL::Relocation<std::uintptr_t> m_scbGet_a{ REL::RelocationID(15569, 15746), REL::VariantOffset(0x383, 0x396, 0x383) };
		inline static REL::Relocation<std::uintptr_t> m_getShieldWeaponSlotNode_a{ REL::RelocationID(15569, 15746), REL::VariantOffset(0x1D1, 0x1C3, 0x1D1) };
		inline static REL::Relocation<std::uintptr_t> m_getStaffSlotNode_a{ REL::RelocationID(15569, 15746), REL::VariantOffset(0x223, 0x217, 0x223) };
		inline static REL::Relocation<std::uintptr_t> m_getShieldArmorSlotNode_a{ REL::RelocationID(15569, 15746), REL::VariantOffset(0x260, 0x255, 0x260) };
		inline static REL::Relocation<std::uintptr_t> m_scbDetach_a{ REL::RelocationID(15496, 15661), REL::VariantOffset(0x1A3, 0x1A5, 0x1A3) };
		inline static REL::Relocation<std::uintptr_t> m_hideShield_a{ REL::RelocationID(36580, 37584), REL::VariantOffset(0x6, 0x6, 0x6) };
		inline static REL::Relocation<std::uintptr_t> m_vtbl_TESObjectWEAP_a{ REL::RelocationID(234396, 189786) };

		inline static REL::Relocation<std::uintptr_t> m_unk140609D50_BShkbAnimationGraph_SetGraphVariableInt_a{ REL::RelocationID(36957, 37982), REL::VariantOffset(0x1DA, 0x19E, 0x1DA) };
		inline static REL::Relocation<std::uintptr_t> m_unk1406097C0_IAnimationGraphManagerHolder_SetVariableOnGraphsInt_a{ REL::RelocationID(36949, 37974), REL::VariantOffset(0x48, 0x48, 0x48) };
		// AE offset moved in 1.6.629 (0x13F -> 0x145); resolved dynamically.
		inline static REL::Relocation<std::uintptr_t> m_unk140634D20_a{ REL::RelocationID(37866, 38821) };
		static std::uintptr_t                         GetUnk140634D20Addr();

		inline static REL::Relocation<std::uintptr_t> m_removeWeaponScabbard_a{ REL::RelocationID(15646, 15872) };

		inline static REL::Relocation<std::uintptr_t> m_initWeapTypeToNodeArray_a{ REL::RelocationID(17742, 18062) };  // inlined on AE

		static std::unique_ptr<EngineExtensions> m_Instance;
	};

	SDS_DEFINE_ENUM_BITWISE(EngineExtensions::MemoryValidationFlags);
}
