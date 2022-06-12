#pragma once

#include "BlenderProcess.h"

#include <UnigineComponentSystem.h>
//#include <UnigineObjects.h>
//#include <UnigineNodes.h>
//#include <UnigineHashMap.h>
//#include <UnigineThread.h>
//#include <UnigineCallback.h>
//#include <UnigineGUID.h>
//#include <UnigineFileSystem.h>
//#include <UnigineDir.h>
//#include <UnigineTimer.h>

namespace GeometryNodes
{
	class BlendAsset final : public Unigine::ComponentBase
	{
	public:
		COMPONENT_DEFINE(BlendAsset, Unigine::ComponentBase)
			COMPONENT_INIT(init)
			COMPONENT_UPDATE(update)
			COMPONENT_SHUTDOWN(shutdown)

			PROP_PARAM(File, blend_asset, "", "", "", "", "filter=.blend")
			PROP_PARAM(Switch, playback, 0, "None,Single Frame,Animation")

			PROP_PARAM(Toggle, dynamic_instances, false, "", "Enable if your instances have changing geometry (slow)", "Settings")
			PROP_PARAM(Toggle, synchronous, false, "", "Enable to wait for each geometry update (useful for videograbbing).\nDrops performance a lot (especially for animation).", "Settings")

			PROP_PARAM(Toggle, update_existing, false, "", "", "Output")
			PROP_PARAM(Node, mesh_update_object, "Mesh Node", "", "Output", "update_existing=1")
			PROP_PARAM(Switch, save_mesh, 0, "Create New Mesh Asset,Overwrite Used Mesh Asset", "", "", "Output", "update_existing=1")

		struct ClusterUpdateParams : public Unigine::ComponentStruct
		{
			PROP_PARAM(Node, node)
			PROP_PARAM(Toggle, replace_transforms, true)
			PROP_PARAM(Toggle, replace_mesh, false)
			PROP_PARAM(Switch, save_mesh, 0, "Create New Mesh Asset,Overwrite Used Mesh Asset", "", "", "", "replace_mesh=1")
		};
			PROP_ARRAY_STRUCT(ClusterUpdateParams, instance_objects, "Instances", "", "Output", "update_existing=1")

			PROP_PARAM(Toggle, clear_meshes, true, "Clear Unused Meshes", "", "Output")

		Unigine::Vector<GNParam> gnParams;

		void updateParameter(Unigine::String id, QVariant val);
		bool skipFrame() { return playback.get() != 1; }
		void shutdown();

	protected:
		void init();
		void update();

	private:
		BlenderProcess *process{};
		Unigine::ObjectMeshStaticPtr mesh;
		Unigine::HashMap<long long, Unigine::ObjectMeshClusterPtr> instances;
		Unigine::Vector<Unigine::UGUID> temporaryMeshes;

		int frame{ 1 };
		int frame_start{ 1 };
		int frame_end{ 250 };

		void cleanup(bool full = true);
		void updateGeometry(GNData &data);
		void animate();
		Unigine::String getLocalPath(Unigine::String sub = "");
		Unigine::Vector<long long> getKnownInstances() { return dynamic_instances.get() ? Unigine::Vector<long long>() : instances.keys(); }
		Unigine::Vector<GNParam> getValidParameters();

		void clearTemporaryMeshes();
		void saveTemporaryMeshes();

		void subscribeToNodeChanges(int nodeid, int param);
		void unsubscribeFromNodeChanges(int param);

		void onPropertyParameterChanged(Unigine::PropertyPtr p, int n);
		void onAssetChanged();
		void onPlaybackChanged();

		int findParameter(Unigine::String id);
		int findParameterByType(GNParam::Type type);
		bool deserializeParameters();
		void serializeParameters();

		void saveData(const char* key, QJsonArray subtree);
		QJsonArray getSavedData(const char* key);
	};

} // namespace GeometryNodes