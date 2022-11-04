# Geometry Nodes Plugin for UNIGINE

**WORK-IN-PROGRESS**

[![UNIGINE 2.15.1](https://img.shields.io/badge/UNIGINE-2.15.1-green.svg)](https://developer.unigine.com/en/docs/2.15.1/)

![Geometry Nodes Plugin](/assets/title.jpg)

A proof of concept for a plugin for [UNIGINE Editor](https://unigine.com/get-unigine/) utilizing Blender Geometry Nodes functionality for procedural geometry.

The plugin establishes a live link with Blender used as back-end for mesh generation, the [Geometry Nodes](https://docs.blender.org/manual/en/latest/modeling/geometry_nodes/introduction.html) feature is the core of this workflow. Seems like a way of creating custom tools for geometry (generators and processors) without saving to FBX as an intermediate step.

Preview on Youtube:
https://youtu.be/irP0aW7qyJA

> **Note**. It is an **editor-only** feature that can be used for simpler level design. Currently, there is no possible way to introduce such procedural geometry to realtime applications based on UNIGINE, unless you want to run Blender in background, which is insane.
>
> It can be possibly achieved via baking (to VATs or custom format), this is open for discussion.

### Requirements
* [UNIGINE 2.15.1](https://unigine.com/get-unigine/) (any SDK edition);
* [Blender 3.0+](https://blender.org).

## Install

**Download here: [v0.0.2](https://github.com/thomalex0/unigine-geometry-nodes/releases/latest)**

You only need to place the content of the `bin` folder of this repo to the `bin` folder of your project based on UNIGINE 2.15.1. Only **Float** projects are supported at the moment.

![copy files to the bin folder](/assets/copy.jpg)

Now you can launch the editor and check if the plugin is loaded by opening the **Tools -> Geometry Nodes** window.

## How to use

> **Note**. The code quality doesn't represent that of UNIGINE, the system is unstable and needs to be heavily refactored. Please do not fully rely on the solutions presented here, of course it's not recommended for production use at all.

### Configuring

By default, the plugin tries to find Blender at the default installation path (*C:/Program Files/Blender Foundation/Blender 3.X/blender.exe*). If you have it somewhere else, specify the path to it by typing `bgn_blender_locate` in the console.

You can also provide the path manually by using the `bgn_blender_path` console command. If you have path to Blender in your %PATH% environment variable, you can simply type `blender`.

### Using

To start a Blender session from UNIGINE Editor, assign the **BlendAsset** property to any node in your world. You can do it via the `Geometry Nodes Plugin` window available under the *Tools* menu.

Then assign a *\*.blend* asset in the property parameters.

> * All children of the node will be deleted.
> * Each `BlendAsset` runs a separate Blender instance.
> * Currently, only imported blends are supported.
> * Only a **single** object with a **Geometry Nodes modifier** will be used.
> * First two UV Maps, as well as `UV0` and `UV1` face corner attributes, are converted to the 1st and 2nd UV channels.

If everything's fine, you'll see new generated child nodes:

* Mesh data becomes a **Mesh Static** object;
* Instances become **Mesh Cluster** objects;
* Other data types are not supported (yet).

These objects will have no mesh assets associated as their geometry is generated on the fly. See [how to save meshes](#saving-progress).

### Parameters

The parameters of the node group are exposed in the `Geometry Nodes Plugin` window. All parameter types are supported, except the *Material*, *Shader*, and *Texture* ones.

#### **Object parameters**

You can use any nodes for *Object* parameters:

* Mesh data is exported from mesh node types:
    - Mesh Static
    - Mesh Dynamic
    - Mesh Skinned
    - Mesh Cluster
    - Mesh Clutter
    - Gui Mesh
    - World Occluder Mesh
    - Mesh Decal
    - Water Mesh
    - Navigation Mesh
    - Landscape Layer Map (converted to a grid of a fixed resolution)
* Curve data is exported from WorldSpline Graph nodes.
* Other node types will be treated as dummy objects that have only transform.
> **Note**. World Spline Graphs are actually graphs unlike curves in Blender. That means that each segment will be converted to a separate curve. This limitation may change in the future.

#### **Geometry parameters**

The same as *Object* but the current node (with the property assigned) will be used for the first *Geometry* parameter. If it has no geometry, the default geometry from Blender project is used.

#### **Collection parameters**

Use grouped nodes as collections (drop the parent node) or use Mesh Clutter/Cluster nodes.

#### **Image parameters**

Use texture assets. The UNIGINE built-in *.texture* format is not supported by Blender.

### Saving progress

By default, child nodes of the current node are created. They have no mesh assets assigned, their geometry lies only in memory.

Simply save the world and all needed *.mesh* assets will be created in a *.blend* asset's subfolder.

At that, the previously used *.mesh* assets will be deleted only if **Clear Unused Meshes** is enabled.

You can simply disable the `BlendAsset` property to disable logic and keep the intermediate result that can be changed later. Make sure that you saved the world first (to save actual meshes) and then disabled the property. When the property or the node with it is disabled, the background Blender instance is also shut down.

If you are satisfied with the result, you can simply delete the property from the node. Later, when you assign the property with the same *.blend* asset, all geometry nodes parameters will be restored for convenience.

<!-- > **Note**. It's not recommended to delete nodes with the `BlendAsset` property assigned as the plugin won't clear subscriptions and the editor will crash on exit. Instead, it's better to delete the property first. This is actually not a problem, anyway, it is just a prototype. -->

### Update Existing Nodes

Enable the `Advanced` flag to override geometry output to the specified existing nodes.

There are 3 modes for instances:
* **Update Node** - overwrite the mesh (for [mesh-based nodes](#object-parameters)) and/or instance transformations (for Mesh Clusters) for the specified node. Here you can also opt in generation of Box collision shapes.
* **Clone Node** - clone the specified node with transformations of instances.
* **Spawn Node Reference** - spawn the specified node reference with transformations of instances.

Writing to Landscape Layer Maps is not supported (yet).

### Animation

Use the `Playback` setting to switch between the static/single frame/animation modes.

The animation is only for preview as it's not realtime. However, you can enable the `Synchronous` option to force wait for each frame update, for example, if you want to grab a video sequence.

Geometry for instances is cached when obtained for the first time. Enable `Dynamic Instances` if instances should have geometry changing each frame.

## Limitations and peculiarities

* Editor-only.
* Only one object with **Geometry Nodes** modifier is supported.
* If your node group expects input curve geometry and outputs meshes and/or instances, there may be strange buggy results. May be fixed in the future. Currently, it's just easier to assign the node group to a mesh object and refer to a curve via an *Object* parameter.
* Instances of Mesh Clusters/Clutters are supported only as collections.
* By default, the plugin tracks changes in transformation of nodes specified for *Object* and *Collection* parameters by creating a special invisible NodeTrigger. If you clone such a node, you'll see that the copy has an unwanted child trigger (safe to delete). If you don't want such behaviour, set the `bgn_track_transforms` console value to **0** by sacrificing the auto transform tracking.

## Roadmap

* Video tutorials, if needed;
* 2.16 support;
* Refactoring and stabilizing;
* Speedup data exchange;
* Implement writing to Landscape Layer Maps;
* Retrieving spline data from Blender
* Resolution settings for Landscape Layer Maps and curves;
* External *.blend* files;
* Textures import (procedural as well);
* Convenient config UI;
* Support for launching Blender in the normal mode;
* Finding appropriate use cases;
* Figure out ways to support realtime apps (VATs, alembic, custom logic?);
* What else?

## License
[MIT](LICENSE).

## 

For updates see [twitter](https://twitter.com/alexfomenk0) or UNIGINE Discord channel.

And, yeah:

![ACKCHYUALLY You can do this without addons](/assets/gigachad.png)

### **Any feedback is much appreciated!**