#include "GeometryNodesPlugin.h"

#include <UnigineWorld.h>
#include <UnigineConsole.h>

//#include <editor/Actions.h>
#include <editor/UnigineConstants.h>
//#include <editor/Selection.h>
//#include <editor/Selector.h>
#include <editor/UnigineAssetManager.h>
#include <editor/UnigineWindowManager.h>
//#include <editor/PluginManager.h>
#include <editor/UnigineSettingManager.h>
#include <editor/UnigineUndo.h>
//
#include <QMenu>
//#include <QCoreApplication>
//#include <QVector>
#include <QDir>
//#include <QFileInfo>
#include <QFileDialog>

namespace GeometryNodes
{
	GeometryNodesPlugin* GeometryNodesPlugin::instance_;

	GeometryNodesPlugin* GeometryNodesPlugin::get()
	{
		return instance_;
	}

	QString GeometryNodesPlugin::getPythonPath()
	{
		QDir dir = QFileInfo(pluginPath_).dir();
		auto pypath = dir.absolutePath() + "/geonodes.py";
		return pypath;
		//return tr("D:/Unigine projects/BGN/source/GeometryNodesPlugin/geonodes.py");
	}

	bool GeometryNodesPlugin::init()
	{
		using std::make_unique;
		//using GeometryNodes::CallbackGuard;

		instance_ = this;

		setup_config();

		using UnigineEditor::WindowManager;
		QMenu* menu = WindowManager::findMenu(UnigineEditor::Constants::MM_TOOLS);
		action_ = menu->addAction("Geometry Nodes Plugin", this,
			&GeometryNodesPlugin::create_window);

		connect(WindowManager::instance(), &WindowManager::windowHidden, this, [this](QWidget* w) {
			if (w == window)
				destroy_window();
		});

		//connect(UnigineEditor::AssetManager::instance(), &UnigineEditor::AssetManager::changed, this, &GeometryNodesPlugin::onAssetChanged);
		callbacks_.push_back(make_unique<CallbackGuard>(
			UnigineEditor::AssetManager::addAssetChangedCallback(Unigine::MakeCallback(this, &GeometryNodesPlugin::onAssetChanged)),
				[](void* p) { UnigineEditor::AssetManager::removeAssetChangedCallback(p); }));
		Unigine::ComponentSystem::get()->initialize();

		return true;
	}

	void GeometryNodesPlugin::shutdown()
	{

		//Editor::Undo::reset();
		// manually shutting down all components
		// strange but otherwise callbacks aren't cleared and editor crashes on exit
		// it may make sense to create a registry for all callback subscriptions for all components
		Unigine::Vector<Unigine::NodePtr> nodes;
		Unigine::World::getNodes(nodes);
		for (auto& n : nodes)
		{
			Unigine::Vector<BlendAsset*> comps;
			Unigine::ComponentSystem::get()->getComponents<BlendAsset>(n, comps);
			for (auto c : comps)
			{
				c->shutdown();
			}
		}

		callbacks_.clear();

		Unigine::Console::removeCommand(BLENDER_PATH_CFG);
		Unigine::Console::removeCommand(BLENDER_FIND_CMD);
		Unigine::Console::removeCommand(TRACK_TRANSFORM_CMD);

		disconnect(UnigineEditor::WindowManager::instance(), nullptr, this, nullptr);

		delete action_;
		action_ = nullptr;

		destroy_window();
		delete instance_;
		instance_ = nullptr;

		//Editor::Undo::reset();
	}

	void GeometryNodesPlugin::setup_config()
	{
		auto plugins = UnigineEditor::PluginManager::plugins();
		for (auto info : plugins)
		{
			if (strcmp(info->name(), "GeometryNodesPlugin") == 0)
			{
				pluginPath_ = info->absoluteFilePath();
			}
		}

		using UnigineEditor::SettingManager;
		if (!SettingManager::userSettings()->contains(BLENDER_PATH_CFG))
		{
			const char* path = "blender";
			QString versions[] = { "3.3", "3.2", "3.1", "3.0" };
			for (auto& v : versions)
			{
				auto bp = tr("C:\\Program Files\\Blender Foundation\\Blender %1\\blender.exe").arg(v);
				if (QFileInfo::exists(bp))
				{
					path = bp.toUtf8().constData();
					break;
				}
			}
			SettingManager::userSettings()->setString(BLENDER_PATH_CFG, path);
			blenderPath_ = tr(path);
		}
		else
		{
			blenderPath_ = SettingManager::userSettings()->getString(BLENDER_PATH_CFG);
		}

		Unigine::Console::addCommand(BLENDER_PATH_CFG, "Path to Blender executable (or just 'blender' if it's in your PATH).", Unigine::MakeCallback([this](int argc, char** argv) {
			if (argc == 1)
			{
				Unigine::Log::message("%s\n", blenderPath_.toUtf8().constData());
			}
			else if (argc == 2)
			{
				SettingManager::userSettings()->setString(BLENDER_PATH_CFG, argv[1]);
				blenderPath_ = tr(argv[1]);
			}
		}));

		Unigine::Console::addCommand(BLENDER_FIND_CMD, "Find the Blender executable in the file system.", Unigine::MakeCallback([this](int argc, char** argv) {
			auto fileName = QFileDialog::getOpenFileName(nullptr,
				tr("Open Executable"), "", tr("All Files (*);;Programs (*.exe)"));
			if (!fileName.isEmpty())
			{
				auto path = fileName.toUtf8().constData();
				SettingManager::userSettings()->setString(BLENDER_PATH_CFG, path);
				blenderPath_ = tr(path);
				Unigine::Log::message("set to:\n%s\n", path);
			}
		}));

		Unigine::Console::addCommand(TRACK_TRANSFORM_CMD, "Set to 0 to disable tracking of objects' transformation.", Unigine::MakeCallback([this](int argc, char** argv) {
			if (argc == 1)
			{
				Unigine::Log::message("%d\n", trackTransform_);
			}
			else if (argc == 2)
			{
				int val = Unigine::String::atoi(argv[1]);
				trackTransform_ = val != 0;
			}
		}));
	}

	void GeometryNodesPlugin::create_window()
	{
		if (window)
		{
			UnigineEditor::WindowManager::show(window);
			return;
		}

		window = new MainWindow;

		UnigineEditor::WindowManager::add(window, UnigineEditor::WindowManager::LAST_USED_AREA);
		UnigineEditor::WindowManager::show(window);
	}

	void GeometryNodesPlugin::meshUpdated()
	{
		if (window)
		{
			window->hideUpdate();
		}
	}

	void GeometryNodesPlugin::paramsUpdated(BlendAsset* comp)
	{
		if (window)
		{
			window->updateParamsGui(comp);
		}
	}

	void GeometryNodesPlugin::onAssetChanged(const char *path)
	{
		Unigine::UGUID guid = UnigineEditor::AssetManager::getAssetGUIDFromPath(path);
		Unigine::Vector<Unigine::NodePtr> nodes;
		Unigine::World::getNodes(nodes);
		for (auto& n : nodes)
		{
			Unigine::Vector<BlendAsset*> comps;
			Unigine::ComponentSystem::get()->getComponents<BlendAsset>(n, comps);
			for (auto c : comps)
			{
				if (c->blend_asset.getParameter()->getValueGUID() == guid)
					c->assetUpdated();
			}
		}
	}

	void GeometryNodesPlugin::destroy_window()
	{
		if (window)
		{
			disconnect(window, nullptr, this, nullptr);
			UnigineEditor::WindowManager::remove(window);
			delete window;
			window = nullptr;
		}
	}
} // namespace GeometryNodes