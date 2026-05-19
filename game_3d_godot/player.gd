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
const WATER_SURFACE_BODY_OFFSET := 0.35
const WATER_CURRENT_STRENGTH := 1.0
const WATER_VERTICAL_DRAG := 0.18
const HOTBAR_ITEMS := ["wood", "stone", "fiber", "food", "axe", "pickaxe", "spear", "torch", "campfire"]
const CRAFTING_RECIPES := [
	{"id": "axe", "name": "Axt", "amount": 1, "ingredients": {"wood": 1, "stone": 1, "fiber": 1}},
	{"id": "pickaxe", "name": "Spitzhacke", "amount": 1, "ingredients": {"wood": 2, "stone": 2, "fiber": 1}},
	{"id": "spear", "name": "Speer", "amount": 1, "ingredients": {"wood": 2, "stone": 1, "fiber": 1}},
	{"id": "torch", "name": "Fackel", "amount": 1, "ingredients": {"wood": 1, "fiber": 2}},
	{"id": "campfire", "name": "Lagerfeuer", "amount": 1, "ingredients": {"wood": 3, "stone": 4}},
]

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
	"spear": 0,
	"torch": 0,
	"campfire": 0,
}
var looked_at_resource: Node3D = null
var selected_item := "axe"
var selected_hotbar_index := 4
var held_item_root: Node3D = null
var invert_mouse_y := false

func _ready():
	configure_player_rig()
	ensure_held_item_root()
	connect_hud_settings()
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
	var menu_open := is_any_menu_open()
	var input_dir: Vector2 = Vector2.ZERO if menu_open else Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
	var direction = (transform.basis * Vector3(input_dir.x, 0, input_dir.y)).normalized()
	
	# Sprint
	is_sprinting = not menu_open and Input.is_action_pressed("sprint") and stamina > 0
	var current_speed = sprint_speed if is_sprinting else speed
	
	if direction:
		velocity.x = direction.x * current_speed
		velocity.z = direction.z * current_speed
	else:
		velocity.x = move_toward(velocity.x, 0, current_speed)
		velocity.z = move_toward(velocity.z, 0, current_speed)
	
	# Gravity
	velocity.y -= gravity * delta
	apply_water_current()
	
	# Jump
	if Input.is_action_just_pressed("ui_accept") and is_on_floor() and not menu_open:
		velocity.y = jump_force
	
	# Move
	move_and_slide()
	
	# Update Stats
	update_survival_stats(delta)
	update_interaction_prompt()

func _input(event):
	if is_keybind_capture_active():
		return

	if event.is_action_pressed("ui_cancel"):
		if is_settings_open():
			close_settings()
		elif is_pause_menu_open():
			close_pause_menu()
		elif is_inventory_open():
			close_inventory()
		else:
			open_pause_menu()
		return

	if is_pause_menu_open() or is_settings_open():
		return

	if event.is_action_pressed("inventory"):
		toggle_inventory()
		return

	if handle_selection_input(event):
		return

	if event is InputEventMouseMotion and not is_any_menu_open():
		rotate_y(-event.relative.x * mouse_sensitivity)
		var pitch_delta: float = event.relative.y * mouse_sensitivity if invert_mouse_y else -event.relative.y * mouse_sensitivity
		$Camera3D.rotate_x(pitch_delta)
		$Camera3D.rotation.x = clamp($Camera3D.rotation.x, -PI/2, PI/2)
	
	if is_primary_action_pressed(event) and not is_any_menu_open():
		interact_with_resource()

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

func apply_water_current():
	var world = get_parent().find_child("World", true, false)
	if not world or not world.has_method("get_water_state_at_position"):
		return

	var water_info: Dictionary = world.get_water_state_at_position(global_position.x, global_position.z)
	if not bool(water_info.get("active", false)):
		return

	var surface_y: float = float(water_info.get("surface_y", -99999.0))
	var submersion: float = clamp((surface_y - global_position.y + WATER_SURFACE_BODY_OFFSET) / BODY_HEIGHT, 0.0, 1.0)
	if submersion <= 0.0:
		return

	var current := Vector2.ZERO
	if water_info.has("current") and water_info["current"] is Vector2:
		current = water_info["current"]

	velocity.x += current.x * WATER_CURRENT_STRENGTH * submersion
	velocity.z += current.y * WATER_CURRENT_STRENGTH * submersion
	if global_position.y < surface_y:
		velocity.y = max(velocity.y, -gravity * WATER_VERTICAL_DRAG)

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

func connect_hud_settings():
	var hud = get_hud()
	if not hud:
		return

	var settings_callable := Callable(self, "apply_settings")
	if hud.has_signal("settings_changed") and not hud.is_connected("settings_changed", settings_callable):
		hud.connect("settings_changed", settings_callable)

	if hud.has_method("get_settings_state"):
		apply_settings(hud.get_settings_state())

func apply_settings(settings: Dictionary):
	if settings.has("mouse_sensitivity"):
		mouse_sensitivity = float(settings.get("mouse_sensitivity"))
	if settings.has("invert_y"):
		invert_mouse_y = bool(settings.get("invert_y"))
	if settings.has("interact_range"):
		interact_range = float(settings.get("interact_range"))

	var camera := get_node_or_null("Camera3D") as Camera3D
	if camera and settings.has("fov"):
		camera.fov = float(settings.get("fov"))

func mark_resource_harvested(resource: Node3D):
	if not resource.has_meta("resource_id"):
		return

	var world = get_parent().find_child("World", true, false)
	if world and world.has_method("mark_resource_harvested"):
		world.mark_resource_harvested(str(resource.get_meta("resource_id")))

func toggle_inventory():
	var hud = get_hud()
	if hud and hud.has_method("toggle_inventory"):
		hud.toggle_inventory()
		refresh_mouse_mode()

func close_inventory():
	var hud = get_hud()
	if hud and hud.has_method("close_inventory"):
		hud.close_inventory()
	refresh_mouse_mode()

func is_inventory_open() -> bool:
	var hud = get_hud()
	if hud and hud.has_method("is_inventory_open"):
		return hud.is_inventory_open()

	return false

func open_settings():
	var hud = get_hud()
	if hud and hud.has_method("open_settings"):
		hud.open_settings()
	refresh_mouse_mode()

func close_settings():
	var hud = get_hud()
	if hud and hud.has_method("close_settings"):
		hud.close_settings()
	refresh_mouse_mode()

func is_settings_open() -> bool:
	var hud = get_hud()
	if hud and hud.has_method("is_settings_open"):
		return hud.is_settings_open()

	return false

func open_pause_menu():
	var hud = get_hud()
	if hud and hud.has_method("open_pause_menu"):
		hud.open_pause_menu()
	refresh_mouse_mode()

func close_pause_menu():
	var hud = get_hud()
	if hud and hud.has_method("close_pause_menu"):
		hud.close_pause_menu()
	refresh_mouse_mode()

func is_pause_menu_open() -> bool:
	var hud = get_hud()
	if hud and hud.has_method("is_pause_menu_open"):
		return hud.is_pause_menu_open()

	return false

func is_any_menu_open() -> bool:
	var hud = get_hud()
	if hud and hud.has_method("is_any_menu_open"):
		return hud.is_any_menu_open()

	return is_inventory_open()

func is_keybind_capture_active() -> bool:
	var hud = get_hud()
	if hud and hud.has_method("is_keybind_capture_active"):
		return hud.is_keybind_capture_active()

	return false

func refresh_mouse_mode():
	Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE if is_any_menu_open() else Input.MOUSE_MODE_CAPTURED)

func is_primary_action_pressed(event: InputEvent) -> bool:
	if event.is_action_pressed("interact"):
		return true

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
		"spear":
			build_held_spear()
		"torch":
			build_held_torch()
		"campfire":
			build_held_campfire()

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

func build_held_spear():
	var shaft := CylinderMesh.new()
	shaft.top_radius = 0.025
	shaft.bottom_radius = 0.035
	shaft.height = 1.15
	shaft.radial_segments = 8
	add_held_mesh(shaft, Vector3(0.0, -0.05, 0.0), Vector3(0.42, 0.0, 0.12), Vector3.ONE, Color(0.36, 0.20, 0.08, 1))

	var tip := CylinderMesh.new()
	tip.bottom_radius = 0.09
	tip.top_radius = 0.0
	tip.height = 0.24
	tip.radial_segments = 8
	add_held_mesh(tip, Vector3(0.0, 0.52, -0.02), Vector3(0.42, 0.0, 0.12), Vector3.ONE, Color(0.55, 0.56, 0.52, 1))

func build_held_torch():
	var handle := CylinderMesh.new()
	handle.top_radius = 0.045
	handle.bottom_radius = 0.055
	handle.height = 0.85
	handle.radial_segments = 8
	add_held_mesh(handle, Vector3(0.0, -0.07, 0.0), Vector3(0.35, 0.0, 0.15), Vector3.ONE, Color(0.34, 0.18, 0.07, 1))

	var flame := SphereMesh.new()
	flame.radius = 0.13
	flame.height = 0.24
	flame.radial_segments = 10
	flame.rings = 5
	add_held_mesh(flame, Vector3(0.0, 0.36, -0.02), Vector3.ZERO, Vector3(0.75, 1.25, 0.75), Color(0.95, 0.42, 0.08, 1))

func build_held_campfire():
	for i in range(3):
		var log_mesh := CylinderMesh.new()
		log_mesh.top_radius = 0.035
		log_mesh.bottom_radius = 0.045
		log_mesh.height = 0.55
		log_mesh.radial_segments = 8
		add_held_mesh(log_mesh, Vector3((i - 1) * 0.06, -0.03, 0.0), Vector3(0.0, i * 0.7, PI / 2.0), Vector3.ONE, Color(0.33, 0.17, 0.07, 1))

	var stone := BoxMesh.new()
	stone.size = Vector3(0.38, 0.12, 0.30)
	add_held_mesh(stone, Vector3(0.0, -0.12, 0.0), Vector3(0.2, 0.4, 0.1), Vector3.ONE, Color(0.36, 0.37, 0.34, 1))

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

func craft_from_ingredients(ingredients: Dictionary) -> bool:
	var recipe := get_crafting_recipe_for_ingredients(ingredients)
	var hud = get_hud()
	if recipe.is_empty():
		if hud and hud.has_method("show_message"):
			hud.show_message("Diese Kombination ergibt nichts")
		return false

	var cost: Dictionary = recipe.get("ingredients", {})
	if not has_inventory_items(cost):
		if hud and hud.has_method("show_message"):
			hud.show_message("Material fehlt")
		return false

	for item_id in cost.keys():
		inventory[item_id] = int(inventory.get(item_id, 0)) - int(cost[item_id])

	var output_id := str(recipe.get("id", ""))
	var amount := int(recipe.get("amount", 1))
	inventory[output_id] = int(inventory.get(output_id, 0)) + amount
	update_hud_inventory()

	if hud and hud.has_method("show_message"):
		hud.show_message("%s hergestellt" % get_item_label(output_id))

	return true

func get_crafting_recipe_for_ingredients(ingredients: Dictionary) -> Dictionary:
	var normalized := normalize_ingredients(ingredients)
	for recipe in CRAFTING_RECIPES:
		var cost: Dictionary = recipe.get("ingredients", {})
		if ingredients_match(normalized, cost):
			return recipe

	return {}

func get_crafting_recipe_suggestions(ingredients: Dictionary) -> Array:
	var normalized := normalize_ingredients(ingredients)
	var suggestions := []
	for recipe in CRAFTING_RECIPES:
		var cost: Dictionary = recipe.get("ingredients", {})
		if normalized.is_empty() or ingredients_are_subset(normalized, cost):
			suggestions.append(recipe)

	return suggestions

func normalize_ingredients(ingredients: Dictionary) -> Dictionary:
	var normalized := {}
	for item_id in ingredients.keys():
		var amount := int(ingredients.get(item_id, 0))
		if item_id != "" and amount > 0:
			normalized[item_id] = amount

	return normalized

func ingredients_match(left: Dictionary, right: Dictionary) -> bool:
	if left.size() != right.size():
		return false

	for item_id in right.keys():
		if int(left.get(item_id, 0)) != int(right.get(item_id, 0)):
			return false

	return true

func ingredients_are_subset(selected: Dictionary, cost: Dictionary) -> bool:
	for item_id in selected.keys():
		if int(selected.get(item_id, 0)) > int(cost.get(item_id, 0)):
			return false

	return true

func has_inventory_items(cost: Dictionary) -> bool:
	for item_id in cost.keys():
		if int(inventory.get(item_id, 0)) < int(cost.get(item_id, 0)):
			return false

	return true

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
		"spear":
			return "Speer"
		"torch":
			return "Fackel"
		"campfire":
			return "Lagerfeuer"
		_:
			return item_id

func get_tool_label(tool_type: String) -> String:
	match tool_type:
		"axe":
			return "Axt"
		"pickaxe":
			return "Spitzhacke"
		"spear":
			return "Speer"
		"torch":
			return "Fackel"
		_:
			return tool_type
