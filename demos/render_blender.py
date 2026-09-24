import bpy, sys, math
argv = sys.argv[sys.argv.index("--") + 1:]
fbx, tex, out = argv[0], argv[1], argv[2]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=fbx)
obj = next(o for o in bpy.data.objects if o.type == "MESH")

mat = bpy.data.materials.new("Painted"); mat.use_nodes = True
nt = mat.node_tree
bsdf = nt.nodes["Principled BSDF"]
img = nt.nodes.new("ShaderNodeTexImage")
img.image = bpy.data.images.load(tex)
img.interpolation = "Closest"
nt.links.new(bsdf.inputs["Base Color"], img.outputs["Color"])
bsdf.inputs["Roughness"].default_value = 0.85
obj.data.materials.clear(); obj.data.materials.append(mat)

# Frame the object
bpy.ops.object.select_all(action="DESELECT"); obj.select_set(True)
scene = bpy.context.scene
scene.render.engine = "BLENDER_EEVEE"
scene.render.resolution_x, scene.render.resolution_y = 1100, 800
scene.render.film_transparent = False
world = bpy.data.worlds.new("W"); scene.world = world
world.use_nodes = True
world.node_tree.nodes["Background"].inputs[0].default_value = (0.06, 0.07, 0.09, 1)
world.node_tree.nodes["Background"].inputs[1].default_value = 1.2

cam_data = bpy.data.cameras.new("Cam"); cam = bpy.data.objects.new("Cam", cam_data)
scene.collection.objects.link(cam); scene.camera = cam
cam.location = (2.4, -2.6, 1.15); cam.rotation_euler = (math.radians(74), 0, math.radians(43))
cam_data.lens = 52

key = bpy.data.objects.new("Key", bpy.data.lights.new("Key", type="AREA"))
scene.collection.objects.link(key)
key.data.energy, key.data.size = 320, 4.0
key.location = (3.0, -2.6, 3.4); key.rotation_euler = (math.radians(48), 0, math.radians(48))
fill = bpy.data.objects.new("Fill", bpy.data.lights.new("Fill", type="AREA"))
scene.collection.objects.link(fill)
fill.data.energy, fill.data.size = 90, 5.0
fill.location = (-3.2, -1.6, 1.6); fill.rotation_euler = (math.radians(80), 0, math.radians(-60))

scene.render.filepath = out
bpy.ops.render.render(write_still=True)
print("rendered", out)
