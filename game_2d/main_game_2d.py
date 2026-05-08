"""
Survival World 2D - Main Game Loop
Starten mit: python main_game_2d.py
"""

import pygame
import sys
import math
from config_2d import *
from player_2d import Player
from world_2d import World
from ui_2d import HUD, Inventory

class SurvivalGame2D:
    def __init__(self):
        # Pygame Setup
        pygame.init()
        self.screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
        pygame.display.set_caption(TITLE)
        self.clock = pygame.time.Clock()
        self.running = True
        
        # Game Objects
        self.player = Player(MAP_WIDTH // 2, MAP_HEIGHT // 2)
        self.world = World()
        self.hud = HUD()
        self.inventory = Inventory()
        
        # Game State
        self.paused = False
        self.frame_count = 0
        
        print("✅ 2D Survival Game gestartet!")
        print("Steuerung:")
        print("  WASD - Bewegung")
        print("  SHIFT - Sprint")
        print("  I - Inventar")
        print("  ESC - Pause")
        print("  Q - Beende Spiel")
    
    def handle_input(self):
        """Verarbeitet Input"""
        keys = pygame.key.get_pressed()
        mouse_pos = pygame.mouse.get_pos()
        
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self.running = False
            
            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    self.paused = not self.paused
                if event.key == pygame.K_q:
                    self.running = False
                if event.key == pygame.K_i:
                    self.inventory.toggle()
                
                # Interaktionen
                if event.key == pygame.K_SPACE:
                    self.player_interact()
        
        # Kontinuierliche Input Verarbeitung
        if not self.paused:
            self.player.handle_input(keys, mouse_pos)
    
    def player_interact(self):
        """Spieler interagiert mit Umgebung"""
        # Suche nach nahestehenden Bäumen/Steinen
        for tree_pos in self.world.tree_positions:
            dist = math.sqrt((self.player.x - tree_pos[0])**2 + (self.player.y - tree_pos[1])**2)
            if dist < 100:
                self.player.wood += 10
                print(f"🪵 +10 Wood! Total: {self.player.wood}")
                return
        
        for rock_pos in self.world.rock_positions:
            dist = math.sqrt((self.player.x - rock_pos[0])**2 + (self.player.y - rock_pos[1])**2)
            if dist < 100:
                self.player.stone += 5
                print(f"🪨 +5 Stone! Total: {self.player.stone}")
                return
    
    def update(self):
        """Updated Game State"""
        if self.paused:
            return
        
        dt = self.clock.get_time() / 1000.0  # Delta Time in Sekunden
        
        # Update Spieler
        self.player.update(dt, self.world)
        
        # Update Welt
        self.world.update(dt, self.player)
        
        # Update HUD
        self.hud.update(dt)
        
        # Kollisionen mit Gegnern
        player_rect = self.player.get_rect()
        for enemy in self.world.enemies:
            if player_rect.colliderect(enemy.get_rect()):
                self.player.take_damage(ENEMY_DAMAGE * dt)
        
        # Game Over Check
        if self.player.health <= 0:
            self.player_died()
    
    def player_died(self):
        """Spieler ist gestorben"""
        print("💀 Du bist gestorben!")
        self.paused = True
    
    def draw(self):
        """Zeichnet alles"""
        self.screen.fill(COLOR_BG)
        
        # Camera folgt Spieler
        camera_x = self.player.x - WINDOW_WIDTH // 2
        camera_y = self.player.y - WINDOW_HEIGHT // 2
        camera_offset = (camera_x, camera_y)
        
        # Draw World
        self.world.draw(self.screen, camera_offset)
        
        # Draw Player
        self.player.draw(self.screen, camera_offset)
        
        # Draw HUD
        self.hud.draw(self.screen, self.player)
        
        # Draw Inventory
        self.inventory.draw(self.screen, self.player)
        
        # Pause Text
        if self.paused:
            font = pygame.font.Font(None, 60)
            pause_text = font.render("PAUSED", True, (255, 255, 255))
            text_rect = pause_text.get_rect(center=(WINDOW_WIDTH//2, WINDOW_HEIGHT//2))
            self.screen.blit(pause_text, text_rect)
        
        # Update Display
        pygame.display.flip()
    
    def run(self):
        """Haupt-Game Loop"""
        while self.running:
            self.handle_input()
            self.update()
            self.draw()
            self.clock.tick(FPS)
            
            self.frame_count += 1
        
        pygame.quit()
        print("Auf Wiedersehen!")
        sys.exit()


if __name__ == "__main__":
    game = SurvivalGame2D()
    game.run()
