# Player.gd - First-Person Player Controller
extends CharacterBody3D

@export var speed = 15.0
@export var sprint_speed = 25.0
@export var jump_force = 15.0
@export var gravity = 30.0
@export var mouse_sensitivity = 0.003
@export var interact_range = 5.0

const BODY_HEIGHT := 1.8
const BODY_RADIUS := 0.35
const EYE_HEIGHT := 1.6
const GROUND_BUFFER := 0.08
const HOTBAR_ITEMS := ["wood", "stone", "fiber", "food", "axe", "pickaxe", "", "", ""]

var is_sprinting = false
var health = 100.0
var hunger = 100.0
var stamina = 100.0
var spawn_safety_time = 0.0
var safe_spawn_floor_y = 0.0
var inventory := {
	"wood": 0,
	"stone": 0,
	"fiber": 0,
	"food": 0,
	"axe": 1,
	"pickaxe": 1,
}
var looked_at_resource: Node3D = null
var selected_item := "axe"
var selected_hotbar_index := 4
var held_item_root: Node3D = null

func _ready():
	configure_player_rig()
	ensure_held_item_root()
	Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)
	update_hud_inventory()
	update_hud_selection()
	update_held_item_visual()
	print("✅ Player Ready")

func configure_player_rig():
	floor_snap_length = 0.35
	floor_max_angle = deg_to_rad(50.0)

	var collision_shape = find_child("CollisionShape3D", false, false)
	if collision_shape:
		collision_shape.position = Vector3(0, BODY_HEIGHT * 0.5, 0)
		if collision_shape.shape is CapsuleShape3D:
			collision_shape.shape.height = BODY_HEIGHT
			collision_shape.shape.radius = BODY_RADIUS

	var camera = find_child("Camera3D", false, false)
	if camera:
		camera.position = Vector3(0, EYE_HEIGHT, 0)

func _physics_process(delta):
	keep_above_spawn_ground(delta)

	# Bewegung
	var inventory_open := is_inventory_open()
	var input_dir: Vector2 = Vector2.ZERO if inventory_open else Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
	var direction = (transform.basis * Vector3(input_dir.x, 0, input_dir.y)).normalized()
	
	# Sprint
	is_sprinting = Input.is_action_pressed("sprint") and stamina > 0
	var current_speed = sprint_speed if is_sprinting else speed
	
	if direction:
		velocity.x = direction.x * current_speed
		velocity.z = direction.z * current_speed
	else:
		velocity.x = move_toward(velocity.x, 0, current_speed)
		velocity.z = move_toward(velocity.z, 0, current_speed)
	
	# Gravity
	velocity.y -= gravity * delta
	
	# Jump
	if Input.is_action_just_pressed("ui_accept") and is_on_floor() and not inventory_open:
		velocity.y = jump_force
	
	# Move
	move_and_slide()
	
	# Update Stats
	update_survival_stats(delta)
	update_interaction_prompt()

func _input(event):
	if event.is_action_pressed("inventory"):
		toggle_inventory()
		return

	if handle_selection_input(event):
		return

	if event is InputEventMouseMotion and not is_inventory_open():
		rotate_y(-event.relative.x * mouse_sensitivity)
		$Camera3D.rotate_x(-event.relative.y * mouse_sensitivity)
		$Camera3D.rotation.x = clamp($Camera3D.rotation.x, -PI/2, PI/2)
	
	if is_primary_action_pressed(event) and not is_inventory_open():
		interact_with_resource()

	if event.is_action_pressed("ui_cancel"):
		if is_inventory_open():
			close_inventory()
			return
		get_tree().quit()

func update_survival_stats(delta):
	# Hunger
	var drain = 0.1 if is_sprinting else 0.05
	hunger -= drain * delta
	
	# Stamina
	if is_sprinting:
		stamina -= 0.2 * delta
	else:
		stamina = move_toward(stamina, 100.0, 0.1 * delta)
	
	# Health von Hunger
	if hunger <= 0:
		health -= 0.1 * delta
	
	# Clamp
	health = clamp(health, 0, 100)
	hunger = clamp(hunger, 0, 100)
	stamina = clamp(stamina, 0, 100)
	
	# Update HUD
	var hud = get_hud()
	if hud:
		hud.update_stats(health, hunger, stamina)

func keep_above_spawn_ground(delta):
	if spawn_safety_time <= 0:
		return

	spawn_safety_time -= delta
	var min_y = max(get_ground_height_at_position(), safe_spawn_floor_y) + GROUND_BUFFER
	if global_position.y < min_y:
		global_position.y = min_y
		velocity.y = 0.0

func teleport_to_safe_spawn(spawn_position: Vector3, floor_y: float):
	global_position = spawn_position
	velocity = Vector3.ZERO
	safe_spawn_floor_y = floor_y
	spawn_safety_time = 1.25
	looked_at_resource = null

func get_ground_height_at_position() -> float:
	var world = get_parent().find_child("World", true, false)
	if world and world.has_method("get_terrain_height"):
		return world.get_terrain_height(global_position.x, global_position.z)

	return 0.0

func interact_with_resource():
	var resource := get_resource_in_view()
	if not resource:
		var hud = get_hud()
		if hud and hud.has_method("show_message"):
			hud.show_message("Keine Ressource in Reichweite")
		return

	harvest_resource(resource)

func harvest_resource(resource: Node3D):
	if not resource.has_meta("resource_type"):
		return

	var resource_type := str(resource.get_meta("resource_type"))
	var amount := int(resource.get_meta("resource_amount")) if resource.has_meta("resource_amount") else 1
	var display_name := str(resource.get_meta("resource_name")) if resource.has_meta("resource_name") else "Ressource"
	var remaining := int(resource.get_meta("harvests_remaining")) if resource.has_meta("harvests_remaining") else 1
	var required_tool := str(resource.get_meta("required_tool")) if resource.has_meta("required_tool") else ""
	var hud = get_hud()

	if required_tool != "" and int(inventory.get(required_tool, 0)) <= 0:
		if hud and hud.has_method("show_message"):
			hud.show_message("%s benoetigt %s" % [display_name, get_tool_label(required_tool)])
		return

	if required_tool != "" and selected_item != required_tool:
		if hud and hud.has_method("show_message"):
			hud.show_message("%s auswaehlen, um %s abzubauen" % [get_tool_label(required_tool), display_name])
		return

	inventory[resource_type] = int(inventory.get(resource_type, 0)) + amount
	remaining -= 1
	resource.set_meta("harvests_remaining", remaining)

	if hud:
		if hud.has_method("update_inventory"):
			hud.update_inventory(inventory)
		if hud.has_method("show_message"):
			var tool_text := " mit %s" % get_tool_label(required_tool) if required_tool != "" else ""
			hud.show_message("+%d %s%s gesammelt" % [amount, get_resource_label(resource_type), tool_text])

	if remaining <= 0:
		if resource == looked_at_resource:
			looked_at_resource = null
		mark_resource_harvested(resource)
		resource.queue_free()
	else:
		resource.scale *= 0.94
		if hud and hud.has_method("show_prompt"):
			var tool_text := " mit %s" % get_tool_label(required_tool) if required_tool != "" else ""
			hud.show_prompt("Links-Klick - %s%s abbauen" % [display_name, tool_text])

func update_interaction_prompt():
	var resource := get_resource_in_view()
	if resource == looked_at_resource:
		return

	looked_at_resource = resource
	var hud = get_hud()
	if not hud or not hud.has_method("show_prompt"):
		return

	if looked_at_resource:
		var display_name := str(looked_at_resource.get_meta("resource_name")) if looked_at_resource.has_meta("resource_name") else "Ressource"
		var required_tool := str(looked_at_resource.get_meta("required_tool")) if looked_at_resource.has_meta("required_tool") else ""
		var tool_text := " mit %s" % get_tool_label(required_tool) if required_tool != "" else ""
		hud.show_prompt("Links-Klick - %s%s abbauen" % [display_name, tool_text])
	else:
		hud.show_prompt("")

func get_resource_in_view() -> Node3D:
	var camera := get_node_or_null("Camera3D") as Camera3D
	if not camera:
		return null

	var from: Vector3 = camera.global_position
	var to: Vector3 = from + -camera.global_transform.basis.z * interact_range
	var query := PhysicsRayQueryParameters3D.create(from, to)
	query.exclude = [self]
	query.collision_mask = 2
	query.collide_with_areas = false
	query.collide_with_bodies = true

	var hit := get_world_3d().direct_space_state.intersect_ray(query)
	if hit.is_empty():
		return null

	return find_resource_parent(hit.get("collider"))

func find_resource_parent(node: Object) -> Node3D:
	var current := node as Node
	while current:
		if current.is_in_group("resources"):
			return current as Node3D
		current = current.get_parent()

	return null

func update_hud_inventory():
	var hud = get_hud()
	if hud and hud.has_method("update_inventory"):
		hud.update_inventory(inventory)
	update_hud_selection()

func update_hud_selection():
	var hud = get_hud()
	if hud and hud.has_method("update_selected_item"):
		hud.update_selected_item(selected_item, selected_hotbar_index)

func get_hud() -> Node:
	return get_parent().find_child("HUD", true, false)

func mark_resource_harvested(resource: Node3D):
	if not resource.has_meta("resource_id"):
		return

	var world = get_parent().find_child("World", true, false)
	if world and world.has_method("mark_resource_harvested"):
		world.mark_resource_harvested(str(resource.get_meta("resource_id")))

func toggle_inventory():
	var hud = get_hud()
	if hud and hud.has_method("toggle_inventory"):
		var opened: bool = bool(hud.toggle_inventory())
		Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE if opened else Input.MOUSE_MODE_CAPTURED)

func close_inventory():
	var hud = get_hud()
	if hud and hud.has_method("close_inventory"):
		hud.close_inventory()
	Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)

func is_inventory_open() -> bool:
	var hud = get_hud()
	if hud and hud.has_method("is_inventory_open"):
		return hud.is_inventory_open()

	return false

func is_primary_action_pressed(event: InputEvent) -> bool:
	return event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT

func handle_selection_input(event: InputEvent) -> bool:
	if event is InputEventKey and event.pressed and not event.echo:
		if event.keycode >= KEY_1 and event.keycode <= KEY_9:
			select_hotbar_index(event.keycode - KEY_1)
			return true

	if event is InputEventMouseButton and event.pressed and not is_inventory_open():
		if event.button_index == MOUSE_BUTTON_WHEEL_UP:
			cycle_selected_item(-1)
			return true
		if event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			cycle_selected_item(1)
			return true

	return false

func select_hotbar_index(index: int):
	if index < 0 or index >= HOTBAR_ITEMS.size():
		return

	select_item(HOTBAR_ITEMS[index], index)

func cycle_selected_item(direction: int):
	var current_index := HOTBAR_ITEMS.find(selected_item)
	if current_index < 0:
		current_index = 0

	for step in range(HOTBAR_ITEMS.size()):
		var next_index := wrapi(current_index + direction * (step + 1), 0, HOTBAR_ITEMS.size())
		var item_id: String = HOTBAR_ITEMS[next_index]
		if int(inventory.get(item_id, 0)) > 0:
			select_item(item_id, next_index)
			return

func select_item(item_id: String, hotbar_index := -1) -> bool:
	if not HOTBAR_ITEMS.has(item_id):
		return false

	if item_id == "":
		selected_item = item_id
		selected_hotbar_index = hotbar_index
		update_hud_selection()
		update_held_item_visual()
		return true

	if int(inventory.get(item_id, 0)) <= 0:
		var hud = get_hud()
		if hud and hud.has_method("show_message"):
			hud.show_message("%s ist noch nicht im Inventar" % get_item_label(item_id))
		return false

	selected_item = item_id
	selected_hotbar_index = hotbar_index if hotbar_index >= 0 else HOTBAR_ITEMS.find(item_id)
	update_hud_selection()
	update_held_item_visual()
	return true

func ensure_held_item_root():
	var camera := get_node_or_null("Camera3D") as Camera3D
	if not camera:
		return

	held_item_root = camera.get_node_or_null("HeldItem") as Node3D
	if not held_item_root:
		held_item_root = Node3D.new()
		held_item_root.name = "HeldItem"
		camera.add_child(held_item_root)

	held_item_root.position = Vector3(0.45, -0.45, -0.85)
	held_item_root.rotation_degrees = Vector3(-10.0, -18.0, 8.0)

func update_held_item_visual():
	ensure_held_item_root()
	if not held_item_root:
		return

	for child in held_item_root.get_children():
		held_item_root.remove_child(child)
		child.queue_free()

	held_item_root.visible = int(inventory.get(selected_item, 0)) > 0
	if not held_item_root.visible:
		return

	match selected_item:
		"wood":
			build_held_wood()
		"stone":
			build_held_stone()
		"fiber":
			build_held_fiber()
		"food":
			build_held_food()
		"axe":
			build_held_axe()
		"pickaxe":
			build_held_pickaxe()

func build_held_wood():
	var mesh := CylinderMesh.new()
	mesh.top_radius = 0.16
	mesh.bottom_radius = 0.20
	mesh.height = 0.75
	mesh.radial_segments = 10
	add_held_mesh(mesh, Vector3.ZERO, Vector3(0, 0, PI / 2.0), Vector3.ONE, Color(0.34, 0.18, 0.07, 1))

func build_held_stone():
	var mesh := BoxMesh.new()
	mesh.size = Vector3(0.42, 0.28, 0.34)
	add_held_mesh(mesh, Vector3.ZERO, Vector3(0.25, 0.35, 0.15), Vector3.ONE, Color(0.42, 0.44, 0.42, 1))

func build_held_fiber():
	for i in range(3):
		var mesh := CylinderMesh.new()
		mesh.top_radius = 0.025
		mesh.bottom_radius = 0.035
		mesh.height = 0.65
		mesh.radial_segments = 6
		add_held_mesh(mesh, Vector3((i - 1) * 0.06, 0.0, 0.0), Vector3(0.35 + i * 0.1, 0.0, 0.25), Vector3.ONE, Color(0.14, 0.48, 0.16, 1))

func build_held_food():
	var mesh := SphereMesh.new()
	mesh.radius = 0.22
	mesh.height = 0.36
	mesh.radial_segments = 14
	mesh.rings = 7
	add_held_mesh(mesh, Vector3.ZERO, Vector3.ZERO, Vector3.ONE, Color(0.72, 0.12, 0.10, 1))

func build_held_axe():
	var handle := CylinderMesh.new()
	handle.top_radius = 0.035
	handle.bottom_radius = 0.045
	handle.height = 0.9
	handle.radial_segments = 8
	add_held_mesh(handle, Vector3(0.0, -0.08, 0.0), Vector3(0.35, 0.0, 0.15), Vector3.ONE, Color(0.36, 0.20, 0.08, 1))

	var head := BoxMesh.new()
	head.size = Vector3(0.12, 0.30, 0.34)
	add_held_mesh(head, Vector3(-0.03, 0.34, -0.03), Vector3(0.0, 0.0, 0.25), Vector3.ONE, Color(0.54, 0.58, 0.58, 1))

func build_held_pickaxe():
	var handle := CylinderMesh.new()
	handle.top_radius = 0.035
	handle.bottom_radius = 0.045
	handle.height = 0.95
	handle.radial_segments = 8
	add_held_mesh(handle, Vector3(0.0, -0.09, 0.0), Vector3(0.35, 0.0, 0.15), Vector3.ONE, Color(0.31, 0.18, 0.08, 1))

	var head := BoxMesh.new()
	head.size = Vector3(0.72, 0.10, 0.12)
	add_held_mesh(head, Vector3(0.0, 0.36, -0.02), Vector3(0.0, 0.0, 0.1), Vector3.ONE, Color(0.46, 0.50, 0.52, 1))

func add_held_mesh(mesh: Mesh, position: Vector3, rotation: Vector3, scale: Vector3, color: Color):
	var mesh_instance := MeshInstance3D.new()
	mesh_instance.mesh = mesh
	mesh_instance.position = position
	mesh_instance.rotation = rotation
	mesh_instance.scale = scale
	mesh_instance.set_surface_override_material(0, create_held_material(color))
	held_item_root.add_child(mesh_instance)

func create_held_material(color: Color) -> StandardMaterial3D:
	var material := StandardMaterial3D.new()
	material.albedo_color = color
	material.roughness = 0.82
	material.metallic = 0.0
	return material

func get_resource_label(resource_type: String) -> String:
	match resource_type:
		"wood":
			return "Holz"
		"stone":
			return "Stein"
		"fiber":
			return "Fasern"
		"food":
			return "Nahrung"
		_:
			return resource_type

func get_item_label(item_id: String) -> String:
	match item_id:
		"wood":
			return "Holz"
		"stone":
			return "Stein"
		"fiber":
			return "Fasern"
		"food":
			return "Nahrung"
		"axe":
			return "Axt"
		"pickaxe":
			return "Spitzhacke"
		_:
			return item_id

func get_tool_label(tool_type: String) -> String:
	match tool_type:
		"axe":
			return "Axt"
		"pickaxe":
			return "Spitzhacke"
		_:
			return tool_type
