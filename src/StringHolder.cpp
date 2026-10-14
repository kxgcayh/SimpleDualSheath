#include "StringHolder.h"

namespace SDS
{
	StringHolder::StringHolder() :
		m_shield(NINODE_SHIELD),
		m_weapon(NINODE_WEAPON),
		m_npcroot(NINODE_NPCROOT),
		m_shieldSheathNode(NINODE_SHIELD_BACK),
		m_iLeftHandType(iLeftHandType),
		m_iLeftHandEquipped(iLeftHandEquipped),
		m_scbLeft(NINODE_SCB_LEFT),
		m_weaponBackAxeMace(NINODE_WEAPON_BACK_AXE_MACE),
		m_weaponCrossbow(NINODE_CROSSBOW)
	{
	}
}
