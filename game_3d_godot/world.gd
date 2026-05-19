# World.gd - Chunk-Terrain und Welt-Management
extends Node3D

const WORLD_COLLISION_LAYER := 1
const RESOURCE_COLLISION_LAYER := 2

const CHUNK_SIZE := 64.0
const CHUNK_STEPS := 24
const CHUNK_VIEW_DISTANCE := 1
const CHUNK_UPDATE_INTERVAL := 0.35
const TERRAIN_HEIGHT := 6.5

const SPAWN_CLEAR_RADIUS := 28.0
const SPAWN_BLEND_RADIUS := 64.0
const SPAWN_POSITION := Vector2(0.0, 0.0)
const PLAYER_SPAWN_HEIGHT := 2.0
const RESOURCE_MIN_SLOPE_NORMAL_Y := 0.72

const SEA_LEVEL := -0.55
const WATER_SPAWN_DRY_RADIUS := 78.0
const WATER_SPAWN_BLEND_RADIUS := 150.0
const WATER_SURFACE_OFFSET := 0.05
const WATER_MIN_DEPTH := 0.08
const WATER_MIN_STRENGTH := 0.10
const WATER_MESH_STEPS := 32
const WATER_EDGE_FADE_DEPTH := 0.55
const WATER_EDGE_FADE_STRENGTH := 0.24
const RIVER_SURFACE_CUT_MIN := 0.24
const RIVER_SURFACE_CUT_MAX := 0.72
const RIVER_DEPTH_MIN := 0.22
const RIVER_DEPTH_MAX := 1.10
const RIVER_SLOPE_SAMPLE_DISTANCE := 18.0
const RIVER_CURRENT_MIN_SPEED := 0.85
const RIVER_CURRENT_MAX_SPEED := 4.4
const WATER_TYPE_NONE := 0
const WATER_TYPE_OCEAN := 1
const WATER_TYPE_LAKE := 2
const WATER_TYPE_POND := 3
const WATER_TYPE_RIVER := 4

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
var water_material: ShaderMaterial

var terrain_noise := FastNoiseLite.new()
var terrain_detail_noise := FastNoiseLite.new()
var moisture_noise := FastNoiseLite.new()
var temperature_noise := FastNoiseLite.new()
var mountain_noise := FastNoiseLite.new()
var continent_noise := FastNoiseLite.new()
var lake_noise := FastNoiseLite.new()
var lake_level_noise := FastNoiseLite.new()
var pond_noise := FastNoiseLite.new()
var river_noise := FastNoiseLite.new()
var river_detail_noise := FastNoiseLite.new()

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
	configure_noise(continent_noise, world_seed + 89, 0.0019, 4, 0.56)
	configure_noise(lake_noise, world_seed + 107, 0.0068, 4, 0.50)
	configure_noise(lake_level_noise, world_seed + 131, 0.0022, 3, 0.45)
	configure_noise(pond_noise, world_seed + 149, 0.021, 2, 0.38)
	configure_noise(river_noise, world_seed + 167, 0.0048, 4, 0.52)
	configure_noise(river_detail_noise, world_seed + 181, 0.014, 2, 0.40)

func configure_noise(noise: FastNoiseLite, seed_value: int, frequency: float, octaves: int, gain: float):
	noise.seed = seed_value
	noise.noise_type = FastNoiseLite.TYPE_SIMPLEX
	noise.frequency = frequency
	noise.fractal_octaves = octaves
	noise.fractal_lacunarity = 2.0
	noise.fractal_gain = gain

func setup_dynamic_world():
	terrain_material = create_terrain_material()
	water_material = create_water_material()

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

	var water_mesh := build_chunk_water_mesh(coord)
	if water_mesh.get_surface_count() > 0:
		var water_instance := MeshInstance3D.new()
		water_instance.name = "WaterMesh"
		water_instance.mesh = water_mesh
		water_instance.set_surface_override_material(0, water_material)
		chunk.add_child(water_instance)

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

func create_water_material() -> ShaderMaterial:
	var shader := Shader.new()
	shader.code = """
shader_type spatial;
render_mode blend_mix, depth_prepass_alpha, cull_disabled, specular_schlick_ggx;

varying vec3 world_position;
varying vec2 water_flow;

void vertex() {
	world_position = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
	water_flow = UV2;
	vec2 flow_dir = water_flow / max(length(water_flow), 0.001);
	float flow_speed = clamp(length(water_flow), 0.10, 5.0);
	float river_mask = clamp(COLOR.g, 0.0, 1.0);
	float wave_scale = mix(0.16, 0.55, COLOR.b);
	float wave_a = sin(world_position.x * 0.28 + TIME * 1.10 + COLOR.r * 2.4);
	float wave_b = cos(world_position.z * 0.22 + TIME * 0.92 + COLOR.g * 3.2);
	float wave_c = sin((world_position.x + world_position.z) * 0.11 + TIME * 0.70);
	vec2 tangent = vec2(-flow_dir.y, flow_dir.x);
	float flow_line = dot(world_position.xz, flow_dir);
	float cross_line = dot(world_position.xz, tangent);
	float still_wave = (wave_a * 0.035 + wave_b * 0.024 + wave_c * 0.016) * wave_scale;
	float river_wave = sin(flow_line * 0.60 - TIME * flow_speed * 2.4) * 0.018 + sin(cross_line * 1.15 + flow_line * 0.20 - TIME * flow_speed * 1.6) * 0.010;
	VERTEX.y += mix(still_wave, river_wave, river_mask);
	NORMAL = normalize(mix(vec3(-wave_a * 0.08, 1.0, -wave_b * 0.08), vec3(-flow_dir.x * river_wave * 0.22, 1.0, -flow_dir.y * river_wave * 0.22), river_mask));
}

void fragment() {
	float opacity = clamp(COLOR.a, 0.0, 1.0);
	float river_mask = clamp(COLOR.g, 0.0, 1.0);
	vec2 flow_dir = water_flow / max(length(water_flow), 0.001);
	float flow_speed = clamp(length(water_flow), 0.10, 5.0);
	vec2 tangent = vec2(-flow_dir.y, flow_dir.x);
	float flow_line = dot(world_position.xz, flow_dir);
	float cross_line = dot(world_position.xz, tangent);
	vec3 pond_color = vec3(0.19, 0.56, 0.52);
	vec3 lake_color = vec3(0.10, 0.40, 0.60);
	vec3 river_color = vec3(0.08, 0.43, 0.64);
	vec3 ocean_color = vec3(0.02, 0.16, 0.32);
	vec3 water_color = mix(pond_color, lake_color, COLOR.b);
	water_color = mix(water_color, ocean_color, COLOR.r);
	water_color = mix(water_color, river_color, river_mask);
	water_color = mix(water_color, vec3(0.01, 0.08, 0.18), opacity * 0.34);
	float still_shimmer = sin((world_position.x - world_position.z) * 0.42 + TIME * 2.3) * 0.025;
	float river_shimmer = sin(flow_line * 1.25 - TIME * flow_speed * 4.0) * 0.040 + sin(cross_line * 1.70 + TIME * flow_speed * 0.8) * 0.012;
	float shimmer = mix(still_shimmer, river_shimmer, river_mask);
	ALBEDO = water_color + shimmer;
	ALPHA = mix(0.0, 0.84, opacity);
	ROUGHNESS = 0.18;
	SPECULAR = 0.78;
}
"""

	var material := ShaderMaterial.new()
	material.shader = shader
	return material

func build_chunk_water_mesh(coord: Vector2i) -> ArrayMesh:
	var mesh := ArrayMesh.new()
	var vertices := PackedVector3Array()
	var normals := PackedVector3Array()
	var colors := PackedColorArray()
	var uvs := PackedVector2Array()
	var flow_uvs := PackedVector2Array()
	var indices := PackedInt32Array()
	var cell_size := CHUNK_SIZE / WATER_MESH_STEPS
	var origin_x := coord.x * CHUNK_SIZE
	var origin_z := coord.y * CHUNK_SIZE

	for z_index in range(WATER_MESH_STEPS):
		for x_index in range(WATER_MESH_STEPS):
			var world_x := origin_x + x_index * cell_size
			var world_z := origin_z + z_index * cell_size
			var samples := [
				get_water_render_sample(world_x, world_z),
				get_water_render_sample(world_x + cell_size, world_z),
				get_water_render_sample(world_x, world_z + cell_size),
				get_water_render_sample(world_x + cell_size, world_z + cell_size),
			]
			var center_sample := get_water_render_sample(world_x + cell_size * 0.5, world_z + cell_size * 0.5)
			var active_count := 0
			for sample in samples:
				if bool(sample.get("active", false)):
					active_count += 1

			if active_count == 0 and not bool(center_sample.get("active", false)):
				continue
			if active_count < 2 and not bool(center_sample.get("active", false)):
				continue

			var start_index := vertices.size()
			var edge_sample := get_first_active_water_sample(samples, center_sample)
			var local_positions := [
				Vector2(x_index * cell_size, z_index * cell_size),
				Vector2((x_index + 1) * cell_size, z_index * cell_size),
				Vector2(x_index * cell_size, (z_index + 1) * cell_size),
				Vector2((x_index + 1) * cell_size, (z_index + 1) * cell_size),
			]

			for i in range(4):
				var sample: Dictionary = samples[i]
				if not bool(sample.get("active", false)):
					sample = create_water_edge_sample(edge_sample)

				var local: Vector2 = local_positions[i]
				var vertex_world_x := origin_x + local.x
				var vertex_world_z := origin_z + local.y
				var surface_y: float = float(sample.get("surface_y", SEA_LEVEL)) + WATER_SURFACE_OFFSET
				vertices.append(Vector3(local.x, surface_y, local.y))
				normals.append(Vector3.UP)
				colors.append(get_water_vertex_color(sample))
				uvs.append(Vector2(vertex_world_x, vertex_world_z) * 0.035)
				flow_uvs.append(get_water_flow_vector(vertex_world_x, vertex_world_z, sample))

			indices.append(start_index)
			indices.append(start_index + 2)
			indices.append(start_index + 1)
			indices.append(start_index + 1)
			indices.append(start_index + 2)
			indices.append(start_index + 3)

	if vertices.is_empty():
		return mesh

	var arrays := []
	arrays.resize(Mesh.ARRAY_MAX)
	arrays[Mesh.ARRAY_VERTEX] = vertices
	arrays[Mesh.ARRAY_NORMAL] = normals
	arrays[Mesh.ARRAY_COLOR] = colors
	arrays[Mesh.ARRAY_TEX_UV] = uvs
	arrays[Mesh.ARRAY_TEX_UV2] = flow_uvs
	arrays[Mesh.ARRAY_INDEX] = indices
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
	return mesh

func get_first_active_water_sample(samples: Array, fallback: Dictionary) -> Dictionary:
	if bool(fallback.get("active", false)):
		return fallback

	for sample in samples:
		var sample_dict: Dictionary = sample
		if bool(sample_dict.get("active", false)):
			return sample_dict

	return fallback

func create_water_edge_sample(source: Dictionary) -> Dictionary:
	var edge_sample := source.duplicate()
	edge_sample["active"] = true
	edge_sample["strength"] = 0.0
	edge_sample["depth"] = 0.0
	return edge_sample

func get_water_render_sample(x: float, z: float) -> Dictionary:
	var base_height := calculate_base_terrain_height(x, z)
	var water_info := get_water_info(x, z, base_height)
	if not bool(water_info.get("active", false)):
		return water_info

	var terrain_height := apply_water_to_height(base_height, water_info)
	var surface_y := float(water_info.get("surface_y", SEA_LEVEL))
	var depth := surface_y - terrain_height
	if depth <= WATER_MIN_DEPTH:
		water_info["active"] = false
		return water_info

	water_info["depth"] = depth
	return water_info

func get_water_state_at_position(x: float, z: float) -> Dictionary:
	var water_info := get_water_render_sample(x, z)
	if not bool(water_info.get("active", false)):
		return water_info

	water_info["current"] = get_water_flow_vector(x, z, water_info)
	return water_info

func get_water_vertex_color(water_info: Dictionary) -> Color:
	var water_type := int(water_info.get("type", WATER_TYPE_NONE))
	var depth: float = float(water_info.get("depth", 0.0))
	var strength: float = clamp(float(water_info.get("strength", 0.0)), 0.0, 1.0)
	var edge_depth_fade: float = smoothstep(0.0, WATER_EDGE_FADE_DEPTH, depth)
	var edge_strength_fade: float = smoothstep(WATER_MIN_STRENGTH, WATER_MIN_STRENGTH + WATER_EDGE_FADE_STRENGTH, strength)
	var opacity: float = clamp(edge_depth_fade * edge_strength_fade, 0.0, 1.0)

	match water_type:
		WATER_TYPE_OCEAN:
			return Color(1.0, 0.0, 1.0, opacity)
		WATER_TYPE_LAKE:
			return Color(0.25, 0.0, 0.78, opacity)
		WATER_TYPE_RIVER:
			return Color(0.0, 1.0, 0.56, opacity)
		WATER_TYPE_POND:
			return Color(0.0, 0.0, 0.20, opacity)
		_:
			return Color(0.0, 0.0, 0.0, opacity)

func get_water_flow_vector(x: float, z: float, water_info: Dictionary) -> Vector2:
	var flow := Vector2(float(water_info.get("flow_x", 0.0)), float(water_info.get("flow_z", 0.0)))
	if flow.length() < 0.001:
		flow = get_river_flow_direction(x, z) if int(water_info.get("type", WATER_TYPE_NONE)) == WATER_TYPE_RIVER else Vector2.RIGHT
	else:
		flow = flow.normalized()

	var current_speed: float = max(0.05, float(water_info.get("current_speed", 0.05)))
	return flow * current_speed

func get_river_flow_direction(x: float, z: float) -> Vector2:
	var direction := Vector2.RIGHT
	var intro_strength := get_intro_river_strength(x, z)
	if intro_strength > 0.15:
		var dz_dx: float = cos(x * 0.024) * 0.024 * 22.0 + cos(x * 0.061) * 0.061 * 5.0
		direction = Vector2(1.0, dz_dx).normalized()
	else:
		var sample_distance: float = max(CHUNK_SIZE / WATER_MESH_STEPS * 2.0, 2.0)
		var gradient: Vector2 = get_river_field_gradient(x, z, sample_distance)
		direction = Vector2(-gradient.y, gradient.x)
		if direction.length() < 0.001:
			direction = Vector2.RIGHT
		else:
			direction = direction.normalized()

	return orient_flow_downhill(direction, x, z, 8.0)

func get_river_field_gradient(x: float, z: float, sample_distance: float) -> Vector2:
	var left: float = get_river_field_value(x - sample_distance, z)
	var right: float = get_river_field_value(x + sample_distance, z)
	var back: float = get_river_field_value(x, z - sample_distance)
	var forward: float = get_river_field_value(x, z + sample_distance)
	return Vector2(right - left, forward - back) / (sample_distance * 2.0)

func get_river_field_value(x: float, z: float) -> float:
	return river_noise.get_noise_2d(x, z) + river_detail_noise.get_noise_2d(x, z) * 0.16

func orient_flow_downhill(direction: Vector2, x: float, z: float, sample_distance: float) -> Vector2:
	var ahead_height: float = calculate_base_terrain_height(x + direction.x * sample_distance, z + direction.y * sample_distance)
	var behind_height: float = calculate_base_terrain_height(x - direction.x * sample_distance, z - direction.y * sample_distance)
	if ahead_height > behind_height:
		return -direction

	return direction

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
	var base_height := calculate_base_terrain_height(x, z)
	var water_info := get_water_info(x, z, base_height)
	return apply_water_to_height(base_height, water_info)

func calculate_base_terrain_height(x: float, z: float) -> float:
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

func get_water_info(x: float, z: float, base_height: float) -> Dictionary:
	var water_info := {
		"active": false,
		"type": WATER_TYPE_NONE,
		"strength": 0.0,
		"surface_y": SEA_LEVEL,
		"target_depth": 0.0,
		"depth": 0.0,
		"flow_x": 0.0,
		"flow_z": 0.0,
		"current_speed": 0.0,
	}
	var distance_from_start := Vector2(x, z).length()
	var spawn_water := smoothstep(WATER_SPAWN_DRY_RADIUS, WATER_SPAWN_BLEND_RADIUS, distance_from_start)
	if spawn_water <= 0.0:
		return water_info

	var moisture := moisture_noise.get_noise_2d(x, z)
	var continent := continent_noise.get_noise_2d(x, z)
	var lake_value := lake_noise.get_noise_2d(x, z)
	var pond_value := pond_noise.get_noise_2d(x, z)
	var river_axis: float = abs(river_noise.get_noise_2d(x, z) + river_detail_noise.get_noise_2d(x, z) * 0.16)
	var ocean_strength: float = (1.0 - smoothstep(-0.30, -0.04, continent)) * spawn_water
	var lake_strength: float = smoothstep(0.34, 0.56, lake_value) * smoothstep(-0.16, 0.30, moisture) * (1.0 - smoothstep(2.8, 6.0, base_height)) * spawn_water
	var pond_strength: float = smoothstep(0.50, 0.72, pond_value) * smoothstep(-0.26, 0.18, moisture) * (1.0 - ocean_strength) * spawn_water
	var river_strength: float = (1.0 - smoothstep(0.020, 0.080, river_axis)) * smoothstep(-0.22, 0.26, moisture) * (1.0 - ocean_strength * 0.65) * spawn_water
	pond_strength = max(pond_strength, get_intro_pond_strength(x, z) * spawn_water)
	river_strength = max(river_strength, get_intro_river_strength(x, z) * spawn_water)

	if ocean_strength >= lake_strength and ocean_strength >= pond_strength and ocean_strength >= river_strength:
		if ocean_strength < WATER_MIN_STRENGTH:
			return water_info
		water_info["active"] = true
		water_info["type"] = WATER_TYPE_OCEAN
		water_info["strength"] = ocean_strength
		water_info["surface_y"] = SEA_LEVEL
		water_info["target_depth"] = lerp(0.55, 5.2, ocean_strength)
		water_info["flow_x"] = 0.86
		water_info["flow_z"] = 0.28
		water_info["current_speed"] = 0.24
		return water_info

	if river_strength >= lake_strength and river_strength >= pond_strength:
		if river_strength < WATER_MIN_STRENGTH:
			return water_info
		var river_profile := get_river_profile(x, z, base_height, river_strength)
		var river_flow: Vector2 = river_profile.get("flow", Vector2.RIGHT)
		water_info["active"] = true
		water_info["type"] = WATER_TYPE_RIVER
		water_info["strength"] = river_strength
		water_info["surface_y"] = float(river_profile.get("surface_y", base_height - RIVER_SURFACE_CUT_MIN))
		water_info["target_depth"] = float(river_profile.get("target_depth", RIVER_DEPTH_MIN))
		water_info["flow_x"] = river_flow.x
		water_info["flow_z"] = river_flow.y
		water_info["current_speed"] = float(river_profile.get("current_speed", RIVER_CURRENT_MIN_SPEED))
		return water_info

	if lake_strength >= pond_strength:
		if lake_strength < WATER_MIN_STRENGTH:
			return water_info
		water_info["active"] = true
		water_info["type"] = WATER_TYPE_LAKE
		water_info["strength"] = lake_strength
		water_info["surface_y"] = clamp(0.62 + lake_level_noise.get_noise_2d(x, z) * 1.7, SEA_LEVEL + 0.24, 3.2)
		water_info["target_depth"] = lerp(0.35, 2.3, lake_strength)
		water_info["flow_x"] = 0.42
		water_info["flow_z"] = 0.18
		water_info["current_speed"] = 0.07
		return water_info

	if pond_strength < WATER_MIN_STRENGTH:
		return water_info

	water_info["active"] = true
	water_info["type"] = WATER_TYPE_POND
	water_info["strength"] = pond_strength
	water_info["surface_y"] = base_height - 0.04 + pond_strength * 0.36
	water_info["target_depth"] = lerp(0.14, 0.70, pond_strength)
	water_info["flow_x"] = 0.20
	water_info["flow_z"] = 0.08
	water_info["current_speed"] = 0.03
	return water_info

func get_river_profile(x: float, z: float, base_height: float, strength: float) -> Dictionary:
	var flow := get_river_flow_direction(x, z)
	var upstream_height: float = calculate_base_terrain_height(x - flow.x * RIVER_SLOPE_SAMPLE_DISTANCE, z - flow.y * RIVER_SLOPE_SAMPLE_DISTANCE)
	var downstream_height: float = calculate_base_terrain_height(x + flow.x * RIVER_SLOPE_SAMPLE_DISTANCE, z + flow.y * RIVER_SLOPE_SAMPLE_DISTANCE)
	var smoothed_height: float = (upstream_height + base_height * 2.0 + downstream_height) * 0.25
	var downhill_slope: float = max((upstream_height - downstream_height) / (RIVER_SLOPE_SAMPLE_DISTANCE * 2.0), 0.003)
	var cut: float = lerp(RIVER_SURFACE_CUT_MIN, RIVER_SURFACE_CUT_MAX, strength)
	var surface_y: float = min(base_height - cut, smoothed_height - cut * 0.85)
	var depth: float = lerp(RIVER_DEPTH_MIN, RIVER_DEPTH_MAX, strength) + clamp(downhill_slope * 5.5, 0.0, 0.45)
	var current_speed: float = lerp(RIVER_CURRENT_MIN_SPEED, RIVER_CURRENT_MAX_SPEED, clamp(strength * 0.48 + downhill_slope * 18.0, 0.0, 1.0))

	return {
		"surface_y": surface_y,
		"target_depth": depth,
		"flow": flow,
		"current_speed": current_speed,
	}

func get_intro_pond_strength(x: float, z: float) -> float:
	var distance_to_pond: float = Vector2(x, z).distance_to(Vector2(96.0, -62.0))
	return 1.0 - smoothstep(18.0, 32.0, distance_to_pond)

func get_intro_river_strength(x: float, z: float) -> float:
	var river_center_z: float = 116.0 + sin(x * 0.024) * 22.0 + sin(x * 0.061) * 5.0
	var channel_distance: float = abs(z - river_center_z)
	var length_fade: float = smoothstep(-230.0, -160.0, x) * (1.0 - smoothstep(240.0, 310.0, x))
	return (1.0 - smoothstep(5.5, 15.0, channel_distance)) * length_fade

func apply_water_to_height(base_height: float, water_info: Dictionary) -> float:
	if not bool(water_info.get("active", false)):
		return base_height

	var strength: float = clamp(float(water_info.get("strength", 0.0)), 0.0, 1.0)
	var surface_y := float(water_info.get("surface_y", SEA_LEVEL))
	var target_depth: float = max(WATER_MIN_DEPTH, float(water_info.get("target_depth", WATER_MIN_DEPTH)))
	var desired_bottom: float = surface_y - target_depth
	var shaped_height: float = min(base_height, desired_bottom)
	return lerp(base_height, shaped_height, strength)

func get_terrain_vertex_color(x: float, z: float, height: float) -> Color:
	if Vector2(x, z).length() <= SPAWN_CLEAR_RADIUS:
		return Color(0.34, 0.52, 0.25, 1)

	var biome := get_biome(x, z)
	var base_height := calculate_base_terrain_height(x, z)
	var water_info := get_water_info(x, z, base_height)
	if bool(water_info.get("active", false)):
		var surface_y := float(water_info.get("surface_y", SEA_LEVEL))
		if height < surface_y + 0.28:
			return get_waterbed_color(biome, int(water_info.get("type", WATER_TYPE_NONE)), surface_y - height)

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

func get_waterbed_color(biome: String, water_type: int, depth: float) -> Color:
	if water_type == WATER_TYPE_OCEAN:
		return Color(0.48, 0.43, 0.30, 1) if depth < 0.55 else Color(0.22, 0.30, 0.26, 1)
	if water_type == WATER_TYPE_RIVER:
		return Color(0.28, 0.34, 0.22, 1) if depth < 0.35 else Color(0.16, 0.24, 0.18, 1)
	if water_type == WATER_TYPE_POND:
		return Color(0.25, 0.38, 0.22, 1) if depth < 0.30 else Color(0.14, 0.25, 0.18, 1)
	if biome == BIOME_SNOW:
		return Color(0.58, 0.66, 0.64, 1)
	return Color(0.38, 0.42, 0.28, 1) if depth < 0.45 else Color(0.20, 0.30, 0.23, 1)

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

	if is_water_position(x, z):
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

func is_water_position(x: float, z: float) -> bool:
	return bool(get_water_render_sample(x, z).get("active", false))

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
