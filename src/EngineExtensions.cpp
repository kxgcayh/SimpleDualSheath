#include "EngineExtensions.h"
#include "Common.h"
#include "NodeUtils.h"
#include "Tasks.h"

namespace SDS
{
	using namespace Xbyak;

	namespace
	{
		[[nodiscard]] bool IsAE()
		{
			return REL::Module::IsAE();
		}

		[[nodiscard]] bool IsG629()
		{
			return REL::Module::get().version() >= SKSE::RUNTIME_SSE_1_6_629;
		}

		bool ValidateMem(std::uintptr_t a_addr, const std::uint8_t* a_data, std::size_t a_size)
		{
			return std::memcmp(reinterpret_cast<const void*>(a_addr), a_data, a_size) == 0;
		}

		void* PlaceCode(Xbyak::CodeGenerator& a_code)
		{
			a_code.ready();
			auto mem = SKSE::GetTrampoline().allocate(a_code.getSize());
			std::memcpy(mem, a_code.getCode(), a_code.getSize());
			return mem;
		}

		[[nodiscard]] std::uintptr_t GetCallDst5(std::uintptr_t a_addr)
		{
			const auto rel = *reinterpret_cast<const std::int32_t*>(a_addr + 1);
			return a_addr + 5 + static_cast<std::intptr_t>(rel);
		}
	}

	std::unique_ptr<EngineExtensions> EngineExtensions::m_Instance;

	EngineExtensions::EngineExtensions(
		const std::shared_ptr<Controller>& a_controller)
	{
		auto& config = a_controller->GetConfig();

		Patch_WeaponObjects_Attach();
		Patch_SCB_Get();
		Patch_SCB_Attach();
		Patch_SCB_Detach();
		Patch_RemoveWeaponScabbard();

		if (config.m_shield.IsEnabled() ||
		    config.m_disableWeapNodeSharing) {
			Patch_ObjectAttachDefault();
		}

		if (config.m_shield.IsEnabled() &&
		    config.m_shieldHandWorkaround) {
			if (!Patch_ShieldHandWorkaround()) {
				SKSE::log::error("Shield hand patches failed");
			}
		}

		if (config.m_shieldHideFlags.test_any(Data::Flags::kEnabled)) {
			Patch_DisableShieldHideOnSit();
		}

		if (config.HasEnabled2HEntries()) {
			if (Hook_TESObjectWEAP_SetEquipSlot()) {
				m_dispatchers.m_setEquipSlot.AddSink(a_controller.get());
			}
		}

		if (config.m_disableWeapNodeSharing) {
			Patch_WeapTypeToNodeArrayInit();
		}

		m_controller = a_controller;
	}

	void EngineExtensions::Initialize(
		const std::shared_ptr<Controller>& a_controller)
	{
		if (!m_Instance) {
			m_Instance = std::make_unique<EngineExtensions>(a_controller);
		}
	}

	std::uintptr_t EngineExtensions::GetUnk140634D20Addr()
	{
		const auto base = m_unk140634D20_a.address();
		if (IsAE()) {
			return base + (IsG629() ? 0x145 : 0x13F);
		}
		return base + 0x17B;
	}

	auto EngineExtensions::ValidateMemory(
		const Config& a_config) -> MemoryValidationFlags
	{
		using flag_t = flag<MemoryValidationFlags>;

		flag_t result(MemoryValidationFlags::kNone);

		constexpr std::uint8_t d_memCall5[]{ 0xE8 };

		if (!ValidateMem(m_getShieldWeaponSlotNode_a.address(), d_memCall5, sizeof(d_memCall5))) {
			result |= MemoryValidationFlags::kWeaponLeftAttach;
		}

		if (!ValidateMem(m_getStaffSlotNode_a.address(), d_memCall5, sizeof(d_memCall5))) {
			result |= MemoryValidationFlags::kStaffAttach;
		}

		if (!ValidateMem(m_scbGet_a.address(), d_memCall5, sizeof(d_memCall5))) {
			result |= MemoryValidationFlags::kScabbardGet;
		}

		if (a_config.m_shield.IsEnabled()) {
			if (!ValidateMem(m_getShieldArmorSlotNode_a.address(), d_memCall5, sizeof(d_memCall5))) {
				result |= MemoryValidationFlags::kShieldAttach;
			}
		}

		if ((a_config.m_shieldHideFlags.underlying() & static_cast<std::uint32_t>(Data::Flags::kEnabled)) != 0) {
			if (IsAE()) {
				constexpr std::uint8_t d_hideShield[]{ 0x4C, 0x8B, 0x0A, 0x4D, 0x85, 0xC9, 0x74, 0x4F };
				if (!ValidateMem(m_hideShield_a.address(), d_hideShield, sizeof(d_hideShield))) {
					result |= MemoryValidationFlags::kDisableShieldHideOnSit;
				}
			} else {
				constexpr std::uint8_t d_hideShield[]{ 0x4C, 0x8B, 0x0A, 0x48, 0x8B, 0x81, 0xF0, 0x01, 0x00, 0x00 };
				if (!ValidateMem(m_hideShield_a.address(), d_hideShield, sizeof(d_hideShield))) {
					result |= MemoryValidationFlags::kDisableShieldHideOnSit;
				}
			}
		}

		if (IsAE()) {
			constexpr std::uint8_t d_scbAttach[]{ 0x80, 0x7D, 0x6F, 0x00, 0x75, 0x1E, 0x48, 0x8B, 0x06 };
			if (!ValidateMem(m_scbAttach_a.address(), d_scbAttach, sizeof(d_scbAttach))) {
				result |= MemoryValidationFlags::kScabbardAttach;
			}

			constexpr std::uint8_t d_scbDetach[]{ 0x0F, 0x84, 0xB1, 0x00, 0x00, 0x00 };
			if (!ValidateMem(m_scbDetach_a.address(), d_scbDetach, sizeof(d_scbDetach))) {
				result |= MemoryValidationFlags::kScabbardDetach;
			}
		} else {
			constexpr std::uint8_t d_scbAttach[]{ 0x80, 0x7D, 0x6F, 0x00, 0x75, 0x1E };
			if (!ValidateMem(m_scbAttach_a.address(), d_scbAttach, sizeof(d_scbAttach))) {
				result |= MemoryValidationFlags::kScabbardAttach;
			}

			constexpr std::uint8_t d_scbDetach[]{ 0x0F, 0x84, 0xB3, 0x00, 0x00, 0x00 };
			if (!ValidateMem(m_scbDetach_a.address(), d_scbDetach, sizeof(d_scbDetach))) {
				result |= MemoryValidationFlags::kScabbardDetach;
			}
		}

		constexpr std::uint8_t d_removeScb[]{ 0x48, 0x89, 0x5C, 0x24, 0x08 };
		if (!ValidateMem(m_removeWeaponScabbard_a.address(), d_removeScb, sizeof(d_removeScb))) {
			result |= MemoryValidationFlags::kRemoveWeaponScabbard;
		}

		return static_cast<MemoryValidationFlags>(result.underlying());
	}

	void EngineExtensions::Patch_SCB_Attach()
	{
		struct Assembly : Xbyak::CodeGenerator
		{
			Assembly(std::uintptr_t targetAddr)
			{
				Xbyak::Label callLabel;
				Xbyak::Label exitContinue;
				Xbyak::Label exitSkip;

				Xbyak::Label cont;
				Xbyak::Label skip;

				cmp(byte[rbp + 0x6F], 0);
				je(cont);  // skip if not left

				mov(rcx, ptr[rbp + 0x77]);
				mov(rcx, ptr[rcx]);        // Biped
				mov(r8, ptr[rbp + 0x5F]);  // root

				if (IsAE()) {
					mov(edx, r15d);  // slot

					call(ptr[rip + callLabel]);

					test(rax, rax);
					je(skip);
					mov(rsi, rax);  // rsi: not used after scb is attached
				} else {
					mov(edx, r14d);  // slot

					push(rax);
					sub(rsp, 0x28);

					call(ptr[rip + callLabel]);
					mov(rdx, rax);

					add(rsp, 0x28);
					pop(rax);

					test(rdx, rdx);
					je(skip);
					mov(rsi, rdx);  // rsi: not used after scb is attached
				}

				L(cont);
				jmp(ptr[rip + exitContinue]);

				L(skip);
				jmp(ptr[rip + exitSkip]);

				L(exitContinue);
				dq(targetAddr + 0x6);

				L(exitSkip);
				dq(targetAddr + 0x24);

				L(callLabel);
				dq(reinterpret_cast<std::uintptr_t>(&GetScbAttachmentNode_Hook));
			}
		};

		SKSE::log::info("Patching SCB attach");
		Assembly code(m_scbAttach_a.address());
		SKSE::GetTrampoline().write_branch<6>(m_scbAttach_a.address(), reinterpret_cast<std::uintptr_t>(PlaceCode(code)));
	}

	void EngineExtensions::Patch_SCB_Detach()
	{
		struct Assembly : Xbyak::CodeGenerator
		{
			Assembly(std::uintptr_t targetAddr)
			{
				Xbyak::Label callLabel;
				Xbyak::Label exitIsWeapon;
				Xbyak::Label exitIsShield;

				Xbyak::Label isWeaponSlot;
				Xbyak::Label ok;

				jne(isWeaponSlot);

				mov(rcx, ptr[rdi]);  // form
				mov(rdx, r13);       // Biped
				call(ptr[rip + callLabel]);
				test(rax, rax);
				jne(ok);
				jmp(ptr[rip + exitIsShield]);

				L(ok);
				mov(r12, rax);

				L(isWeaponSlot);
				jmp(ptr[rip + exitIsWeapon]);

				L(exitIsShield);
				dq(targetAddr + (IsAE() ? 0xB7 : 0xB9));

				L(exitIsWeapon);
				dq(targetAddr + 0x6);

				L(callLabel);
				dq(reinterpret_cast<std::uintptr_t>(&GetScbAttachmentNode_Cleanup_Hook));
			}
		};

		SKSE::log::info("Patching SCB detach");
		Assembly code(m_scbDetach_a.address());
		SKSE::GetTrampoline().write_branch<6>(m_scbDetach_a.address(), reinterpret_cast<std::uintptr_t>(PlaceCode(code)));
	}

	void EngineExtensions::Patch_SCB_Get()
	{
		struct Assembly : Xbyak::CodeGenerator
		{
			Assembly(std::uintptr_t targetAddr)
			{
				Xbyak::Label retnLabel;
				Xbyak::Label callLabel;

				if (IsAE()) {
					mov(r8d, 0x1);
					xor_(r9d, r9d);
					mov(rax, ptr[rbp - 0x41]);
					cmp(ptr[rbp - 0x39], rax);
					cmove(r9d, r8d);  // is 1p
				} else {
					mov(r9b, byte[rbp - 0x51]);  // is 1p
				}

				mov(r8b, byte[rbp + 0x6F]);  // is left weapon

				call(ptr[rip + callLabel]);

				jmp(ptr[rip + retnLabel]);

				L(retnLabel);
				dq(targetAddr + 0x5);

				L(callLabel);
				dq(reinterpret_cast<std::uintptr_t>(&GetScabbardNode_Hook));
			}
		};

		SKSE::log::info("Patching SCB get");
		Assembly code(m_scbGet_a.address());
		SKSE::GetTrampoline().write_branch<5>(m_scbGet_a.address(), reinterpret_cast<std::uintptr_t>(PlaceCode(code)));
	}

	void EngineExtensions::Patch_RemoveWeaponScabbard()
	{
		struct Assembly : Xbyak::CodeGenerator
		{
			Assembly()
			{
				Xbyak::Label rplLabel;

				jmp(ptr[rip + rplLabel]);

				L(rplLabel);
				dq(reinterpret_cast<std::uintptr_t>(&RemoveWeaponScabbard_Rpl));
			}
		};

		SKSE::log::info("Patching RemoveWeaponScabbard");
		Assembly code;
		SKSE::GetTrampoline().write_branch<5>(m_removeWeaponScabbard_a.address(), reinterpret_cast<std::uintptr_t>(PlaceCode(code)));
	}

	// right staff and shield slot (weapon)
	void EngineExtensions::Patch_WeaponObjects_Attach()
	{
		struct Assembly : Xbyak::CodeGenerator
		{
			Assembly(
				std::uintptr_t a_targetAddr,
				std::uintptr_t a_callAddr,
				std::uintptr_t a_retnNoHiddenOffset)
			{
				Xbyak::Label callLabel;
				Xbyak::Label retnLabel;
				Xbyak::Label retnNoHiddenLabel;

				Xbyak::Label skipHide;

				sub(rsp, 0x40);

				if (IsAE()) {
					mov(r9d, 0x1);
					xor_(r8d, r8d);
					mov(rax, ptr[rbp - 0x41]);
					cmp(ptr[rbp - 0x39], rax);
					cmove(r8d, r9d);
					mov(byte[rsp + 0x20], r8b);  // is 1p

					mov(r9d, r15d);  // slot
				} else {
					mov(al, byte[rbp - 0x51]);
					mov(byte[rsp + 0x20], al);  // is 1p

					mov(r9d, r14d);  // slot
				}

				mov(r8, ptr[rbp + 0x77]);
				mov(r8, ptr[r8]);  // Biped

				lea(rax, ptr[rsp + 0x30]);
				mov(ptr[rsp + 0x28], rax);  // skip hide bool

				call(ptr[rip + callLabel]);
				mov(dl, byte[rsp + 0x30]);

				add(rsp, 0x40);

				test(dl, dl);
				jne(skipHide);

				jmp(ptr[rip + retnLabel]);

				L(skipHide);

				mov(rsi, rax);  // store attachment node

				jmp(ptr[rip + retnNoHiddenLabel]);

				L(retnLabel);
				dq(a_targetAddr + 0x5);

				L(retnNoHiddenLabel);
				dq(a_targetAddr + a_retnNoHiddenOffset);

				L(callLabel);
				dq(a_callAddr);
			}
		};

		SKSE::log::info("Patching weapon attach");
		{
			Assembly code(
				m_getShieldWeaponSlotNode_a.address(),
				reinterpret_cast<std::uintptr_t>(&GetWeaponShieldSlotNode_Hook),
				IsAE() ? 0x28 : 0x26);
			SKSE::GetTrampoline().write_branch<5>(m_getShieldWeaponSlotNode_a.address(), reinterpret_cast<std::uintptr_t>(PlaceCode(code)));
		}

		{
			Assembly code(
				m_getStaffSlotNode_a.address(),
				reinterpret_cast<std::uintptr_t>(&GetWeaponStaffSlotNode_Hook),
				IsAE() ? 0x24 : 0x26);
			SKSE::GetTrampoline().write_branch<5>(m_getStaffSlotNode_a.address(), reinterpret_cast<std::uintptr_t>(PlaceCode(code)));
		}
	}

	void EngineExtensions::Patch_ObjectAttachDefault()
	{
		struct Assembly : Xbyak::CodeGenerator
		{
			Assembly(
				std::uintptr_t a_targetAddr)
			{
				Xbyak::Label callLabel;
				Xbyak::Label retnLabel;

				sub(rsp, 0x30);

				if (IsAE()) {
					mov(r9d, 0x1);
					xor_(r8d, r8d);
					mov(rax, ptr[rbp - 0x41]);
					cmp(ptr[rbp - 0x39], rax);
					cmove(r8d, r9d);
					mov(byte[rsp + 0x20], r8b);  // is 1p

					mov(r9d, r15d);  // slot
				} else {
					mov(al, byte[rbp - 0x51]);
					mov(byte[rsp + 0x20], al);  // is 1p

					mov(r9d, r14d);  // slot
				}

				mov(r8, ptr[rbp + 0x77]);
				mov(r8, ptr[r8]);  // Biped

				call(ptr[rip + callLabel]);

				add(rsp, 0x30);

				jmp(ptr[rip + retnLabel]);

				L(retnLabel);
				dq(a_targetAddr + 0x5);

				L(callLabel);
				dq(reinterpret_cast<std::uintptr_t>(&GetSlotNodeDefault_Hook));
			}
		};

		SKSE::log::info("Patching default attach");
		Assembly code(m_getShieldArmorSlotNode_a.address());
		SKSE::GetTrampoline().write_branch<5>(m_getShieldArmorSlotNode_a.address(), reinterpret_cast<std::uintptr_t>(PlaceCode(code)));
	}

	void EngineExtensions::Patch_DisableShieldHideOnSit()
	{
		struct Assembly : Xbyak::CodeGenerator
		{
			Assembly(
				std::uintptr_t a_targetAddr)
			{
				Xbyak::Label exitContinue;
				Xbyak::Label exitSkip;
				Xbyak::Label callLabel;

				Xbyak::Label skip;
				Xbyak::Label cont;

				test(r8b, r8b);  // bool: true = hide, false = show
				je(cont);

				push(rcx);
				push(rdx);
				push(r8);
				sub(rsp, 0x28);

				call(ptr[rip + callLabel]);

				add(rsp, 0x28);
				pop(r8);
				pop(rdx);
				pop(rcx);

				test(al, al);
				jne(skip);

				L(cont);
				mov(r9, ptr[rdx]);  // Biped
				if (IsAE()) {
					test(r9, r9);
				} else {
					mov(rax, ptr[rcx + 0x1F0]);  // TESRace
				}
				jmp(ptr[rip + exitContinue]);

				L(skip);
				jmp(ptr[rip + exitSkip]);

				L(exitContinue);
				dq(a_targetAddr + (IsAE() ? 0x6 : 0xA));

				L(exitSkip);
				dq(a_targetAddr + (IsAE() ? 0x57 : 0x53));

				L(callLabel);
				dq(reinterpret_cast<std::uintptr_t>(&ShouldBlockShieldHide));
			}
		};

		SKSE::log::info("Patching shield hide on sit");
		Assembly code(m_hideShield_a.address());
		SKSE::GetTrampoline().write_branch<6>(m_hideShield_a.address(), reinterpret_cast<std::uintptr_t>(PlaceCode(code)));
	}

	bool EngineExtensions::Hook_TESObjectWEAP_SetEquipSlot()
	{
		REL::Relocation<std::uintptr_t> vtbl{ TESObjectWEAP::VTABLE[0] };

		m_TESObjectWEAP_SetEquipSlot_o = reinterpret_cast<decltype(m_TESObjectWEAP_SetEquipSlot_o)>(
			vtbl.write_vfunc(0x8B, &TESObjectWEAP_SetEquipSlot_Hook));

		if (m_TESObjectWEAP_SetEquipSlot_o) {
			SKSE::log::info("Hooked TESObjectWEAP::SetEquipSlot");
			return true;
		}

		SKSE::log::error("TESObjectWEAP::SetEquipSlot hook failed");
		return false;
	}

	bool EngineExtensions::Patch_ShieldHandWorkaround()
	{
		const auto getCallDst = [](std::uintptr_t a_addr, const char* a_what) -> std::uintptr_t {
			if (*reinterpret_cast<const std::uint8_t*>(a_addr) != 0xE8) {
				SKSE::log::error("Shield hand patch ({}): no call at {:X}", a_what, a_addr);
				return 0;
			}
			return GetCallDst5(a_addr);
		};

		const auto dst1 = getCallDst(m_unk140609D50_BShkbAnimationGraph_SetGraphVariableInt_a.address(), "k1");
		const auto dst2 = getCallDst(m_unk1406097C0_IAnimationGraphManagerHolder_SetVariableOnGraphsInt_a.address(), "k2");
		const auto k3addr = GetUnk140634D20Addr();
		const auto dst3 = getCallDst(k3addr, "k3");

		if (!dst1 || !dst2 || !dst3) {
			return false;
		}

		m_BShkbAnimationGraph_SetGraphVariableInt_o = reinterpret_cast<BShkbAnimationGraph_SetGraphVariableInt_t>(dst1);
		m_unk1406097C0_IAnimationGraphManagerHolder_SetVariableOnGraphsInt_o = reinterpret_cast<IAnimationGraphManagerHolder_SetVariableOnGraphsInt_t>(dst2);
		m_unk140634D20_IAnimationGraphManagerHolder_SetVariableOnGraphsInt_o = reinterpret_cast<IAnimationGraphManagerHolder_SetVariableOnGraphsInt_t>(dst3);

		enum class HookTarget
		{
			k1,
			k2,
			k3
		};

		struct Assembly : Xbyak::CodeGenerator
		{
			Assembly(HookTarget a_target, std::uintptr_t a_k3addr)
			{
				Xbyak::Label retnLabel;
				Xbyak::Label callLabel;

				std::uintptr_t targetAddr;
				std::uintptr_t callAddr;

				switch (a_target) {
				case HookTarget::k1:
					targetAddr = m_unk140609D50_BShkbAnimationGraph_SetGraphVariableInt_a.address();
					callAddr   = reinterpret_cast<std::uintptr_t>(&Unk140609D50_BShkbAnimationGraph_SetGraphVariableInt_Hook);
					lea(r9, ptr[rbp - 0x38]);
					break;
				case HookTarget::k2:
					targetAddr = m_unk1406097C0_IAnimationGraphManagerHolder_SetVariableOnGraphsInt_a.address();
					callAddr   = reinterpret_cast<std::uintptr_t>(&Unk1406097C0_IAnimationGraphManagerHolder_SetVariableOnGraphsInt_Hook);
					mov(r9, rsi);
					break;
				case HookTarget::k3:
					targetAddr = a_k3addr;
					callAddr   = reinterpret_cast<std::uintptr_t>(&Unk140634D20_IAnimationGraphManagerHolder_SetVariableOnGraphsInt_Hook);
					lea(r9, ptr[r15 - (IsG629() ? 0xC0 : 0xB8)]);
					break;
				default:
					throw std::exception();
				}

				call(ptr[rip + callLabel]);
				jmp(ptr[rip + retnLabel]);

				L(retnLabel);
				dq(targetAddr + 0x5);

				L(callLabel);
				dq(callAddr);
			}
		};

		SKSE::log::info("Patching shield hand workaround k1");
		{
			Assembly code(HookTarget::k1, 0);
			SKSE::GetTrampoline().write_branch<5>(
				m_unk140609D50_BShkbAnimationGraph_SetGraphVariableInt_a.address(),
				reinterpret_cast<std::uintptr_t>(PlaceCode(code)));
		}

		SKSE::log::info("Patching shield hand workaround k2");
		{
			Assembly code(HookTarget::k2, 0);
			SKSE::GetTrampoline().write_branch<5>(
				m_unk1406097C0_IAnimationGraphManagerHolder_SetVariableOnGraphsInt_a.address(),
				reinterpret_cast<std::uintptr_t>(PlaceCode(code)));
		}

		SKSE::log::info("Patching shield hand workaround k3");
		{
			Assembly code(HookTarget::k3, k3addr);
			SKSE::GetTrampoline().write_branch<5>(
				k3addr,
				reinterpret_cast<std::uintptr_t>(PlaceCode(code)));
		}

		return true;
	}

	void EngineExtensions::Patch_WeapTypeToNodeArrayInit()
	{
		const auto func = [&](std::uintptr_t a_offset, std::uintptr_t a_callAddr) {
			struct Assembly : Xbyak::CodeGenerator
			{
				Assembly(
					std::uintptr_t a_targetAddr,
					std::uintptr_t a_callAddr)
				{
					Xbyak::Label callLabel;
					Xbyak::Label retnLabel;

					call(ptr[rip + callLabel]);
					mov(rdx, rax);
					jmp(ptr[rip + retnLabel]);

					L(callLabel);
					dq(a_callAddr);

					L(retnLabel);
					dq(a_targetAddr + 0xC);
				}
			};

			SKSE::log::info("Patching weapon node array init at {:X}", m_initWeapTypeToNodeArray_a.address() + a_offset);

			const auto addr = m_initWeapTypeToNodeArray_a.address() + a_offset;

			Assembly code(addr, a_callAddr);
			SKSE::GetTrampoline().write_branch<5>(addr, reinterpret_cast<std::uintptr_t>(PlaceCode(code)));
		};

		func(IsAE() ? 0xBE : 0xB1, reinterpret_cast<std::uintptr_t>(&WeapTypeToNodeInit1_Hook));
		func(IsAE() ? 0x106 : 0xF9, reinterpret_cast<std::uintptr_t>(&WeapTypeToNodeInit2_Hook));
	}

	bool EngineExtensions::Unk140609D50_BShkbAnimationGraph_SetGraphVariableInt_Hook(
		BShkbAnimationGraph* a_graph,
		const BSFixedString& a_name,
		std::int32_t         a_value,
		Actor*               a_actor)
	{
		auto controller = m_Instance->m_controller.get();

		if (a_value == 10 &&
		    a_actor &&
		    controller->IsShieldEnabled(a_actor) &&
		    controller->GetShieldOnBackSwitch(a_actor) &&
		    (controller->GetConfig().m_shwForceIfDrawn ||
		     !a_actor->IsWeaponDrawn()) &&
		    Common::IsShieldEquipped(a_actor)) {
			a_value = 0;
		}

		return m_Instance->m_BShkbAnimationGraph_SetGraphVariableInt_o(a_graph, a_name, a_value);
	}

	bool EngineExtensions::Unk1406097C0_IAnimationGraphManagerHolder_SetVariableOnGraphsInt_Hook(
		RE::IAnimationGraphManagerHolder* a_holder,
		const BSFixedString&              a_name,
		std::int32_t                      a_value,
		Actor*                            a_actor)
	{
		auto controller = m_Instance->m_controller.get();

		if (a_value == 10 &&
		    a_actor &&
		    controller->IsShieldEnabled(a_actor) &&
		    controller->GetShieldOnBackSwitch(a_actor) &&
		    (controller->GetConfig().m_shwForceIfDrawn ||
		     !a_actor->IsWeaponDrawn()) &&
		    Common::IsShieldEquipped(a_actor)) {
			a_value = 0;
		}

		return m_Instance->m_unk1406097C0_IAnimationGraphManagerHolder_SetVariableOnGraphsInt_o(a_holder, a_name, a_value);
	}

	bool EngineExtensions::Unk140634D20_IAnimationGraphManagerHolder_SetVariableOnGraphsInt_Hook(
		RE::IAnimationGraphManagerHolder* a_holder,
		const BSFixedString&              a_name,
		std::int32_t                      a_value,
		Actor*                            a_actor)
	{
		auto controller = m_Instance->m_controller.get();

		if ((a_value == 0 || a_value == 10) &&
		    a_actor &&
		    controller->IsShieldEnabled(a_actor) &&
		    controller->GetShieldOnBackSwitch(a_actor) &&
		    Common::IsShieldEquipped(a_actor)) {
			a_holder->SetGraphVariableInt(
				controller->GetStringHolder()->m_iLeftHandType,
				a_value);
		}

		return m_Instance->m_unk140634D20_IAnimationGraphManagerHolder_SetVariableOnGraphsInt_o(a_holder, a_name, a_value);
	}

	const BSFixedString& EngineExtensions::WeapTypeToNodeInit1_Hook()
	{
		return m_Instance->m_controller->GetStringHolder()->m_weaponBackAxeMace;
	}

	const BSFixedString& EngineExtensions::WeapTypeToNodeInit2_Hook()
	{
		return m_Instance->m_controller->GetStringHolder()->m_weaponCrossbow;
	}

	static bool RemoveWeaponScabbardImpl(NiNode* a_node, const char* a_scbNodeName)
	{
		const BSFixedString scbNodeName(a_scbNodeName);

		for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(a_node->children.size()); i++) {
			const auto& e = a_node->children[i];
			if (e) {
				if (const auto p = e->name.c_str()) {
					if (::_stricmp(p, a_scbNodeName) == 0) {
						a_node->DetachChildAt2(i);
						return true;
					}
					if (::_strnicmp(p, "FadeNode ", 9) == 0) {
						if (const auto n = e->AsNode()) {
							return RemoveWeaponScabbardImpl(n, a_scbNodeName);
						}
						break;
					}
				}
			}
		}

		return false;
	}

	bool EngineExtensions::RemoveWeaponScabbard_Rpl(NiNode* a_node)
	{
		if (a_node) {
			RemoveWeaponScabbardImpl(a_node, "Scb");
			RemoveWeaponScabbardImpl(a_node, "ScbLeft");
		}

		return true;
	}

	void EngineExtensions::TESObjectWEAP_SetEquipSlot_Hook(BGSEquipType* a_this, BGSEquipSlot* a_slot)
	{
		// this just writes BGSEquipSlot* @rcx+8, but we call here on the off chance something else hooked it
		m_Instance->m_TESObjectWEAP_SetEquipSlot_o(a_this, a_slot);

		Events::OnSetEquipSlot evn{};
		m_Instance->m_dispatchers.m_setEquipSlot.SendEvent(evn);
	}

	NiNode* EngineExtensions::GetScbAttachmentNode_Hook(
		BipedAnim*   a_biped,
		BIPED_OBJECT a_bipedSlot,
		NiNode*      a_root)
	{
		// these checks should never fail
		if (!a_root || !a_biped || a_bipedSlot >= BIPED_OBJECT::kTotal) {
			return nullptr;
		}

		NiPointer<TESObjectREFR> ref = a_biped->actorRef.get();
		if (!ref) {
			return nullptr;
		}

		auto actor = ref->As<Actor>();
		if (!actor) {
			return nullptr;
		}

		auto form = a_biped->objects[std::to_underlying(a_bipedSlot)].item;
		if (!form) {
			return nullptr;
		}

		auto weapon = form->As<TESObjectWEAP>();
		if (!weapon) {
			return nullptr;
		}

		return m_Instance->m_controller->GetScbAttachmentNode(
			actor,
			weapon,
			a_root,
			false  // this hook won't run for 1p
		);
	}

	NiNode* EngineExtensions::GetScbAttachmentNode_Cleanup_Hook(
		TESForm*   a_form,
		BipedAnim* a_biped)
	{
		if (!a_form || !a_biped) {
			return nullptr;
		}

		auto weapon = a_form->As<TESObjectWEAP>();
		if (!weapon) {
			return nullptr;
		}

		auto root = a_biped->root;
		if (!root) {
			return nullptr;
		}

		auto name = m_Instance->m_controller->GetScbAttachmentNodeName(
			root,
			weapon);

		if (!name) {
			return nullptr;
		}

		auto object = NodeUtils::GetNodeByName(root, *name);
		if (!object) {
			return nullptr;
		}

		return object->AsNode();
	}

	NiAVObject* EngineExtensions::GetWeaponShieldSlotNode_Hook(
		NiNode*              a_root,
		const BSFixedString& a_nodeName,
		BipedAnim*           a_biped,
		BIPED_OBJECT         a_bipedSlot,
		bool                 a_is1p,
		bool&                a_skipHide)
	{
		const auto str = m_Instance->GetWeaponAttachmentNodeName(
			a_biped,
			a_bipedSlot,
			a_is1p,
			true);

		if (str) {
			if (auto result = NodeUtils::GetNodeByName(a_root, *str)) {
				a_skipHide = true;

				return result;
			}
		}

		a_skipHide = false;

		return NodeUtils::GetNodeByName(a_root, a_nodeName);
	}

	NiAVObject* EngineExtensions::GetWeaponStaffSlotNode_Hook(
		NiNode*              a_root,
		const BSFixedString& a_nodeName,
		BipedAnim*           a_biped,
		BIPED_OBJECT         a_bipedSlot,
		bool                 a_is1p,
		bool&                a_skipHide)
	{
		const auto str = m_Instance->GetWeaponAttachmentNodeName(
			a_biped,
			a_bipedSlot,
			a_is1p,
			false);

		if (str) {
			if (auto result = NodeUtils::GetNodeByName(a_root, *str)) {
				a_skipHide = true;

				return result;
			}
		}

		a_skipHide = false;

		return NodeUtils::GetNodeByName(a_root, a_nodeName);
	}

	NiAVObject* EngineExtensions::GetSlotNodeDefault_Hook(
		NiNode*              a_root,
		const BSFixedString& a_nodeName,
		BipedAnim*           a_biped,
		BIPED_OBJECT         a_bipedSlot,
		bool                 a_is1p)
	{
		if (a_biped && a_bipedSlot < BIPED_OBJECT::kTotal) {
			NiPointer<TESObjectREFR> ref = a_biped->actorRef.get();
			if (ref) {
				if (const auto actor = ref->As<Actor>()) {
					if (const auto form = a_biped->objects[std::to_underlying(a_bipedSlot)].item) {
						if (Controller::GetShieldBipedObject(actor) == a_bipedSlot) {
							if (const auto armor = form->As<TESObjectARMO>()) {
								if (const auto str = m_Instance->m_controller->GetShieldAttachmentNodeName(actor, armor, a_is1p)) {
									if (const auto result = NodeUtils::GetNodeByName(a_root, *str)) {
										return result;
									}
									// fall back to the node requested by the game if we find nothing
								}
							}
						} else if (m_Instance->m_controller->GetConfig().m_disableWeapNodeSharing) {
							switch (a_bipedSlot) {
							case BIPED_OBJECT::kTwoHandMelee:
								if (const auto weap = form->As<TESObjectWEAP>()) {
									if (weap->GetWeaponType() == WEAPON_TYPE::kTwoHandAxe) {
										const auto stringHolder = m_Instance->m_controller->GetStringHolder();

										return NodeUtils::GetNodeByName(a_root, stringHolder->m_weaponBackAxeMace);
									}
								}
								break;
							case BIPED_OBJECT::kCrossbow:
								if (const auto weap = form->As<TESObjectWEAP>()) {
									if (weap->GetWeaponType() == WEAPON_TYPE::kCrossbow) {
										const auto stringHolder = m_Instance->m_controller->GetStringHolder();

										return NodeUtils::GetNodeByName(a_root, stringHolder->m_weaponCrossbow);
									}
								}
								break;
							default:
								break;
							}
						}
					}
				}
			}
		}

		return NodeUtils::GetNodeByName(a_root, a_nodeName);
	}

	NiAVObject* EngineExtensions::GetScabbardNode_Hook(
		NiNode*              a_node,
		const BSFixedString& a_nodeName,  // Scb
		bool                 a_left,
		bool                 a_is1p)
	{
		auto stringHolder = m_Instance->m_controller->GetStringHolder();

		NiPointer<NiAVObject> scbNode{ NodeUtils::GetNodeByName(a_node, a_nodeName) };
		NiPointer<NiAVObject> scbLeftNode{ NodeUtils::GetNodeByName(a_node, stringHolder->m_scbLeft) };

		auto& config = m_Instance->m_controller->GetConfig();

		if (config.m_disableScabbards) {
			bool shrink = false;

			if (scbNode) {
				scbNode->parent->DetachChild2(scbNode.get());
				shrink = true;
			}

			if (scbLeftNode) {
				scbLeftNode->parent->DetachChild2(scbLeftNode.get());
				shrink = true;
			}

			if (shrink) {
				NodeUtils::ShrinkToSize(a_node);
			}

			return nullptr;
		}

		if (!a_left || a_is1p) {
			if (scbLeftNode && scbLeftNode->parent) {
				scbLeftNode->parent->DetachChild2(scbLeftNode.get());
				NodeUtils::ShrinkToSize(a_node);
			}

			return scbNode.get();
		}

		if (!scbLeftNode) {
			return scbNode.get();
		}

		if (scbNode && scbNode->parent) {
			scbNode->parent->DetachChild2(scbNode.get());
			NodeUtils::ShrinkToSize(a_node);
		}

		NodeUtils::SetVisible(scbLeftNode.get(), true);

		return scbLeftNode.get();
	}

	bool EngineExtensions::ShouldBlockShieldHide(
		Actor* a_actor)
	{
		return a_actor && m_Instance->m_controller->ShouldBlockShieldHide(a_actor);
	}

	const BSFixedString* EngineExtensions::GetWeaponAttachmentNodeName(
		BipedAnim*   a_biped,
		BIPED_OBJECT a_bipedSlot,
		bool         a_is1p,
		bool         a_left)
	{
		if (!a_biped || a_bipedSlot >= BIPED_OBJECT::kTotal) {
			return nullptr;
		}

		NiPointer<TESObjectREFR> ref = a_biped->actorRef.get();
		if (!ref) {
			return nullptr;
		}

		auto actor = ref->As<Actor>();
		if (!actor) {
			return nullptr;
		}

		auto form = a_biped->objects[std::to_underlying(a_bipedSlot)].item;
		if (!form) {
			return nullptr;
		}

		auto weapon = form->As<TESObjectWEAP>();
		if (!weapon) {
			return nullptr;
		}

		return m_controller->GetWeaponAttachmentNodeName(actor, weapon, a_is1p, a_left);
	}
}
