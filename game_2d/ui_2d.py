"""
2D HUD und UI
"""

import pygame
from config_2d import *

class HUD:
    def __init__(self):
        self.font_large = pygame.font.Font(None, 36)
        self.font_small = pygame.font.Font(None, 24)
        self.time_elapsed = 0
    
    def update(self, dt):
        self.time_elapsed += dt
    
    def draw(self, screen, player):
        """Zeichnet HUD auf dem Bildschirm"""
        # Semi-transparent Background für Stats
        bg_surface = pygame.Surface((400, 200))
        bg_surface.set_alpha(180)
        bg_surface.fill((0, 0, 0))
        screen.blit(bg_surface, (10, 10))
        
        # Health
        health_text = self.font_small.render(f"❤ Health: {player.health:.0f}/{PLAYER_MAX_HEALTH}", True, (255, 50, 50))
        screen.blit(health_text, (20, 20))
        self.draw_bar(screen, 20, 45, 200, 20, player.health, PLAYER_MAX_HEALTH, (255, 50, 50))
        
        # Hunger
        hunger_text = self.font_small.render(f"🍗 Hunger: {player.hunger:.0f}/{PLAYER_MAX_HUNGER}", True, (255, 200, 50))
        screen.blit(hunger_text, (20, 75))
        self.draw_bar(screen, 20, 100, 200, 20, player.hunger, PLAYER_MAX_HUNGER, (255, 200, 50))
        
        # Stamina
        stamina_text = self.font_small.render(f"⚡ Stamina: {player.stamina:.0f}/{PLAYER_MAX_STAMINA}", True, (50, 200, 255))
        screen.blit(stamina_text, (20, 130))
        self.draw_bar(screen, 20, 155, 200, 20, player.stamina, PLAYER_MAX_STAMINA, (50, 200, 255))
        
        # Ressourcen
        res_text = self.font_small.render(f"Wood: {player.wood} | Stone: {player.stone} | Food: {player.food}", True, COLOR_TEXT)
        screen.blit(res_text, (20, 185))
        
        # Zeit
        minutes = int(self.time_elapsed / 60)
        seconds = int(self.time_elapsed % 60)
        time_text = self.font_small.render(f"Time: {minutes:02d}:{seconds:02d}", True, COLOR_TEXT)
        screen.blit(time_text, (WINDOW_WIDTH - 200, 20))
    
    def draw_bar(self, screen, x, y, width, height, current, max_val, color):
        """Zeichnet eine Stat-Bar"""
        # Background
        pygame.draw.rect(screen, (100, 100, 100), (x, y, width, height))
        
        # Gefüllt
        fill_width = (current / max_val) * width if max_val > 0 else 0
        pygame.draw.rect(screen, color, (x, y, fill_width, height))
        
        # Border
        pygame.draw.rect(screen, (255, 255, 255), (x, y, width, height), 2)


class Inventory:
    def __init__(self):
        self.visible = False
        self.font = pygame.font.Font(None, 28)
    
    def toggle(self):
        self.visible = not self.visible
    
    def draw(self, screen, player):
        """Zeichnet Inventar"""
        if not self.visible:
            return
        
        # Semi-transparent Background
        bg_surface = pygame.Surface((600, 400))
        bg_surface.set_alpha(200)
        bg_surface.fill((30, 30, 30))
        screen.blit(bg_surface, (300, 200))
        
        # Titel
        title = self.font.render("INVENTORY (I zum Schließen)", True, COLOR_TEXT)
        screen.blit(title, (320, 220))
        
        # Items anzeigen
        items = [
            f"🪵 Wood: {player.wood}",
            f"🪨 Stone: {player.stone}",
            f"🍗 Food: {player.food}"
        ]
        
        y = 280
        for item in items:
            text = self.font.render(item, True, COLOR_TEXT)
            screen.blit(text, (320, y))
            y += 40
