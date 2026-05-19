extends Control

const GAME_SCENE_PATH := "res://main.tscn"
const DEFAULT_WORLD_NAME := "Neue Welt"
const PANEL_WIDTH := 560.0
const PANEL_MIN_HEIGHT := 560.0
const BUTTON_HEIGHT := 56.0

var content_box: VBoxContainer
var status_label: Label
var world_name_input: LineEdit
var seed_input: LineEdit

func _ready() -> void:
	Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)
	build_menu_shell()
	show_main_menu()

func build_menu_shell() -> void:
	var background := ColorRect.new()
	background.name = "Background"
	background.color = Color(0.035, 0.047, 0.055)
	background.anchor_right = 1.0
	background.anchor_bottom = 1.0
	add_child(background)

	var top_band := ColorRect.new()
	top_band.name = "TopBand"
	top_band.color = Color(0.08, 0.26, 0.22, 0.55)
	top_band.anchor_right = 1.0
	top_band.offset_bottom = 160.0
	add_child(top_band)

	var bottom_band := ColorRect.new()
	bottom_band.name = "BottomBand"
	bottom_band.color = Color(0.10, 0.12, 0.14, 0.65)
	bottom_band.anchor_top = 0.78
	bottom_band.anchor_right = 1.0
	bottom_band.anchor_bottom = 1.0
	add_child(bottom_band)

	var outer_margin := MarginContainer.new()
	outer_margin.name = "OuterMargin"
	outer_margin.anchor_right = 1.0
	outer_margin.anchor_bottom = 1.0
	outer_margin.add_theme_constant_override("margin_left", 32)
	outer_margin.add_theme_constant_override("margin_top", 32)
	outer_margin.add_theme_constant_override("margin_right", 32)
	outer_margin.add_theme_constant_override("margin_bottom", 32)
	add_child(outer_margin)

	var center := CenterContainer.new()
	center.name = "Center"
	center.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	center.size_flags_vertical = Control.SIZE_EXPAND_FILL
	outer_margin.add_child(center)

	var panel := PanelContainer.new()
	panel.name = "MenuPanel"
	panel.custom_minimum_size = Vector2(PANEL_WIDTH, PANEL_MIN_HEIGHT)
	panel.add_theme_stylebox_override("panel", create_panel_style())
	center.add_child(panel)

	var panel_margin := MarginContainer.new()
	panel_margin.name = "PanelMargin"
	panel_margin.add_theme_constant_override("margin_left", 42)
	panel_margin.add_theme_constant_override("margin_top", 38)
	panel_margin.add_theme_constant_override("margin_right", 42)
	panel_margin.add_theme_constant_override("margin_bottom", 38)
	panel.add_child(panel_margin)

	content_box = VBoxContainer.new()
	content_box.name = "Content"
	content_box.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	content_box.size_flags_vertical = Control.SIZE_EXPAND_FILL
	content_box.add_theme_constant_override("separation", 14)
	panel_margin.add_child(content_box)

func show_main_menu() -> void:
	clear_content()
	add_header("Survival World 3D", "Hauptmenü")
	add_spacer(8)

	var singleplayer_button := create_menu_button("Einzelspieler")
	singleplayer_button.pressed.connect(show_world_create_menu)
	content_box.add_child(singleplayer_button)

	var multiplayer_button := create_menu_button("Multiplayer")
	multiplayer_button.pressed.connect(show_multiplayer_menu)
	content_box.add_child(multiplayer_button)

	var settings_button := create_menu_button("Einstellungen")
	settings_button.pressed.connect(show_settings_menu)
	content_box.add_child(settings_button)

	add_spacer(10)

	var quit_button := create_menu_button("Spiel beenden", true)
	quit_button.pressed.connect(_on_quit_pressed)
	content_box.add_child(quit_button)

	add_status_label("")

func show_world_create_menu() -> void:
	clear_content()
	add_header("Einzelspieler", "Neue Welt erstellen")

	content_box.add_child(create_field_label("Weltname"))
	world_name_input = create_line_edit(DEFAULT_WORLD_NAME, "Name deiner Welt")
	world_name_input.text_submitted.connect(_on_text_submitted)
	content_box.add_child(world_name_input)

	content_box.add_child(create_field_label("Seed"))
	seed_input = create_line_edit("", "Optional: Zahl oder Text")
	seed_input.text_submitted.connect(_on_text_submitted)
	content_box.add_child(seed_input)

	add_spacer(8)

	var create_button := create_menu_button("Welt erstellen")
	create_button.pressed.connect(_on_create_world_pressed)
	content_box.add_child(create_button)

	var back_button := create_menu_button("Zurück", true)
	back_button.pressed.connect(show_main_menu)
	content_box.add_child(back_button)

	add_status_label("Leerer Seed erzeugt automatisch eine neue Welt.")

func show_multiplayer_menu() -> void:
	clear_content()
	add_header("Multiplayer", "Bereit für den nächsten Ausbau")
	content_box.add_child(create_body_label("Der Multiplayer-Button ist angelegt. Die Verbindungsliste, Server-Erstellung und Beitrittslogik können später hier landen."))
	add_spacer(16)

	var back_button := create_menu_button("Zurück", true)
	back_button.pressed.connect(show_main_menu)
	content_box.add_child(back_button)

	add_status_label("")

func show_settings_menu() -> void:
	clear_content()
	add_header("Einstellungen", "Grundoptionen")

	var fullscreen_check := CheckBox.new()
	fullscreen_check.text = "Vollbild"
	fullscreen_check.button_pressed = DisplayServer.window_get_mode() == DisplayServer.WINDOW_MODE_FULLSCREEN
	fullscreen_check.add_theme_font_size_override("font_size", 20)
	fullscreen_check.toggled.connect(_on_fullscreen_toggled)
	content_box.add_child(fullscreen_check)

	content_box.add_child(create_field_label("Master-Lautstärke"))
	var volume_slider := HSlider.new()
	volume_slider.min_value = 0.0
	volume_slider.max_value = 100.0
	volume_slider.step = 1.0
	volume_slider.value = db_to_linear(AudioServer.get_bus_volume_db(0)) * 100.0
	volume_slider.custom_minimum_size = Vector2(0, 40)
	volume_slider.value_changed.connect(_on_volume_changed)
	content_box.add_child(volume_slider)

	add_spacer(18)

	var back_button := create_menu_button("Zurück", true)
	back_button.pressed.connect(show_main_menu)
	content_box.add_child(back_button)

	add_status_label("")

func add_header(title_text: String, subtitle_text: String) -> void:
	var title := Label.new()
	title.text = title_text
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 38)
	title.add_theme_color_override("font_color", Color(0.90, 0.96, 0.90))
	title.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	content_box.add_child(title)

	var subtitle := Label.new()
	subtitle.text = subtitle_text
	subtitle.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	subtitle.add_theme_font_size_override("font_size", 21)
	subtitle.add_theme_color_override("font_color", Color(0.66, 0.78, 0.72))
	subtitle.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	content_box.add_child(subtitle)

func add_status_label(text: String) -> void:
	status_label = create_body_label(text)
	status_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	status_label.add_theme_color_override("font_color", Color(0.70, 0.78, 0.72))
	content_box.add_child(status_label)

func add_spacer(height: float) -> void:
	var spacer := Control.new()
	spacer.custom_minimum_size = Vector2(0, height)
	content_box.add_child(spacer)

func create_menu_button(text: String, secondary := false) -> Button:
	var button := Button.new()
	button.text = text
	button.custom_minimum_size = Vector2(0, BUTTON_HEIGHT)
	button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	button.clip_text = true
	button.add_theme_font_size_override("font_size", 23)
	button.add_theme_stylebox_override("normal", create_button_style(secondary, false, false))
	button.add_theme_stylebox_override("hover", create_button_style(secondary, true, false))
	button.add_theme_stylebox_override("pressed", create_button_style(secondary, true, true))
	button.add_theme_stylebox_override("focus", create_focus_style())
	return button

func create_line_edit(text: String, placeholder: String) -> LineEdit:
	var input := LineEdit.new()
	input.text = text
	input.placeholder_text = placeholder
	input.custom_minimum_size = Vector2(0, 48)
	input.add_theme_font_size_override("font_size", 20)
	input.add_theme_stylebox_override("normal", create_input_style())
	input.add_theme_stylebox_override("focus", create_input_focus_style())
	return input

func create_field_label(text: String) -> Label:
	var label := Label.new()
	label.text = text
	label.add_theme_font_size_override("font_size", 18)
	label.add_theme_color_override("font_color", Color(0.76, 0.86, 0.78))
	return label

func create_body_label(text: String) -> Label:
	var label := Label.new()
	label.text = text
	label.add_theme_font_size_override("font_size", 18)
	label.add_theme_color_override("font_color", Color(0.80, 0.86, 0.82))
	label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	return label

func create_panel_style() -> StyleBoxFlat:
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.075, 0.095, 0.105, 0.96)
	style.border_color = Color(0.22, 0.36, 0.30, 0.92)
	style.set_border_width_all(1)
	style.set_corner_radius_all(8)
	style.shadow_color = Color(0, 0, 0, 0.38)
	style.shadow_size = 18
	style.shadow_offset = Vector2(0, 8)
	return style

func create_button_style(secondary: bool, hover: bool, pressed: bool) -> StyleBoxFlat:
	var style := StyleBoxFlat.new()
	var base_color := Color(0.13, 0.34, 0.27) if not secondary else Color(0.12, 0.14, 0.15)
	if hover:
		base_color = base_color.lightened(0.12)
	if pressed:
		base_color = base_color.darkened(0.10)

	style.bg_color = base_color
	style.border_color = Color(0.44, 0.68, 0.55, 0.85) if not secondary else Color(0.30, 0.36, 0.34, 0.85)
	style.set_border_width_all(1)
	style.set_corner_radius_all(8)
	style.content_margin_left = 18
	style.content_margin_right = 18
	return style

func create_focus_style() -> StyleBoxFlat:
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.22, 0.42, 0.35, 0.55)
	style.border_color = Color(0.72, 0.92, 0.76, 0.95)
	style.set_border_width_all(2)
	style.set_corner_radius_all(8)
	return style

func create_input_style() -> StyleBoxFlat:
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.04, 0.05, 0.055, 0.92)
	style.border_color = Color(0.24, 0.34, 0.30, 0.92)
	style.set_border_width_all(1)
	style.set_corner_radius_all(6)
	style.content_margin_left = 12
	style.content_margin_right = 12
	return style

func create_input_focus_style() -> StyleBoxFlat:
	var style := create_input_style()
	style.border_color = Color(0.58, 0.78, 0.64, 1.0)
	style.set_border_width_all(2)
	return style

func clear_content() -> void:
	if content_box == null:
		return

	for child in content_box.get_children():
		content_box.remove_child(child)
		child.queue_free()

	status_label = null
	world_name_input = null
	seed_input = null

func _on_create_world_pressed() -> void:
	var world_settings: Node = get_node_or_null("/root/WorldSettings")
	if world_settings and world_settings.has_method("configure_new_world"):
		world_settings.call("configure_new_world", world_name_input.text, seed_input.text)

	var error: int = get_tree().change_scene_to_file(GAME_SCENE_PATH)
	if error != OK and status_label:
		status_label.text = "Spielszene konnte nicht geladen werden."

func _on_text_submitted(_text: String) -> void:
	_on_create_world_pressed()

func _on_fullscreen_toggled(enabled: bool) -> void:
	var mode: int = DisplayServer.WINDOW_MODE_FULLSCREEN if enabled else DisplayServer.WINDOW_MODE_WINDOWED
	DisplayServer.window_set_mode(mode)

func _on_volume_changed(value: float) -> void:
	var linear_volume: float = max(value / 100.0, 0.001)
	AudioServer.set_bus_volume_db(0, linear_to_db(linear_volume))

func _on_quit_pressed() -> void:
	get_tree().quit()
