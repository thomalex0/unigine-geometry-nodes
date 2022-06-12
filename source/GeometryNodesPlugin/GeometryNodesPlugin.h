#pragma once

#include "BlendAsset.h"
#include "Gui/MainWindow.h"

#include <editor/Plugin.h>
#include <editor/PluginInfo.h>
#include <editor/PluginManager.h>

class QAction;

namespace GeometryNodes
{
	static const char BLENDER_PATH_CFG[] = "bgn_blender_path";
	static const char BLENDER_FIND_CMD[] = "bgn_blender_locate";
	static const char TRACK_TRANSFORM_CMD[] = "bgn_track_transforms";

	class GeometryNodesPlugin final
		: public QObject
		, public ::Editor::Plugin
	{
		Q_OBJECT
			Q_PLUGIN_METADATA(IID "com.unigine.EditorPlugin" FILE "GeometryNodesPlugin.json")
			Q_INTERFACES(Editor::Plugin)

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

		QAction* action_{};
		MainWindow* window{};
		QString pluginPath_;
		QString blenderPath_;
		bool trackTransform_{ true };

		static GeometryNodesPlugin* instance_;
	};
} // namespace GeometryNodes