extends Node

const DEFAULT_WORLD_NAME := "Neue Welt"

var world_name := DEFAULT_WORLD_NAME
var world_seed := 0
var seed_text := ""
var has_world_seed := false

func configure_new_world(requested_name: String, requested_seed: String) -> void:
	world_name = requested_name.strip_edges()
	if world_name == "":
		world_name = DEFAULT_WORLD_NAME

	seed_text = requested_seed.strip_edges()
	if seed_text == "":
		world_seed = generate_random_seed()
		seed_text = str(world_seed)
	else:
		world_seed = parse_seed(seed_text)

	has_world_seed = true

func get_world_name() -> String:
	return world_name

func get_world_seed() -> int:
	if not has_world_seed:
		world_seed = generate_random_seed()
		seed_text = str(world_seed)
		has_world_seed = true

	return world_seed

func generate_random_seed() -> int:
	var generator := RandomNumberGenerator.new()
	generator.randomize()
	return generator.randi()

func parse_seed(value: String) -> int:
	if value.is_valid_int():
		return int(value)

	return int(value.hash())
