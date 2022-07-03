#include "BlendAsset.h"
#include "GeometryNodesPlugin.h"
#include <UnigineGame.h>
//#include <UnigineWorld.h>
#include <UnigineVisualizer.h>
//
#include <QFile>
//#include <QDir>
//#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>

using namespace Unigine;


namespace GeometryNodes
{

	REGISTER_COMPONENT(BlendAsset)

		void BlendAsset::init()
	{
		World::addCallback(World::CALLBACK_PRE_WORLD_SAVE, MakeCallback(this, &BlendAsset::saveTemporaryMeshes));

		PropertyPtr prop = getProperty();
		prop->addCallback(Property::CALLBACK_PARAMETER_CHANGED, MakeCallback(this, &BlendAsset::onPropertyParameterChanged));

		auto tm = getSavedData("temp_meshes");
		for (int mIndex = 0; mIndex < tm.size(); mIndex++)
		{
			temporaryMeshes.append(UGUID(tm[mIndex].toString().toUtf8().constData()));
		}

		onAssetChanged();
	}

	void BlendAsset::shutdown()
	{
		Log::message("component shutdown\n");
		World::clearCallbacks(World::CALLBACK_PRE_WORLD_SAVE);
		PropertyPtr prop = getProperty();
		prop->clearCallbacks(Property::CALLBACK_PARAMETER_CHANGED);
		saveData("temp_meshes", QJsonArray());
		cleanup(false);
	}

	void BlendAsset::on_enable()
	{
		Log::message("component enabled\n");
	}

	void BlendAsset::on_disable()
	{
		Log::message("component disabled\n");
	}

	void BlendAsset::update()
	{
		if (process)
		{
			auto state = process->getState();
			if (state == State::IDLE)
			{
				if (playback.get() == 2)
				{
					animate();
				}
				return;
			}
			if (state == State::READY)
			{
				process->request("pars");
			}
			if (state == State::PARAMS)
			{
				auto p = process->getParameters();
				gnParams = p;

				bool all = deserializeParameters();

				GeometryNodesPlugin::get()->paramsUpdated(this);

				int geoParam = findParameterByType(GNParam::Type::GEOMETRY);

				if (geoParam >= 0 && (node->getType() >= Node::OBJECT_MESH_STATIC && node->getType() <= Node::OBJECT_MESH_DYNAMIC ||
					node->getType() == Node::WORLD_SPLINE_GRAPH ||
					node->getType() == Node::LANDSCAPE_LAYER_MAP))
				{
					gnParams[geoParam].value = QVariant(node->getID());

					if (all)
					{
						process->requestMesh(getValidParameters(), getKnownInstances());
					}
					else
					{
						process->updateParameter(gnParams[geoParam], getKnownInstances());
					}
					/*updateParameter(gnParams[geoParam].id, QVariant(node->getID()));*/

					if (node->getType() == Node::WORLD_SPLINE_GRAPH)
					{
						auto wsg = static_ptr_cast<WorldSplineGraph>(node);
						wsg->addRebuildingFinishedCallback(MakeCallback([&](WorldSplineGraphPtr w) {
							if (!node->isEnabled() || !isEnabled()) { return; }
							int geoParam = findParameterByType(GNParam::Type::GEOMETRY);
							process->updateParameter(gnParams[geoParam], getKnownInstances());
						}));
					}
				}
				else
				{
					if (all)
					{
						process->requestMesh(getValidParameters(), getKnownInstances());
					}
					else
					{
						process->requestMesh();
					}

				}

				int fr = findParameter("Frame");
				if (fr >= 0)
				{
					auto& frp = gnParams[fr];
					frame_start = frp.min.i;
					frame_end = frp.max.i;
				}
			}
			if (state == State::MESH)
			{
				auto m = process->getMeshData();

				updateGeometry(m);
				GeometryNodesPlugin::get()->meshUpdated();
			}
		}
	}

	Vector<GNParam> BlendAsset::getValidParameters()
	{
		Vector<GNParam> res;
		for (const auto& i : gnParams)
		{
			if ((i.type == GNParam::Type::OBJECT || i.type == GNParam::Type::COLLECTION) && i.value.toInt() <= 0)
				continue;
			if (i.type == GNParam::Type::IMAGE)
			{
				UGUID guid = UGUID(i.value.toString().toUtf8().constData());
				if (!guid.isValid() || guid.isEmpty())
				{
					continue;
				}
			}
			res.append(i);
		}
		return res;
	}

	void BlendAsset::animate()
	{
		if (Game::isEnabled())
		{
			updateParameter("Frame", QVariant(frame));
			//frame++;
			// TODO: use float
			frame += Game::getScale();
			if (frame > frame_end) frame = frame_start;
		}
	}

	void BlendAsset::cleanup(bool full)
	{

		for (int i = 0; i < gnParams.size(); i++)
		{
			auto& param = gnParams[i];
			if (param.type == GNParam::Type::OBJECT || param.type == GNParam::Type::COLLECTION)
			{
				unsubscribeFromNodeChanges(i);
			}
		}

		gnParams.clear();
		GeometryNodesPlugin::get()->paramsUpdated(this);

		if (process)
		{
			if (process->isOpen())
			{
				process->request("exit");

				if (!process->waitForFinished(100))
				{
					process->kill();
					//Log::message("force killed\n");
				}
			}
			delete process;
			process = nullptr;
		}

		if (full)
		{
			if (!mesh.isNull())
			{
				mesh->deleteLater();
				mesh.clear();
			}
			for (const auto& cl : instances)
			{
				if (!cl.data.isNull()) cl.data->deleteLater();
			}

			for (int i = 0; i < node->getNumChildren(); i++)
			{
				auto ch = node->getChild(i);
				if (!ch.isNull())
				{
					ch->deleteLater();
				}
			}
		}
		instances.clear();
		auto wsg = checked_ptr_cast<WorldSplineGraph>(node);
		if (wsg) { wsg->clearRebuildingFinishedCallbacks(); }
	}

	void BlendAsset::onPropertyParameterChanged(PropertyPtr p, int n)
	{
		if (!node->isEnabled() || !isEnabled()) { return; }
		if (n == blend_asset.getID()) { onAssetChanged(); }
		else if (n == playback.getID()) { onPlaybackChanged(); }
	}

	void BlendAsset::onAssetChanged()
	{
		cleanup();
		auto guid = blend_asset.getParameter()->getValueGUID();
		if (guid.isEmpty())
		{
			return;
		}

		String abspath = FileSystem::getAbsolutePath(guid);
		Log::message("Running Blender with %s\n", abspath.get());
		process = new BlenderProcess(QString::number(node->getID()));
		if (!process->run(abspath))
		{
			cleanup();
			Visualizer::renderMessage2D(Math::vec3(0.5f), Math::vec3_zero, "Something went wrong.\nCheck the Console", Math::vec4_red, 1, 20, 10);
			blend_asset.getParameter()->resetValue();
		}
	}

	void BlendAsset::onPlaybackChanged()
	{
		GeometryNodesPlugin::get()->paramsUpdated(this);
	}

	int BlendAsset::findParameter(String id)
	{
		for (int i = 0; i < gnParams.size(); i++)
		{
			if (gnParams[i].id == id)
			{
				return i;
			}
		}
		return -1;
	}

	int BlendAsset::findParameterByType(GNParam::Type type)
	{
		for (int i = 0; i < gnParams.size(); i++)
		{
			if (gnParams[i].type == type)
			{
				return i;
			}
		}
		return -1;
	}

	void BlendAsset::updateParameter(String id, QVariant val)
	{
		int param = findParameter(id);
		if (param >= 0)
		{
			if (gnParams[param].type == GNParam::Type::OBJECT || gnParams[param].type == GNParam::Type::COLLECTION)
			{
				// subscribe to changes
				// create a nodetrigger for the node and push it to the list of subscriptions. also, this node also has to be tracked, so do it on start
				// for collections, all children must be tracked
				// also perform cleanup here
				// old
				unsubscribeFromNodeChanges(param);
				// new
				int nodeid = val.toInt();
				subscribeToNodeChanges(nodeid, param);

				gnParams[param].geoNode = node->getID();
			}
			gnParams[param].value = val;
			process->updateParameter(gnParams[param], getKnownInstances(), synchronous.get());
		}
	}

	void BlendAsset::unsubscribeFromNodeChanges(int param)
	{
		int ndid = gnParams[param].value.toInt();
		if (ndid > 0)
		{
			NodePtr nd = World::getNodeById(ndid);
			if (!nd.isNull())
			{
				if (nd->getType() == Node::WORLD_SPLINE_GRAPH)
				{
					auto wsg = static_ptr_cast<WorldSplineGraph>(nd);
					wsg->clearRebuildingFinishedCallbacks();
				}
				int tri = nd->findChild("bgn_trigger");
				if (tri >= 0)
				{
					NodePtr trn = nd->getChild(tri);
					auto tr = checked_ptr_cast<NodeTrigger>(trn);
					if (tr)
					{
						tr->clearPositionCallbacks();
						tr->deleteLater();
					}
				}
			}
		}
	}

	void BlendAsset::subscribeToNodeChanges(int nodeid, int param)
	{
		if (nodeid > 0 && GeometryNodesPlugin::get()->isTrackTransform())
		{
			NodePtr nd = World::getNodeById(nodeid);
			if (!nd.isNull())
			{
				if (nd->getType() == Node::WORLD_SPLINE_GRAPH)
				{
					auto wsg = static_ptr_cast<WorldSplineGraph>(nd);
					wsg->addRebuildingFinishedCallback(MakeCallback([=](WorldSplineGraphPtr w) {
						if (!node->isEnabled() || !isEnabled()) { return; }
						process->updateParameter(gnParams[param], getKnownInstances());
					}));
				}

				NodeTriggerPtr tr;
				NodePtr trn = nd->findNode("bgn_trigger");
				if (!trn.isNull() && trn->getType() == Node::NODE_TRIGGER)
				{
					tr = static_ptr_cast<NodeTrigger>(trn);
				}
				else
				{
					tr = NodeTrigger::create();
					tr->setName("bgn_trigger");
					tr->setParent(nd);
				}
				tr->setShowInEditorEnabled(false);
				tr->setSaveToWorldEnabled(false);
				tr->setTransform(Math::mat4_identity);
				tr->addPositionCallback(MakeCallback([=](NodeTriggerPtr tr) {
					if (!node->isEnabled() || !isEnabled()) { return; }
					process->updateParameter(gnParams[param], getKnownInstances());
				}));
			}

		}
	}

	void BlendAsset::updateGeometry(GNData& data)
	{
		if (!data.mesh.isEmpty())
		{
			if (update_existing.get() && !mesh_update_object.isEmpty())
			{
				if (mesh && !mesh.isNull())
				{
					mesh->deleteLater();
					mesh.clear();
				}

				NodePtr mo = mesh_update_object.get();
				setMesh(mo, generateMesh(data.mesh));

				if (ObjectPtr obj = checked_ptr_cast<Object>(mo))
				{
					setMaterials(obj, data.mesh.materials, getLocalPath("materials"));
				}
			}
			else
			{
				if (mesh.isNull())
				{
					mesh = ObjectMeshStatic::create("");
					mesh->setParent(node);
					mesh->setShowInEditorEnabled(true);
					mesh->setSaveToWorldEnabled(true);
				}

				mesh->setTransform(data.transform);
				mesh->setName(data.mesh.name);
				//mesh->setMeshName("");

				MeshPtr msh = generateMesh(data.mesh);
				mesh->setMesh(msh);

				// required for ViewportManager::placeNode with a just loaded blend
				/*for (int i = 0; i < mesh->getNumSurfaces(); i++)
				{
					mesh->setIntersection(false, i);
				}*/

				setMaterials(mesh, data.mesh.materials, getLocalPath("materials"));
				msh.clear();
			}
		}
		else if (mesh && !mesh.isNull())
		{
			mesh->deleteLater();
			mesh.clear();
		}

		Vector<long long> to_be_deleted;
		for (const auto& i : instances)
		{
			auto iter = std::find_if(data.instances.begin(), data.instances.end(), [i](const GNInstanceData& ins)
			{
				return i.key == ins.hash;
			});
			if (iter == data.instances.end())
			{
				to_be_deleted.append(i.key);
			}
		}
		for (const auto i : to_be_deleted)
		{
			if (!instances.get(i).isNull()) {
				instances.get(i)->deleteLater();
			}
			instances.remove(i);
		}
		to_be_deleted.clear();


		for (int i = 0; i < data.instances.size(); i++)
		{
			auto& ins = data.instances[i];

			ClusterUpdateParams* params = nullptr;
			if (update_existing && i < instance_objects.size())
			{
				params = &instance_objects.get(i).get();
			}

			bool isExternal = false;
			int action = 0;

			if (params)
			{
				action = params->action.get();
				switch (action)
				{
				case 0:
				case 1: isExternal = !params->node.isEmpty(); break;
				case 2: isExternal = !params->node_asset.nullCheck(); break;
				}
			}

			NodePtr instance_node;

			if (isExternal)
			{
				if (instances.contains(ins.hash))
				{
					if (!instances.get(ins.hash).isNull()) {
						instances.get(ins.hash)->deleteLater();
					}
					instances.remove(ins.hash);
				}
				if (action < 2)
				{
					instance_node = params->node.get();
					if (action == 1)
					{
						// clone nodes
						NodeDummyPtr group = NodeDummy::create();
						group->setParent(node);
						group->setTransform(Math::mat4_identity);
						group->setName(instance_node->getName());

						for (int im = 0; im < ins.matrices.size(); im++)
						{
							NodePtr nd = instance_node->clone();
							nd->setParent(group);
							nd->setTransform(ins.matrices[im]);
						}
						group->setShowInEditorEnabledRecursive(true);
						group->setSaveToWorldEnabledRecursive(true);
						instances.append(ins.hash, group);
						return;
					}
				}
				else
				{
					// noderefs
					UGUID guid = params->node_asset.getParameter()->getValueGUID();
					UGUID asset_guid = FileSystemAssets::resolveAsset(guid);
					String name = FileSystem::getVirtualPath(asset_guid).filename();

					NodeDummyPtr group = NodeDummy::create();
					group->setParent(node);
					group->setTransform(Math::mat4_identity);
					group->setName(name.get());

					for (int im = 0; im < ins.matrices.size(); im++)
					{
						NodeReferencePtr nr = NodeReference::create(guid);
						nr->setParent(group);
						nr->setTransform(ins.matrices[im]);
						nr->setName(name.get());
					}
					group->setShowInEditorEnabledRecursive(true);
					group->setSaveToWorldEnabledRecursive(true);
					instances.append(ins.hash, group);
					return;
				}
			}
			else
			{
				if (instances.contains(ins.hash))
				{
					instance_node = instances.get(ins.hash);
				}
				else if (!ins.mesh.isEmpty())
				{
					ObjectMeshClusterPtr cl = ObjectMeshCluster::create("");
					cl->setParent(node);
					cl->setTransform(Math::mat4_identity);
					cl->setShowInEditorEnabled(true);
					cl->setSaveToWorldEnabled(true);
					instances.append(ins.hash, cl);
					instance_node = cl;
				}
			}

			if (!ins.mesh.isEmpty())
			{
				if (isExternal)
				{
					if (params->replace_mesh.get())
					{
						setMesh(instance_node, generateMesh(ins.mesh));

						if (ObjectPtr obj = checked_ptr_cast<Object>(instance_node))
						{
							setMaterials(obj, ins.mesh.materials, getLocalPath("materials"));
						}
					}
				}
				else
				{
					instance_node->setName(ins.mesh.name);
					//instance_node->setMeshName("");
					setMesh(instance_node, generateMesh(ins.mesh));

					if (ObjectPtr obj = checked_ptr_cast<Object>(instance_node))
					{
						setMaterials(obj, ins.mesh.materials, getLocalPath("materials"));
					}
				}
			}

			if (!instance_node.isNull() && instance_node->getType() == Node::OBJECT_MESH_CLUSTER)
			{
				if (isExternal && instance_objects.get(i).get().replace_transforms.get() || !isExternal)
				{
					ObjectMeshClusterPtr cl = static_ptr_cast<ObjectMeshCluster>(instance_node);
					cl->clearMeshes();
					for (int im = 0; im < ins.matrices.size(); im++)
					{
						ins.matrices[im] = cl->getWorldTransform() * ins.matrices[im];
					}

					cl->createMeshes(ins.matrices);
				}
			}
		}
	}

	String BlendAsset::getLocalPath(String sub)
	{

		auto currdir = FileSystem::getVirtualPath(blend_asset.getParameter()->getValueGUID());
		auto blenddir = String::joinPaths(currdir.dirname(), currdir.filename());
		if (sub.size() > 0)
		{
			blenddir = String::joinPaths(blenddir, sub);
		}
		return blenddir;
	}

	void BlendAsset::clearTemporaryMeshes()
	{
		if (clear_meshes.get())
		{
			for (const auto& i : temporaryMeshes)
			{
				const char* meshPath = i.getFileSystemString();
				String abspath = FileSystem::getAbsolutePath(i);
				if (FileSystem::isFileExist(meshPath))
				{
					// TODO: is it a normal way to delete assets?

					const QString path = abspath.get();
					if (QFile::exists(path) && QFile::remove(path))
					{
						Log::message("removed %s\n", abspath.basename().get());
					}
					else { Log::message("removal of %s failed\n", abspath.basename().get()); }

				}
				//else { Log::message("%s doesn't exist\n", meshPath); }
			}
		}
		temporaryMeshes.clear();
	}

	void BlendAsset::saveTemporaryMeshes()
	{
		if (!node->isEnabled() || !isEnabled()) { return; }

		clearTemporaryMeshes();

		auto saveUniqueMesh = [&](String name, MeshPtr m)
		{
			auto meshPath = getUniqueFilePath(getLocalPath(String::joinPaths("meshes", String(name) + ".mesh")));
			if (m->save(meshPath.get()))
			{
				Log::message("saved %s\n", meshPath.basename().get());
				auto guid = FileSystem::getGUID("asset://" + meshPath);
				temporaryMeshes.append(guid);
				return String(guid.getFileSystemString());
			}
			else
			{
				Log::message("saving %s failed\n", meshPath.basename().get());
				return String();
			}
		};

		auto saveMesh = [&](NodePtr mesh_node, bool overwrite)
		{
			if (mesh_node.isNull()) { return; }
			auto m = getMesh(mesh_node);
			if (m.first.isNull()) { return; }

			m.first->optimizeIndices(Mesh::VERTEX_CACHE);
			setMesh(mesh_node, m.first);

			if (overwrite && m.second.size() > 0)
			{
				auto guid = FileSystem::getGUID(m.second.get());
				auto alias = FileSystemAssets::getRuntimeAlias(guid);
				if (strlen(alias) > 0)
				{
					// can't overwrite mesh inside fbx, so create new
					auto asset = String::filename(alias);
					String path = saveUniqueMesh(asset, m.first);
					setMeshName(mesh_node, path);
				}
				else
				{
					// mesh asset
					auto meshPath = FileSystem::getVirtualPath(guid);
					if (m.first->save(meshPath.get()))
					{
						Log::message("saved %s\n", meshPath.basename().get());
					}
					else
					{
						Log::message("saving %s failed\n", meshPath.basename().get());
					}
				}
			}
			else
			{
				// save to blend folder
				String path = saveUniqueMesh(String(mesh_node->getName()), m.first);
				setMeshName(mesh_node, path);
			}
		};

		if (update_existing && !mesh_update_object.isEmpty())
		{
			saveMesh(mesh_update_object, save_mesh.get() == 1);
		}
		else if (!mesh.isNull())
		{
			saveMesh(mesh, false);
		}

		for (const auto& cl : instances.values())
		{
			saveMesh(cl, false);
		}

		if (update_existing && instance_objects.size() > 0)
		{
			for (int i = 0; i < instance_objects.size(); i++)
			{
				const auto& ins = instance_objects.get(i).get();
				if (!ins.node.isEmpty() && ins.replace_mesh.get())
				{
					saveMesh(ins.node.get(), ins.save_mesh.get() == 1);
				}
			}
		}

		QJsonArray js;
		for (const auto& i : temporaryMeshes)
		{
			js.append(i.getString());
		}
		saveData("temp_meshes", js);
		serializeParameters();
	}

	void BlendAsset::saveData(const char* key, QJsonArray subtree)
	{
		auto data = node->getData("bgn_data");
		auto qdata = QByteArray(data);

		QJsonObject bgn_data = QJsonDocument::fromJson(qdata).object();
		bgn_data[key] = subtree;

		node->setData("bgn_data", QJsonDocument(bgn_data).toJson(QJsonDocument::Compact));
		//node->setData("bgn_data", NULL);
	}

	QJsonArray BlendAsset::getSavedData(const char* key)
	{
		auto data = node->getData("bgn_data");
		auto qdata = QByteArray(data);

		QJsonObject bgn_data = QJsonDocument::fromJson(qdata).object();
		if (bgn_data.contains(key) && bgn_data[key].isArray())
			return bgn_data[key].toArray();
		else return QJsonArray();
	}

	bool BlendAsset::deserializeParameters()
	{
		QJsonArray json_params = getSavedData("params");
		if (json_params.size() > 0)
		{
			Vector<GNParam> updatedParams;
			for (auto i : json_params)
			{
				if (i.isArray())
				{
					QJsonArray jpar = i.toArray();
					auto id = jpar[0].toString();
					int param = findParameter(id.toUtf8().constData());
					if (param >= 0 && jpar.size() > 2)
					{
						gnParams[param].deserialize(jpar[1].toInt(), jpar[2]);
						if (gnParams[param].type == GNParam::Type::OBJECT || gnParams[param].type == GNParam::Type::COLLECTION)
						{
							gnParams[param].geoNode = node->getID();
							subscribeToNodeChanges(gnParams[param].value.toInt(), param);
						}
					}
				}
			}
			return true;
		}
		return false;
	}

	void BlendAsset::serializeParameters()
	{
		QJsonArray jpar;
		for (auto& param : gnParams)
		{
			if (param.type != GNParam::Type::GEOMETRY)
				jpar.append(param.serialize());
		}
		saveData("params", jpar);
	}
} // namespace GeometryNodes