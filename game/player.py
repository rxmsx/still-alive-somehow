"""
3D Player Klasse mit First-Person Controls
"""

from ursina import *
from config_3d import *
from math import sin, cos, radians

class Player:
    """First-Person Spieler mit Survival-Stats"""
    
    def __init__(self, pos=Vec3(0, 0.5, -1.5)):
        # Position & Kamera
        self.pos = pos
        camera.position = self.pos  # Kamera direkt beim Spieler
        camera.rotation = (0, 0, 0)  # Schau geradeaus
        
        # Maus konfigurieren - mit leerer Textur (unsichtbar)
        try:
            from panda3d.core import PNMImage
            empty_cursor = PNMImage(1, 1)
            empty_cursor.fill(0, 0, 0, 0)  # Transparentes Bild
            props = window.get_properties()
            props.set_cursor_filename(empty_cursor)
            window.request_properties(props)
        except:
            # Fallback: einfach mit normalen Settings versuchen
            mouse.visible = False
        
        self.last_mouse_x = 0
        self.last_mouse_y = 0
        
        # Survival Stats
        self.health = PLAYER_MAX_HEALTH
        self.hunger = PLAYER_MAX_HUNGER
        self.stamina = PLAYER_MAX_STAMINA
        
        # Movement
        self.velocity = Vec3(0, 0, 0)
        self.speed = PLAYER_SPEED
        self.sprint_speed = PLAYER_SPRINT_SPEED
        self.is_sprinting = False
        self.is_jumping = False
        self.jump_force = 15
        
        # Physics
        self.gravity = 30
        self.ground_level = 0  # Boden auf y=0
        
        # Rotation
        self.camera_rotation = Vec3(0, 0, 0)
        self.sensitivity = 0.05  # Normal FPS sensitivity (wird mit 100 multipliziert)
        
        # Input
        self.input_direction = Vec3(0, 0, 0)
    
    def handle_input(self):
        """Verarbeitet Spieler-Input"""
        # Bewegung
        self.input_direction = Vec3(0, 0, 0)
        
        if held_keys.get('w', False):
            self.input_direction.z += 1
        if held_keys.get('s', False):
            self.input_direction.z -= 1
        if held_keys.get('a', False):
            self.input_direction.x -= 1
        if held_keys.get('d', False):
            self.input_direction.x += 1
        
        # Normalisiere Input
        if self.input_direction.length() > 0:
            self.input_direction = self.input_direction.normalized()
        
        # Sprint
        self.is_sprinting = held_keys.get('shift', False)
        
        # Jump
        if held_keys.get('space', False) and not self.is_jumping:
            self.velocity.y = self.jump_force
            self.is_jumping = True
    
    def update(self, dt):
        """Updated Spieler Position & Stats"""
        self.handle_input()
        
        # Bewegung basierend auf Kamerarichtung
        speed = self.sprint_speed if self.is_sprinting else self.speed
        
        # Berechne Forward & Right basierend auf Kamera-Rotation
        forward = Vec3(
            sin(radians(self.camera_rotation.y)),
            0,
            cos(radians(self.camera_rotation.y))
        )
        right = Vec3(
            cos(radians(self.camera_rotation.y)),
            0,
            -sin(radians(self.camera_rotation.y))
        )
        
        # Wende Input auf Bewegung an
        move_direction = forward * self.input_direction.z + right * self.input_direction.x
        self.velocity.x = move_direction.x * speed
        self.velocity.z = move_direction.z * speed
        
        # Gravität
        self.velocity.y -= self.gravity * dt
        
        # Update Position
        self.pos += self.velocity * dt
        
        # Bodenphysik (vereinfacht)
        if self.pos.y <= 0:
            self.pos.y = 0
            self.velocity.y = 0
            self.is_jumping = False
        
        # Update Kamera Position - WICHTIG!
        camera.position = self.pos
        
        # Survival Stats
        self.update_survival_stats(dt)
    
    def update_survival_stats(self, dt):
        """Updated Hunger, Stamina, etc."""
        # Hunger
        if self.is_sprinting:
            self.hunger -= HUNGER_DRAIN_RATE * 2 * dt
        else:
            self.hunger -= HUNGER_DRAIN_RATE * dt
        
        # Stamina
        if self.is_sprinting:
            self.stamina -= STAMINA_DRAIN_RATE * dt
        else:
            self.stamina = min(self.stamina + STAMINA_REGEN_RATE * dt, PLAYER_MAX_STAMINA)
        
        # Health von Hunger
        if self.hunger <= 0:
            self.health -= 10 * dt
            self.hunger = 0
        
        # Clamp values
        self.health = max(0, self.health)
        self.hunger = max(0, self.hunger)
    
    def handle_mouse_look(self):
        """FPS-Style Maus-Look - Teleportiert Maus wenn sie den Edge erreicht"""
        from panda3d.core import WindowProperties
        
        # Fenster-Dimensionen
        win_width = window.size[0]
        win_height = window.size[1]
        center_x = win_width / 2
        center_y = win_height / 2
        
        # Aktuelle Mausposition
        current_x = mouse.x
        current_y = mouse.y
        
        # Berechne Delta
        delta_x = (current_x - self.last_mouse_x) * 100
        delta_y = (current_y - self.last_mouse_y) * 100
        
        # Wenn Maus am Edge ist, teleportiere sie zur anderen Seite
        # (verhindert dass Maus "steckenbleibt")
        if current_x < 0.1 or current_x > 0.9:
            current_x = 0.5
        if current_y < 0.1 or current_y > 0.9:
            current_y = 0.5
        
        # Update Position für nächsten Frame
        self.last_mouse_x = current_x
        self.last_mouse_y = current_y
        
        # Berechne Kamera-Rotation
        self.camera_rotation.y += delta_x * self.sensitivity
        self.camera_rotation.x -= delta_y * self.sensitivity
        
        # Limitiere X Rotation
        self.camera_rotation.x = clamp(self.camera_rotation.x, -90, 90)
        
        # Wende Rotation an
        camera.rotation = (self.camera_rotation.x, self.camera_rotation.y, 0)
    
    def take_damage(self, amount):
        """Spieler nimmt Schaden"""
        self.health -= amount
    
    def eat_food(self, amount):
        """Spieler isst und regeneriert Hunger"""
        self.hunger = min(self.hunger + amount, PLAYER_MAX_HUNGER)
