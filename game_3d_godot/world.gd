# World.gd - Terrain und Welt-Management
extends Node3D

const ROCK_POSITIONS := [
	Vector3(68, 0, -42),
	Vector3(-72, 0, -36),
	Vector3(70, 0, 30),
	Vector3(-64, 0, 52),
	Vector3(42, 0, 68),
]

const TREE_COUNT := 34
const TREE_CLUSTER_COUNT := 6
const TREE_CLUSTER_RADIUS := 18.0
const TREE_MIN_DISTANCE := 5.5
const STRUCTURE_COUNT := 42
const WORLD_RADIUS := 85.0
const PLAYER_SAFE_RADIUS := 52.0
const ROCK_SAFE_RADIUS := 48.0
const TERRAIN_SIZE := 200.0
const TERRAIN_STEPS := 96
const TERRAIN_HEIGHT := 3.2
const SPAWN_HEIGHT := 3.0
const SPAWN_PLATFORM_SIZE := 26.0

var rng := RandomNumberGenerator.new()
var terrain_noise := FastNoiseLite.new()

func _ready():
	rng.randomize()
	setup_terrain_noise()
	generate_terrain()
	create_spawn_platform()
	place_player_at_spawn()
	call_deferred("place_player_at_spawn")
	spawn_trees()
	spawn_rocks()
	spawn_random_structures()
	print("World geladen")

func setup_terrain_noise():
	terrain_noise.seed = rng.randi()
	terrain_noise.noise_type = FastNoiseLite.TYPE_SIMPLEX
	terrain_noise.frequency = 0.026
	terrain_noise.fractal_octaves = 4
	terrain_noise.fractal_lacunarity = 2.0
	terrain_noise.fractal_gain = 0.45

func generate_terrain():
	var terrain = find_child("Terrain", true, false)
	if not terrain or not terrain is MeshInstance3D:
		print("Terrain node nicht gefunden")
		return

	var mesh := ArrayMesh.new()
	var vertices := PackedVector3Array()
	var normals := PackedVector3Array()
	var uvs := PackedVector2Array()
	var indices := PackedInt32Array()
	var cell_size := TERRAIN_SIZE / TERRAIN_STEPS
	var half_size := TERRAIN_SIZE * 0.5

	for z_index in range(TERRAIN_STEPS + 1):
		for x_index in range(TERRAIN_STEPS + 1):
			var x := -half_size + x_index * cell_size
			var z := -half_size + z_index * cell_size
			var y := get_terrain_height(x, z)
			vertices.append(Vector3(x, y, z))
			normals.append(get_terrain_normal(x, z, cell_size))
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
	arrays[Mesh.ARRAY_TEX_UV] = uvs
	arrays[Mesh.ARRAY_INDEX] = indices
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
	mesh.surface_set_material(0, create_material(Color(0.30, 0.47, 0.23, 1), 0.95))

	terrain.mesh = mesh
	terrain.position = Vector3.ZERO

	var collision_shape = find_child("TerrainCollider", true, false).find_child("CollisionShape3D", true, false) if find_child("TerrainCollider", true, false) else null
	if collision_shape:
		collision_shape.shape = mesh.create_trimesh_shape()

	print("Huegeliges Terrain generiert")

func place_player_at_spawn():
	var player = get_parent().find_child("Player", true, false)
	if not player:
		print("Player node nicht gefunden")
		return

	player.global_position = Vector3(0, SPAWN_HEIGHT, 0)
	player.velocity = Vector3.ZERO
	print("Player am sicheren Spawn platziert")

func create_spawn_platform():
	var old_platform = find_child("SpawnPlatform", true, false)
	if old_platform:
		old_platform.queue_free()

	var platform := StaticBody3D.new()
	platform.name = "SpawnPlatform"
	add_child(platform)

	var mesh_instance := MeshInstance3D.new()
	mesh_instance.name = "SpawnGround"
	var mesh := BoxMesh.new()
	mesh.size = Vector3(SPAWN_PLATFORM_SIZE, 0.2, SPAWN_PLATFORM_SIZE)
	mesh_instance.mesh = mesh
	mesh_instance.position.y = -0.1
	mesh_instance.set_surface_override_material(0, create_material(Color(0.34, 0.42, 0.22, 1), 0.95))
	platform.add_child(mesh_instance)

	var collision := CollisionShape3D.new()
	var shape := BoxShape3D.new()
	shape.size = Vector3(SPAWN_PLATFORM_SIZE, 0.2, SPAWN_PLATFORM_SIZE)
	collision.shape = shape
	collision.position.y = -0.1
	platform.add_child(collision)

	print("Sichere Spawn-Plattform erstellt")

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

func spawn_random_structures():
	var structures_node = find_child("Structures", true, false)
	if not structures_node:
		structures_node = Node3D.new()
		structures_node.name = "Structures"
		add_child(structures_node)

	clear_children(structures_node)

	for i in range(STRUCTURE_COUNT):
		var structure_type = rng.randi_range(0, 3)
		var structure: Node3D

		match structure_type:
			0:
				structure = create_small_hill(i)
			1:
				structure = create_bush(i)
			2:
				structure = create_fallen_log(i)
			_:
				structure = create_tree_stump(i)

		structure.position = get_random_world_position()
		structure.rotation.y = rng.randf_range(0.0, TAU)
		structures_node.add_child(structure)

	print("%d zufaellige Welt-Strukturen gespawnt" % STRUCTURE_COUNT)

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
	tree.add_to_group("resources")

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
	rock.add_to_group("resources")

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

func create_small_hill(index: int) -> StaticBody3D:
	var hill := StaticBody3D.new()
	hill.name = "Hill_%02d" % (index + 1)

	var mesh_instance := MeshInstance3D.new()
	mesh_instance.name = "HillMesh"
	var mesh := SphereMesh.new()
	mesh.radius = rng.randf_range(3.5, 7.5)
	mesh.height = rng.randf_range(1.0, 2.2)
	mesh.radial_segments = 16
	mesh.rings = 8
	mesh_instance.mesh = mesh
	mesh_instance.position.y = mesh.height * 0.25
	mesh_instance.scale = Vector3(rng.randf_range(1.0, 1.8), 0.35, rng.randf_range(0.8, 1.5))
	mesh_instance.set_surface_override_material(0, create_material(Color(0.28, 0.42, 0.20, 1), 0.9))
	hill.add_child(mesh_instance)

	var collision := CollisionShape3D.new()
	var shape := SphereShape3D.new()
	shape.radius = mesh.radius * 0.65
	collision.shape = shape
	collision.position.y = 0.35
	collision.scale = Vector3(mesh_instance.scale.x, 0.25, mesh_instance.scale.z)
	hill.add_child(collision)

	return hill

func create_bush(index: int) -> Node3D:
	var bush := Node3D.new()
	bush.name = "Bush_%02d" % (index + 1)

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

	return bush

func create_fallen_log(index: int) -> StaticBody3D:
	var log_body := StaticBody3D.new()
	log_body.name = "FallenLog_%02d" % (index + 1)

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
	var distance_from_start := Vector2(x, z).length()
	if distance_from_start < PLAYER_SAFE_RADIUS:
		return 0.0

	var start_flatten := smoothstep(PLAYER_SAFE_RADIUS, 92.0, distance_from_start)
	var edge_falloff := 1.0 - smoothstep(86.0, 100.0, max(abs(x), abs(z)))
	var broad_hills := terrain_noise.get_noise_2d(x, z) * TERRAIN_HEIGHT
	var rolling_waves := (sin(x * 0.07) + cos(z * 0.06) + sin((x + z) * 0.04)) * 0.8
	var detail := terrain_noise.get_noise_2d(x * 2.6 + 31.0, z * 2.6 - 17.0) * 0.7

	return (broad_hills + rolling_waves + detail) * start_flatten * edge_falloff

func get_terrain_normal(x: float, z: float, sample_distance: float) -> Vector3:
	var left := get_terrain_height(x - sample_distance, z)
	var right := get_terrain_height(x + sample_distance, z)
	var back := get_terrain_height(x, z - sample_distance)
	var forward := get_terrain_height(x, z + sample_distance)

	return Vector3(left - right, sample_distance * 2.0, back - forward).normalized()

func clear_children(node: Node):
	for child in node.get_children():
		child.queue_free()
