# Player.gd - First-Person Player Controller
extends CharacterBody3D

@export var speed = 15.0
@export var sprint_speed = 25.0
@export var jump_force = 15.0
@export var gravity = 30.0
@export var mouse_sensitivity = 0.003

var is_sprinting = false
var health = 100.0
var hunger = 100.0
var stamina = 100.0

func _ready():
	Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)
	print("✅ Player Ready")

func _physics_process(delta):
	# Bewegung
	var input_dir = Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
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
	if Input.is_action_just_pressed("ui_accept") and is_on_floor():
		velocity.y = jump_force
	
	# Move
	move_and_slide()
	
	# Update Stats
	update_survival_stats(delta)

func _input(event):
	if event is InputEventMouseMotion:
		rotate_y(-event.relative.x * mouse_sensitivity)
		$Camera3D.rotate_x(-event.relative.y * mouse_sensitivity)
		$Camera3D.rotation.x = clamp($Camera3D.rotation.x, -PI/2, PI/2)
	
	if event.is_action_pressed("ui_cancel"):
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
	var hud = get_parent().find_child("HUD", true, false)
	if hud:
		hud.update_stats(health, hunger, stamina)
