import bpy, sys, datetime, zlib
from multiprocessing import shared_memory, Process, Lock
import numpy as np
from struct import pack, unpack, calcsize
from io import BytesIO

socketTypes = ['CUSTOM', 'VALUE', 'INT', 'BOOLEAN', 'VECTOR',
'STRING', 'RGBA', 'SHADER', 'OBJECT', 'IMAGE',
'GEOMETRY', 'COLLECTION', 'TEXTURE', 'MATERIAL']

def getGeometryNodesObject():
	geoObj = None
	for obj in bpy.data.objects:
		for modifier in obj.modifiers:
			if modifier.type == 'NODES' and obj.visible_get():
				geoObj = obj
	return geoObj

def getParameters(obj):
	if obj is None:
		return 0

	GN = next(modifier for modifier in obj.modifiers if modifier.type == 'NODES')
	group = GN.node_group
	params = bytes()
	num = 1
	
	params += packString("Frame")
	params += packString("Current Frame")
	params += pack('<I', socketTypes.index("INT"))
	params += pack('<i', bpy.context.scene.frame_current)
	params += pack('<i', bpy.context.scene.frame_start)
	params += pack('<i', bpy.context.scene.frame_end)

	for input in group.inputs:

		if input.type in ['SHADER', 'TEXTURE', 'MATERIAL'] or input.type not in socketTypes:
			continue
		if input.hide_value:
			continue
		num += 1
		params += packString(input.identifier)
		params += packString(input.name)
		params += pack('<I', socketTypes.index(input.type))

		if input.type == 'VALUE':
			params += pack('<fff', input.default_value, input.min_value, input.max_value)
		if input.type == 'INT':
			params += pack('<iii', input.default_value, input.min_value, input.max_value)
		if input.type == 'BOOLEAN':
			params += pack('<?', input.default_value)
		if input.type == 'VECTOR':
			params += pack('<3fff', *input.default_value, input.min_value, input.max_value)
		if input.type == 'STRING':
			params += packString(input.default_value)
		if input.type == 'RGBA':
			params += pack('<4f', *input.default_value)
			# print(input.default_value[:], flush=True)

	return pack('<I', num) + params

def getMaterials(mesh : bpy.types.Mesh):
	mats = []
	for m in mesh.materials:
		n = []
		if m is not None:
			n.extend([m.name_full, True, m.blend_method, m.use_backface_culling])
			if m.use_nodes:
				output = m.node_tree.get_output_node('ALL')
				if output is not None:
					socket = output.inputs["Surface"]
					if socket.is_linked:
						shader = socket.links[0].from_node
						if isinstance(shader, bpy.types.ShaderNodeBsdfPrincipled):
							i = shader.inputs
							#  + [i['Alpha'].default_value]
							n.append(i['Base Color'].default_value[:])
							n.append(i['Metallic'].default_value)
							n.append(i['Roughness'].default_value)
							n.append(i['Specular'].default_value)
							mats.append(n)
							continue
			n.extend([m.diffuse_color[:], m.metallic, m.roughness, m.specular_intensity])
		else:
			n.extend(['Material', False])
		mats.append(n)
	return mats

def packString(s):
	b = s.encode()
	return pack('<I', len(b)) + b

def splitSurfaces(materials, min, max, coords, cindices, tindices, normals, uv0, uv1, vc):
	data = bytearray()
	coords = coords.reshape(-1, 3)
	cindices = cindices.reshape(-1, 3)
	tindices = tindices.reshape(-1, 3)
	normals = normals.reshape(-1, 3)
	uv0 = uv0.reshape(-1, 2)
	uv1 = uv1.reshape(-1, 2)
	vc = vc.reshape(-1, 4)
	
	for s in range(min, max):

		mat_mask = materials == s
		src_cindices = cindices[mat_mask].flat
		src_tindices = tindices[mat_mask].flat

		c_vals, s_cinds = np.unique(src_cindices, return_inverse=True)
		s_co = coords[c_vals]
		
		data.extend(pack('<I', s_co.shape[0]) + s_co.tobytes())
		data.extend(pack('<I', s_cinds.size) + s_cinds.astype('<I').tobytes())

		t_vals, s_tinds = np.unique(src_tindices, return_inverse=True)

		data.extend(pack('<I', s_tinds.size) + s_tinds.astype('<I').tobytes())

		s_n = normals[t_vals]
		data.extend(pack('<I', s_n.shape[0]) + s_n.tobytes())

		if uv0.size > 0:
			s_uv0 = uv0[t_vals]
			data.extend(pack('<I', s_uv0.shape[0]) + s_uv0.tobytes())
		else:
			data.extend(pack('<I', 0))

		if uv1.size > 0:
			s_uv1 = uv1[t_vals]
			data.extend(pack('<I', s_uv1.shape[0]) + s_uv1.tobytes())
		else:
			data.extend(pack('<I', 0))

		if vc.size > 0:
			s_vc = vc[t_vals]
			data.extend(pack('<I', s_vc.shape[0]) + s_vc.tobytes())
		else:
			data.extend(pack('<I', 0))
	return data

def getMeshData(mesh):

	mesh.calc_loop_triangles()
	mesh.calc_normals_split()

	n_tris = len(mesh.loop_triangles)

	if n_tris < 1:
		return pack('<?', False)
	
	mesh_bytes = bytearray()
	mesh_bytes += pack('<?', True)
	mesh_bytes.extend(packString(mesh.name))
	
	# coords
	n_verts = len(mesh.vertices)
	co_num = n_verts * 3
	co_bytes = bytes(co_num * 4)
	coords : np.ndarray = np.ndarray(co_num, buffer=co_bytes, dtype='<f')
	mesh.vertices.foreach_get('co', coords)

	# cindices (vertex indices)
	n_indices = n_tris * 3
	cindices_bytes = bytes(n_indices * 4)
	cindices = np.ndarray(n_indices, buffer=cindices_bytes, dtype='<I')
	mesh.loop_triangles.foreach_get('vertices', cindices)

	# tindices (face corner indices)
	tindices_bytes = bytes(n_indices * 4)
	tindices = np.ndarray(n_indices, buffer=tindices_bytes, dtype='<I')
	mesh.loop_triangles.foreach_get('loops', tindices)

	n_loops = len(mesh.loops)
	
	# normals
	normals_bytes = bytes(n_loops * 3 * 4)
	normals = np.ndarray(n_loops * 3, buffer=normals_bytes, dtype='<f')
	mesh.loops.foreach_get('normal', normals)

	# uvs
	attr_uv0 = mesh.attributes.get('UV0')
	if attr_uv0 and attr_uv0.domain == 'CORNER' and len(attr_uv0.data) > 0 and len(attr_uv0.data[0].vector) == 3:
		uv0 = np.empty(n_loops * 3, dtype=np.float32)
		attr_uv0.data.foreach_get('vector', uv0)
		uv0 = uv0.reshape(-1,3)
		uv0 = np.delete(uv0, 2, 1)
		uv0 = uv0.reshape(-1)
		uv0_bytes = uv0.tobytes()
	elif len(mesh.uv_layers) > 0:
		uv0_bytes = bytes(n_loops * 2 * 4)
		uv0 = np.ndarray(n_loops * 2, buffer=uv0_bytes, dtype='<f')
		mesh.uv_layers[0].data.foreach_get('uv', uv0)
	else:
		uv0 = np.array([])
		uv0_bytes = bytes()

	attr_uv1 = mesh.attributes.get('UV1')
	if attr_uv1 and attr_uv1.domain == 'CORNER' and len(attr_uv1.data) > 0 and len(attr_uv1.data[0].vector) == 3:
		uv1 = np.empty(n_loops * 3, dtype=np.float32)
		attr_uv1.data.foreach_get('vector', uv1)
		uv1 = uv1.reshape(-1,3)
		uv1 = np.delete(uv1, 2, 1)
		uv1 = uv1.reshape(-1)
		uv1_bytes = uv1.tobytes()
	elif len(mesh.uv_layers) > 1:
		uv1_bytes = bytes(n_loops * 2 * 4)
		uv1 = np.ndarray(n_loops * 2, buffer=uv1_bytes, dtype='<f')
		mesh.uv_layers[1].data.foreach_get('uv', uv1)
	else:
		uv1 = np.array([])
		uv1_bytes = bytes()

	if len(mesh.vertex_colors) > 0:
		vc_bytes = bytes(n_loops * 4 * 4)
		vc = np.ndarray(n_loops * 4, buffer=vc_bytes, dtype='<f')
		mesh.vertex_colors[0].data.foreach_get('color', vc)
	else:
		vc = np.array([])
		vc_bytes = bytes()

	material_indices = np.empty(n_tris, dtype='<I')
	mesh.loop_triangles.foreach_get('material_index', material_indices)

	max = material_indices.max() + 1
	min = material_indices.min()
	n_surfaces = max - min

	mesh_bytes.extend(pack('<I', n_surfaces))

	if n_surfaces > 1:
		mesh_bytes.extend(splitSurfaces(material_indices, min, max, coords, cindices, tindices, normals, uv0, uv1, vc))
	else:
		mesh_bytes.extend(pack('<I', n_verts) + co_bytes)
		mesh_bytes.extend(pack('<I', n_indices) + cindices_bytes)
		mesh_bytes.extend(pack('<I', n_indices) + tindices_bytes)
		mesh_bytes.extend(pack('<I', n_loops) + normals_bytes)
		mesh_bytes.extend(pack('<I', uv0.size // 2) + uv0_bytes)
		mesh_bytes.extend(pack('<I', uv1.size // 2) + uv1_bytes)
		mesh_bytes.extend(pack('<I', vc.size // 4) + vc_bytes)

	materials = getMaterials(mesh)[min:max]

	mesh_bytes.extend(pack('<I', len(materials)))
	for m in materials:
		# name
		mesh_bytes.extend(packString(m[0]))
		# not empty
		mesh_bytes.extend(pack('<?', m[1]))
		if m[1]:
			mesh_bytes.extend(packString(m[2]))
			mesh_bytes.extend(pack('<?', m[3]))
			mesh_bytes.extend(pack('<4f', *m[4]))
			mesh_bytes.extend(pack('<f', m[5]))
			mesh_bytes.extend(pack('<f', m[6]))
			mesh_bytes.extend(pack('<f', m[7]))

	return mesh_bytes

def getInstanceData(hsh, mesh, matrices, is_known = False):
	data = bytearray()
	data += pack('<q', hsh)
	data += pack('<I', len(matrices))
	for mat in matrices:
		matrix:np.ndarray = np.array(mat, dtype='<f').reshape(4,4)
		data += matrix.transpose().tobytes()
	data += pack('?', not is_known)
	if not is_known:
		data += getMeshData(mesh)
	return data

def exportGeometryData(geoObj, known_hashes = []):
	if geoObj is None:
		return b''
	depsgraph = bpy.context.evaluated_depsgraph_get()
	evaluated_obj = geoObj.evaluated_get(depsgraph)

	geometry_bytes = bytearray()

	matrix:np.ndarray = np.array(evaluated_obj.matrix_world, dtype='<f')
	geometry_bytes.extend(matrix.tobytes())

	# Write main mesh
	md = getMeshData(evaluated_obj.data) if evaluated_obj.type == 'MESH' else pack('<?', False)
	geometry_bytes.extend(md)

	mesh_hashes = []
	mesh_objects = []
	instance_matrices = []

	# Get instances and their meshes
	for instance in depsgraph.object_instances:
		if instance.instance_object and instance.object.type == 'MESH' and instance.parent and instance.parent.original == geoObj:
			hsh = hash(instance.object.original.data)
			if hsh not in mesh_hashes:
				mesh_hashes.append(hsh)
				mesh_objects.append(instance.object.data)
				instance_matrices.append([instance.matrix_world.copy()])
			else:
				instance_matrices[mesh_hashes.index(hsh)].append(instance.matrix_world.copy())

	# Write Instances
	geometry_bytes.extend(pack('<I', len(mesh_objects)))
	for hsh, mesh, matrices in zip(mesh_hashes, mesh_objects, instance_matrices):
		geometry_bytes.extend(getInstanceData(hsh, mesh, matrices, hsh in known_hashes))

	return geometry_bytes


# IMPORT

def read(stream, fmt):
	size = calcsize(fmt)
	buf = stream.read(size)
	if not buf or len(buf) < size:
		return None
	res = unpack(fmt, buf)
	return res if len(res) > 1 else res[0]

def readString(stream):
	num = read(stream, '<I')
	if num is None:
		return ""
	return stream.read(num).decode()

def readArray(s, t, modif = 1, add = 0):
	num = read(s, '<I')
	if modif is not 1:
		num = int(num//modif)
	arr = []
	for i in range(num):
		it = read(s, t)
		if add is not 0:
			if type(it) is tuple:
				it = [(x + add) for x in it]
			else: it += add
		arr.append(it)
	return arr

def createMesh(name, vt, cind, norms, mats, uv0, uv1, materials):
	# name = name + '_mesh'
	mesh = bpy.data.meshes.get(name)
    
	if mesh is None:
		mesh = bpy.data.meshes.new(name)
	
	mesh.clear_geometry()
	if len(materials) > 0:
		mesh.materials.clear()
		for matName in materials:
			m = bpy.data.materials.get(matName)
			if m is None:
				m = bpy.data.materials.new(matName)
			mesh.materials.append(m)
	
	mesh.from_pydata(vt, [], cind)
	mesh.use_auto_smooth = True
	if len(norms) > 0:
		mesh.normals_split_custom_set(norms)
	else:
		mesh.auto_smooth_angle = 10
	if len(uv0) > 0:
		mesh.uv_layers.new()
		if len(uv1) > 0:
			mesh.uv_layers.new()
	mesh.vertex_colors.new()
	if len(mats) > 0:
		loop_index = 0
		for i, face in enumerate(mesh.polygons):
			face.material_index = mats[i]
			if len(uv0) > 0:
				layer = mesh.uv_layers[0].data
				layer[loop_index].uv = uv0[loop_index]
				layer[loop_index+1].uv = uv0[loop_index+1]
				layer[loop_index+2].uv = uv0[loop_index+2]
			if len(mesh.uv_layers) > 1 and len(uv1) > 0:
				layer = mesh.uv_layers[1].data
				layer[loop_index].uv = uv1[loop_index]
				layer[loop_index+1].uv = uv1[loop_index+1]
				layer[loop_index+2].uv = uv1[loop_index+2]
			loop_index += 3
	# useful for development when the mesh may be invalid.
	# mesh.validate(verbose=True)
	return mesh

def createSpline(points, name):
	name = name + '_curve'
	curve = bpy.data.curves.get(name)
    
	if curve == None:
		curve = bpy.data.curves.new(name, type='CURVE')
	curve.splines.clear()
	curve.dimensions = '3D'
	# curve.resolution_u = 12
	for p0, t0, p1, t1 in points:
		spline = curve.splines.new('BEZIER')
		spline.use_cyclic_u = False
		spline.bezier_points.add(1)
		spline.bezier_points[0].co = p0
		spline.bezier_points[0].handle_right_type = 'FREE'
		spline.bezier_points[0].handle_left_type = 'FREE'
		spline.bezier_points[0].handle_right = t0
		spline.bezier_points[0].handle_left = p0
		spline.bezier_points[1].co = p1
		spline.bezier_points[1].handle_left_type = 'FREE'
		spline.bezier_points[1].handle_right_type = 'FREE'
		spline.bezier_points[1].handle_left = t1
		spline.bezier_points[1].handle_right = p1
	# print('curwa data %d' % (len(curve.splines)), flush=True)
	return curve

def readMeshData(s):
	isMeshAvailable = read(s, '<?')
	if isMeshAvailable:
		meshname = readString(s)
		numSurfaces = read(s, '<I')

		vertices = []
		cindices = []
		normals = []
		uv0 = []
		uv1 = []
		mats = []
		for i in range(numSurfaces):
			v = readArray(s, '<3f')
			cindices += readArray(s, '<3i', 3, len(vertices))
			ti = readArray(s, '<i')
			vertices += v
			n = readArray(s, '<3f')
			u0 = readArray(s, '<2f')
			u1 = readArray(s, '<2f')
			normals += [n[x] for x in ti] if len(n) > 0 else ([(0,0,0)] * len(ti))
			uv0 += [u0[x] for x in ti] if len(u0) > 0 else ([(0,0)] * len(ti))
			uv1 += [u1[x] for x in ti] if len(u1) > 0 else ([(0,0)] * len(ti))
			mats += [i] * (len(ti) // 3)
		
		materials = []
		numMaterials = read(s, '<I')
		for i in range(numMaterials):
			materials.append(readString(s))
		
		if numSurfaces > 0:
			return createMesh(meshname, vertices, cindices, normals, mats, uv0, uv1, materials)
	return None

def importGeometryData(s, name, geoObj = None):
	data = None
	transform = np.array(read(s, '<16f'), np.float32)
	transform = np.reshape(transform, (4, 4)).tolist()
	
	data = readMeshData(s)
	
	numInstances = read(s, '<I')
	if numInstances > 0:
		transforms = readArray(s, '<16f')
		mesh = readMeshData(s)
		name += '_coll'
		data = bpy.data.collections.new(name)
		for i, tr in enumerate(transforms):
			objname = name + '_obj_' + str(i)
			obj = bpy.data.objects.get(objname)
			if obj is None:
				obj = bpy.data.objects.new(objname, mesh)
			else: obj.data = mesh
			if obj is not None:
				tr = np.array(tr, np.float32)
				obj.matrix_world = np.reshape(tr, (4, 4)).tolist()
				if data not in obj.users_collection:
					data.objects.link(obj)

	curves = readArray(s, '<3f', 0.25)
	# print('spline got %d' % curves, flush=True)
	if len(curves) > 0:
		p = iter(curves)
		data = createSpline(zip(p, p, p, p), name)
	
	if isinstance(data, bpy.types.Collection):
		return data

	if geoObj is None:
		geoObj = bpy.data.objects.get(name)
	if geoObj is None:
		geoObj = bpy.data.objects.new(name, data)
	elif data is not None:
		oldata = geoObj.data
		geoObj.data = data
		if oldata is not data:
			if isinstance(oldata, bpy.types.Mesh):
				bpy.data.meshes.remove(oldata)
			elif isinstance(oldata, bpy.types.Curve):
				bpy.data.curves.remove(oldata)

	geoObj.matrix_world = transform
	return geoObj

def updateParameters(geoObj, b):
	
	updateHashedInstances = False

	num = read(b, '<I')
	if num is None: return False

	bpy.context.view_layer.objects.active = geoObj
	GN = next(modifier for modifier in bpy.context.object.modifiers if modifier.type == 'NODES')

	for p in range(num):
		id: str = readString(b)
		type = read(b, '<I')
		if type not in range(len(socketTypes)):
			continue
		typename = socketTypes[type]
		if typename == 'VECTOR':
			x = read(b, '<f')
			y = read(b, '<f')
			z = read(b, '<f')
			GN[id][:] = [x, y, z]
		elif typename == 'INT':
			if id.startswith("Frame"):
				bpy.context.scene.frame_set(bpy.context.scene.frame_start + (read(b, '<i') % (bpy.context.scene.frame_end - bpy.context.scene.frame_start)))
			else:
				GN[id] = read(b, '<i')
		elif typename == 'VALUE':
			GN[id] = read(b, '<f')
		elif typename == 'BOOLEAN':
			GN[id] = read(b, '<?')
		elif typename == 'STRING':
			GN[id] = readString(b)
		if typename == 'RGBA':
			c = read(b, '<4f')
			GN[id][:] = c[:]
		elif typename == 'GEOMETRY':
			name = 'bgn_%s_%s' % (typename, id)
			importGeometryData(b, name, geoObj)
			# TODO: if collection create a collection instance?
			updateHashedInstances = True
		elif typename == 'OBJECT':
			name = 'bgn_%s_%s' % (typename, id)
			obj = importGeometryData(b, name, GN[id])
			if obj is not None and isinstance(obj, bpy.types.Object):
				GN[id] = obj
				updateHashedInstances = True
		elif typename == 'COLLECTION':
			numCh = read(b, '<I')
			# print("num children %d" % numCh)
			if numCh > 0:
				name = 'bgn_%s_%s' % (typename, id)
				coll = bpy.data.collections.get(name)
				if coll is None:
					coll = bpy.data.collections.new(name)
					bpy.context.scene.collection.children.link(coll)
				else:
					for linked in coll.objects:
						coll.objects.unlink(linked)
						bpy.data.objects.remove(linked)
					for child in coll.children:
						coll.children.unlink(child)
						bpy.data.collections.remove(child)
				for i in range(numCh):
					nameObj = name + '_obj_' + str(i)
					obj = importGeometryData(b, nameObj)
					if isinstance(obj, bpy.types.Object):
						if obj is not None and coll not in obj.users_collection:
							coll.objects.link(obj)
					elif isinstance(obj, bpy.types.Collection):
						coll.children.link(obj)
				GN[id] = coll
		elif typename == 'IMAGE':
			isInternal = read(b, '<?')
			if isInternal:
				pass
			else:
				img = bpy.data.images.load(readString(b), check_existing=True)
				if img is not None:
					GN[id] = img

	for obj in bpy.data.objects:
		if obj.data and obj.type == "MESH":
			obj.data.update()
		if obj.data and obj.type == "CURVE":
			obj.data.resolution_u = obj.data.resolution_u

	return updateHashedInstances

def readKnownHashes(stream : BytesIO):
	num = read(stream, '<I')
	if num is None: return []
	h = []
	for i in range(num):
		h.append(read(stream, '<q'))
	return h

def main():
	# practice shows shared memory is not faster than pipe and limited
	# try:
	# 	index = sys.argv.index("--") + 1
	# except ValueError:
	# 	print("no shared memory key", flush=True)
	# 	sys.exit()
	# shmemKey = sys.argv[index]
	# shm = shared_memory.SharedMemory(name=shmemKey)
	# buf = shm.buf
	if (3, 0, 0) > bpy.app.version:
		print("Your Blender version is too old!", flush=True)
		sys.exit()
	geoObj = getGeometryNodesObject()
	if not geoObj:
		print("the project doesn't contain an obj with a geometry nodes modifier", flush=True)
		sys.exit()
	print('BGN_READY', flush=True)
	cmd = ""
	stream = sys.stdin.buffer
	while cmd != 'exit':
		cmd = stream.read(4).decode()
		if cmd == 'exit':
			break
		elif cmd == 'pars':
			try:
				sys.stdout.buffer.write('BGN_PARAM'.encode("latin_1") + getParameters(geoObj))
				# pr = getParameters(geoObj)
				# buf[:len(pr)] = pr
				# sys.stdout.buffer.write('BGN_PARAM'.encode("latin_1"))
				sys.stdout.flush()
			except Exception as e:
				print(e, flush=True)
		elif cmd == 'mesh':
			knownHashes = []
			try:
				knownHashes = readKnownHashes(stream)
				if updateParameters(geoObj, stream):
					knownHashes = []
			except Exception as e:
				print('params update failed: ', e, flush=True)
				continue
			try:
				meshes = exportGeometryData(geoObj, knownHashes)

				# c_m = zlib.compress(meshes)
				# with open('D:/res.bt', 'wb') as file2:
					# file2.write(meshes)
				# print("len meshes %d %d" % (len(meshes), len(c_m)), flush=True)

				# buf[: min(len(meshes), shm.size)] = bytes(meshes)
				# sys.stdout.buffer.write('BGN_MESHS'.encode("latin_1"))
				sys.stdout.buffer.write('BGN_MESHS'.encode("latin_1") + meshes)
				sys.stdout.flush()
			except Exception as e:
				print('mesh export failed: ', e, flush=True)
				continue
		else:
			print('unknown command: ', cmd, flush=True)
	
	# shm.close()

if __name__ == "__main__":
	main()