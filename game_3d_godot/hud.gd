# HUD.gd - User Interface
extends CanvasLayer

signal settings_changed(settings: Dictionary)

var inventory_panel: PanelContainer
var inventory_grid: GridContainer
var hotbar_panel: PanelContainer
var hotbar_container: HBoxContainer
var crafting_items_container: HBoxContainer
var crafting_result_label: Label
var crafting_suggestions_container: VBoxContainer
var recipe_book_panel: PanelContainer
var recipe_book_body: VBoxContainer
var recipe_book_container: VBoxContainer
var recipe_search_input: LineEdit
var recipe_book_toggle: Button
var craft_button: Button
var pause_panel: Control
var settings_panel: Control
var settings_tabs: TabContainer
var prompt_label: Label
var message_label: Label
var message_time := 0.0
var inventory_open := false
var pause_open := false
var settings_open := false
var recipe_book_open := false
var recipe_search_text := ""
var ui_scale := 1.0
var current_inventory := {}
var crafting_items := {}
var selected_item := "axe"
var selected_hotbar_index := 4
var last_viewport_size := Vector2.ZERO
var layout_refresh_pending := false
var settings := {
	"hud_scale": 1.0,
	"icon_scale": 1.0,
	"mouse_sensitivity": 0.003,
	"invert_y": false,
	"fov": 75.0,
	"interact_range": 5.0,
	"show_hotbar": true,
	"show_stats": true,
}
var keybind_buttons := {}
var keybind_capture_action := ""

const INVENTORY_ITEMS := ["wood", "stone", "fiber", "food", "axe", "pickaxe", "spear", "torch", "campfire"]
const INVENTORY_SLOT_COUNT := 36
const INVENTORY_COLUMNS := 9
const INVENTORY_SLOT_SIZE := Vector2(118, 132)
const INVENTORY_ICON_SIZE := 92
const CRAFTING_COLUMN_WIDTH := 430
const HOTBAR_ITEMS := ["wood", "stone", "fiber", "food", "axe", "pickaxe", "spear", "torch", "campfire"]
const SETTINGS_PATH := "user://settings.cfg"
const DEFAULT_SETTINGS := {
	"hud_scale": 1.0,
	"icon_scale": 1.0,
	"mouse_sensitivity": 0.003,
	"invert_y": false,
	"fov": 75.0,
	"interact_range": 5.0,
	"show_hotbar": true,
	"show_stats": true,
}
const KEY_BIND_ACTIONS := [
	{"action": "ui_up", "label": "Vorwaerts", "default_key": KEY_W},
	{"action": "ui_down", "label": "Rueckwaerts", "default_key": KEY_S},
	{"action": "ui_left", "label": "Links", "default_key": KEY_A},
	{"action": "ui_right", "label": "Rechts", "default_key": KEY_D},
	{"action": "ui_accept", "label": "Springen", "default_key": KEY_SPACE},
	{"action": "sprint", "label": "Sprint", "default_key": KEY_SHIFT},
	{"action": "inventory", "label": "Inventar", "default_key": KEY_E},
	{"action": "interact", "label": "Interagieren", "default_key": KEY_F},
]

func _ready():
	load_settings()
	connect_viewport_resize()
	update_ui_scale()
	last_viewport_size = get_viewport().get_visible_rect().size
	ensure_extra_labels()
	ensure_inventory_panel()
	ensure_hotbar()
	ensure_pause_panel()
	ensure_settings_panel()
	apply_visual_settings(true)
	settings_changed.emit(get_settings_state())
	print("✅ HUD geladen")

func update_ui_scale():
	var viewport_size := get_viewport().get_visible_rect().size
	var scale_setting := get_setting_float("hud_scale")
	ui_scale = clamp(min(viewport_size.x / 1920.0, viewport_size.y / 1080.0) * scale_setting, 0.35, 1.80)

func connect_viewport_resize():
	var viewport := get_viewport()
	var resize_callable := Callable(self, "_on_viewport_resized")
	if not viewport.is_connected("size_changed", resize_callable):
		viewport.connect("size_changed", resize_callable)

func _on_viewport_resized():
	if layout_refresh_pending:
		return

	layout_refresh_pending = true
	call_deferred("refresh_layout_for_viewport")

func refresh_layout_for_viewport():
	layout_refresh_pending = false
	var viewport_size := get_viewport().get_visible_rect().size
	if viewport_size == last_viewport_size:
		return

	last_viewport_size = viewport_size
	apply_visual_settings(true)

func scaled(value: float) -> float:
	return value * ui_scale

func scaled_int(value: float) -> int:
	return int(round(value * ui_scale))

func scaled_vec(value: Vector2) -> Vector2:
	return value * ui_scale

func get_icon_scale() -> float:
	return get_setting_float("icon_scale")

func scaled_icon_size(value: float) -> int:
	return max(18, int(round(value * ui_scale * get_icon_scale())))

func _process(delta):
	if message_time <= 0.0:
		return

	message_time -= delta
	if message_time <= 0.0:
		show_message("")

func _input(event):
	if keybind_capture_action == "":
		return

	if event is InputEventKey and event.pressed and not event.echo:
		var keycode: int = event.keycode if event.keycode != KEY_NONE else event.physical_keycode
		if keycode == KEY_ESCAPE:
			cancel_keybind_capture()
		elif keycode != KEY_NONE:
			capture_key_binding(keybind_capture_action, keycode)
		get_viewport().set_input_as_handled()

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
	clamp_crafting_items()
	rebuild_inventory_grid()
	rebuild_hotbar()
	rebuild_crafting_panel()
	rebuild_recipe_book()
	bring_settings_to_front()

func update_selected_item(item_id: String, hotbar_index := -1):
	selected_item = item_id
	selected_hotbar_index = hotbar_index
	rebuild_inventory_grid()
	rebuild_hotbar()
	rebuild_crafting_panel()
	rebuild_recipe_book()

func toggle_inventory() -> bool:
	ensure_inventory_panel()
	inventory_open = not inventory_open
	inventory_panel.visible = inventory_open
	rebuild_crafting_panel()
	rebuild_recipe_book()
	return inventory_open

func close_inventory():
	ensure_inventory_panel()
	inventory_open = false
	inventory_panel.visible = false

func is_inventory_open() -> bool:
	return inventory_open

func open_pause_menu() -> bool:
	ensure_pause_panel()
	ensure_settings_panel()
	close_inventory()
	cancel_keybind_capture()
	settings_open = false
	settings_panel.visible = false
	pause_open = true
	pause_panel.visible = true
	bring_settings_to_front()
	request_player_mouse_mode_refresh()
	return pause_open

func close_pause_menu():
	ensure_pause_panel()
	cancel_keybind_capture()
	pause_open = false
	pause_panel.visible = false
	settings_open = false
	if settings_panel:
		settings_panel.visible = false
	request_player_mouse_mode_refresh()

func toggle_pause_menu() -> bool:
	if pause_open:
		close_pause_menu()
	else:
		open_pause_menu()
	return pause_open

func is_pause_menu_open() -> bool:
	return pause_open

func resume_game():
	close_pause_menu()

func leave_game():
	get_tree().quit()

func open_settings() -> bool:
	ensure_pause_panel()
	ensure_settings_panel()
	close_inventory()
	pause_open = true
	pause_panel.visible = false
	settings_open = true
	settings_panel.visible = true
	bring_settings_to_front()
	request_player_mouse_mode_refresh()
	return settings_open

func close_settings(return_to_pause := true):
	ensure_settings_panel()
	cancel_keybind_capture()
	settings_open = false
	settings_panel.visible = false
	if return_to_pause and pause_open and pause_panel:
		pause_panel.visible = true
		bring_settings_to_front()
	request_player_mouse_mode_refresh()

func toggle_settings() -> bool:
	if settings_open:
		close_settings()
	else:
		open_settings()
	return settings_open

func is_settings_open() -> bool:
	return settings_open

func is_any_menu_open() -> bool:
	return inventory_open or pause_open or settings_open

func is_keybind_capture_active() -> bool:
	return keybind_capture_action != ""

func get_settings_state() -> Dictionary:
	return settings.duplicate()

func request_player_mouse_mode_refresh():
	var player = get_player()
	if player and player.has_method("refresh_mouse_mode"):
		player.refresh_mouse_mode()

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

	update_ui_scale()
	inventory_panel = PanelContainer.new()
	inventory_panel.name = "InventoryPanel"
	inventory_panel.set_anchors_preset(Control.PRESET_FULL_RECT)
	inventory_panel.offset_left = scaled(30)
	inventory_panel.offset_top = scaled(30)
	inventory_panel.offset_right = -scaled(30)
	inventory_panel.offset_bottom = -scaled(30)
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
	root.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	root.size_flags_vertical = Control.SIZE_EXPAND_FILL
	root.add_theme_constant_override("separation", scaled_int(14))
	inventory_panel.add_child(root)

	var title := Label.new()
	title.text = "Inventar"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", scaled_int(32))
	title.add_theme_color_override("font_color", Color(0.92, 0.96, 0.82, 1))
	root.add_child(title)

	var body := HBoxContainer.new()
	body.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	body.size_flags_vertical = Control.SIZE_EXPAND_FILL
	body.add_theme_constant_override("separation", scaled_int(22))
	root.add_child(body)

	var inventory_column := VBoxContainer.new()
	var inventory_slot_width_scale: float = lerp(1.0, get_icon_scale(), 0.45)
	inventory_column.custom_minimum_size = Vector2(scaled(INVENTORY_SLOT_SIZE.x * inventory_slot_width_scale * INVENTORY_COLUMNS + 8.0 * (INVENTORY_COLUMNS - 1)), scaled(620))
	inventory_column.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	inventory_column.add_theme_constant_override("separation", scaled_int(10))
	body.add_child(inventory_column)

	inventory_grid = GridContainer.new()
	inventory_grid.columns = INVENTORY_COLUMNS
	inventory_grid.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	inventory_grid.size_flags_vertical = Control.SIZE_EXPAND_FILL
	inventory_grid.add_theme_constant_override("h_separation", scaled_int(8))
	inventory_grid.add_theme_constant_override("v_separation", scaled_int(9))
	inventory_column.add_child(inventory_grid)

	var side_column := VBoxContainer.new()
	side_column.custom_minimum_size = Vector2(scaled(CRAFTING_COLUMN_WIDTH), scaled(620))
	side_column.add_theme_constant_override("separation", scaled_int(14))
	body.add_child(side_column)

	var crafting_panel := PanelContainer.new()
	crafting_panel.name = "CraftingMat"
	crafting_panel.custom_minimum_size = Vector2(scaled(CRAFTING_COLUMN_WIDTH), scaled(260))
	side_column.add_child(crafting_panel)

	var crafting_style := StyleBoxFlat.new()
	crafting_style.bg_color = Color(0.10, 0.105, 0.085, 0.92)
	crafting_style.border_color = Color(0.40, 0.45, 0.34, 1)
	crafting_style.set_border_width_all(1)
	crafting_style.corner_radius_top_left = 8
	crafting_style.corner_radius_top_right = 8
	crafting_style.corner_radius_bottom_left = 8
	crafting_style.corner_radius_bottom_right = 8
	crafting_panel.add_theme_stylebox_override("panel", crafting_style)

	var crafting_root := VBoxContainer.new()
	crafting_root.add_theme_constant_override("separation", scaled_int(10))
	crafting_panel.add_child(crafting_root)

	var crafting_title := Label.new()
	crafting_title.text = "Kombinieren"
	crafting_title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	crafting_title.add_theme_font_size_override("font_size", scaled_int(23))
	crafting_title.add_theme_color_override("font_color", Color(0.92, 0.96, 0.82, 1))
	crafting_root.add_child(crafting_title)

	crafting_items_container = HBoxContainer.new()
	crafting_items_container.alignment = BoxContainer.ALIGNMENT_CENTER
	crafting_items_container.custom_minimum_size = Vector2(scaled(CRAFTING_COLUMN_WIDTH - 30), scaled(112))
	crafting_items_container.add_theme_constant_override("separation", scaled_int(12))
	crafting_root.add_child(crafting_items_container)

	crafting_result_label = Label.new()
	crafting_result_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	crafting_result_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	crafting_result_label.custom_minimum_size = Vector2(scaled(CRAFTING_COLUMN_WIDTH - 30), scaled(40))
	crafting_result_label.add_theme_font_size_override("font_size", scaled_int(19))
	crafting_result_label.add_theme_color_override("font_color", Color(0.92, 0.90, 0.74, 1))
	crafting_root.add_child(crafting_result_label)

	var action_row := HBoxContainer.new()
	action_row.alignment = BoxContainer.ALIGNMENT_CENTER
	action_row.add_theme_constant_override("separation", scaled_int(8))
	crafting_root.add_child(action_row)

	craft_button = Button.new()
	craft_button.text = "Kombinieren"
	craft_button.custom_minimum_size = scaled_vec(Vector2(200, 46))
	craft_button.pressed.connect(craft_selected_items)
	action_row.add_child(craft_button)

	var clear_button := Button.new()
	clear_button.text = "Leeren"
	clear_button.custom_minimum_size = scaled_vec(Vector2(108, 46))
	clear_button.pressed.connect(clear_crafting_selection)
	action_row.add_child(clear_button)

	crafting_suggestions_container = VBoxContainer.new()
	crafting_suggestions_container.visible = false
	crafting_suggestions_container.add_theme_constant_override("separation", scaled_int(5))
	crafting_root.add_child(crafting_suggestions_container)

	recipe_book_panel = PanelContainer.new()
	recipe_book_panel.name = "RecipeBook"
	recipe_book_panel.custom_minimum_size = Vector2(scaled(CRAFTING_COLUMN_WIDTH), scaled(64))
	recipe_book_panel.size_flags_vertical = Control.SIZE_SHRINK_BEGIN
	side_column.add_child(recipe_book_panel)

	var book_style := StyleBoxFlat.new()
	book_style.bg_color = Color(0.12, 0.105, 0.075, 0.94)
	book_style.border_color = Color(0.50, 0.39, 0.24, 1)
	book_style.set_border_width_all(1)
	book_style.corner_radius_top_left = 8
	book_style.corner_radius_top_right = 8
	book_style.corner_radius_bottom_left = 8
	book_style.corner_radius_bottom_right = 8
	recipe_book_panel.add_theme_stylebox_override("panel", book_style)

	var book_root := VBoxContainer.new()
	book_root.add_theme_constant_override("separation", scaled_int(10))
	recipe_book_panel.add_child(book_root)

	recipe_book_toggle = Button.new()
	recipe_book_toggle.text = "Buch >"
	recipe_book_toggle.custom_minimum_size = scaled_vec(Vector2(CRAFTING_COLUMN_WIDTH - 28, 44))
	recipe_book_toggle.pressed.connect(toggle_recipe_book)
	book_root.add_child(recipe_book_toggle)

	recipe_book_body = VBoxContainer.new()
	recipe_book_body.visible = recipe_book_open
	recipe_book_body.add_theme_constant_override("separation", scaled_int(8))
	book_root.add_child(recipe_book_body)

	recipe_search_input = LineEdit.new()
	recipe_search_input.placeholder_text = "Suchen"
	recipe_search_input.custom_minimum_size = scaled_vec(Vector2(CRAFTING_COLUMN_WIDTH - 28, 40))
	recipe_search_input.text_changed.connect(update_recipe_search)
	recipe_book_body.add_child(recipe_search_input)

	recipe_book_container = VBoxContainer.new()
	recipe_book_container.size_flags_vertical = Control.SIZE_EXPAND_FILL
	recipe_book_container.add_theme_constant_override("separation", scaled_int(7))
	recipe_book_body.add_child(recipe_book_container)

	rebuild_inventory_grid()
	rebuild_crafting_panel()
	rebuild_recipe_book()

func ensure_hotbar():
	if hotbar_panel:
		return

	hotbar_panel = PanelContainer.new()
	hotbar_panel.name = "HotbarPanel"
	hotbar_panel.set_anchors_preset(Control.PRESET_CENTER_BOTTOM)
	var compact_slot_size := scaled_vec(Vector2(78, 64) * get_icon_scale())
	var hotbar_width := compact_slot_size.x * HOTBAR_ITEMS.size() + scaled(8) * (HOTBAR_ITEMS.size() - 1) + scaled(24)
	var hotbar_height := compact_slot_size.y + scaled(16)
	hotbar_panel.position = Vector2(-hotbar_width * 0.5, -hotbar_height - scaled(10))
	hotbar_panel.size = Vector2(hotbar_width, hotbar_height)
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
	hotbar_container.add_theme_constant_override("separation", scaled_int(8))
	hotbar_panel.add_child(hotbar_container)

	rebuild_hotbar()
	apply_hotbar_visibility()

func ensure_pause_panel():
	if pause_panel:
		return

	pause_panel = Control.new()
	pause_panel.name = "PauseMenu"
	pause_panel.set_anchors_preset(Control.PRESET_FULL_RECT)
	pause_panel.mouse_filter = Control.MOUSE_FILTER_STOP
	pause_panel.visible = pause_open
	add_child(pause_panel)

	pause_panel.add_child(create_overlay_dim())

	var center := CenterContainer.new()
	center.set_anchors_preset(Control.PRESET_FULL_RECT)
	pause_panel.add_child(center)

	var card := PanelContainer.new()
	card.custom_minimum_size = get_responsive_panel_size(Vector2(500, 440), Vector2(320, 320), Vector2(28, 28))
	card.add_theme_stylebox_override("panel", create_panel_style(Color(0.045, 0.055, 0.050, 0.98), Color(0.72, 0.80, 0.58, 1), 8, 2))
	center.add_child(card)

	var root := VBoxContainer.new()
	root.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	root.size_flags_vertical = Control.SIZE_EXPAND_FILL
	root.add_theme_constant_override("separation", max(8, scaled_int(18)))
	card.add_child(root)

	var title := Label.new()
	title.text = "Pause"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", max(24, scaled_int(42)))
	title.add_theme_color_override("font_color", Color(0.96, 0.98, 0.86, 1))
	root.add_child(title)

	var subtitle := Label.new()
	subtitle.text = "Survival World 3D"
	subtitle.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	subtitle.add_theme_font_size_override("font_size", max(12, scaled_int(17)))
	subtitle.add_theme_color_override("font_color", Color(0.62, 0.70, 0.58, 1))
	root.add_child(subtitle)

	var separator := ColorRect.new()
	separator.color = Color(0.62, 0.70, 0.52, 0.35)
	separator.custom_minimum_size = Vector2(1, 1)
	separator.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	root.add_child(separator)

	var buttons := VBoxContainer.new()
	buttons.size_flags_vertical = Control.SIZE_EXPAND_FILL
	buttons.alignment = BoxContainer.ALIGNMENT_CENTER
	buttons.add_theme_constant_override("separation", max(8, scaled_int(12)))
	root.add_child(buttons)

	var resume_button := create_menu_button("Spiel fortsetzen", "primary")
	resume_button.pressed.connect(resume_game)
	buttons.add_child(resume_button)

	var settings_button := create_menu_button("Einstellungen", "secondary")
	settings_button.pressed.connect(open_settings)
	buttons.add_child(settings_button)

	var leave_button := create_menu_button("Spiel verlassen", "danger")
	leave_button.pressed.connect(leave_game)
	buttons.add_child(leave_button)

func ensure_settings_panel():
	if settings_panel:
		return

	settings_panel = Control.new()
	settings_panel.name = "SettingsPanel"
	settings_panel.set_anchors_preset(Control.PRESET_FULL_RECT)
	settings_panel.mouse_filter = Control.MOUSE_FILTER_STOP
	settings_panel.visible = settings_open
	add_child(settings_panel)

	settings_panel.add_child(create_overlay_dim())

	var center := CenterContainer.new()
	center.set_anchors_preset(Control.PRESET_FULL_RECT)
	settings_panel.add_child(center)

	var card := PanelContainer.new()
	card.custom_minimum_size = get_responsive_panel_size(Vector2(900, 600), Vector2(360, 360), Vector2(24, 24))
	card.add_theme_stylebox_override("panel", create_panel_style(Color(0.050, 0.060, 0.055, 0.985), Color(0.64, 0.74, 0.54, 1), 8, 2))
	center.add_child(card)

	var root := VBoxContainer.new()
	root.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	root.size_flags_vertical = Control.SIZE_EXPAND_FILL
	root.add_theme_constant_override("separation", max(8, scaled_int(16)))
	card.add_child(root)

	var header := HBoxContainer.new()
	header.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	header.add_theme_constant_override("separation", max(8, scaled_int(12)))
	root.add_child(header)

	var title_column := VBoxContainer.new()
	title_column.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	title_column.add_theme_constant_override("separation", max(1, scaled_int(2)))
	header.add_child(title_column)

	var title := Label.new()
	title.text = "Einstellungen"
	title.add_theme_font_size_override("font_size", max(22, scaled_int(34)))
	title.add_theme_color_override("font_color", Color(0.94, 0.97, 0.84, 1))
	title_column.add_child(title)

	var subtitle := Label.new()
	subtitle.text = "Steuerung, HUD und Spielgefuehl"
	subtitle.add_theme_font_size_override("font_size", max(12, scaled_int(16)))
	subtitle.add_theme_color_override("font_color", Color(0.62, 0.70, 0.58, 1))
	title_column.add_child(subtitle)

	var reset_button := Button.new()
	reset_button.text = "Zuruecksetzen"
	reset_button.custom_minimum_size = Vector2(max(118, scaled(160)), max(34, scaled(42)))
	style_menu_button(reset_button, "secondary")
	reset_button.pressed.connect(reset_settings)
	header.add_child(reset_button)

	var close_button := Button.new()
	close_button.text = "Zurueck"
	close_button.custom_minimum_size = Vector2(max(94, scaled(120)), max(34, scaled(42)))
	style_menu_button(close_button, "primary")
	close_button.pressed.connect(func(): close_settings())
	header.add_child(close_button)

	settings_tabs = TabContainer.new()
	settings_tabs.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	settings_tabs.size_flags_vertical = Control.SIZE_EXPAND_FILL
	settings_tabs.add_theme_font_size_override("font_size", max(13, scaled_int(17)))
	root.add_child(settings_tabs)

	settings_tabs.add_child(create_gameplay_settings_tab())
	settings_tabs.add_child(create_keybind_settings_tab())
	bring_settings_to_front()

func create_overlay_dim() -> ColorRect:
	var dim := ColorRect.new()
	dim.set_anchors_preset(Control.PRESET_FULL_RECT)
	dim.color = Color(0.0, 0.0, 0.0, 0.58)
	dim.mouse_filter = Control.MOUSE_FILTER_STOP
	return dim

func get_responsive_panel_size(base_size: Vector2, min_size: Vector2, margin: Vector2) -> Vector2:
	var viewport_size := get_viewport().get_visible_rect().size
	var desired_size := scaled_vec(base_size)
	var max_size := Vector2(max(220.0, viewport_size.x - margin.x * 2.0), max(220.0, viewport_size.y - margin.y * 2.0))
	return Vector2(
		min(max(desired_size.x, min_size.x), max_size.x),
		min(max(desired_size.y, min_size.y), max_size.y)
	)

func create_panel_style(bg_color: Color, border_color: Color, radius := 8, border_width := 1) -> StyleBoxFlat:
	var panel_style := StyleBoxFlat.new()
	panel_style.bg_color = bg_color
	panel_style.border_color = border_color
	panel_style.set_border_width_all(border_width)
	panel_style.content_margin_left = max(8, scaled_int(24))
	panel_style.content_margin_top = max(8, scaled_int(22))
	panel_style.content_margin_right = max(8, scaled_int(24))
	panel_style.content_margin_bottom = max(8, scaled_int(22))
	panel_style.corner_radius_top_left = radius
	panel_style.corner_radius_top_right = radius
	panel_style.corner_radius_bottom_left = radius
	panel_style.corner_radius_bottom_right = radius
	return panel_style

func create_menu_button(text: String, kind := "secondary") -> Button:
	var button := Button.new()
	button.text = text
	button.custom_minimum_size = Vector2(max(230, scaled(340)), max(42, scaled(54)))
	button.add_theme_font_size_override("font_size", max(15, scaled_int(21)))
	style_menu_button(button, kind)
	return button

func style_menu_button(button: Button, kind := "secondary"):
	var normal_color := Color(0.18, 0.24, 0.18, 1)
	var hover_color := Color(0.24, 0.32, 0.22, 1)
	var pressed_color := Color(0.12, 0.18, 0.13, 1)
	var border_color := Color(0.52, 0.66, 0.45, 1)
	var font_color := Color(0.94, 0.98, 0.88, 1)

	if kind == "primary":
		normal_color = Color(0.30, 0.43, 0.25, 1)
		hover_color = Color(0.40, 0.55, 0.32, 1)
		pressed_color = Color(0.22, 0.34, 0.20, 1)
		border_color = Color(0.76, 0.88, 0.58, 1)
	elif kind == "danger":
		normal_color = Color(0.38, 0.16, 0.14, 1)
		hover_color = Color(0.54, 0.22, 0.18, 1)
		pressed_color = Color(0.28, 0.11, 0.10, 1)
		border_color = Color(0.92, 0.48, 0.36, 1)
		font_color = Color(1.0, 0.90, 0.84, 1)

	button.add_theme_color_override("font_color", font_color)
	button.add_theme_color_override("font_hover_color", Color(1, 1, 0.92, 1))
	button.add_theme_color_override("font_pressed_color", font_color)
	button.add_theme_stylebox_override("normal", create_panel_style(normal_color, border_color, 8, 1))
	button.add_theme_stylebox_override("hover", create_panel_style(hover_color, border_color.lightened(0.12), 8, 2))
	button.add_theme_stylebox_override("pressed", create_panel_style(pressed_color, border_color.darkened(0.16), 8, 1))
	button.add_theme_stylebox_override("focus", create_panel_style(Color(0.0, 0.0, 0.0, 0.0), Color(0.95, 0.88, 0.54, 0.95), 8, 2))

func create_gameplay_settings_tab() -> Control:
	var scroll := ScrollContainer.new()
	scroll.name = "Spiel"
	scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL

	var list := VBoxContainer.new()
	list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	list.add_theme_constant_override("separation", max(8, scaled_int(12)))
	scroll.add_child(list)

	list.add_child(create_slider_setting("HUD-Groesse", "hud_scale", 0.75, 1.45, 0.05, 0, "%", 100.0))
	list.add_child(create_slider_setting("Icon-Groesse", "icon_scale", 0.75, 1.35, 0.05, 0, "%", 100.0))
	list.add_child(create_slider_setting("Maus-Sensitivitaet", "mouse_sensitivity", 0.001, 0.010, 0.0005, 1, "", 1000.0))
	list.add_child(create_slider_setting("Sichtfeld", "fov", 60.0, 100.0, 1.0, 0, " Grad", 1.0))
	list.add_child(create_slider_setting("Interaktionsreichweite", "interact_range", 3.0, 10.0, 0.5, 1, " m", 1.0))
	list.add_child(create_toggle_setting("Maus Y-Achse invertieren", "invert_y"))
	list.add_child(create_toggle_setting("Statusleisten anzeigen", "show_stats"))
	list.add_child(create_toggle_setting("Hotbar anzeigen", "show_hotbar"))

	return scroll

func create_keybind_settings_tab() -> Control:
	var scroll := ScrollContainer.new()
	scroll.name = "Tasten"
	scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL

	var list := VBoxContainer.new()
	list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	list.add_theme_constant_override("separation", max(8, scaled_int(10)))
	scroll.add_child(list)

	var hint := Label.new()
	hint.text = "Aktion anklicken, dann neue Taste druecken. ESC bricht die Eingabe ab."
	hint.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	hint.add_theme_font_size_override("font_size", max(12, scaled_int(17)))
	hint.add_theme_color_override("font_color", Color(0.78, 0.82, 0.70, 1))
	list.add_child(hint)

	for entry in KEY_BIND_ACTIONS:
		list.add_child(create_keybind_row(str(entry.get("label", "")), str(entry.get("action", ""))))

	return scroll

func create_slider_setting(label_text: String, key: String, min_value: float, max_value: float, step: float, decimals: int, suffix: String, display_multiplier := 1.0) -> Control:
	var row := VBoxContainer.new()
	row.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	row.add_theme_constant_override("separation", max(4, scaled_int(8)))

	var label := Label.new()
	label.add_theme_font_size_override("font_size", max(12, scaled_int(17)))
	label.add_theme_color_override("font_color", Color(0.90, 0.94, 0.82, 1))
	row.add_child(label)

	var slider := HSlider.new()
	slider.min_value = min_value
	slider.max_value = max_value
	slider.step = step
	slider.value = get_setting_float(key)
	slider.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	row.add_child(slider)

	var update_label := func(value: float):
		label.text = "%s: %s%s" % [label_text, format_setting_number(value * display_multiplier, decimals), suffix]

	update_label.call(slider.value)
	slider.value_changed.connect(func(value: float):
		update_label.call(value)
		set_setting_value(key, value)
	)

	return create_setting_card(row)

func create_toggle_setting(label_text: String, key: String) -> Control:
	var checkbox := CheckBox.new()
	checkbox.text = label_text
	checkbox.button_pressed = get_setting_bool(key)
	checkbox.add_theme_font_size_override("font_size", max(13, scaled_int(18)))
	checkbox.add_theme_color_override("font_color", Color(0.92, 0.94, 0.84, 1))
	checkbox.toggled.connect(func(pressed: bool):
		set_setting_value(key, pressed)
	)
	return create_setting_card(checkbox)

func create_keybind_row(label_text: String, action: String) -> Control:
	var row := HBoxContainer.new()
	row.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	row.add_theme_constant_override("separation", max(8, scaled_int(12)))

	var label := Label.new()
	label.text = label_text
	label.custom_minimum_size = Vector2(max(140, scaled(240)), max(32, scaled(40)))
	label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	label.add_theme_font_size_override("font_size", max(13, scaled_int(18)))
	label.add_theme_color_override("font_color", Color(0.92, 0.94, 0.84, 1))
	row.add_child(label)

	var button := Button.new()
	button.custom_minimum_size = Vector2(max(128, scaled(220)), max(32, scaled(40)))
	button.size_flags_horizontal = Control.SIZE_SHRINK_BEGIN
	style_menu_button(button, "secondary")
	button.pressed.connect(func():
		start_keybind_capture(action)
	)
	row.add_child(button)
	keybind_buttons[action] = button
	update_keybind_button(action)

	return create_setting_card(row)

func create_setting_card(content: Control) -> PanelContainer:
	var card := PanelContainer.new()
	card.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	card.add_theme_stylebox_override("panel", create_panel_style(Color(0.095, 0.115, 0.095, 0.88), Color(0.30, 0.38, 0.27, 0.90), 8, 1))
	card.add_child(content)
	return card

func load_settings():
	settings = DEFAULT_SETTINGS.duplicate()
	var config := ConfigFile.new()
	var load_result := config.load(SETTINGS_PATH)
	ensure_default_key_bindings(load_result != OK)

	if load_result == OK:
		for key in DEFAULT_SETTINGS.keys():
			if config.has_section_key("settings", key):
				settings[key] = config.get_value("settings", key)

		for entry in KEY_BIND_ACTIONS:
			var action := str(entry.get("action", ""))
			if config.has_section_key("keybinds", action):
				apply_key_binding(action, int(config.get_value("keybinds", action)), false)

	clamp_settings()

func ensure_default_key_bindings(force_defaults := false):
	for entry in KEY_BIND_ACTIONS:
		var action := str(entry.get("action", ""))
		var default_key := int(entry.get("default_key", KEY_NONE))
		if action == "":
			continue
		if not InputMap.has_action(action):
			InputMap.add_action(action)
		if (force_defaults or InputMap.action_get_events(action).is_empty()) and default_key != KEY_NONE:
			apply_key_binding(action, default_key, false)

func save_settings():
	var config := ConfigFile.new()
	for key in DEFAULT_SETTINGS.keys():
		config.set_value("settings", key, settings.get(key, DEFAULT_SETTINGS.get(key)))

	for entry in KEY_BIND_ACTIONS:
		var action := str(entry.get("action", ""))
		config.set_value("keybinds", action, get_binding_keycode(action))

	config.save(SETTINGS_PATH)

func reset_settings():
	settings = DEFAULT_SETTINGS.duplicate()
	for entry in KEY_BIND_ACTIONS:
		apply_key_binding(str(entry.get("action", "")), int(entry.get("default_key", KEY_NONE)), false)
	save_settings()
	recreate_scaled_panels()
	refresh_settings_controls()
	apply_visual_settings(false)
	settings_changed.emit(get_settings_state())
	show_message("Einstellungen zurueckgesetzt")

func set_setting_value(key: String, value):
	settings[key] = value
	clamp_settings()
	save_settings()
	apply_visual_settings(key == "hud_scale" or key == "icon_scale")
	settings_changed.emit(get_settings_state())

func clamp_settings():
	settings["hud_scale"] = clamp(get_setting_float("hud_scale"), 0.75, 1.45)
	settings["icon_scale"] = clamp(get_setting_float("icon_scale"), 0.75, 1.35)
	settings["mouse_sensitivity"] = clamp(get_setting_float("mouse_sensitivity"), 0.001, 0.010)
	settings["fov"] = clamp(get_setting_float("fov"), 60.0, 100.0)
	settings["interact_range"] = clamp(get_setting_float("interact_range"), 3.0, 10.0)
	settings["invert_y"] = get_setting_bool("invert_y")
	settings["show_hotbar"] = get_setting_bool("show_hotbar")
	settings["show_stats"] = get_setting_bool("show_stats")

func get_setting_float(key: String) -> float:
	return float(settings.get(key, DEFAULT_SETTINGS.get(key, 0.0)))

func get_setting_bool(key: String) -> bool:
	return bool(settings.get(key, DEFAULT_SETTINGS.get(key, false)))

func apply_visual_settings(rebuild_panels := true):
	update_ui_scale()
	apply_extra_label_layout()
	apply_stats_visibility()
	apply_hotbar_visibility()
	if rebuild_panels:
		recreate_scaled_panels()

func recreate_scaled_panels():
	recreate_hotbar()
	recreate_inventory_panel()
	recreate_pause_panel()
	recreate_settings_panel()
	bring_settings_to_front()

func recreate_hotbar():
	if hotbar_panel:
		remove_child(hotbar_panel)
		hotbar_panel.queue_free()
	hotbar_panel = null
	hotbar_container = null
	ensure_hotbar()

func recreate_inventory_panel():
	var was_open := inventory_open
	if inventory_panel:
		remove_child(inventory_panel)
		inventory_panel.queue_free()
	inventory_panel = null
	inventory_grid = null
	crafting_items_container = null
	crafting_result_label = null
	crafting_suggestions_container = null
	recipe_book_panel = null
	recipe_book_body = null
	recipe_book_container = null
	recipe_search_input = null
	recipe_book_toggle = null
	craft_button = null
	ensure_inventory_panel()
	inventory_panel.visible = was_open

func recreate_pause_panel():
	var was_visible: bool = pause_panel != null and pause_panel.visible
	if pause_panel:
		remove_child(pause_panel)
		pause_panel.queue_free()
	pause_panel = null
	ensure_pause_panel()
	pause_panel.visible = was_visible

func recreate_settings_panel():
	var was_visible: bool = settings_panel != null and settings_panel.visible
	keybind_buttons.clear()
	if settings_panel:
		remove_child(settings_panel)
		settings_panel.queue_free()
	settings_panel = null
	settings_tabs = null
	ensure_settings_panel()
	settings_panel.visible = was_visible

func apply_extra_label_layout():
	if prompt_label:
		prompt_label.position = scaled_vec(Vector2(-220, -136))
		prompt_label.size = scaled_vec(Vector2(440, 42))
		prompt_label.add_theme_font_size_override("font_size", scaled_int(24))
	if message_label:
		message_label.position = scaled_vec(Vector2(-220, 86))
		message_label.size = scaled_vec(Vector2(440, 42))
		message_label.add_theme_font_size_override("font_size", scaled_int(22))

func apply_stats_visibility():
	var stats_container = find_child("VBoxContainer", true, false)
	if stats_container:
		stats_container.visible = get_setting_bool("show_stats")
		for child in stats_container.get_children():
			if child is Label:
				child.add_theme_font_size_override("font_size", scaled_int(24))

func apply_hotbar_visibility():
	if hotbar_panel:
		hotbar_panel.visible = get_setting_bool("show_hotbar")

func bring_settings_to_front():
	if pause_panel and pause_panel.get_parent() == self:
		move_child(pause_panel, get_child_count() - 1)
	if settings_panel and settings_panel.get_parent() == self:
		move_child(settings_panel, get_child_count() - 1)

func refresh_settings_controls():
	recreate_settings_panel()
	settings_panel.visible = settings_open
	bring_settings_to_front()

func start_keybind_capture(action: String):
	keybind_capture_action = action
	for key in keybind_buttons.keys():
		update_keybind_button(str(key))

func cancel_keybind_capture():
	if keybind_capture_action == "":
		return
	var old_action := keybind_capture_action
	keybind_capture_action = ""
	update_keybind_button(old_action)

func capture_key_binding(action: String, keycode: int):
	if keycode == KEY_NONE:
		cancel_keybind_capture()
		return

	var duplicate_action := get_action_for_keycode(keycode, action)
	if duplicate_action != "":
		show_message("%s ist schon belegt" % get_keybind_label(duplicate_action))
		cancel_keybind_capture()
		return

	apply_key_binding(action, keycode, true)
	keybind_capture_action = ""
	for key in keybind_buttons.keys():
		update_keybind_button(str(key))
	show_message("%s gesetzt" % get_keybind_label(action))

func apply_key_binding(action: String, keycode: int, should_save := true):
	if action == "" or keycode == KEY_NONE:
		return

	if not InputMap.has_action(action):
		InputMap.add_action(action)

	InputMap.action_erase_events(action)
	var key_event := InputEventKey.new()
	key_event.keycode = keycode
	InputMap.action_add_event(action, key_event)

	update_keybind_button(action)
	if should_save:
		save_settings()

func get_binding_keycode(action: String) -> int:
	if not InputMap.has_action(action):
		return KEY_NONE

	for event in InputMap.action_get_events(action):
		if event is InputEventKey:
			var key_event := event as InputEventKey
			return key_event.keycode if key_event.keycode != KEY_NONE else key_event.physical_keycode

	return KEY_NONE

func get_action_for_keycode(keycode: int, ignored_action: String) -> String:
	for entry in KEY_BIND_ACTIONS:
		var action := str(entry.get("action", ""))
		if action != ignored_action and get_binding_keycode(action) == keycode:
			return action

	return ""

func update_keybind_button(action: String):
	if not keybind_buttons.has(action):
		return

	var button := keybind_buttons[action] as Button
	if keybind_capture_action == action:
		button.text = "Taste druecken..."
	else:
		button.text = get_keycode_label(get_binding_keycode(action))

func get_keybind_label(action: String) -> String:
	for entry in KEY_BIND_ACTIONS:
		if str(entry.get("action", "")) == action:
			return str(entry.get("label", action))
	return action

func get_keycode_label(keycode: int) -> String:
	if keycode == KEY_NONE:
		return "-"
	var key_text := OS.get_keycode_string(keycode)
	return key_text if key_text != "" else str(keycode)

func format_setting_number(value: float, decimals: int) -> String:
	match decimals:
		0:
			return "%.0f" % value
		1:
			return "%.1f" % value
		2:
			return "%.2f" % value
		_:
			return "%.3f" % value

func rebuild_inventory_grid():
	if not inventory_grid:
		return

	clear_children(inventory_grid)
	for item_index in range(INVENTORY_SLOT_COUNT):
		var item_id := ""
		if item_index < INVENTORY_ITEMS.size():
			item_id = INVENTORY_ITEMS[item_index]
		inventory_grid.add_child(create_inventory_slot(item_id, int(current_inventory.get(item_id, 0)), false, item_index + 1, item_index))

func rebuild_hotbar():
	if not hotbar_container:
		return

	clear_children(hotbar_container)
	for item_index in range(HOTBAR_ITEMS.size()):
		var item_id: String = HOTBAR_ITEMS[item_index]
		hotbar_container.add_child(create_inventory_slot(item_id, int(current_inventory.get(item_id, 0)), true, item_index + 1, item_index))

func rebuild_crafting_panel():
	if not crafting_items_container or not crafting_result_label or not crafting_suggestions_container or not craft_button:
		return

	clear_children(crafting_items_container)
	if not crafting_items.is_empty():
		for item_id in crafting_items.keys():
			crafting_items_container.add_child(create_crafting_token(str(item_id), int(crafting_items[item_id])))

	var player = get_player()
	var recipe := {}
	var suggestions := []
	if player:
		if player.has_method("get_crafting_recipe_for_ingredients"):
			recipe = player.get_crafting_recipe_for_ingredients(crafting_items)
		if player.has_method("get_crafting_recipe_suggestions"):
			suggestions = player.get_crafting_recipe_suggestions(crafting_items)

	if recipe.is_empty():
		crafting_result_label.text = ""
		craft_button.disabled = true
	else:
		var output_id := str(recipe.get("id", ""))
		var amount := int(recipe.get("amount", 1))
		crafting_result_label.text = "%s x%d" % [get_item_label(output_id), amount]
		craft_button.disabled = false

	rebuild_crafting_suggestions(suggestions)

func rebuild_recipe_book():
	if not recipe_book_container:
		return

	apply_recipe_book_state()
	clear_children(recipe_book_container)
	var player = get_player()
	var recipes := []
	if player and player.has_method("get_crafting_recipe_suggestions"):
		recipes = player.get_crafting_recipe_suggestions({})

	for recipe in recipes:
		var recipe_dict: Dictionary = recipe
		if recipe_matches_search(recipe_dict):
			recipe_book_container.add_child(create_recipe_book_entry(recipe_dict))

func toggle_recipe_book():
	recipe_book_open = not recipe_book_open
	apply_recipe_book_state()
	rebuild_recipe_book()

func update_recipe_search(text: String):
	recipe_search_text = text.strip_edges().to_lower()
	rebuild_recipe_book()

func apply_recipe_book_state():
	if recipe_book_toggle:
		recipe_book_toggle.text = "Buch v" if recipe_book_open else "Buch >"
	if recipe_book_body:
		recipe_book_body.visible = recipe_book_open
	if recipe_book_panel:
		recipe_book_panel.size_flags_vertical = Control.SIZE_EXPAND_FILL if recipe_book_open else Control.SIZE_SHRINK_BEGIN
		recipe_book_panel.custom_minimum_size = Vector2(scaled(CRAFTING_COLUMN_WIDTH), scaled(430 if recipe_book_open else 64))

func recipe_matches_search(recipe: Dictionary) -> bool:
	if recipe_search_text == "":
		return true

	var output_id := str(recipe.get("id", ""))
	if get_item_label(output_id).to_lower().contains(recipe_search_text):
		return true

	var ingredients: Dictionary = recipe.get("ingredients", {})
	for item_id in ingredients.keys():
		if get_item_label(str(item_id)).to_lower().contains(recipe_search_text):
			return true

	return false

func create_recipe_book_entry(recipe: Dictionary) -> Control:
	var entry := PanelContainer.new()
	entry.custom_minimum_size = scaled_vec(Vector2(CRAFTING_COLUMN_WIDTH - 28, 82))

	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.18, 0.155, 0.105, 0.92)
	style.border_color = Color(0.45, 0.34, 0.18, 0.82)
	style.set_border_width_all(1)
	style.corner_radius_top_left = 6
	style.corner_radius_top_right = 6
	style.corner_radius_bottom_left = 6
	style.corner_radius_bottom_right = 6
	entry.add_theme_stylebox_override("panel", style)

	var row := HBoxContainer.new()
	row.alignment = BoxContainer.ALIGNMENT_BEGIN
	row.add_theme_constant_override("separation", scaled_int(12))
	entry.add_child(row)

	var output_id := str(recipe.get("id", ""))
	row.add_child(create_item_icon(output_id, scaled_int(58)))

	var text_column := VBoxContainer.new()
	text_column.custom_minimum_size = scaled_vec(Vector2(CRAFTING_COLUMN_WIDTH - 110, 70))
	text_column.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	text_column.add_theme_constant_override("separation", scaled_int(2))
	row.add_child(text_column)

	var name_label := Label.new()
	name_label.text = get_item_label(output_id)
	name_label.add_theme_font_size_override("font_size", scaled_int(18))
	name_label.add_theme_color_override("font_color", Color(0.97, 0.90, 0.70, 1))
	text_column.add_child(name_label)

	var ingredients_label := Label.new()
	ingredients_label.text = format_ingredients(recipe.get("ingredients", {}))
	ingredients_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	ingredients_label.custom_minimum_size = scaled_vec(Vector2(CRAFTING_COLUMN_WIDTH - 118, 38))
	ingredients_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	ingredients_label.add_theme_font_size_override("font_size", scaled_int(15))
	ingredients_label.add_theme_color_override("font_color", Color(0.78, 0.72, 0.60, 1))
	text_column.add_child(ingredients_label)

	return entry

func rebuild_crafting_suggestions(suggestions: Array):
	if not crafting_suggestions_container:
		return

	clear_children(crafting_suggestions_container)
	var shown := 0
	for recipe in suggestions:
		if shown >= 4:
			break

		var recipe_dict: Dictionary = recipe
		var recipe_label := Label.new()
		recipe_label.text = "%s: %s" % [get_item_label(str(recipe_dict.get("id", ""))), format_ingredients(recipe_dict.get("ingredients", {}))]
		recipe_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		recipe_label.add_theme_font_size_override("font_size", 13)
		recipe_label.add_theme_color_override("font_color", Color(0.82, 0.86, 0.74, 1))
		crafting_suggestions_container.add_child(recipe_label)
		shown += 1

	if shown == 0:
		var empty_label := Label.new()
		empty_label.text = "-"
		empty_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		empty_label.add_theme_color_override("font_color", Color(0.55, 0.58, 0.52, 1))
		crafting_suggestions_container.add_child(empty_label)

func create_crafting_token(item_id: String, amount: int) -> Control:
	var token := VBoxContainer.new()
	token.custom_minimum_size = scaled_vec(Vector2(96, 100))
	token.alignment = BoxContainer.ALIGNMENT_CENTER
	token.mouse_filter = Control.MOUSE_FILTER_STOP
	token.gui_input.connect(func(event: InputEvent):
		if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
			remove_crafting_item(item_id)
	)

	token.add_child(create_item_icon(item_id, scaled_int(72)))

	var amount_label := Label.new()
	amount_label.text = "x%d" % amount
	amount_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	amount_label.add_theme_font_size_override("font_size", scaled_int(17))
	amount_label.add_theme_color_override("font_color", Color(1.0, 0.92, 0.60, 1))
	amount_label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	token.add_child(amount_label)

	return token

func request_crafting_item(item_id: String):
	if item_id == "":
		return

	var available := int(current_inventory.get(item_id, 0)) - int(crafting_items.get(item_id, 0))
	if available <= 0:
		show_message("%s fehlt" % get_item_label(item_id))
		return

	crafting_items[item_id] = int(crafting_items.get(item_id, 0)) + 1
	rebuild_inventory_grid()
	rebuild_crafting_panel()

func remove_crafting_item(item_id: String):
	if not crafting_items.has(item_id):
		return

	var amount := int(crafting_items[item_id]) - 1
	if amount <= 0:
		crafting_items.erase(item_id)
	else:
		crafting_items[item_id] = amount

	rebuild_inventory_grid()
	rebuild_crafting_panel()

func clear_crafting_selection():
	crafting_items.clear()
	rebuild_inventory_grid()
	rebuild_crafting_panel()

func craft_selected_items():
	var player = get_player()
	if not player or not player.has_method("craft_from_ingredients"):
		return

	if bool(player.craft_from_ingredients(crafting_items)):
		crafting_items.clear()
		rebuild_inventory_grid()
		rebuild_crafting_panel()

func clamp_crafting_items():
	for item_id in crafting_items.keys():
		var selected_amount := int(crafting_items.get(item_id, 0))
		var available_amount := int(current_inventory.get(item_id, 0))
		if available_amount <= 0:
			crafting_items.erase(item_id)
		elif selected_amount > available_amount:
			crafting_items[item_id] = available_amount

func format_ingredients(ingredients_value) -> String:
	var ingredients: Dictionary = ingredients_value
	var parts := []
	for item_id in ingredients.keys():
		parts.append("%s %d" % [get_item_label(str(item_id)), int(ingredients[item_id])])

	return ", ".join(parts)

func create_inventory_slot(item_id: String, amount: int, compact: bool, slot_number: int, slot_index: int) -> Control:
	var slot := PanelContainer.new()
	var inventory_slot_scale := Vector2(lerp(1.0, get_icon_scale(), 0.45), get_icon_scale())
	slot.custom_minimum_size = scaled_vec(Vector2(78, 64) * get_icon_scale()) if compact else scaled_vec(INVENTORY_SLOT_SIZE * inventory_slot_scale)
	slot.mouse_filter = Control.MOUSE_FILTER_STOP
	slot.gui_input.connect(func(event: InputEvent):
		if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
			if compact:
				request_item_selection(item_id, slot_index)
			else:
				request_crafting_item(item_id)
	)

	var slot_style := StyleBoxFlat.new()
	var is_on_mat := item_id != "" and int(crafting_items.get(item_id, 0)) > 0
	var is_selected := slot_index == selected_hotbar_index if compact else item_id != "" and (item_id == selected_item or is_on_mat)
	slot_style.bg_color = Color(0.18, 0.21, 0.17, 1) if is_selected else Color(0.13, 0.15, 0.13, 1)
	slot_style.border_color = Color(0.46, 0.78, 0.92, 1) if is_on_mat and not compact else Color(0.95, 0.78, 0.30, 1) if is_selected else Color(0.32, 0.38, 0.30, 1)
	slot_style.set_border_width_all(3 if is_selected else 1)
	slot_style.corner_radius_top_left = 6
	slot_style.corner_radius_top_right = 6
	slot_style.corner_radius_bottom_left = 6
	slot_style.corner_radius_bottom_right = 6
	slot.add_theme_stylebox_override("panel", slot_style)

	var content := VBoxContainer.new()
	content.alignment = BoxContainer.ALIGNMENT_CENTER
	content.mouse_filter = Control.MOUSE_FILTER_IGNORE
	content.add_theme_constant_override("separation", scaled_int(2) if compact else scaled_int(5))
	slot.add_child(content)

	var number_label := Label.new()
	number_label.text = str(slot_number)
	number_label.custom_minimum_size = scaled_vec(Vector2(22, 20)) if compact else scaled_vec(Vector2(INVENTORY_SLOT_SIZE.x * inventory_slot_scale.x - 12.0, 18))
	number_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	number_label.add_theme_font_size_override("font_size", scaled_int(13) if compact else scaled_int(15))
	number_label.add_theme_color_override("font_color", Color(0.75, 0.78, 0.68, 1))
	number_label.mouse_filter = Control.MOUSE_FILTER_IGNORE

	if compact:
		var top_row := HBoxContainer.new()
		top_row.alignment = BoxContainer.ALIGNMENT_CENTER
		top_row.mouse_filter = Control.MOUSE_FILTER_IGNORE
		content.add_child(top_row)
		top_row.add_child(number_label)
		top_row.add_child(create_item_icon(item_id, scaled_int(34)))
	else:
		content.add_child(number_label)
		content.add_child(create_item_icon(item_id, scaled_int(INVENTORY_ICON_SIZE)))

	if not compact and item_id != "":
		var name_label := Label.new()
		name_label.text = get_item_label(item_id)
		name_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		name_label.add_theme_font_size_override("font_size", scaled_int(15))
		name_label.add_theme_color_override("font_color", Color(0.90, 0.92, 0.84, 1))
		name_label.mouse_filter = Control.MOUSE_FILTER_IGNORE
		content.add_child(name_label)

	if item_id != "":
		var amount_label := Label.new()
		amount_label.text = "x%d" % amount
		amount_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		amount_label.add_theme_font_size_override("font_size", scaled_int(14) if compact else scaled_int(18))
		amount_label.add_theme_color_override("font_color", Color(1.0, 0.92, 0.60, 1))
		amount_label.mouse_filter = Control.MOUSE_FILTER_IGNORE
		content.add_child(amount_label)

	return slot

func create_item_icon(item_id: String, icon_size: int = 62) -> Control:
	icon_size = max(18, int(round(icon_size * get_icon_scale())))
	if item_id == "":
		var empty_icon := PanelContainer.new()
		empty_icon.custom_minimum_size = Vector2(icon_size, icon_size)
		empty_icon.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		empty_icon.size_flags_vertical = Control.SIZE_EXPAND_FILL
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
			texture_icon.size_flags_horizontal = Control.SIZE_EXPAND_FILL
			texture_icon.size_flags_vertical = Control.SIZE_EXPAND_FILL
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
			texture_rect.custom_minimum_size = Vector2(max(icon_size - 10, 24), max(icon_size - 10, 24))
			texture_rect.size_flags_horizontal = Control.SIZE_EXPAND_FILL
			texture_rect.size_flags_vertical = Control.SIZE_EXPAND_FILL
			texture_rect.expand_mode = TextureRect.EXPAND_FIT_WIDTH_PROPORTIONAL
			texture_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
			texture_rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
			texture_icon.add_child(texture_rect)

			return texture_icon

	var icon := PanelContainer.new()
	icon.custom_minimum_size = Vector2(icon_size, icon_size)
	icon.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	icon.size_flags_vertical = Control.SIZE_EXPAND_FILL
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
	symbol.custom_minimum_size = Vector2(icon_size, icon_size)
	symbol.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	symbol.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	symbol.add_theme_font_size_override("font_size", max(18, int(icon_size * 0.38)))
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
		"spear":
			return Color(0.38, 0.27, 0.13, 1)
		"torch":
			return Color(0.70, 0.30, 0.08, 1)
		"campfire":
			return Color(0.46, 0.30, 0.18, 1)
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
		"spear":
			return "SP"
		"torch":
			return "F"
		"campfire":
			return "L"
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
		"spear":
			return "Speer"
		"torch":
			return "Fackel"
		"campfire":
			return "Lagerfeuer"
		_:
			return item_id

func clear_children(node: Node):
	for child in node.get_children():
		node.remove_child(child)
		child.queue_free()

func request_item_selection(item_id: String, slot_index := -1):
	var player = get_player()
	if player and player.has_method("select_item"):
		player.select_item(item_id, slot_index)

func get_player() -> Node:
	return get_parent().find_child("Player", true, false)
