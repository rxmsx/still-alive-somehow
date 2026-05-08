"""
2D Welt - Terrain, Gegner, Items
"""

import pygame
import random
import math
from config_2d import *

class World:
    def __init__(self):
        self.tiles = []  # Terrain Tiles
        self.enemies = []
        self.items = []
        self.tree_positions = []
        self.rock_positions = []
        
        self.enemy_spawn_timer = 0
        self.generate_world()
        
    def generate_world(self):
        """Generiert einfache Welt mit Perlin-ähnlichem Noise"""
        # Vereinfachtes Terrain
        for x in range(0, MAP_WIDTH, TILE_SIZE):
            for y in range(0, MAP_HEIGHT, TILE_SIZE):
                # Random Biome
                rand = random.random()
                
                if rand < 0.7:
                    tile_type = 'grass'
                    color = COLOR_GRASS
                elif rand < 0.85:
                    tile_type = 'sand'
                    color = COLOR_SAND
                elif rand < 0.95:
                    tile_type = 'stone'
                    color = COLOR_STONE
                    self.rock_positions.append((x + TILE_SIZE//2, y + TILE_SIZE//2))
                else:
                    tile_type = 'water'
                    color = COLOR_WATER
                
                # Bäume in Gras-Bereichen
                if tile_type == 'grass' and random.random() < 0.05:
                    self.tree_positions.append((x + TILE_SIZE//2, y + TILE_SIZE//2))
                
                self.tiles.append({
                    'x': x,
                    'y': y,
                    'type': tile_type,
                    'color': color,
                    'rect': pygame.Rect(x, y, TILE_SIZE, TILE_SIZE)
                })
    
    def update(self, dt, player):
        """Updated Welt-Objekte"""
        # Gegner spawnen
        self.enemy_spawn_timer += dt
        if self.enemy_spawn_timer > 1/ENEMY_SPAWN_RATE and len(self.enemies) < 10:
            self.spawn_enemy(player)
            self.enemy_spawn_timer = 0
        
        # Update Gegner
        for enemy in self.enemies[:]:
            enemy.update(dt, player)
            
            # Gegner ist zu weit weg? Entferne
            dist = math.sqrt((enemy.x - player.x)**2 + (enemy.y - player.y)**2)
            if dist > 2000:
                self.enemies.remove(enemy)
    
    def spawn_enemy(self, player):
        """Spawnt Gegner in der Nähe des Spielers"""
        angle = random.random() * 2 * math.pi
        distance = 800  # Viel weiter weg
        x = player.x + math.cos(angle) * distance
        y = player.y + math.sin(angle) * distance
        
        # Überprüfe ob Spawning auf Wasser - wenn ja, spawn woanders
        attempts = 0
        while self.is_water(x, y) and attempts < 10:
            angle = random.random() * 2 * math.pi
            x = player.x + math.cos(angle) * distance
            y = player.y + math.sin(angle) * distance
            attempts += 1
        
        if attempts < 10:  # Nur spawnen wenn gutes Terrain gefunden
            self.enemies.append(Enemy(x, y))
    
    def is_water(self, x, y):
        """Überprüft ob Position im Wasser ist"""
        tile_x = int(x // TILE_SIZE) * TILE_SIZE
        tile_y = int(y // TILE_SIZE) * TILE_SIZE
        
        for tile in self.tiles:
            if tile['x'] == tile_x and tile['y'] == tile_y:
                return tile['type'] == 'water'
        return False
    
    def draw(self, screen, camera_offset):
        """Zeichnet Welt"""
        # Terrain
        for tile in self.tiles:
            rect = tile['rect'].copy()
            rect.x -= camera_offset[0]
            rect.y -= camera_offset[1]
            
            # Nur zeichnen wenn sichtbar
            if -TILE_SIZE < rect.x < 1200 + TILE_SIZE and -TILE_SIZE < rect.y < 800 + TILE_SIZE:
                pygame.draw.rect(screen, tile['color'], rect)
                pygame.draw.rect(screen, (0, 0, 0), rect, 1)  # Border
        
        # Bäume
        for pos in self.tree_positions:
            screen_pos = (pos[0] - camera_offset[0], pos[1] - camera_offset[1])
            if -50 < screen_pos[0] < 1200 + 50 and -50 < screen_pos[1] < 800 + 50:
                pygame.draw.circle(screen, (100, 200, 100), screen_pos, 15)
        
        # Steine
        for pos in self.rock_positions:
            screen_pos = (pos[0] - camera_offset[0], pos[1] - camera_offset[1])
            if -50 < screen_pos[0] < 1200 + 50 and -50 < screen_pos[1] < 800 + 50:
                pygame.draw.circle(screen, (150, 150, 150), screen_pos, 10)
        
        # Gegner
        for enemy in self.enemies:
            enemy.draw(screen, camera_offset)


class Enemy:
    def __init__(self, x, y):
        self.x = x
        self.y = y
        self.width = ENEMY_SIZE
        self.height = ENEMY_SIZE
        self.health = ENEMY_HEALTH
        self.speed = ENEMY_SPEED
    
    def update(self, dt, player):
        """Bewegt sich zum Spieler"""
        dx = player.x - self.x
        dy = player.y - self.y
        distance = math.sqrt(dx**2 + dy**2)
        
        if distance > 0:
            # Normalisiere Richtung
            dx /= distance
            dy /= distance
            
            # Bewegung
            self.x += dx * self.speed * dt
            self.y += dy * self.speed * dt
    
    def get_rect(self):
        return pygame.Rect(self.x, self.y, self.width, self.height)
    
    def draw(self, screen, camera_offset):
        rect = self.get_rect()
        rect.x -= camera_offset[0]
        rect.y -= camera_offset[1]
        
        pygame.draw.rect(screen, COLOR_ENEMY, rect)
        # Health Bar
        pygame.draw.rect(screen, (255, 0, 0), (rect.x, rect.y - 10, ENEMY_SIZE, 5))
        pygame.draw.rect(screen, (0, 255, 0), (rect.x, rect.y - 10, ENEMY_SIZE * max(0, self.health/ENEMY_HEALTH), 5))
