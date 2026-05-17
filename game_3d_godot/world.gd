# World.gd - Terrain und Welt-Management
extends Node3D

const ROCK_POSITIONS := [
	Vector3(68, 0, -42),
	Vector3(-72, 0, -36),
	Vector3(70, 0, 30),
	Vector3(-64, 0, 52),
	Vector3(42, 0, 68),
]
const STARTER_RESOURCE_POSITIONS := [
	Vector3(18, 0, -16),
	Vector3(-22, 0, 18),
	Vector3(28, 0, 24),
	Vector3(-30, 0, -24),
]

const TREE_COUNT := 34
const TREE_CLUSTER_COUNT := 6
const TREE_CLUSTER_RADIUS := 18.0
const TREE_MIN_DISTANCE := 5.5
const WORLD_RADIUS := 85.0
const PLAYER_SAFE_RADIUS := 52.0
const ROCK_SAFE_RADIUS := 48.0
const WORLD_COLLISION_LAYER := 1
const RESOURCE_COLLISION_LAYER := 2
const TERRAIN_SIZE := 240.0
const TERRAIN_STEPS := 96
const TERRAIN_HEIGHT := 5.5
const SPAWN_CLEAR_RADIUS := 28.0
const SPAWN_BLEND_RADIUS := 58.0
const SPAWN_POSITION := Vector2(0.0, 0.0)
const PLAYER_SPAWN_HEIGHT := 2.0

var rng := RandomNumberGenerator.new()
var terrain_noise := FastNoiseLite.new()
var terrain_detail_noise := FastNoiseLite.new()
var terrain_heights := PackedFloat32Array()

func _ready():
	rng.randomize()
	setup_environment()
	setup_terrain_noise()
	generate_terrain()
	spawn_trees()
	spawn_rocks()
	spawn_starter_resources()
	call_deferred("place_player_at_safe_spawn")
	print("World geladen")

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
	terrain_noise.seed = rng.randi()
	terrain_noise.noise_type = FastNoiseLite.TYPE_SIMPLEX
	terrain_noise.frequency = 0.012
	terrain_noise.fractal_octaves = 5
	terrain_noise.fractal_lacunarity = 2.0
	terrain_noise.fractal_gain = 0.48

	terrain_detail_noise.seed = rng.randi()
	terrain_detail_noise.noise_type = FastNoiseLite.TYPE_SIMPLEX
	terrain_detail_noise.frequency = 0.045
	terrain_detail_noise.fractal_octaves = 3
	terrain_detail_noise.fractal_lacunarity = 2.1
	terrain_detail_noise.fractal_gain = 0.42

func generate_terrain():
	var terrain = find_child("Terrain", true, false)
	if not terrain or not terrain is MeshInstance3D:
		print("Terrain node nicht gefunden")
		return

	terrain_heights = build_terrain_heightmap()
	var mesh := build_terrain_mesh(terrain_heights)
	mesh.surface_set_material(0, create_terrain_material())

	terrain.mesh = mesh
	terrain.position = Vector3.ZERO

	var terrain_collider = find_child("TerrainCollider", true, false)
	var collision_shape = terrain_collider.find_child("CollisionShape3D", true, false) if terrain_collider else null
	if collision_shape:
		terrain_collider.position = Vector3.ZERO
		collision_shape.position = Vector3.ZERO
		collision_shape.rotation = Vector3.ZERO
		collision_shape.scale = Vector3.ONE

		var terrain_shape := mesh.create_trimesh_shape()
		if terrain_shape is ConcavePolygonShape3D:
			terrain_shape.backface_collision = true
		collision_shape.shape = terrain_shape

	print("Neues Terrain generiert")

func build_terrain_heightmap() -> PackedFloat32Array:
	var heights := PackedFloat32Array()
	heights.resize((TERRAIN_STEPS + 1) * (TERRAIN_STEPS + 1))

	var cell_size := TERRAIN_SIZE / TERRAIN_STEPS
	var half_size := TERRAIN_SIZE * 0.5

	for z_index in range(TERRAIN_STEPS + 1):
		for x_index in range(TERRAIN_STEPS + 1):
			var x := -half_size + x_index * cell_size
			var z := -half_size + z_index * cell_size
			heights[get_height_index(x_index, z_index)] = calculate_terrain_height(x, z)

	return heights

func build_terrain_mesh(heights: PackedFloat32Array) -> ArrayMesh:
	var mesh := ArrayMesh.new()
	var vertices := PackedVector3Array()
	var normals := PackedVector3Array()
	var colors := PackedColorArray()
	var uvs := PackedVector2Array()
	var indices := PackedInt32Array()
	var cell_size := TERRAIN_SIZE / TERRAIN_STEPS
	var half_size := TERRAIN_SIZE * 0.5

	for z_index in range(TERRAIN_STEPS + 1):
		for x_index in range(TERRAIN_STEPS + 1):
			var x := -half_size + x_index * cell_size
			var z := -half_size + z_index * cell_size
			var y := heights[get_height_index(x_index, z_index)]
			vertices.append(Vector3(x, y, z))
			normals.append(get_terrain_vertex_normal(x_index, z_index, heights))
			colors.append(get_terrain_vertex_color(x, z, y))
			uvs.append(Vector2(float(x_index) / TERRAIN_STEPS, float(z_index) / TERRAIN_STEPS) * 12.0)

	for z_index in range(TERRAIN_STEPS):
		for x_index in range(TERRAIN_STEPS):
			var top_left := z_index * (TERRAIN_STEPS + 1) + x_index
			var top_right := top_left + 1
			var bottom_left := top_left + TERRAIN_STEPS + 1
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

func get_height_index(x_index: int, z_index: int) -> int:
	return z_index * (TERRAIN_STEPS + 1) + x_index

func get_terrain_vertex_normal(x_index: int, z_index: int, heights: PackedFloat32Array) -> Vector3:
	var left: int = max(x_index - 1, 0)
	var right: int = min(x_index + 1, TERRAIN_STEPS)
	var back: int = max(z_index - 1, 0)
	var forward: int = min(z_index + 1, TERRAIN_STEPS)
	var cell_size := TERRAIN_SIZE / TERRAIN_STEPS

	var left_height := heights[get_height_index(left, z_index)]
	var right_height := heights[get_height_index(right, z_index)]
	var back_height := heights[get_height_index(x_index, back)]
	var forward_height := heights[get_height_index(x_index, forward)]

	return Vector3(left_height - right_height, cell_size * 2.0, back_height - forward_height).normalized()

func create_terrain_material() -> StandardMaterial3D:
	var material := StandardMaterial3D.new()
	material.albedo_color = Color(1, 1, 1, 1)
	material.vertex_color_use_as_albedo = true
	material.cull_mode = BaseMaterial3D.CULL_DISABLED
	material.roughness = 0.96
	material.metallic = 0.0
	return material

func get_terrain_vertex_color(x: float, z: float, height: float) -> Color:
	var distance_from_spawn := Vector2(x, z).length()
	if distance_from_spawn <= SPAWN_CLEAR_RADIUS:
		return Color(0.34, 0.52, 0.25, 1)

	if height < -1.2:
		return Color(0.22, 0.36, 0.20, 1)

	if height > 3.4:
		return Color(0.42, 0.43, 0.35, 1)

	return Color(0.25, 0.46, 0.23, 1)

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

func place_player_at_spawn():
	var player = get_parent().find_child("Player", true, false)
	if not player:
		print("Player node nicht gefunden")
		return

	var floor_y := get_spawn_floor_y()
	var spawn_pos := Vector3(SPAWN_POSITION.x, floor_y + PLAYER_SPAWN_HEIGHT, SPAWN_POSITION.y)
	player.global_position = spawn_pos
	player.velocity = Vector3.ZERO
	print("Player am Spawn platziert bei ", spawn_pos)

func get_spawn_floor_y() -> float:
	return get_terrain_height(SPAWN_POSITION.x, SPAWN_POSITION.y)

func spawn_trees():
	var trees_node = find_child("Trees", true, false)
	if not trees_node:
		print("Trees node nicht gefunden")
		return

	clear_children(trees_node)

	var tree_positions := generate_tree_positions()
	for i in range(tree_positions.size()):
		var tree = create_tree(i)
		tree.position = tree_positions[i]
		tree.rotation.y = rng.randf_range(0.0, TAU)
		tree.scale = Vector3.ONE * rng.randf_range(0.82, 1.28)
		trees_node.add_child(tree)
		print("Baum %d gespawnt bei %s" % [i + 1, tree.position])

	print("%d zufaellige Baeume gespawnt" % tree_positions.size())

func spawn_rocks():
	var rocks_node = find_child("Rocks", true, false)
	if not rocks_node:
		print("Rocks node nicht gefunden")
		return

	clear_children(rocks_node)

	for i in range(ROCK_POSITIONS.size()):
		var rock = create_rock(i)
		rock.position = with_terrain_y(ROCK_POSITIONS[i])
		if Vector2(rock.position.x, rock.position.z).length() < ROCK_SAFE_RADIUS:
			rock.position = get_random_world_position(ROCK_SAFE_RADIUS)
		rocks_node.add_child(rock)
		print("Fels %d gespawnt bei %s" % [i + 1, rock.position])

	print("%d sichtbare Felsen gespawnt" % ROCK_POSITIONS.size())

func spawn_starter_resources():
	var starter_node = find_child("StarterResources", true, false)
	if not starter_node:
		starter_node = Node3D.new()
		starter_node.name = "StarterResources"
		add_child(starter_node)

	clear_children(starter_node)

	for i in range(STARTER_RESOURCE_POSITIONS.size()):
		var resource: Node3D
		match i:
			0:
				resource = create_fallen_log(100 + i)
			1:
				resource = create_bush(100 + i)
			2:
				resource = create_rock(100 + i)
			_:
				resource = create_tree_stump(100 + i)

		resource.position = with_terrain_y(STARTER_RESOURCE_POSITIONS[i])
		resource.rotation.y = rng.randf_range(0.0, TAU)
		starter_node.add_child(resource)

	print("%d Starter-Ressourcen gespawnt" % STARTER_RESOURCE_POSITIONS.size())

func generate_tree_positions() -> Array[Vector3]:
	var positions: Array[Vector3] = []
	var cluster_centers: Array[Vector3] = []

	for i in range(TREE_CLUSTER_COUNT):
		cluster_centers.append(get_random_world_position(PLAYER_SAFE_RADIUS + 12.0))

	var attempts := 0
	while positions.size() < TREE_COUNT and attempts < TREE_COUNT * 40:
		attempts += 1
		var cluster_center := cluster_centers[rng.randi_range(0, cluster_centers.size() - 1)]
		var offset := Vector2.from_angle(rng.randf_range(0.0, TAU)) * rng.randf_range(0.0, TREE_CLUSTER_RADIUS)
		var candidate := with_terrain_y(Vector3(cluster_center.x + offset.x, 0, cluster_center.z + offset.y))

		if abs(candidate.x) > WORLD_RADIUS or abs(candidate.z) > WORLD_RADIUS:
			continue

		if not is_tree_position_valid(candidate, positions):
			continue

		positions.append(candidate)

	while positions.size() < TREE_COUNT:
		var fallback := get_random_world_position(PLAYER_SAFE_RADIUS + 6.0)
		if is_tree_position_valid(fallback, positions):
			positions.append(fallback)

	return positions

func is_tree_position_valid(candidate: Vector3, positions: Array[Vector3]) -> bool:
	if Vector2(candidate.x, candidate.z).length() < PLAYER_SAFE_RADIUS + 8.0:
		return false

	var terrain_normal := get_terrain_normal(candidate.x, candidate.z, TERRAIN_SIZE / TERRAIN_STEPS)
	if terrain_normal.y < 0.78:
		return false

	for position in positions:
		var distance := Vector2(candidate.x - position.x, candidate.z - position.z).length()
		if distance < TREE_MIN_DISTANCE:
			return false

	return true

func create_tree(index: int) -> Node3D:
	var tree := StaticBody3D.new()
	tree.name = "Tree_%02d" % (index + 1)
	tree.add_to_group("trees")
	setup_resource(tree, "wood", "Baum", rng.randi_range(2, 4), rng.randi_range(3, 5), "axe", true)

	var trunk := MeshInstance3D.new()
	trunk.name = "Trunk"
	var trunk_mesh := CylinderMesh.new()
	trunk_mesh.top_radius = 0.35
	trunk_mesh.bottom_radius = 0.55
	trunk_mesh.height = 5.5
	trunk_mesh.radial_segments = 10
	trunk.mesh = trunk_mesh
	trunk.position.y = 2.75
	trunk.set_surface_override_material(0, create_material(Color(0.36, 0.20, 0.08, 1), 0.85))
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
	leaves.set_surface_override_material(0, create_material(Color(0.08, 0.48, 0.13, 1), 0.7))
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
	crown.set_surface_override_material(0, create_material(Color(0.12, 0.58, 0.16, 1), 0.65))
	tree.add_child(crown)

	return tree

func create_rock(index: int) -> StaticBody3D:
	var rock := StaticBody3D.new()
	rock.name = "Rock_%02d" % (index + 1)
	rock.add_to_group("rocks")
	setup_resource(rock, "stone", "Fels", rng.randi_range(2, 3), rng.randi_range(3, 5), "pickaxe", true)

	var mesh_instance := MeshInstance3D.new()
	mesh_instance.name = "RockMesh"
	var mesh := BoxMesh.new()
	mesh.size = Vector3(2.4, 1.4, 1.8)
	mesh_instance.mesh = mesh
	mesh_instance.position.y = 0.7
	mesh_instance.rotation = Vector3(0.2, randf_range(0.0, TAU), 0.1)
	mesh_instance.scale = Vector3(randf_range(0.8, 1.25), randf_range(0.75, 1.15), randf_range(0.8, 1.2))
	mesh_instance.set_surface_override_material(0, create_material(Color(0.42, 0.43, 0.40, 1), 0.9))
	rock.add_child(mesh_instance)

	var collision := CollisionShape3D.new()
	var shape := BoxShape3D.new()
	shape.size = Vector3(2.4, 1.4, 1.8)
	collision.shape = shape
	collision.position.y = 0.7
	rock.add_child(collision)

	return rock

func create_bush(index: int) -> StaticBody3D:
	var bush := StaticBody3D.new()
	bush.name = "Bush_%02d" % (index + 1)
	setup_resource(bush, "fiber", "Busch", rng.randi_range(1, 2), rng.randi_range(1, 2), "", false)

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
		clump.set_surface_override_material(0, create_material(Color(rng.randf_range(0.08, 0.18), rng.randf_range(0.35, 0.58), rng.randf_range(0.09, 0.18), 1), 0.75))
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
	setup_resource(log_body, "wood", "Baumstamm", rng.randi_range(2, 3), rng.randi_range(2, 3), "axe", false)

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

func get_random_world_position(min_distance_from_player := PLAYER_SAFE_RADIUS) -> Vector3:
	var position := Vector3.ZERO
	for i in range(20):
		position = Vector3(rng.randf_range(-WORLD_RADIUS, WORLD_RADIUS), 0, rng.randf_range(-WORLD_RADIUS, WORLD_RADIUS))
		if Vector2(position.x, position.z).length() >= min_distance_from_player:
			return with_terrain_y(position)

	return with_terrain_y(Vector3(min_distance_from_player + 2.0, 0, min_distance_from_player + 2.0))

func with_terrain_y(position: Vector3) -> Vector3:
	position.y = get_terrain_height(position.x, position.z)
	return position

func get_terrain_height(x: float, z: float) -> float:
	return calculate_terrain_height(x, z)

func calculate_terrain_height(x: float, z: float) -> float:
	var distance_from_start := Vector2(x, z).length()
	if distance_from_start <= SPAWN_CLEAR_RADIUS:
		return 0.0

	var spawn_blend := smoothstep(SPAWN_CLEAR_RADIUS, SPAWN_BLEND_RADIUS, distance_from_start)
	var edge_falloff := 1.0 - smoothstep(TERRAIN_SIZE * 0.45, TERRAIN_SIZE * 0.5, max(abs(x), abs(z)))
	var broad_hills := terrain_noise.get_noise_2d(x, z) * TERRAIN_HEIGHT
	var rolling_land := (sin(x * 0.026) + cos(z * 0.021) + sin((x - z) * 0.018)) * 0.65
	var detail := terrain_detail_noise.get_noise_2d(x, z) * 1.1

	return (broad_hills + rolling_land + detail) * spawn_blend * edge_falloff

func get_terrain_normal(x: float, z: float, sample_distance: float) -> Vector3:
	var left := get_terrain_height(x - sample_distance, z)
	var right := get_terrain_height(x + sample_distance, z)
	var back := get_terrain_height(x, z - sample_distance)
	var forward := get_terrain_height(x, z + sample_distance)

	return Vector3(left - right, sample_distance * 2.0, back - forward).normalized()

func clear_children(node: Node):
	for child in node.get_children():
		child.queue_free()
