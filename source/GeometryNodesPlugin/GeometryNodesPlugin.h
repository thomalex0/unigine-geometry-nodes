#pragma once

#include "BlendAsset.h"
#include "Gui/MainWindow.h"
#include "utils/CallbackGuard.h"

#include <UnigineVector.h>

#include <editor/UniginePlugin.h>
#include <editor/UniginePluginInfo.h>
#include <editor/UniginePluginManager.h>

class QAction;

namespace GeometryNodes
{
	static const char BLENDER_PATH_CFG[] = "bgn_blender_path";
	static const char BLENDER_FIND_CMD[] = "bgn_blender_locate";
	static const char TRACK_TRANSFORM_CMD[] = "bgn_track_transforms";

	class GeometryNodesPlugin final
		: public QObject
		, public ::UnigineEditor::Plugin
	{
		Q_OBJECT
			Q_PLUGIN_METADATA(IID "com.unigine.EditorPlugin" FILE "GeometryNodesPlugin.json")
			Q_INTERFACES(UnigineEditor::Plugin)

	public:

		static GeometryNodesPlugin* get();
		QString getPythonPath();
		QString getBlenderPath() { return blenderPath_; }
		bool isTrackTransform() { return trackTransform_; }

		bool init() override;
		void shutdown() override;

		void paramsUpdated(BlendAsset* comp);
		void meshUpdated();

	private:
		void setup_config();
		void create_window();
		void destroy_window();
		void onAssetChanged(const char *path);

		QAction* action_{};
		MainWindow* window{};
		QString pluginPath_;
		QString blenderPath_;
		bool trackTransform_{ true };

		Unigine::Vector<CallbackGuardUPtr> callbacks_;
		static GeometryNodesPlugin* instance_;
	};
} // namespace GeometryNodes