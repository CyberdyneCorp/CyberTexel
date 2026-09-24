import bpy, sys, numpy as np
argv = sys.argv[sys.argv.index("--") + 1:]
src, dst = argv[0], argv[1]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=src)
meshes = [o for o in bpy.data.objects if o.type == "MESH"]
print("objects:", [(o.name, len(o.data.polygons)) for o in meshes])
out = {}
for i, obj in enumerate(meshes):
    me = obj.data
    me.calc_loop_triangles()
    try: me.calc_normals_split()
    except Exception: pass
    verts = np.array([v.co[:] for v in me.vertices], dtype=np.float32)
    tris = np.array([t.vertices[:] for t in me.loop_triangles], dtype=np.uint32)
    uvlayer = me.uv_layers.active
    print(f"  {obj.name}: verts={len(verts)} tris={len(tris)} uv={'yes' if uvlayer else 'NO'}")
    # UVs live per loop; build per-vertex UV by taking the first loop that uses it.
    uv = np.zeros((len(verts), 2), dtype=np.float32)
    nrm = np.zeros((len(verts), 3), dtype=np.float32)
    if uvlayer:
        for poly in me.polygons:
            for li in poly.loop_indices:
                vi = me.loops[li].vertex_index
                uv[vi] = uvlayer.data[li].uv[:]
    for v in me.vertices:
        nrm[v.index] = v.normal[:]
    out[f"pos{i}"] = verts; out[f"nrm{i}"] = nrm; out[f"uv{i}"] = uv; out[f"tri{i}"] = tris
    out[f"name{i}"] = np.array(obj.name)
out["count"] = np.array(len(meshes))
np.savez(dst, **out)
print("saved", dst)
