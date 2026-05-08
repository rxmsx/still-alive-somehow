"""
Survival World - Main Game Loop
Starten mit: python main_game.py
"""

from ursina import *
from config_3d import *
from world import World
from player import Player
from ui import HUD, Inventory

# Globale Game Instance
game_instance = None

class SurvivalGame:
    """Hauptspiel-Klasse"""
    
    def __init__(self):
        # Fenster Setup
        window.title = TITLE
        window.size = (WINDOW_WIDTH, WINDOW_HEIGHT)
        # Himmelsblau - verwende Vec4 statt color.rgb()
        from panda3d.core import Vec4
        window.color = Vec4(0.502, 0.702, 1.0, 1.0)  # RGB normalisiert
        
        # Initialisiere Spiel-Komponenten
        print("🌍 Lade Terrain...")
        self.world = World()
        self.player = Player()
        self.hud = HUD()
        self.inventory = Inventory()
        
        # Game State
        self.time_of_day = 0
        self.running = True
        self.paused = False
        self.escape_pressed = False
        
        print("✅ Survival World geladen!")
        print("\nSteuerung:")
        print("  WASD - Bewegung")
        print("  SPACE - Jump")
        print("  SHIFT - Sprint")
        print("  Maus - Umschauen")
    
    def update(self):
        """Haupt-Update Loop"""
        if not self.running:
            return
        
        dt = 0.016  # ~60 FPS
        
        # Escape zum Pausieren
        if held_keys.get('escape', False) and not self.escape_pressed:
            self.paused = not self.paused
            self.escape_pressed = True
        elif not held_keys.get('escape', False):
            self.escape_pressed = False
        
        if self.paused:
            return
        
        # Update Spieler
        self.player.handle_mouse_look()
        self.player.update(dt)
        
        # Update Welt
        self.world.update(self.player.pos)
        
        # Update Zeit
        self.time_of_day += dt
        if self.time_of_day >= DAY_NIGHT_CYCLE:
            self.time_of_day = 0
        
        # Update UI
        self.hud.update(self.player, self.time_of_day)
        self.inventory.update_ui()

if __name__ == "__main__":
    app = Ursina()
    
    # Füge helles Ambient Lighting hinzu
    from panda3d.core import AmbientLight, Vec4
    
    # Erstelle Ambient Light
    alight = AmbientLight('ambient')
    alight.setColor(Vec4(1, 1, 1, 1))  # Weiß, volle Helligkeit
    alightAttrib = app.render.attachNewNode(alight)
    app.render.setLight(alightAttrib)
    
    print("💡 Lighting aktiviert")
    
    game_instance = SurvivalGame()
    
    # Globale update Funktion für Ursina
    def game_loop():
        global game_instance
        if game_instance:
            game_instance.update()
    
    # Registriere die Update Loop
    update = game_loop
    
    app.run()
