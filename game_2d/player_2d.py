"""
2D Player Klasse - Top-Down Survival Game
"""

import pygame
import math
from config_2d import *

class Player:
    def __init__(self, x, y):
        self.x = x
        self.y = y
        self.width = PLAYER_SIZE
        self.height = PLAYER_SIZE
        
        # Bewegung
        self.velocity_x = 0
        self.velocity_y = 0
        self.speed = PLAYER_SPEED
        self.is_sprinting = False
        
        # Survival Stats
        self.health = PLAYER_MAX_HEALTH
        self.hunger = PLAYER_MAX_HUNGER
        self.stamina = PLAYER_MAX_STAMINA
        
        # Kampf
        self.last_attack_time = 0
        self.attack_cooldown = 0.5  # sekunden
        
        # Ressourcen
        self.wood = 0
        self.stone = 0
        self.food = 0
        
    def handle_input(self, keys, mouse_pos):
        """Verarbeitet Spieler-Input"""
        self.velocity_x = 0
        self.velocity_y = 0
        
        # Bewegung
        if keys[pygame.K_w]:
            self.velocity_y -= 1
        if keys[pygame.K_s]:
            self.velocity_y += 1
        if keys[pygame.K_a]:
            self.velocity_x -= 1
        if keys[pygame.K_d]:
            self.velocity_x += 1
        
        # Normalisiere Diagonale Bewegung
        length = math.sqrt(self.velocity_x**2 + self.velocity_y**2)
        if length > 0:
            self.velocity_x /= length
            self.velocity_y /= length
        
        # Sprint
        self.is_sprinting = keys[pygame.K_LSHIFT] and self.stamina > 0
        
        speed = PLAYER_SPRINT_SPEED if self.is_sprinting else PLAYER_SPEED
        self.velocity_x *= speed
        self.velocity_y *= speed
    
    def update(self, dt, world):
        """Updated Position und Stats"""
        # Bewegung
        self.x += self.velocity_x * dt
        self.y += self.velocity_y * dt
        
        # Map Grenzen
        self.x = max(0, min(self.x, MAP_WIDTH - self.width))
        self.y = max(0, min(self.y, MAP_HEIGHT - self.height))
        
        # Survival Stats
        hunger_drain = HUNGER_DRAIN_RATE * 2 if self.is_sprinting else HUNGER_DRAIN_RATE
        self.hunger = max(0, self.hunger - hunger_drain * dt / 60)
        
        # Stamina
        if self.is_sprinting:
            self.stamina = max(0, self.stamina - STAMINA_DRAIN_RATE * dt / 60)
        else:
            self.stamina = min(PLAYER_MAX_STAMINA, self.stamina + STAMINA_REGEN_RATE * dt / 60)
        
        # Health von Hunger
        if self.hunger <= 0:
            self.health -= 1 * dt / 60  # Sehr langsam
        
        # Clamp werte
        self.health = max(0, min(self.health, PLAYER_MAX_HEALTH))
        self.hunger = max(0, min(self.hunger, PLAYER_MAX_HUNGER))
    
    def take_damage(self, amount):
        """Spieler nimmt Schaden"""
        self.health -= amount
    
    def eat_food(self, amount=FOOD_RESTORE):
        """Spieler isst"""
        self.hunger = min(self.hunger + amount, PLAYER_MAX_HUNGER)
    
    def drink_water(self):
        """Spieler trinkt Wasser"""
        self.hunger = min(self.hunger + 15, PLAYER_MAX_HUNGER)
    
    def get_rect(self):
        """Gibt Rectangle für Rendering zurück"""
        return pygame.Rect(self.x, self.y, self.width, self.height)
    
    def draw(self, screen, camera_offset):
        """Zeichnet den Spieler"""
        rect = self.get_rect()
        rect.x -= camera_offset[0]
        rect.y -= camera_offset[1]
        
        pygame.draw.rect(screen, COLOR_PLAYER, rect)
        pygame.draw.circle(screen, (255, 255, 100), rect.center, 5)  # Augen
