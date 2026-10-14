#pragma once

// Engine node helpers: direct-call wrappers around relocated engine
// functions plus small scene-graph utilities.
namespace SDS
{
	using namespace RE;

	class NodeUtils
	{
	public:
		using GetNodeByName_t = NiAVObject* (*)(NiNode* a_root, const BSFixedString& a_name, bool a_unk);
		using ShrinkToSize_t  = NiAVObject* (*)(NiNode* a_node);

		static NiAVObject* GetNodeByName(NiNode* a_root, const BSFixedString& a_name);

		static NiAVObject* FindChildObject(NiNode* a_node, const BSFixedString& a_name);

		static void AttachToNode(NiAVObject* a_object, NiNode* a_node);

		static void SetVisible(NiAVObject* a_object, bool a_visible);

		static NiAVObject* ShrinkToSize(NiNode* a_node);
	};

	struct NiRootNodes
	{
		NiNode* m_nodes[2]{ nullptr, nullptr };

		explicit NiRootNodes(Actor* a_actor);

		void GetNPCRoots(const BSFixedString& a_npcRootName);
	};
}
