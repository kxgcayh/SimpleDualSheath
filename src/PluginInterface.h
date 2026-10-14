#pragma once

#include "Events.h"

namespace SDS
{
	using namespace RE;

	class Controller;

	// Local mirror of the legacy native interface other SKSE plugins consume
	// through the SKMP_GetPluginInterface export. Layout: five identifying
	// virtuals followed by the three feature virtuals below. Deliberately no
	// virtual destructor: the legacy interface is plain virtuals only, and an
	// extra slot 0 entry would shift the whole table for existing consumers.
	class PluginInterfaceBase
	{
	public:
		virtual std::uint32_t GetPluginVersion() const    = 0;
		virtual const char*   GetPluginName() const       = 0;
		virtual std::uint32_t GetInterfaceVersion() const = 0;
		virtual const char*   GetInterfaceName() const    = 0;
		virtual std::uint64_t GetUniqueID() const         = 0;
	};

	class PluginInterface :
		public PluginInterfaceBase
	{
	public:
		static constexpr std::uint32_t VERSION_MAJOR    = 1;
		static constexpr std::uint32_t VERSION_MINOR    = 6;
		static constexpr std::uint32_t VERSION_REVISION = 0;

		static constexpr std::uint32_t VERSION_PACKED =
			((VERSION_MAJOR & 0xFF) << 16) | ((VERSION_MINOR & 0xFF) << 8) | (VERSION_REVISION & 0xFF);

		// FNV-1a 64 of "SlavicPotato_SimpleDualSheath" (matches the legacy build).
		static constexpr std::uint64_t UNIQUE_ID = 0x3180B30EFCC0DB62;

		PluginInterface(const std::shared_ptr<Controller>& a_controller);

		std::uint32_t GetPluginVersion() const override;
		const char*   GetPluginName() const override;
		std::uint32_t GetInterfaceVersion() const override;
		const char*   GetInterfaceName() const override;
		std::uint64_t GetUniqueID() const override;

		virtual bool GetShieldOnBackEnabled(Actor* a_actor) const;
		virtual void RegisterForPlayerShieldOnBackEvent(Events::EventSink<SDSPlayerShieldOnBackSwitchEvent>* a_sink);
		virtual bool IsWeaponNodeSharingDisabled() const;

	private:
		const std::shared_ptr<Controller> m_controller;
	};
}
