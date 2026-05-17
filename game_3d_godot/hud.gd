# HUD.gd - User Interface
extends CanvasLayer

var inventory_panel: PanelContainer
var inventory_grid: GridContainer
var hotbar_panel: PanelContainer
var hotbar_container: HBoxContainer
var prompt_label: Label
var message_label: Label
var message_time := 0.0
var inventory_open := false
var current_inventory := {}
var selected_item := "axe"
var selected_hotbar_index := 4

const INVENTORY_ITEMS := ["wood", "stone", "fiber", "food", "axe", "pickaxe"]
const HOTBAR_ITEMS := ["wood", "stone", "fiber", "food", "axe", "pickaxe", "", "", ""]

func _ready():
	ensure_extra_labels()
	ensure_inventory_panel()
	ensure_hotbar()
	print("✅ HUD geladen")

func _process(delta):
	if message_time <= 0.0:
		return

	message_time -= delta
	if message_time <= 0.0:
		show_message("")

func update_stats(health: float, hunger: float, stamina: float):
	var health_label = find_child("HealthLabel")
	var hunger_label = find_child("HungerLabel")
	var stamina_label = find_child("StaminaLabel")
	
	if health_label:
		health_label.text = "❤ Health: %.1f/100" % health
		health_label.add_theme_color_override("font_color", Color.GREEN if health > 30 else Color.RED)
	
	if hunger_label:
		hunger_label.text = "🍗 Hunger: %.1f/100" % hunger
		hunger_label.add_theme_color_override("font_color", Color.GREEN if hunger > 30 else Color.ORANGE)
	
	if stamina_label:
		stamina_label.text = "⚡ Stamina: %.1f/100" % stamina
		stamina_label.add_theme_color_override("font_color", Color.GREEN if stamina > 30 else Color.YELLOW)

func update_inventory(inventory: Dictionary):
	ensure_extra_labels()
	ensure_inventory_panel()
	ensure_hotbar()
	current_inventory = inventory.duplicate()
	rebuild_inventory_grid()
	rebuild_hotbar()

func update_selected_item(item_id: String, hotbar_index := -1):
	selected_item = item_id
	selected_hotbar_index = hotbar_index
	rebuild_inventory_grid()
	rebuild_hotbar()

func toggle_inventory() -> bool:
	ensure_inventory_panel()
	inventory_open = not inventory_open
	inventory_panel.visible = inventory_open
	return inventory_open

func close_inventory():
	ensure_inventory_panel()
	inventory_open = false
	inventory_panel.visible = false

func is_inventory_open() -> bool:
	return inventory_open

func show_prompt(text: String):
	ensure_extra_labels()
	prompt_label.text = text
	prompt_label.visible = text != ""

func show_message(text: String):
	ensure_extra_labels()
	message_label.text = text
	message_label.visible = text != ""
	message_time = 2.0 if text != "" else 0.0

func ensure_extra_labels():
	var stats_container = find_child("VBoxContainer", true, false)
	if stats_container:
		var old_inventory_label = stats_container.find_child("InventoryLabel", true, false)
		if old_inventory_label:
			old_inventory_label.queue_free()

	if not prompt_label:
		prompt_label = find_child("PromptLabel", true, false)
		if not prompt_label:
			prompt_label = Label.new()
			prompt_label.name = "PromptLabel"
			prompt_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
			prompt_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
			prompt_label.set_anchors_preset(Control.PRESET_CENTER_BOTTOM)
			prompt_label.position = Vector2(-220, -136)
			prompt_label.size = Vector2(440, 42)
			add_child(prompt_label)
		prompt_label.visible = false
		prompt_label.add_theme_font_size_override("font_size", 24)
		prompt_label.add_theme_color_override("font_color", Color(1.0, 0.95, 0.72, 1.0))

	if not message_label:
		message_label = find_child("MessageLabel", true, false)
		if not message_label:
			message_label = Label.new()
			message_label.name = "MessageLabel"
			message_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
			message_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
			message_label.set_anchors_preset(Control.PRESET_CENTER_TOP)
			message_label.position = Vector2(-220, 86)
			message_label.size = Vector2(440, 42)
			add_child(message_label)
		message_label.visible = false
		message_label.add_theme_font_size_override("font_size", 22)
		message_label.add_theme_color_override("font_color", Color(0.82, 1.0, 0.78, 1.0))

func ensure_inventory_panel():
	if inventory_panel:
		return

	inventory_panel = PanelContainer.new()
	inventory_panel.name = "InventoryPanel"
	inventory_panel.set_anchors_preset(Control.PRESET_CENTER)
	inventory_panel.position = Vector2(-220, -190)
	inventory_panel.size = Vector2(440, 380)
	inventory_panel.visible = false
	add_child(inventory_panel)

	var panel_style := StyleBoxFlat.new()
	panel_style.bg_color = Color(0.07, 0.085, 0.075, 0.94)
	panel_style.border_color = Color(0.55, 0.66, 0.48, 1)
	panel_style.set_border_width_all(2)
	panel_style.corner_radius_top_left = 8
	panel_style.corner_radius_top_right = 8
	panel_style.corner_radius_bottom_left = 8
	panel_style.corner_radius_bottom_right = 8
	inventory_panel.add_theme_stylebox_override("panel", panel_style)

	var root := VBoxContainer.new()
	root.add_theme_constant_override("separation", 12)
	inventory_panel.add_child(root)

	var title := Label.new()
	title.text = "Inventar"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 28)
	title.add_theme_color_override("font_color", Color(0.92, 0.96, 0.82, 1))
	root.add_child(title)

	inventory_grid = GridContainer.new()
	inventory_grid.columns = 3
	inventory_grid.add_theme_constant_override("h_separation", 10)
	inventory_grid.add_theme_constant_override("v_separation", 10)
	root.add_child(inventory_grid)

	var hint := Label.new()
	hint.text = "Gegenstand auswaehlen"
	hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	hint.add_theme_font_size_override("font_size", 16)
	hint.add_theme_color_override("font_color", Color(0.72, 0.76, 0.66, 1))
	root.add_child(hint)

	rebuild_inventory_grid()

func ensure_hotbar():
	if hotbar_panel:
		return

	hotbar_panel = PanelContainer.new()
	hotbar_panel.name = "HotbarPanel"
	hotbar_panel.set_anchors_preset(Control.PRESET_CENTER_BOTTOM)
	hotbar_panel.position = Vector2(-373, -82)
	hotbar_panel.size = Vector2(746, 72)
	add_child(hotbar_panel)

	var panel_style := StyleBoxFlat.new()
	panel_style.bg_color = Color(0.05, 0.06, 0.05, 0.76)
	panel_style.border_color = Color(0.25, 0.30, 0.24, 0.9)
	panel_style.set_border_width_all(1)
	panel_style.corner_radius_top_left = 8
	panel_style.corner_radius_top_right = 8
	panel_style.corner_radius_bottom_left = 8
	panel_style.corner_radius_bottom_right = 8
	hotbar_panel.add_theme_stylebox_override("panel", panel_style)

	hotbar_container = HBoxContainer.new()
	hotbar_container.alignment = BoxContainer.ALIGNMENT_CENTER
	hotbar_container.add_theme_constant_override("separation", 8)
	hotbar_panel.add_child(hotbar_container)

	rebuild_hotbar()

func rebuild_inventory_grid():
	if not inventory_grid:
		return

	clear_children(inventory_grid)
	for item_index in range(INVENTORY_ITEMS.size()):
		var item_id: String = INVENTORY_ITEMS[item_index]
		inventory_grid.add_child(create_inventory_slot(item_id, int(current_inventory.get(item_id, 0)), false, item_index + 1, item_index))

func rebuild_hotbar():
	if not hotbar_container:
		return

	clear_children(hotbar_container)
	for item_index in range(HOTBAR_ITEMS.size()):
		var item_id: String = HOTBAR_ITEMS[item_index]
		hotbar_container.add_child(create_inventory_slot(item_id, int(current_inventory.get(item_id, 0)), true, item_index + 1, item_index))

func create_inventory_slot(item_id: String, amount: int, compact: bool, slot_number: int, slot_index: int) -> Control:
	var slot := PanelContainer.new()
	slot.custom_minimum_size = Vector2(78, 64) if compact else Vector2(128, 130)
	slot.mouse_filter = Control.MOUSE_FILTER_STOP
	slot.gui_input.connect(func(event: InputEvent):
		if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
			request_item_selection(item_id, slot_index if compact else -1)
	)

	var slot_style := StyleBoxFlat.new()
	var is_selected := slot_index == selected_hotbar_index if compact else item_id != "" and item_id == selected_item
	slot_style.bg_color = Color(0.18, 0.21, 0.17, 1) if is_selected else Color(0.13, 0.15, 0.13, 1)
	slot_style.border_color = Color(0.95, 0.78, 0.30, 1) if is_selected else Color(0.32, 0.38, 0.30, 1)
	slot_style.set_border_width_all(3 if is_selected else 1)
	slot_style.corner_radius_top_left = 6
	slot_style.corner_radius_top_right = 6
	slot_style.corner_radius_bottom_left = 6
	slot_style.corner_radius_bottom_right = 6
	slot.add_theme_stylebox_override("panel", slot_style)

	var content := VBoxContainer.new()
	content.alignment = BoxContainer.ALIGNMENT_CENTER
	content.mouse_filter = Control.MOUSE_FILTER_IGNORE
	content.add_theme_constant_override("separation", 2 if compact else 6)
	slot.add_child(content)

	var top_row := HBoxContainer.new()
	top_row.alignment = BoxContainer.ALIGNMENT_CENTER
	top_row.mouse_filter = Control.MOUSE_FILTER_IGNORE
	content.add_child(top_row)

	var number_label := Label.new()
	number_label.text = str(slot_number)
	number_label.custom_minimum_size = Vector2(16, 16)
	number_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	number_label.add_theme_font_size_override("font_size", 12)
	number_label.add_theme_color_override("font_color", Color(0.75, 0.78, 0.68, 1))
	number_label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	top_row.add_child(number_label)

	top_row.add_child(create_item_icon(item_id, 34 if compact else 62))

	if not compact:
		var name_label := Label.new()
		name_label.text = get_item_label(item_id)
		name_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		name_label.add_theme_font_size_override("font_size", 15)
		name_label.add_theme_color_override("font_color", Color(0.90, 0.92, 0.84, 1))
		name_label.mouse_filter = Control.MOUSE_FILTER_IGNORE
		content.add_child(name_label)

	if item_id != "":
		var amount_label := Label.new()
		amount_label.text = "x%d" % amount
		amount_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		amount_label.add_theme_font_size_override("font_size", 14 if compact else 20)
		amount_label.add_theme_color_override("font_color", Color(1.0, 0.92, 0.60, 1))
		amount_label.mouse_filter = Control.MOUSE_FILTER_IGNORE
		content.add_child(amount_label)

	return slot

func create_item_icon(item_id: String, icon_size: int = 62) -> Control:
	if item_id == "":
		var empty_icon := PanelContainer.new()
		empty_icon.custom_minimum_size = Vector2(icon_size, icon_size)
		empty_icon.mouse_filter = Control.MOUSE_FILTER_IGNORE

		var empty_style := StyleBoxFlat.new()
		empty_style.bg_color = Color(0.07, 0.08, 0.07, 0.82)
		empty_style.border_color = Color(0.22, 0.25, 0.22, 0.85)
		empty_style.set_border_width_all(1)
		empty_style.corner_radius_top_left = 8
		empty_style.corner_radius_top_right = 8
		empty_style.corner_radius_bottom_left = 8
		empty_style.corner_radius_bottom_right = 8
		empty_icon.add_theme_stylebox_override("panel", empty_style)

		return empty_icon

	var icon_path := "res://icons/%s.svg" % item_id
	if ResourceLoader.exists(icon_path, "Texture2D"):
		var texture := ResourceLoader.load(icon_path, "Texture2D") as Texture2D
		if texture:
			var texture_icon := PanelContainer.new()
			texture_icon.custom_minimum_size = Vector2(icon_size, icon_size)
			texture_icon.mouse_filter = Control.MOUSE_FILTER_IGNORE

			var texture_style := StyleBoxFlat.new()
			texture_style.bg_color = Color(0.09, 0.10, 0.09, 1)
			texture_style.border_color = Color(0.95, 0.95, 0.82, 0.45)
			texture_style.set_border_width_all(2)
			texture_style.corner_radius_top_left = 8
			texture_style.corner_radius_top_right = 8
			texture_style.corner_radius_bottom_left = 8
			texture_style.corner_radius_bottom_right = 8
			texture_icon.add_theme_stylebox_override("panel", texture_style)

			var texture_rect := TextureRect.new()
			texture_rect.texture = texture
			texture_rect.custom_minimum_size = Vector2(max(icon_size - 8, 24), max(icon_size - 8, 24))
			texture_rect.expand_mode = TextureRect.EXPAND_FIT_WIDTH_PROPORTIONAL
			texture_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
			texture_rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
			texture_icon.add_child(texture_rect)

			return texture_icon

	var icon := PanelContainer.new()
	icon.custom_minimum_size = Vector2(icon_size, icon_size)
	icon.mouse_filter = Control.MOUSE_FILTER_IGNORE

	var style := StyleBoxFlat.new()
	style.bg_color = get_item_icon_color(item_id)
	style.border_color = Color(0.95, 0.95, 0.82, 0.55)
	style.set_border_width_all(2)
	style.corner_radius_top_left = 8
	style.corner_radius_top_right = 8
	style.corner_radius_bottom_left = 8
	style.corner_radius_bottom_right = 8
	icon.add_theme_stylebox_override("panel", style)

	var symbol := Label.new()
	symbol.text = get_item_icon_symbol(item_id)
	symbol.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	symbol.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	symbol.add_theme_font_size_override("font_size", 18 if icon_size < 50 else 30)
	symbol.add_theme_color_override("font_color", Color(0.98, 0.96, 0.86, 1))
	symbol.mouse_filter = Control.MOUSE_FILTER_IGNORE
	icon.add_child(symbol)

	return icon

func get_item_icon_color(item_id: String) -> Color:
	match item_id:
		"wood":
			return Color(0.49, 0.27, 0.11, 1)
		"stone":
			return Color(0.42, 0.45, 0.45, 1)
		"fiber":
			return Color(0.20, 0.47, 0.16, 1)
		"food":
			return Color(0.65, 0.15, 0.13, 1)
		"axe":
			return Color(0.30, 0.37, 0.42, 1)
		"pickaxe":
			return Color(0.24, 0.30, 0.36, 1)
		_:
			return Color(0.25, 0.25, 0.25, 1)

func get_item_icon_symbol(item_id: String) -> String:
	match item_id:
		"wood":
			return "H"
		"stone":
			return "S"
		"fiber":
			return "F"
		"food":
			return "N"
		"axe":
			return "A"
		"pickaxe":
			return "P"
		_:
			return "?"

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

func clear_children(node: Node):
	for child in node.get_children():
		node.remove_child(child)
		child.queue_free()

func request_item_selection(item_id: String, slot_index := -1):
	var player = get_parent().find_child("Player", true, false)
	if player and player.has_method("select_item"):
		player.select_item(item_id, slot_index)
