# HUD.gd - User Interface
extends CanvasLayer

func _ready():
	print("✅ HUD geladen")

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
