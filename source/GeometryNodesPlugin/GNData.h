#pragma once

#include <UnigineString.h>
#include <UnigineVector.h>
#include <UnigineMesh.h>
#include <UnigineMaterials.h>
#include <UnigineObjects.h>
#include <UnigineMaterial.h>
#include <UnigineFileSystem.h>
#include <UnigineWorld.h>
#include <UnigineSplineGraph.h>
#include <UnigineWorlds.h>
#include <UnigineDecals.h>
#include <UniginePathFinding.h>
#include <UniginePair.h>
//
//#include <QString>
//#include <QByteArray>
#include <QVariant>
#include <QDataStream>
#include <QJsonArray>
//#include <QJsonValueRef>

namespace GeometryNodes
{

	struct GNParam
	{
	public:
		Unigine::String id;
		Unigine::String name;
		enum class Type
		{
			CUSTOM = 0,
			VALUE,
			INT,
			BOOLEAN,
			VECTOR,
			STRING,
			RGBA,
			SHADER,
			OBJECT,
			IMAGE,
			GEOMETRY,
			COLLECTION,
			TEXTURE,
			MATERIAL
		} type;

		QVariant value;
		int geoNode;

		union MinMaxVal
		{
			float f;
			int i;
		};

		MinMaxVal min, max;
		bool isValid() { return id.size() > 0; }

		void deserialize(int typeInt, QJsonValue val)
		{
			auto t = static_cast<GNParam::Type>(typeInt);
			if (t != type) { return; }


			switch (type)
			{
			case Type::VECTOR:
			{
				auto vec = val.toArray();
				QList<double> l = { vec[0].toDouble(), vec[1].toDouble(), vec[2].toDouble() };
				value = QVariant::fromValue(l);
			}
			break;
			case Type::RGBA:
			{
				auto vec = val.toArray();
				QList<double> l = { vec[0].toDouble(), vec[1].toDouble(), vec[2].toDouble(), vec[4].toDouble() };
				value = QVariant::fromValue(l);
			}
			break;
			default:
				value = val.toVariant();
				break;
			}
		}

		QJsonValue serialize()
		{
			QJsonArray jpar;
			jpar.append(id.get());
			auto t = static_cast<int>(type);
			jpar.append(t);
			switch (type)
			{
			case Type::RGBA:
			case Type::VECTOR:
			{
				QJsonArray vec;
				QSequentialIterable it = value.value<QSequentialIterable>();
				for (const QVariant& v : it)
				{
					vec.append(v.toFloat());
				}
				jpar.append(vec);
			}
			break;
			default:
				jpar.append(value.toJsonValue());
				break;
			}
			return jpar;
		}
	};

	struct GNSurfaceData
	{
		Unigine::Vector<Unigine::Math::vec3> vertices;
		Unigine::Vector<int> Cindices;
		Unigine::Vector<int> Tindices;
		Unigine::Vector<Unigine::Math::vec3> normals;
		Unigine::Vector<Unigine::Math::vec2> uv0;
		Unigine::Vector<Unigine::Math::vec2> uv1;
		Unigine::Vector<Unigine::Math::vec4> colors;

		bool isEmpty() { return vertices.size() == 0; }
	};

	struct GNMaterialData
	{
		Unigine::String name;
		bool valid{ false };
		Unigine::String blend_mode{ "" };
		bool two_sided{ false };
		Unigine::Math::vec4 base_color{ Unigine::Math::vec4_zero };
		float metallic{ 0.0f };
		float roughness{ 1.0f };
		float specular{ 0.5f };

		GNMaterialData() { }

		GNMaterialData(Unigine::String n)
		{
			name = n;
		}
	};

	struct GNMeshData
	{
		Unigine::Vector<GNSurfaceData> surfaces;
		Unigine::Vector<GNMaterialData> materials;
		Unigine::String name;
		bool isEmpty() { return surfaces.size() == 0; }
	};

	struct GNInstanceData
	{
		long long hash{ 0 };
		Unigine::Vector<Unigine::Math::mat4> matrices;
		GNMeshData mesh;
	};

	struct GNCurveData
	{
		Unigine::Math::vec3 start;
		Unigine::Math::vec3 startTangent;
		Unigine::Math::vec3 end;
		Unigine::Math::vec3 endTangent;
	};

	struct GNInputSplinePointData
	{
		Unigine::Math::vec3 pos;
		Unigine::Math::vec3 startTangent;
		Unigine::Math::vec3 endTangent;
	};

	struct GNInputSplineData
	{
		Unigine::Vector<GNInputSplinePointData> points;
	};

	struct GNInputCurveData
	{
		Unigine::String name;
		Unigine::Math::mat4 transform;
		Unigine::Vector<GNInputSplineData> splines;
	};

	struct GNData
	{
		Unigine::Math::mat4 transform;
		GNMeshData mesh;
		Unigine::Vector<GNInstanceData> instances;
		Unigine::Vector<GNInputCurveData> curves;
		Unigine::Vector<GNCurveData> segments;
	};

	static Unigine::Pair<Unigine::MeshPtr, Unigine::String> getMesh(Unigine::NodePtr node)
	{
		Unigine::MeshPtr m = Unigine::Mesh::create();
		const char* meshname = "";

		switch (node->getType())
		{
		case Unigine::Node::OBJECT_MESH_STATIC:
		{
			auto oms = Unigine::static_ptr_cast<Unigine::ObjectMeshStatic>(node);
			oms->getMesh(m);
			meshname = oms->getMeshName();
		}
		break;
		case Unigine::Node::OBJECT_MESH_DYNAMIC:
		{
			auto omd = Unigine::static_ptr_cast<Unigine::ObjectMeshDynamic>(node);
			omd->getMesh(m);
			meshname = omd->getMeshName();
		}
		break;
		case Unigine::Node::OBJECT_MESH_SKINNED:
		{
			auto oms = Unigine::static_ptr_cast<Unigine::ObjectMeshSkinned>(node);
			oms->getMesh(m);
			meshname = oms->getMeshName();
		}
		break;
		case Unigine::Node::OBJECT_MESH_CLUSTER:
		{
			auto omc = Unigine::static_ptr_cast<Unigine::ObjectMeshCluster>(node);
			omc->getMesh(m);
			meshname = omc->getMeshName();
		}
		break;
		case Unigine::Node::OBJECT_MESH_CLUTTER:
		{
			auto omc = Unigine::static_ptr_cast<Unigine::ObjectMeshClutter>(node);
			omc->getMesh(m);
			meshname = omc->getMeshName();
		}
		break;
		case Unigine::Node::OBJECT_GUI_MESH:
		{
			auto ogm = Unigine::static_ptr_cast<Unigine::ObjectGuiMesh>(node);
			ogm->getMesh(m);
			meshname = ogm->getMeshName();
		}
		break;
		case Unigine::Node::WORLD_OCCLUDER_MESH:
		{
			auto wom = Unigine::static_ptr_cast<Unigine::WorldOccluderMesh>(node);
			wom->getMesh(m);
			meshname = wom->getMeshName();
		}
		break;
		case Unigine::Node::DECAL_MESH:
		{
			auto dm = Unigine::static_ptr_cast<Unigine::DecalMesh>(node);
			dm->getMesh(m);
			meshname = dm->getMeshName();
		}
		break;
		case Unigine::Node::OBJECT_WATER_MESH:
		{
			auto owm = Unigine::static_ptr_cast<Unigine::ObjectWaterMesh>(node);
			owm->getMesh(m);
			meshname = owm->getMeshName();
		}
		break;
		case Unigine::Node::NAVIGATION_MESH:
		{
			auto nm = Unigine::static_ptr_cast<Unigine::NavigationMesh>(node);
			nm->getMesh(m);
			meshname = nm->getMeshName();
		}
		break;
		default:
			return Unigine::Pair<Unigine::MeshPtr, Unigine::String>();
		}
		return Unigine::Pair<Unigine::MeshPtr, Unigine::String>(m, Unigine::String(meshname));
	}

	static Unigine::String getMeshAssetName(Unigine::String meshname)
	{
		auto guid = Unigine::FileSystem::getGUID(meshname.get());
		auto alias = Unigine::FileSystemAssets::getRuntimeAlias(guid);
		if (strlen(alias) > 0)
		{
			return Unigine::String::filename(alias);
		}
		else
		{
			auto filepath = Unigine::FileSystem::getVirtualPath(guid);
			return filepath.filename();
		}
	}

	static GNMeshData getMeshData(Unigine::NodePtr node)
	{
		GNMeshData md;
		auto mpair = getMesh(node);
		Unigine::MeshPtr m = mpair.first;
		auto& meshname = mpair.second;
		if (m.isNull()) { return md; }

		md.name = getMeshAssetName(meshname);

		//ObjectPtr object = checked_ptr_cast<Object>(node);
		//m->createNormals();

		for (int i = 0; i < m->getNumSurfaces(); i++)
		{
			GNSurfaceData s;
			s.vertices = m->getVertices(i);
			s.Cindices = m->getCIndices(i);
			s.Tindices = m->getTIndices(i);
			if (m->getNumTangents(i) == 0)
			{
				m->createTangents(i);
			}
			auto& tan = m->getTangents(i);
			s.normals.reserve(tan.size());
			for (const auto& t : tan)
			{
				s.normals.append(t.getNormal());
			}

			s.uv0 = m->getTexCoords0(i);
			s.uv1 = m->getTexCoords1(i);
			md.surfaces.append(s);

			// TODO: what's better - surface name or material name
			/*auto matAsset = FileSystemAssets::resolveAsset(object->getMaterial(i)->getGUID());
			auto matAssetName = FileSystem::getVirtualPath(matAsset).filename();*/
			md.materials.append(GNMaterialData(Unigine::String(m->getSurfaceName(i))));
		}
		m.clear();
		return md;
	}

	static GNMeshData fetchTerrain(Unigine::LandscapeLayerMapPtr node)
	{
		Unigine::Math::ivec2 resolution{ 256, 256 };
		int max_fetchers = 256;
		Unigine::Vector<Unigine::Math::vec2> fetch_positions;
		Unigine::Vector<float> fetch_data;
		int fishined_fetch_count{ 0 };
		int next_fetch_index{ 0 };

		struct Fetcher
		{
			Unigine::LandscapeFetchPtr landscape_fetch;
			int out_data_index{ 0 };
		};
		Unigine::Vector<Fetcher> fetchers;

		Unigine::Math::vec2 size = node->getSize();
		Unigine::Math::mat4 transform = node->getWorldTransform() * Unigine::Math::scale(size.x * 0.99f, size.y * 0.99f, 1.0f);
		Unigine::Math::vec3 bottom_left = transform * Unigine::Math::vec3(0.005f, 0.005f, 0.0f);

		transform.setColumn3(3, Unigine::Math::vec3(0.0f, 0.0f, 0.0f));
		Unigine::Math::vec3 delta_x = transform * Unigine::Math::vec3(1.0f / resolution.x, 0.0f, 0.0f);
		Unigine::Math::vec3 delta_y = transform * Unigine::Math::vec3(0.0f, 1.0f / resolution.y, 0.0f);

		fetch_positions.clear();
		for (int i = 0; i <= resolution.y; ++i)
		{
			for (int j = 0; j <= resolution.x; ++j)
			{
				Unigine::Math::vec3 sample_point = bottom_left + delta_y * static_cast<Unigine::Math::Scalar>(i) + delta_x * static_cast<Unigine::Math::Scalar>(j);
				fetch_positions.push_back(sample_point.xy);
			}
		}

		fetch_data.resize(fetch_positions.size());

		while (fetchers.size() < Unigine::Math::min(max_fetchers, fetch_positions.size()))
		{
			Fetcher& f = fetchers.emplace_back();
			f.landscape_fetch = Unigine::LandscapeFetch::create();
			f.landscape_fetch->setUsesHeight(true);
			//f.landscape_fetch->setUsesAlbedo(true);
		}

		while (fetchers.size() > Unigine::Math::min(max_fetchers, fetch_positions.size()))
			fetchers.removeLast();

		next_fetch_index = 0;
		fishined_fetch_count = 0;
		for (int i = 0; i < fetchers.size(); ++i)
		{
			fetchers[i].landscape_fetch->fetchAsync(fetch_positions[next_fetch_index]);
			//fetchers[i].landscape_fetch->fetchForce(fetch_positions[next_fetch_index]);
			fetchers[i].out_data_index = next_fetch_index;

			next_fetch_index += 1;
		}

		while (fishined_fetch_count < fetch_positions.size())
		{
			for (int i = 0; i < fetchers.size(); ++i)
			{
				if (fetchers[i].landscape_fetch->isAsyncCompleted())
				{
					fishined_fetch_count += 1;

					fetch_data[fetchers[i].out_data_index] = fetchers[i].landscape_fetch->getHeight();
					//fetch_data[fetchers[i].out_data_index].albedo = fetchers[i].landscape_fetch->getAlbedo();

					if (next_fetch_index < fetch_positions.size())
					{
						fetchers[i].landscape_fetch->fetchAsync(fetch_positions[next_fetch_index]);
						fetchers[i].out_data_index = next_fetch_index;

						next_fetch_index += 1;
					}
				}
			}
		}

		GNSurfaceData s;
		s.vertices.reserve((resolution.x + 1) * (resolution.y + 1));
		s.Cindices.reserve(resolution.x * resolution.y * 6);
		s.Tindices.reserve(resolution.x * resolution.y * 6);
		s.uv0.reserve((resolution.x + 1) * (resolution.y + 1));

		for (int i = 0; i <= resolution.y; ++i)
		{
			for (int j = 0; j <= resolution.x; ++j)
			{
				float height = fetch_data[(resolution.x + 1) * i + j];
				const auto& position = Unigine::Math::vec3(Unigine::Math::vec2(fetch_positions[(resolution.x + 1) * i + j]), height);

				s.vertices.append(inverse(node->getWorldTransform()) * position);
				s.uv0.append(Unigine::Math::vec2((float)j / resolution.x, 1 - (float)i / resolution.y));
			}
		}

		for (int i = 0; i < resolution.y; ++i)
		{
			auto pitch = resolution.x + 1;
			auto offset = pitch * i;

			for (int j = 0; j < resolution.x; ++j)
			{
				s.Cindices.appendFast(offset + j + 1);
				s.Cindices.appendFast(offset + pitch + j);
				s.Cindices.appendFast(offset + j);

				s.Cindices.appendFast(offset + j + 1);
				s.Cindices.appendFast(offset + pitch + j + 1);
				s.Cindices.appendFast(offset + pitch + j);


				s.Tindices.appendFast(offset + j + 1);
				s.Tindices.appendFast(offset + pitch + j);
				s.Tindices.appendFast(offset + j);

				s.Tindices.appendFast(offset + j + 1);
				s.Tindices.appendFast(offset + pitch + j + 1);
				s.Tindices.appendFast(offset + pitch + j);
			}
		}
		GNMeshData m;
		m.surfaces.append(s);
		return m;
	}

	static GNData getGeometryData(Unigine::NodePtr node, int geoNodeId = -1, bool isCollection = false)
	{
		GNData data;
		if (!node) { return data; }

		if (geoNodeId > 0)
		{
			data.transform = node->getWorldTransform();
			Unigine::NodePtr geoNode = Unigine::World::getNodeByID(geoNodeId);
			if (!geoNode.isNull())
			{
				// getting the world matrix relative to the main node
				Unigine::Math::mat4 parent = geoNode->getWorldTransform();
				data.transform = inverse(parent) * data.transform;
			}
		}

		auto type = node->getType();
		if (type == Unigine::Node::TYPE::OBJECT_MESH_CLUSTER)
		{
			if (isCollection)
			{
				GNInstanceData ins;
				ins.mesh = getMeshData(node);
				auto cluster = Unigine::static_ptr_cast<Unigine::ObjectMeshCluster>(node);
				ins.matrices.reserve(cluster->getNumMeshes());
				for (int i = 0; i < cluster->getNumMeshes(); i++)
				{
					ins.matrices.append(data.transform * cluster->getMeshTransform(i));
				}
				data.instances.append(ins);
			}
			else
			{
				data.mesh = getMeshData(node);
			}
		}
		else if (type == Unigine::Node::TYPE::OBJECT_MESH_CLUTTER)
		{
			if (isCollection)
			{
				GNInstanceData ins;
				ins.mesh = getMeshData(node);
				auto clutter = Unigine::static_ptr_cast<Unigine::ObjectMeshClutter>(node);
				clutter->getClutterTransforms(ins.matrices);
				for (int i = 0; i < ins.matrices.size(); i++)
				{
					ins.matrices[i] = data.transform * ins.matrices[i];
				}
				data.instances.append(ins);
			}
			else
			{
				data.mesh = getMeshData(node);
			}
		}
		else if (type == Unigine::Node::TYPE::WORLD_SPLINE_GRAPH)
		{
			auto wsg = Unigine::static_ptr_cast<Unigine::WorldSplineGraph>(node);
			Unigine::Vector<Unigine::SplinePointPtr> points;
			Unigine::Vector<Unigine::SplineSegmentPtr> segments;
			wsg->getSplinePoints(points);
			wsg->getSplineSegments(segments);
			for (const auto& seg : segments)
			{
				if (!seg->isEnabled())
				{
					continue;
				}
				GNCurveData c;
				c.start = seg->getStartPoint()->getPosition();
				c.startTangent = c.start + seg->getStartTangent();
				c.end = seg->getEndPoint()->getPosition();
				c.endTangent = c.end + seg->getEndTangent();
				data.segments.append(c);
			}
		}
		else if (type == Unigine::Node::TYPE::LANDSCAPE_LAYER_MAP)
		{
			Unigine::LandscapeLayerMapPtr lmap = Unigine::static_ptr_cast<Unigine::LandscapeLayerMap>(node);
			data.mesh = fetchTerrain(lmap);
		}
		else
		{
			data.mesh = getMeshData(node);
		}
		return data;
	}

	static Unigine::String getUniqueFilePath(Unigine::String path)
	{
		if (!Unigine::FileSystem::isFileExist(path))
			return path;
		auto dir = path.dirname();
		auto name = path.filename();
		auto ext = path.extension();

		int underscore = name.rfind('_');
		// TODO: add isDigits check
		if (underscore >= 0)
		{
			name = name.substr(0, underscore);
		}
		int index = 1;
		Unigine::String newpath;
		do
		{
			newpath = Unigine::String::format("%s%s_%d.%s", dir.get(), name.get(), index++, ext.get());
		} while (Unigine::FileSystem::isFileExist(newpath));
		return newpath;
	}

	static Unigine::MeshPtr generateMesh(GNMeshData& mesh)
	{
		Unigine::MeshPtr m = Unigine::Mesh::create();
		for (int surface = 0; surface < mesh.surfaces.size(); surface++)
		{
			auto& surfaceData = mesh.surfaces[surface];


			auto name = Unigine::String::format("Surface %d", surface);
			if (surface < mesh.materials.size())
			{
				name = mesh.materials[surface].name;
			}
			m->addSurface(name.get());

			m->addVertex(surfaceData.vertices, surface);
			m->addCIndices(surfaceData.Cindices, surface);
			m->addTIndices(surfaceData.Tindices, surface);
			m->addNormals(surfaceData.normals, surface);
			if (surfaceData.uv0.size() > 0)
				m->addTexCoords0(surfaceData.uv0, surface);

			if (surfaceData.uv1.size() > 0)
				m->addTexCoords1(surfaceData.uv1, surface);

			if (surfaceData.colors.size() > 0)
				m->addColors(surfaceData.colors, surface);

			//if (m->getNumCVertex(surface) == 0) continue;
			m->createTangents(surface);
			m->createBounds(surface);
		}

		// vertex cache to be optimized later when exporting
		//m->optimizeIndices(Mesh::VERTEX_CACHE);
		return m;
	}

	static Unigine::MaterialPtr findMaterialByName(Unigine::String name)
	{
		// dang it
		for (int i = 0; i < Unigine::Materials::getNumMaterials(); i++)
		{
			auto m = Unigine::Materials::getMaterial(i);
			if (m->isInternal()) continue;
			auto asset = Unigine::FileSystemAssets::resolveAsset(m->getGUID());
			auto assetname = Unigine::FileSystem::getVirtualPath(asset).filename();
			if (Unigine::String::equal(name, assetname))
			{
				return m;
			}
		}
		return Unigine::MaterialPtr(nullptr);
	}

	static void setMaterials(Unigine::ObjectPtr obj, Unigine::Vector<GNMaterialData>& materials, Unigine::String dir)
	{
		for (int i = 0; i < Unigine::Math::min(obj->getNumSurfaces(), materials.size()); i++)
		{
			auto& material = materials[i];

			if (!material.valid) { continue; }

			auto path = Unigine::String::joinPaths(dir, material.name) + Unigine::String(".mat");
			Unigine::MaterialPtr mat = Unigine::Materials::findMaterialByPath(path.get());
			if (mat.isNull()) { mat = findMaterialByName(material.name); }
			if (mat.isNull())
			{
				Unigine::MaterialPtr parent = Unigine::Materials::findManualMaterial("Unigine::mesh_base");
				if (parent)
				{
					mat = parent->inherit();

					int blend = 0;
					if (material.blend_mode == "OPAQUE") blend = 0;
					else if (material.blend_mode == "CLIP") blend = 1;
					else if (material.blend_mode == "HASHED" || material.blend_mode == "BLEND") blend = 2;
					mat->setTransparent(blend);
					mat->setTwoSided(material.two_sided);
					mat->setParameterFloat4("albedo_color", material.base_color);
					mat->setParameterFloat("metalness", material.metallic);
					mat->setParameterFloat("roughness", material.roughness);
					mat->setParameterFloat("specular", material.specular);

					mat->createMaterialFile(path.get());
				}
			}
			if (!mat.isNull()) { obj->setMaterial(mat, i); }

			// not needed
			/*else
			{
				MaterialPtr mesh_base = Materials::findManualMaterial("Unigine::mesh_base");
				if (mesh_base)
				{
					obj->setMaterial(mesh_base, i);
				}
			}*/
		}
	}

	static void setMesh(Unigine::NodePtr node, Unigine::MeshPtr m)
	{
		if (node.isNull() || m.isNull()) { return; }
		switch (node->getType())
		{
		case Unigine::Node::OBJECT_MESH_STATIC:
		{
			auto oms = Unigine::static_ptr_cast<Unigine::ObjectMeshStatic>(node);
			oms->setMesh(m);
			oms->updateSurfaceBounds();
		}
		break;
		case Unigine::Node::OBJECT_MESH_DYNAMIC:
		{
			auto omd = Unigine::static_ptr_cast<Unigine::ObjectMeshDynamic>(node);
			omd->setMesh(m);
			omd->updateBounds();
		}
		break;
		case Unigine::Node::OBJECT_MESH_SKINNED:
		{
			auto oms = Unigine::static_ptr_cast<Unigine::ObjectMeshSkinned>(node);
			oms->setMesh(m);
			oms->updateSurfaceBounds();
		}
		break;
		case Unigine::Node::OBJECT_MESH_CLUSTER:
		{
			auto omc = Unigine::static_ptr_cast<Unigine::ObjectMeshCluster>(node);
			omc->setMesh(m);
		}
		break;
		case Unigine::Node::OBJECT_MESH_CLUTTER:
		{
			auto omc = Unigine::static_ptr_cast<Unigine::ObjectMeshClutter>(node);
			omc->setMesh(m);
		}
		break;
		case Unigine::Node::OBJECT_GUI_MESH:
		{
			auto ogm = Unigine::static_ptr_cast<Unigine::ObjectGuiMesh>(node);
			ogm->setMesh(m);
		}
		break;
		case Unigine::Node::WORLD_OCCLUDER_MESH:
		{
			auto wom = Unigine::static_ptr_cast<Unigine::WorldOccluderMesh>(node);
			wom->setMesh(m);
		}
		break;
		case Unigine::Node::DECAL_MESH:
		{
			auto dm = Unigine::static_ptr_cast<Unigine::DecalMesh>(node);
			dm->setMesh(m);
		}
		break;
		case Unigine::Node::OBJECT_WATER_MESH:
		{
			auto owm = Unigine::static_ptr_cast<Unigine::ObjectWaterMesh>(node);
			owm->setMesh(m);
		}
		break;
		case Unigine::Node::NAVIGATION_MESH:
		{
			auto nm = Unigine::static_ptr_cast<Unigine::NavigationMesh>(node);
			nm->setMesh(m);
		}
		break;
		}
		m.clear();
	}

	static void setMeshName(Unigine::NodePtr node, Unigine::String data)
	{
		if (node.isNull() || strlen(data) == 0) { return; }
		switch (node->getType())
		{
		case Unigine::Node::OBJECT_MESH_STATIC:
		{
			auto oms = Unigine::static_ptr_cast<Unigine::ObjectMeshStatic>(node);
			oms->setMeshName(data.get());
		}
		break;
		case Unigine::Node::OBJECT_MESH_DYNAMIC:
		{
			auto omd = Unigine::static_ptr_cast<Unigine::ObjectMeshDynamic>(node);
			omd->setMeshName(data.get());
		}
		break;
		case Unigine::Node::OBJECT_MESH_SKINNED:
		{
			auto oms = Unigine::static_ptr_cast<Unigine::ObjectMeshSkinned>(node);
			oms->setMeshName(data.get());
		}
		break;
		case Unigine::Node::OBJECT_MESH_CLUSTER:
		{
			auto omc = Unigine::static_ptr_cast<Unigine::ObjectMeshCluster>(node);
			omc->setMeshName(data.get());
		}
		break;
		case Unigine::Node::OBJECT_MESH_CLUTTER:
		{
			auto omc = Unigine::static_ptr_cast<Unigine::ObjectMeshClutter>(node);
			omc->setMeshName(data.get());
		}
		break;
		case Unigine::Node::OBJECT_GUI_MESH:
		{
			auto ogm = Unigine::static_ptr_cast<Unigine::ObjectGuiMesh>(node);
			ogm->setMeshName(data.get());
		}
		break;
		case Unigine::Node::WORLD_OCCLUDER_MESH:
		{
			auto wom = Unigine::static_ptr_cast<Unigine::WorldOccluderMesh>(node);
			wom->setMeshName(data.get());
		}
		break;
		case Unigine::Node::DECAL_MESH:
		{
			auto dm = Unigine::static_ptr_cast<Unigine::DecalMesh>(node);
			dm->setMeshName(data.get());
		}
		break;
		case Unigine::Node::OBJECT_WATER_MESH:
		{
			auto owm = Unigine::static_ptr_cast<Unigine::ObjectWaterMesh>(node);
			owm->setMeshName(data.get());
		}
		break;
		case Unigine::Node::NAVIGATION_MESH:
		{
			auto nm = Unigine::static_ptr_cast<Unigine::NavigationMesh>(node);
			nm->setMeshName(data.get());
		}
		break;
		}
	}

	// import

	template <typename T>
	inline QDataStream& operator>>(QDataStream& in, Unigine::Vector<T>& v)
	{
		v.clear();
		int n;
		in >> n;
		if (n < 0) return in;
		v.reserve(n);
		for (int i = 0; i < n; ++i) {
			T t;
			in >> t;
			if (in.status() != QDataStream::Ok) {
				v.clear();
				break;
			}
			//v.append(t);
			v.appendFast(t);
		}
		return in;
	}

	inline QDataStream& operator>>(QDataStream& in, Unigine::Vector<int>& v)
	{
		v.clear();
		int n;
		in >> n;
		if (n < 0) return in;
		v.reserve(n);

		int nb = n * sizeof(int);
		char* bt = new char[nb];
		in.readRawData(bt, nb);

		v.append(reinterpret_cast<const int*>(bt), n);
		//v.append((const int*)bt, n);

		delete[] bt;
		return in;
	}

	inline QDataStream& operator>>(QDataStream& in, Unigine::Vector<Unigine::Math::mat4>& v)
	{
		v.clear();
		int n;
		in >> n;
		if (n < 0) return in;
		v.reserve(n);

		int nb = n * sizeof(Unigine::Math::mat4);
		char* bt = new char[nb];
		in.readRawData(bt, nb);

		v.append(reinterpret_cast<const Unigine::Math::mat4*>(bt), n);

		delete[] bt;
		return in;
	}

	inline QDataStream& operator>>(QDataStream& in, Unigine::Vector<Unigine::Math::vec3>& v)
	{
		v.clear();
		int n;
		in >> n;
		if (n < 0) return in;
		v.reserve(n);

		size_t sf = sizeof(float);
		int rb = n * sf * 3;
		size_t nb = n * sizeof(Unigine::Math::vec3);
		char* bt = new char[nb];
		in.readRawData(bt, rb);

		// alignment 3 + 1 floats

		for (int i = nb - sf, j = rb - sf; i >= 0, j >= 0;)
		{
			for (int k = 0; k < sf; k++)
			{
				bt[i + k] = '\0';
			}

			i -= sf;

			for (int k = 0; k < 3; k++)
			{
				for (int l = 0; l < sf; l++)
				{
					bt[i + l] = bt[j + l];
				}
				i -= sf;
				j -= sf;
			}
		}

		v.append(reinterpret_cast<const Unigine::Math::vec3*>(bt), n);

		delete[] bt;
		return in;
	}

	inline QDataStream& operator>>(QDataStream& in, Unigine::String& s)
	{
		QByteArray qa;
		in >> qa;
		s = Unigine::String(qa.constData());
		return in;
	}

	inline QDataStream& operator>>(QDataStream& in, Unigine::Math::vec4& vec)
	{
		float x, y, z, w;
		in >> x;
		in >> y;
		in >> z;
		in >> w;
		vec = Unigine::Math::vec4(x, y, z, w);
		return in;
	}

	inline QDataStream& operator>>(QDataStream& in, Unigine::Math::vec3& vec)
	{
		float x, y, z;
		in >> x;
		in >> y;
		in >> z;
		vec = Unigine::Math::vec3(x, y, z);
		return in;
	}

	inline QDataStream& operator>>(QDataStream& in, Unigine::Math::vec2& vec)
	{
		float x, y;
		in >> x;
		in >> y;
		vec = Unigine::Math::vec2(x, 1.0f - y);
		return in;
	}

	inline QDataStream& operator>>(QDataStream& in, Unigine::Math::mat4& m)
	{
		float m00, m10, m20, m30;
		float m01, m11, m21, m31;
		float m02, m12, m22, m32;
		float m03, m13, m23, m33;

		// transposed
		in >> m00; in >> m01; in >> m02; in >> m03;
		in >> m10; in >> m11; in >> m12; in >> m13;
		in >> m20; in >> m21; in >> m22; in >> m23;
		in >> m30; in >> m31; in >> m32; in >> m33;

		m = Unigine::Math::mat4(m00, m10, m20, m30, m01, m11, m21, m31, m02, m12, m22, m32, m03, m13, m23, m33);
		return in;
	}

	inline QDataStream& operator>>(QDataStream& in, GNParam& p)
	{
		in >> p.id;
		in >> p.name;

		int type;
		in >> type;
		p.type = static_cast<GNParam::Type>(type);

		switch (p.type)
		{
		case GNParam::Type::VALUE:
			float f;
			in >> f;
			p.value = QVariant(f);
			in >> p.min.f;
			in >> p.max.f;
			break;
		case GNParam::Type::INT:
			qint32 i;
			in >> i;
			p.value = QVariant(i);
			in >> p.min.i;
			in >> p.max.i;
			break;
		case GNParam::Type::BOOLEAN:
			bool b;
			in >> b;
			p.value = QVariant(b);
			break;
		case GNParam::Type::VECTOR:
		{
			float x, y, z;
			in >> x;
			in >> y;
			in >> z;
			QList<float> l = { x, y, z };
			p.value = QVariant::fromValue(l);
			in >> p.min.f;
			in >> p.max.f;
		}
		break;
		case GNParam::Type::RGBA:
		{
			float x, y, z, w;
			in >> x;
			in >> y;
			in >> z;
			in >> w;
			QList<float> l = { x, y, z, w };
			p.value = QVariant::fromValue(l);
		}
		break;
		case GNParam::Type::STRING:
		{
			Unigine::String s;
			in >> s;
			p.value = QVariant(QString(s.get()));
		}
		break;
		case GNParam::Type::OBJECT:
		case GNParam::Type::COLLECTION:
			p.value = QVariant(-1);
			break;
		}

		return in;
	}

	inline QDataStream& operator>>(QDataStream& in, GNSurfaceData& surface)
	{
		in >> surface.vertices;
		in >> surface.Cindices;
		in >> surface.Tindices;
		in >> surface.normals;
		in >> surface.uv0;
		in >> surface.uv1;
		in >> surface.colors;
		return in;
	}

	inline QDataStream& operator>>(QDataStream& in, GNMaterialData& m)
	{
		in >> m.name;
		bool b;
		in >> b;
		m.valid = b;
		if (b)
		{
			in >> m.blend_mode;
			in >> m.two_sided;
			in >> m.base_color;
			in >> m.metallic;
			in >> m.roughness;
			in >> m.specular;
		}
		return in;
	}

	inline QDataStream& operator>>(QDataStream& in, GNMeshData& mesh)
	{
		bool b;
		in >> b;
		if (b)
		{
			in >> mesh.name;
			in >> mesh.surfaces;
			in >> mesh.materials;
		}
		return in;
	}

	inline QDataStream& operator>>(QDataStream& in, GNInstanceData& ins)
	{
		in >> ins.hash;
		in >> ins.matrices;
		bool b;
		in >> b;
		if (b)
		{
			in >> ins.mesh;
		}
		return in;
	}

	inline QDataStream& operator>>(QDataStream& in, GNInputSplinePointData& ins)
	{
		in >> ins.pos;
		in >> ins.startTangent;
		in >> ins.endTangent;
		return in;
	}

	inline QDataStream& operator>>(QDataStream& in, GNInputSplineData& ins)
	{
		in >> ins.points;
		return in;
	}

	inline QDataStream& operator>>(QDataStream& in, GNInputCurveData& ins)
	{
		in >> ins.name;
		in >> ins.transform;
		in >> ins.splines;
		return in;
	}

	inline QDataStream& operator>>(QDataStream& in, GNData& data)
	{
		in >> data.transform;
		in >> data.mesh;
		//in >> data.curves;
		in >> data.instances;
		return in;
	}

	// export

	template <typename T>
	inline QDataStream& operator<<(QDataStream& out, Unigine::Vector<T>& v)
	{
		out << v.size();
		for (auto i : v)
		{
			out << i;
		}
		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, Unigine::String& s)
	{
		out << s.get();
		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, Unigine::Math::vec3& vec)
	{
		out << vec.x;
		out << vec.y;
		out << vec.z;
		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, Unigine::Math::vec2& vec)
	{
		out << vec.x;
		out << (1.0f - vec.y);
		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, Unigine::Math::mat4& m)
	{
		out << m.m00; out << m.m10; out << m.m20; out << m.m30;
		out << m.m01; out << m.m11; out << m.m21; out << m.m31;
		out << m.m02; out << m.m12; out << m.m22; out << m.m32;
		out << m.m03; out << m.m13; out << m.m23; out << m.m33;
		return out;
	}


	inline QDataStream& operator<<(QDataStream& out, GNSurfaceData& surface)
	{
		out << surface.vertices;
		out << surface.Cindices;
		out << surface.Tindices;
		out << surface.normals;
		out << surface.uv0;
		out << surface.uv1;
		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, GNMaterialData& m)
	{
		out << m.name;
		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, GNMeshData& mesh)
	{
		if (mesh.isEmpty())
		{
			out << false;
		}
		else
		{
			out << true;
			out << mesh.name;
			out << mesh.surfaces;
			out << mesh.materials;
		}
		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, GNCurveData& curve)
	{
		out << curve.start;
		out << curve.startTangent;
		out << curve.end;
		out << curve.endTangent;
		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, GNData& data)
	{
		out << data.transform;
		out << data.mesh;
		out << data.instances;
		out << data.segments;
		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, GNInstanceData& ins)
	{
		out << ins.matrices;
		out << ins.mesh;
		return out;
	}

	inline QDataStream& operator<<(QDataStream& out, GNParam& p)
	{
		out << QString(p.id.get()).toUtf8();
		out << static_cast<int>(p.type);

		switch (p.type)
		{
		case GNParam::Type::VALUE:
			out << p.value.toFloat();
			break;
		case GNParam::Type::INT:
			out << (qint32)p.value.toInt();
			break;
		case GNParam::Type::BOOLEAN:
			out << p.value.toBool();
			break;
		case GNParam::Type::RGBA:
		case GNParam::Type::VECTOR:
		{
			QSequentialIterable it = p.value.value<QSequentialIterable>();
			for (const QVariant& v : it)
			{
				out << v.toFloat();
			}
		}
		break;
		case GNParam::Type::STRING:
			out << p.value.toString().toUtf8();
			break;
		case GNParam::Type::GEOMETRY:
		{
			Unigine::NodePtr node = Unigine::World::getNodeByID(p.value.toInt());
			out << getGeometryData(node);
		}
		break;
		case GNParam::Type::OBJECT:
		{
			Unigine::NodePtr node = Unigine::World::getNodeByID(p.value.toInt());
			out << getGeometryData(node, p.geoNode);
		}
		break;
		case GNParam::Type::COLLECTION:
		{
			Unigine::NodePtr node = Unigine::World::getNodeByID(p.value.toInt());
			Unigine::Vector<GNData> data;
			if (node)
			{
				Unigine::Vector<Unigine::NodePtr> children;
				//data.reserve(node->getNumChildren());
				node->getHierarchy(children);
				for (auto& n : children)
				{
					if (!n->isShowInEditorEnabled() || !n->isSaveToWorldEnabled()) { continue; }
					data.append(getGeometryData(n, p.geoNode, true));
				}
			}
			out << data;
		}
		break;
		case GNParam::Type::IMAGE:
		{
			Unigine::UGUID guid = Unigine::UGUID(p.value.toString().toUtf8().constData());
			auto path = Unigine::FileSystem::getAbsolutePath(guid);
			out << false;
			out << path;
		}
		break;
		}

		return out;
	}

	//static MeshPtr generateMeshOld(GNMeshData &mesh)
	//{
	//	if (mesh.material_names.size() == 0)
	//	{
	//		mesh.material_names.append("mesh_base");
	//	}
	//	MeshPtr m = Mesh::create();
	//	for (int surface = 0; surface < mesh.material_names.size(); surface++)
	//	{
	//		// vertex index -> cindex, apparently not the best solution
	//		HashMap<int, int> cv;
	//		int cIndex_counter = 0, tIndex_counter = 0;
	//		for (int ind_i = 0, ind_mat_i = 0; ind_i < mesh.Tindices.size(); ind_i += 3, ind_mat_i++)
	//		{
	//			if (mesh.materials[ind_mat_i] != surface)
	//			{
	//				continue;
	//			}
	//			auto vr = [&](int triangle_loop_i)
	//			{
	//				// CVertices
	//				int loop_index = 0; // mesh.tr_loops[triangle_loop_i];
	//				int vertex_index = 0; // mesh.loops[loop_index];
	//				int cIndex;
	//				if (cv.contains(vertex_index))
	//				{
	//					cIndex = cv.get(vertex_index);
	//				}
	//				else
	//				{
	//					m->addVertex(mesh.vertices[vertex_index], surface);
	//					cv.append(vertex_index, cIndex_counter);
	//					cIndex = cIndex_counter;
	//					cIndex_counter++;
	//				}
	//				m->addCIndex(cIndex, surface);
	//				// TVertices, TODO: duplicates
	//				m->addNormal(mesh.normals[triangle_loop_i], surface);
	//				vec2 uv = mesh.uv0[loop_index];
	//				m->addTexCoord0(vec2(uv.x, 1.0f - uv.y), surface);
	//				uv = mesh.uv1[loop_index];
	//				m->addTexCoord1(vec2(uv.x, 1.0f - uv.y), surface);
	//				m->addTIndex(tIndex_counter, surface);
	//				tIndex_counter++;
	//			};
	//			vr(ind_i);
	//			vr(ind_i + 1);
	//			vr(ind_i + 2);
	//			// too lame, duplicates
	//			/*int loop_index = mesh.tr_loops[ind_i];
	//				int vertex_index = mesh.loops[loop_index];*/
	//				/*m->addVertex(mesh.vertices[mesh.loops[mesh.tr_loops[ind_i]]], surface);
	//				m->addNormal(mesh.normals[ind_i], surface);
	//				m->addVertex(mesh.vertices[mesh.loops[mesh.tr_loops[ind_i + 1]]], surface);
	//				m->addNormal(mesh.normals[ind_i + 1], surface);
	//				m->addVertex(mesh.vertices[mesh.loops[mesh.tr_loops[ind_i + 2]]], surface);
	//				m->addNormal(mesh.normals[ind_i + 2], surface);*/
	//		}
	//		if (m->getNumVertex(surface) > 0)
	//		{
	//			//m->createIndices(surface);
	//			m->createTangents(surface);
	//			m->createBounds(surface);
	//		}
	//		cv.clear();
	//	}
	//	// vertex cache to be optimized later when exporting
	//	//m->optimizeIndices(Mesh::VERTEX_CACHE);
	//	return m;
	//}
} // namespace GeometryNodes