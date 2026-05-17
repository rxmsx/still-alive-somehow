# World.gd - Chunk-Terrain und Welt-Management
extends Node3D

const WORLD_COLLISION_LAYER := 1
const RESOURCE_COLLISION_LAYER := 2

const CHUNK_SIZE := 64.0
const CHUNK_STEPS := 24
const CHUNK_VIEW_DISTANCE := 2
const CHUNK_UPDATE_INTERVAL := 0.35
const TERRAIN_HEIGHT := 6.5

const SPAWN_CLEAR_RADIUS := 28.0
const SPAWN_BLEND_RADIUS := 64.0
const SPAWN_POSITION := Vector2(0.0, 0.0)
const PLAYER_SPAWN_HEIGHT := 2.0
const RESOURCE_MIN_SLOPE_NORMAL_Y := 0.72

const BIOME_MEADOW := "meadow"
const BIOME_FOREST := "forest"
const BIOME_ROCKY := "rocky"
const BIOME_DRY := "dry"
const BIOME_SNOW := "snow"

var rng := RandomNumberGenerator.new()
var world_seed := 0
var chunk_update_time := 0.0
var current_player_chunk := Vector2i(999999, 999999)
var chunks := {}
var harvested_resource_ids := {}

var chunks_node: Node3D
var starter_node: Node3D
var terrain_material: StandardMaterial3D

var terrain_noise := FastNoiseLite.new()
var terrain_detail_noise := FastNoiseLite.new()
var moisture_noise := FastNoiseLite.new()
var temperature_noise := FastNoiseLite.new()
var mountain_noise := FastNoiseLite.new()

func _ready():
	rng.randomize()
	world_seed = rng.randi()
	setup_environment()
	setup_terrain_noise()
	setup_dynamic_world()
	update_chunks_around(SPAWN_POSITION, true)
	spawn_starter_resources()
	call_deferred("place_player_at_safe_spawn")
	print("Unendliche Chunk-Welt geladen, Seed: ", world_seed)

func _process(delta):
	chunk_update_time -= delta
	if chunk_update_time > 0.0:
		return

	chunk_update_time = CHUNK_UPDATE_INTERVAL
	var player = get_parent().find_child("Player", true, false)
	if player:
		update_chunks_around(Vector2(player.global_position.x, player.global_position.z))

func setup_environment():
	var sun = get_parent().find_child("DirectionalLight3D", true, false)
	if sun and sun is DirectionalLight3D:
		sun.light_energy = 1.45
		sun.shadow_enabled = true
		sun.rotation_degrees = Vector3(-50, 35, 0)

	var world_environment = get_parent().find_child("WorldEnvironment", true, false)
	if world_environment and world_environment is WorldEnvironment:
		var environment := Environment.new()
		environment.background_mode = Environment.BG_COLOR
		environment.background_color = Color(0.55, 0.72, 0.92, 1)
		environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
		environment.ambient_light_color = Color(0.58, 0.64, 0.70, 1)
		environment.ambient_light_energy = 0.75
		world_environment.environment = environment

func setup_terrain_noise():
	configure_noise(terrain_noise, world_seed + 11, 0.010, 5, 0.48)
	configure_noise(terrain_detail_noise, world_seed + 23, 0.055, 3, 0.42)
	configure_noise(moisture_noise, world_seed + 37, 0.0045, 4, 0.52)
	configure_noise(temperature_noise, world_seed + 53, 0.0038, 4, 0.50)
	configure_noise(mountain_noise, world_seed + 71, 0.0065, 5, 0.54)

func configure_noise(noise: FastNoiseLite, seed_value: int, frequency: float, octaves: int, gain: float):
	noise.seed = seed_value
	noise.noise_type = FastNoiseLite.TYPE_SIMPLEX
	noise.frequency = frequency
	noise.fractal_octaves = octaves
	noise.fractal_lacunarity = 2.0
	noise.fractal_gain = gain

func setup_dynamic_world():
	terrain_material = create_terrain_material()

	var old_terrain = find_child("Terrain", true, false)
	if old_terrain and old_terrain is MeshInstance3D:
		old_terrain.visible = false
		old_terrain.mesh = null

	var old_terrain_collider = find_child("TerrainCollider", true, false)
	if old_terrain_collider and old_terrain_collider is CollisionObject3D:
		old_terrain_collider.collision_layer = 0
		old_terrain_collider.collision_mask = 0
		var old_shape = old_terrain_collider.find_child("CollisionShape3D", true, false)
		if old_shape and old_shape is CollisionShape3D:
			old_shape.shape = null

	var old_trees = find_child("Trees", true, false)
	if old_trees:
		clear_children(old_trees)

	var old_rocks = find_child("Rocks", true, false)
	if old_rocks:
		clear_children(old_rocks)

	chunks_node = find_child("Chunks", true, false)
	if not chunks_node:
		chunks_node = Node3D.new()
		chunks_node.name = "Chunks"
		add_child(chunks_node)
	clear_children(chunks_node)
	chunks.clear()

	starter_node = find_child("StarterResources", true, false)
	if not starter_node:
		starter_node = Node3D.new()
		starter_node.name = "StarterResources"
		add_child(starter_node)
	clear_children(starter_node)

func update_chunks_around(world_position: Vector2, force := false):
	var center_chunk := get_chunk_coord(world_position)
	if not force and center_chunk == current_player_chunk:
		return

	current_player_chunk = center_chunk
	var needed := {}

	for z_offset in range(-CHUNK_VIEW_DISTANCE, CHUNK_VIEW_DISTANCE + 1):
		for x_offset in range(-CHUNK_VIEW_DISTANCE, CHUNK_VIEW_DISTANCE + 1):
			var coord := Vector2i(center_chunk.x + x_offset, center_chunk.y + z_offset)
			needed[coord] = true
			if not chunks.has(coord):
				chunks[coord] = create_chunk(coord)

	for coord in chunks.keys():
		if not needed.has(coord):
			var chunk = chunks[coord]
			if chunk and is_instance_valid(chunk):
				chunk.queue_free()
			chunks.erase(coord)

func get_chunk_coord(world_position: Vector2) -> Vector2i:
	return Vector2i(floori(world_position.x / CHUNK_SIZE), floori(world_position.y / CHUNK_SIZE))

func create_chunk(coord: Vector2i) -> Node3D:
	var chunk := Node3D.new()
	chunk.name = "Chunk_%d_%d" % [coord.x, coord.y]
	chunk.position = Vector3(coord.x * CHUNK_SIZE, 0.0, coord.y * CHUNK_SIZE)
	chunks_node.add_child(chunk)

	var terrain_body := StaticBody3D.new()
	terrain_body.name = "TerrainBody"
	terrain_body.collision_layer = WORLD_COLLISION_LAYER
	terrain_body.collision_mask = 0
	chunk.add_child(terrain_body)

	var mesh := build_chunk_mesh(coord)
	var mesh_instance := MeshInstance3D.new()
	mesh_instance.name = "TerrainMesh"
	mesh_instance.mesh = mesh
	mesh_instance.set_surface_override_material(0, terrain_material)
	terrain_body.add_child(mesh_instance)

	var collision_shape := CollisionShape3D.new()
	collision_shape.name = "TerrainCollision"
	var terrain_shape := mesh.create_trimesh_shape()
	if terrain_shape is ConcavePolygonShape3D:
		terrain_shape.backface_collision = true
	collision_shape.shape = terrain_shape
	terrain_body.add_child(collision_shape)

	spawn_chunk_resources(chunk, coord)
	return chunk

func build_chunk_mesh(coord: Vector2i) -> ArrayMesh:
	var mesh := ArrayMesh.new()
	var vertices := PackedVector3Array()
	var normals := PackedVector3Array()
	var colors := PackedColorArray()
	var uvs := PackedVector2Array()
	var indices := PackedInt32Array()
	var cell_size := CHUNK_SIZE / CHUNK_STEPS
	var origin_x := coord.x * CHUNK_SIZE
	var origin_z := coord.y * CHUNK_SIZE

	for z_index in range(CHUNK_STEPS + 1):
		for x_index in range(CHUNK_STEPS + 1):
			var local_x := x_index * cell_size
			var local_z := z_index * cell_size
			var world_x := origin_x + local_x
			var world_z := origin_z + local_z
			var height := calculate_terrain_height(world_x, world_z)

			vertices.append(Vector3(local_x, height, local_z))
			normals.append(get_terrain_normal(world_x, world_z, cell_size))
			colors.append(get_terrain_vertex_color(world_x, world_z, height))
			uvs.append(Vector2(float(x_index) / CHUNK_STEPS, float(z_index) / CHUNK_STEPS) * 4.0)

	for z_index in range(CHUNK_STEPS):
		for x_index in range(CHUNK_STEPS):
			var top_left := z_index * (CHUNK_STEPS + 1) + x_index
			var top_right := top_left + 1
			var bottom_left := top_left + CHUNK_STEPS + 1
			var bottom_right := bottom_left + 1

			indices.append(top_left)
			indices.append(bottom_left)
			indices.append(top_right)
			indices.append(top_right)
			indices.append(bottom_left)
			indices.append(bottom_right)

	var arrays := []
	arrays.resize(Mesh.ARRAY_MAX)
	arrays[Mesh.ARRAY_VERTEX] = vertices
	arrays[Mesh.ARRAY_NORMAL] = normals
	arrays[Mesh.ARRAY_COLOR] = colors
	arrays[Mesh.ARRAY_TEX_UV] = uvs
	arrays[Mesh.ARRAY_INDEX] = indices
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)

	return mesh

func create_terrain_material() -> StandardMaterial3D:
	var material := StandardMaterial3D.new()
	material.albedo_color = Color(1, 1, 1, 1)
	material.vertex_color_use_as_albedo = true
	material.cull_mode = BaseMaterial3D.CULL_DISABLED
	material.roughness = 0.96
	material.metallic = 0.0
	return material

func get_biome(x: float, z: float) -> String:
	if Vector2(x, z).length() <= SPAWN_BLEND_RADIUS:
		return BIOME_MEADOW

	var moisture := moisture_noise.get_noise_2d(x, z)
	var temperature := temperature_noise.get_noise_2d(x, z)
	var mountain := mountain_noise.get_noise_2d(x, z)

	if temperature < -0.30:
		return BIOME_SNOW
	if mountain > 0.38:
		return BIOME_ROCKY
	if moisture < -0.34 and temperature > -0.05:
		return BIOME_DRY
	if moisture > 0.16:
		return BIOME_FOREST

	return BIOME_MEADOW

func calculate_terrain_height(x: float, z: float) -> float:
	var distance_from_start := Vector2(x, z).length()
	if distance_from_start <= SPAWN_CLEAR_RADIUS:
		return 0.0

	var biome := get_biome(x, z)
	var broad_hills := terrain_noise.get_noise_2d(x, z)
	var detail := terrain_detail_noise.get_noise_2d(x, z)
	var mountain := mountain_noise.get_noise_2d(x, z)
	var rolling_land := (sin(x * 0.022) + cos(z * 0.019) + sin((x - z) * 0.014)) * 0.45
	var height := 0.0

	match biome:
		BIOME_FOREST:
			height = broad_hills * TERRAIN_HEIGHT * 0.62 + detail * 1.2 + rolling_land
		BIOME_ROCKY:
			height = broad_hills * TERRAIN_HEIGHT * 1.05 + abs(mountain) * 5.5 + detail * 1.6
		BIOME_DRY:
			height = broad_hills * TERRAIN_HEIGHT * 0.52 + detail * 0.75 + rolling_land * 0.65
		BIOME_SNOW:
			height = broad_hills * TERRAIN_HEIGHT * 0.85 + abs(mountain) * 3.5 + detail * 0.8
		_:
			height = broad_hills * TERRAIN_HEIGHT * 0.42 + detail * 0.65 + rolling_land * 0.55

	var spawn_blend := smoothstep(SPAWN_CLEAR_RADIUS, SPAWN_BLEND_RADIUS, distance_from_start)
	return height * spawn_blend

func get_terrain_vertex_color(x: float, z: float, height: float) -> Color:
	if Vector2(x, z).length() <= SPAWN_CLEAR_RADIUS:
		return Color(0.34, 0.52, 0.25, 1)

	var biome := get_biome(x, z)
	match biome:
		BIOME_FOREST:
			return Color(0.12, 0.38, 0.16, 1) if height < 3.5 else Color(0.24, 0.35, 0.20, 1)
		BIOME_ROCKY:
			return Color(0.43, 0.44, 0.40, 1) if height < 6.0 else Color(0.54, 0.55, 0.53, 1)
		BIOME_DRY:
			return Color(0.54, 0.47, 0.27, 1) if height < 2.5 else Color(0.45, 0.40, 0.28, 1)
		BIOME_SNOW:
			return Color(0.78, 0.84, 0.82, 1) if height > 2.0 else Color(0.44, 0.55, 0.49, 1)
		_:
			return Color(0.25, 0.46, 0.23, 1) if height < 2.8 else Color(0.34, 0.45, 0.28, 1)

func get_terrain_height(x: float, z: float) -> float:
	return calculate_terrain_height(x, z)

func get_terrain_normal(x: float, z: float, sample_distance: float) -> Vector3:
	var left := get_terrain_height(x - sample_distance, z)
	var right := get_terrain_height(x + sample_distance, z)
	var back := get_terrain_height(x, z - sample_distance)
	var forward := get_terrain_height(x, z + sample_distance)

	return Vector3(left - right, sample_distance * 2.0, back - forward).normalized()

func spawn_chunk_resources(chunk: Node3D, coord: Vector2i):
	var resource_rng := RandomNumberGenerator.new()
	resource_rng.seed = get_chunk_seed(coord, 101)
	var resources_node := Node3D.new()
	resources_node.name = "Resources"
	chunk.add_child(resources_node)

	var biome := get_biome((coord.x + 0.5) * CHUNK_SIZE, (coord.y + 0.5) * CHUNK_SIZE)
	var counts := get_biome_resource_counts(biome)

	spawn_resources_of_type(resources_node, coord, resource_rng, biome, "tree", int(counts.get("tree", 0)))
	spawn_resources_of_type(resources_node, coord, resource_rng, biome, "rock", int(counts.get("rock", 0)))
	spawn_resources_of_type(resources_node, coord, resource_rng, biome, "bush", int(counts.get("bush", 0)))
	spawn_resources_of_type(resources_node, coord, resource_rng, biome, "log", int(counts.get("log", 0)))
	spawn_resources_of_type(resources_node, coord, resource_rng, biome, "stump", int(counts.get("stump", 0)))

func get_biome_resource_counts(biome: String) -> Dictionary:
	match biome:
		BIOME_FOREST:
			return {"tree": 10, "rock": 2, "bush": 5, "log": 2, "stump": 1}
		BIOME_ROCKY:
			return {"tree": 1, "rock": 9, "bush": 1, "log": 0, "stump": 0}
		BIOME_DRY:
			return {"tree": 2, "rock": 4, "bush": 2, "log": 1, "stump": 2}
		BIOME_SNOW:
			return {"tree": 3, "rock": 6, "bush": 1, "log": 0, "stump": 1}
		_:
			return {"tree": 4, "rock": 3, "bush": 5, "log": 1, "stump": 1}

func spawn_resources_of_type(resources_node: Node3D, coord: Vector2i, resource_rng: RandomNumberGenerator, biome: String, resource_kind: String, count: int):
	var spawned := 0
	var attempts := 0
	while spawned < count and attempts < count * 12:
		attempts += 1
		var world_x := coord.x * CHUNK_SIZE + resource_rng.randf_range(4.0, CHUNK_SIZE - 4.0)
		var world_z := coord.y * CHUNK_SIZE + resource_rng.randf_range(4.0, CHUNK_SIZE - 4.0)
		if not is_resource_position_valid(world_x, world_z, resource_kind):
			continue

		var resource_id := "%d_%d_%s_%d" % [coord.x, coord.y, resource_kind, attempts]
		if harvested_resource_ids.has(resource_id):
			continue

		var resource := create_resource_for_kind(resource_kind, spawned, biome)
		resource.set_meta("resource_id", resource_id)
		resource.position = Vector3(world_x - coord.x * CHUNK_SIZE, get_terrain_height(world_x, world_z), world_z - coord.y * CHUNK_SIZE)
		resource.rotation.y = resource_rng.randf_range(0.0, TAU)
		resource.scale = Vector3.ONE * resource_rng.randf_range(0.85, 1.18)
		resources_node.add_child(resource)
		spawned += 1

func is_resource_position_valid(x: float, z: float, resource_kind: String) -> bool:
	if Vector2(x, z).length() < SPAWN_CLEAR_RADIUS + 9.0:
		return false

	var normal := get_terrain_normal(x, z, CHUNK_SIZE / CHUNK_STEPS)
	if normal.y < RESOURCE_MIN_SLOPE_NORMAL_Y:
		return false

	var biome := get_biome(x, z)
	if resource_kind == "tree" and biome == BIOME_ROCKY:
		return mountain_noise.get_noise_2d(x, z) < 0.52
	if resource_kind == "bush" and biome == BIOME_SNOW:
		return moisture_noise.get_noise_2d(x, z) > -0.05

	return true

func create_resource_for_kind(resource_kind: String, index: int, biome: String) -> Node3D:
	match resource_kind:
		"tree":
			return create_tree(index, biome)
		"rock":
			return create_rock(index)
		"bush":
			return create_bush(index, biome)
		"log":
			return create_fallen_log(index)
		"stump":
			return create_tree_stump(index)
		_:
			return create_bush(index, biome)

func get_chunk_seed(coord: Vector2i, salt: int) -> int:
	return abs(hash("%d:%d:%d:%d" % [world_seed, coord.x, coord.y, salt]))

func place_player_at_safe_spawn():
	await get_tree().physics_frame

	var player = get_parent().find_child("Player", true, false)
	if not player:
		print("Player node nicht gefunden")
		return

	var floor_y := get_spawn_floor_y()
	var spawn_pos := Vector3(SPAWN_POSITION.x, floor_y + PLAYER_SPAWN_HEIGHT, SPAWN_POSITION.y)
	if player.has_method("teleport_to_safe_spawn"):
		player.teleport_to_safe_spawn(spawn_pos, floor_y)
	else:
		player.global_position = spawn_pos
		player.velocity = Vector3.ZERO

	print("Player bei 0:0 auf Bodenhoehe gespawnt: floor=", floor_y, " spawn=", spawn_pos)

func get_spawn_floor_y() -> float:
	return get_terrain_height(SPAWN_POSITION.x, SPAWN_POSITION.y)

func spawn_starter_resources():
	if not starter_node:
		return

	clear_children(starter_node)
	var starter_positions := [
		Vector3(18, 0, -16),
		Vector3(-22, 0, 18),
		Vector3(28, 0, 24),
		Vector3(-30, 0, -24),
	]

	for i in range(starter_positions.size()):
		var resource: Node3D
		match i:
			0:
				resource = create_fallen_log(100 + i)
			1:
				resource = create_bush(100 + i, BIOME_MEADOW)
			2:
				resource = create_rock(100 + i)
			_:
				resource = create_tree_stump(100 + i)

		resource.position = with_terrain_y(starter_positions[i])
		resource.rotation.y = rng.randf_range(0.0, TAU)
		starter_node.add_child(resource)

	print("%d Starter-Ressourcen gespawnt" % starter_positions.size())

func with_terrain_y(position: Vector3) -> Vector3:
	position.y = get_terrain_height(position.x, position.z)
	return position

func create_tree(index: int, biome := BIOME_FOREST) -> Node3D:
	var tree := StaticBody3D.new()
	tree.name = "Tree_%02d" % (index + 1)
	tree.add_to_group("trees")
	setup_resource(tree, "wood", "Baum", 0, rng.randi_range(4, 6), "axe", true)
	tree.set_meta("resource_kind", "tree")
	tree.set_meta("log_drop_count", rng.randi_range(3, 5))
	tree.set_meta("being_felled", false)

	var trunk := MeshInstance3D.new()
	trunk.name = "Trunk"
	var trunk_mesh := CylinderMesh.new()
	trunk_mesh.top_radius = 0.35
	trunk_mesh.bottom_radius = 0.55
	trunk_mesh.height = 5.5
	trunk_mesh.radial_segments = 10
	trunk.mesh = trunk_mesh
	trunk.position.y = 2.75
	trunk.set_surface_override_material(0, create_material(get_tree_trunk_color(biome), 0.85))
	tree.add_child(trunk)

	var trunk_collision := CollisionShape3D.new()
	var trunk_shape := CylinderShape3D.new()
	trunk_shape.radius = 0.65
	trunk_shape.height = 5.5
	trunk_collision.shape = trunk_shape
	trunk_collision.position.y = 2.75
	tree.add_child(trunk_collision)

	var leaves := MeshInstance3D.new()
	leaves.name = "Leaves"
	var leaves_mesh := SphereMesh.new()
	leaves_mesh.radius = 2.7
	leaves_mesh.height = 4.2
	leaves_mesh.radial_segments = 16
	leaves_mesh.rings = 8
	leaves.mesh = leaves_mesh
	leaves.position.y = 6.2
	leaves.scale = Vector3(1.1, 0.9, 1.1)
	leaves.set_surface_override_material(0, create_material(get_tree_leaf_color(biome), 0.7))
	tree.add_child(leaves)

	var crown := MeshInstance3D.new()
	crown.name = "Crown"
	var crown_mesh := SphereMesh.new()
	crown_mesh.radius = 2.1
	crown_mesh.height = 3.0
	crown_mesh.radial_segments = 16
	crown_mesh.rings = 8
	crown.mesh = crown_mesh
	crown.position.y = 7.9
	crown.set_surface_override_material(0, create_material(get_tree_leaf_color(biome).lightened(0.08), 0.65))
	tree.add_child(crown)

	return tree

func create_rock(index: int) -> StaticBody3D:
	var rock := StaticBody3D.new()
	rock.name = "Rock_%02d" % (index + 1)
	rock.add_to_group("rocks")
	setup_resource(rock, "stone", "Fels", rng.randi_range(2, 3), rng.randi_range(3, 5), "pickaxe", true)
	rock.set_meta("resource_kind", "rock")

	var mesh_instance := MeshInstance3D.new()
	mesh_instance.name = "RockMesh"
	var mesh := BoxMesh.new()
	mesh.size = Vector3(2.4, 1.4, 1.8)
	mesh_instance.mesh = mesh
	mesh_instance.position.y = 0.7
	mesh_instance.rotation = Vector3(0.2, rng.randf_range(0.0, TAU), 0.1)
	mesh_instance.scale = Vector3(rng.randf_range(0.8, 1.25), rng.randf_range(0.75, 1.15), rng.randf_range(0.8, 1.2))
	mesh_instance.set_surface_override_material(0, create_material(Color(0.42, 0.43, 0.40, 1), 0.9))
	rock.add_child(mesh_instance)

	var collision := CollisionShape3D.new()
	var shape := BoxShape3D.new()
	shape.size = Vector3(2.4, 1.4, 1.8)
	collision.shape = shape
	collision.position.y = 0.7
	rock.add_child(collision)

	return rock

func create_bush(index: int, biome := BIOME_MEADOW) -> StaticBody3D:
	var bush := StaticBody3D.new()
	bush.name = "Bush_%02d" % (index + 1)
	setup_resource(bush, "fiber", "Busch", rng.randi_range(1, 2), rng.randi_range(1, 2), "", false)
	bush.set_meta("resource_kind", "bush")

	var clump_count = rng.randi_range(3, 5)
	for i in range(clump_count):
		var clump := MeshInstance3D.new()
		clump.name = "Clump_%02d" % (i + 1)
		var mesh := SphereMesh.new()
		mesh.radius = rng.randf_range(0.8, 1.35)
		mesh.height = rng.randf_range(1.2, 1.9)
		mesh.radial_segments = 10
		mesh.rings = 5
		clump.mesh = mesh
		clump.position = Vector3(rng.randf_range(-0.9, 0.9), rng.randf_range(0.45, 0.8), rng.randf_range(-0.9, 0.9))
		clump.scale = Vector3(rng.randf_range(0.9, 1.3), rng.randf_range(0.65, 1.0), rng.randf_range(0.9, 1.3))
		clump.set_surface_override_material(0, create_material(get_bush_color(biome).lightened(rng.randf_range(-0.03, 0.08)), 0.75))
		bush.add_child(clump)

	var collision := CollisionShape3D.new()
	var shape := SphereShape3D.new()
	shape.radius = 1.4
	collision.shape = shape
	collision.position.y = 0.75
	collision.scale = Vector3(1.2, 0.6, 1.2)
	bush.add_child(collision)

	return bush

func create_fallen_log(index: int) -> StaticBody3D:
	var log_body := StaticBody3D.new()
	log_body.name = "FallenLog_%02d" % (index + 1)
	setup_resource(log_body, "wood", "Baumstamm", 1, 1, "", false)
	log_body.set_meta("resource_kind", "fallen_log")

	var log_mesh_instance := MeshInstance3D.new()
	log_mesh_instance.name = "LogMesh"
	var log_mesh := CylinderMesh.new()
	log_mesh.top_radius = rng.randf_range(0.35, 0.55)
	log_mesh.bottom_radius = rng.randf_range(0.45, 0.7)
	log_mesh.height = rng.randf_range(3.5, 6.5)
	log_mesh.radial_segments = 10
	log_mesh_instance.mesh = log_mesh
	log_mesh_instance.position.y = 0.55
	log_mesh_instance.rotation.z = PI / 2.0
	log_mesh_instance.set_surface_override_material(0, create_material(Color(0.28, 0.16, 0.07, 1), 0.9))
	log_body.add_child(log_mesh_instance)

	var collision := CollisionShape3D.new()
	var shape := CylinderShape3D.new()
	shape.radius = 0.65
	shape.height = log_mesh.height
	collision.shape = shape
	collision.position.y = 0.55
	collision.rotation.z = PI / 2.0
	log_body.add_child(collision)

	return log_body

func create_tree_stump(index: int) -> StaticBody3D:
	var stump := StaticBody3D.new()
	stump.name = "Stump_%02d" % (index + 1)
	setup_resource(stump, "wood", "Baumstumpf", 1, rng.randi_range(1, 2), "axe", false)
	stump.set_meta("resource_kind", "stump")

	var mesh_instance := MeshInstance3D.new()
	mesh_instance.name = "StumpMesh"
	var mesh := CylinderMesh.new()
	mesh.top_radius = rng.randf_range(0.55, 0.85)
	mesh.bottom_radius = rng.randf_range(0.7, 1.0)
	mesh.height = rng.randf_range(0.8, 1.4)
	mesh.radial_segments = 9
	mesh_instance.mesh = mesh
	mesh_instance.position.y = mesh.height * 0.5
	mesh_instance.set_surface_override_material(0, create_material(Color(0.34, 0.19, 0.08, 1), 0.95))
	stump.add_child(mesh_instance)

	var collision := CollisionShape3D.new()
	var shape := CylinderShape3D.new()
	shape.radius = mesh.bottom_radius
	shape.height = mesh.height
	collision.shape = shape
	collision.position.y = mesh.height * 0.5
	stump.add_child(collision)

	return stump

func get_tree_leaf_color(biome: String) -> Color:
	match biome:
		BIOME_DRY:
			return Color(0.42, 0.42, 0.18, 1)
		BIOME_SNOW:
			return Color(0.10, 0.34, 0.27, 1)
		BIOME_MEADOW:
			return Color(0.12, 0.54, 0.18, 1)
		_:
			return Color(0.08, 0.42, 0.13, 1)

func get_tree_trunk_color(biome: String) -> Color:
	return Color(0.30, 0.18, 0.08, 1) if biome == BIOME_DRY else Color(0.36, 0.20, 0.08, 1)

func get_bush_color(biome: String) -> Color:
	match biome:
		BIOME_DRY:
			return Color(0.36, 0.39, 0.18, 1)
		BIOME_SNOW:
			return Color(0.20, 0.38, 0.31, 1)
		BIOME_FOREST:
			return Color(0.08, 0.36, 0.12, 1)
		_:
			return Color(0.13, 0.46, 0.14, 1)

func fell_tree(tree: Node3D, fall_direction: Vector3):
	if not tree or not is_instance_valid(tree):
		return

	var direction := fall_direction
	direction.y = 0.0
	if direction.length_squared() < 0.01:
		direction = Vector3.FORWARD
	direction = direction.normalized()

	disable_collision_shapes(tree)
	if tree is CollisionObject3D:
		tree.collision_layer = 0
		tree.collision_mask = 0

	tree.rotation.y = atan2(direction.x, direction.z)

	var tween := create_tween()
	tween.tween_property(tree, "rotation:x", deg_to_rad(86.0), 0.95).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN_OUT)
	tween.tween_callback(Callable(self, "spawn_logs_from_tree").bind(tree, direction))
	tween.tween_interval(0.25)
	tween.tween_callback(Callable(tree, "queue_free"))

func spawn_logs_from_tree(tree: Node3D, fall_direction: Vector3):
	var parent := tree.get_parent()
	if not parent:
		parent = self

	var drop_count := int(tree.get_meta("log_drop_count")) if tree.has_meta("log_drop_count") else 3
	var tree_id := str(tree.get_meta("resource_id")) if tree.has_meta("resource_id") else "tree_%d" % Time.get_ticks_msec()
	var base_position := tree.global_position
	var side := Vector3(-fall_direction.z, 0.0, fall_direction.x)

	for i in range(drop_count):
		var log := create_fallen_log(900 + i)
		log.name = "DroppedLog_%02d" % (i + 1)
		log.set_meta("resource_id", "%s_log_%d" % [tree_id, i])
		parent.add_child(log)

		var offset := fall_direction * (1.2 + i * 1.35) + side * rng.randf_range(-0.65, 0.65)
		var log_x := base_position.x + offset.x
		var log_z := base_position.z + offset.z
		log.global_position = Vector3(log_x, get_terrain_height(log_x, log_z), log_z)
		log.rotation.y = atan2(fall_direction.x, fall_direction.z) + rng.randf_range(-0.20, 0.20)
		log.scale = Vector3.ONE * rng.randf_range(0.78, 1.04)

func disable_collision_shapes(node: Node):
	for child in node.get_children():
		if child is CollisionShape3D:
			child.disabled = true
		disable_collision_shapes(child)

func create_material(color: Color, roughness: float) -> StandardMaterial3D:
	var material := StandardMaterial3D.new()
	material.albedo_color = color
	material.roughness = roughness
	material.metallic = 0.0
	return material

func setup_resource(node: Node3D, resource_type: String, display_name: String, amount_per_harvest: int, harvests: int, required_tool: String, blocks_player: bool):
	node.add_to_group("resources")
	node.set_meta("resource_type", resource_type)
	node.set_meta("resource_name", display_name)
	node.set_meta("resource_amount", amount_per_harvest)
	node.set_meta("harvests_remaining", harvests)
	node.set_meta("required_tool", required_tool)

	if node is CollisionObject3D:
		node.collision_layer = RESOURCE_COLLISION_LAYER
		if blocks_player:
			node.collision_layer |= WORLD_COLLISION_LAYER
		node.collision_mask = 0

func mark_resource_harvested(resource_id: String):
	if resource_id != "":
		harvested_resource_ids[resource_id] = true

func clear_children(node: Node):
	for child in node.get_children():
		node.remove_child(child)
		child.queue_free()
