#include "NodeUtils.h"

namespace SDS
{
	namespace
	{
		REL::Relocation<NodeUtils::GetNodeByName_t> g_getNodeByName{ REL::RelocationID(74481, 76207) };
		REL::Relocation<NodeUtils::ShrinkToSize_t>  g_shrinkToSize{ REL::RelocationID(15571, 15748) };
	}

	NiAVObject* NodeUtils::GetNodeByName(NiNode* a_root, const BSFixedString& a_name)
	{
		if (!a_root) {
			return nullptr;
		}
		return g_getNodeByName(a_root, a_name, true);
	}

	NiAVObject* NodeUtils::FindChildObject(NiNode* a_node, const BSFixedString& a_name)
	{
		if (!a_node) {
			return nullptr;
		}
		for (const auto& child : a_node->children) {
			if (!child) {
				continue;
			}
			if (_stricmp(child->name.c_str(), a_name.c_str()) == 0) {
				return child.get();
			}
			if (const auto node = child->AsNode()) {
				if (const auto found = FindChildObject(node, a_name)) {
					return found;
				}
			}
		}
		return nullptr;
	}

	void NodeUtils::AttachToNode(NiAVObject* a_object, NiNode* a_node)
	{
		if (a_object && a_node && a_object->parent != a_node) {
			a_node->AttachChild(a_object, true);
		}
	}

	void NodeUtils::SetVisible(NiAVObject* a_object, bool a_visible)
	{
		if (a_object) {
			a_object->SetAppCulled(!a_visible);
		}
	}

	NiAVObject* NodeUtils::ShrinkToSize(NiNode* a_node)
	{
		if (!a_node) {
			return nullptr;
		}
		return g_shrinkToSize(a_node);
	}

	NiRootNodes::NiRootNodes(Actor* a_actor)
	{
		if (!a_actor) {
			return;
		}
		if (const auto& biped = a_actor->GetBiped1(false); biped) {
			m_nodes[0] = biped->root;
		}
		if (const auto& biped = a_actor->GetBiped1(true); biped) {
			m_nodes[1] = biped->root;
		}
	}

	void NiRootNodes::GetNPCRoots(const BSFixedString& a_npcRootName)
	{
		for (auto& root : m_nodes) {
			if (!root) {
				continue;
			}
			if (const auto obj = root->GetObjectByName(a_npcRootName)) {
				if (const auto node = obj->AsNode()) {
					root = node;
				}
			}
		}
	}
}
