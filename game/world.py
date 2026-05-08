"""
3D Terrain Generator - Ultra-vereinfacht ohne Shader
"""

from ursina import *
from config_3d import *

class World:
    """Minimale Welt mit einfachen Terrain-Blöcken"""
    
    def __init__(self):
        print("🏗️ Erstelle Terrain...")
        self.terrain_models = {}
        
        # Erstelle nur ein einfaches Test-Terrain
        self.create_test_terrain()
        print(f"✅ {len(self.terrain_models)} Blöcke erstellt")
    
    def create_test_terrain(self):
        """Erstellt nur TEST-Objekte direkt sichtbar"""
        # Ein großer grüner Block direkt vor dem Spieler
        ground_base = Entity(
            model='cube',
            scale=(5, 0.2, 5),
            pos=(0, -0.5, 0),
            color=(0, 1, 0, 1),  # GRÜN
            unshaded=True  # Keine Shader - macOS fix
        )
        self.terrain_models['ground'] = ground_base
        print(f"  -> Grüner Boden erstellt bei (0, -0.5, 0)")
        
        # Ein grauer Block zum Klettern
        stone = Entity(
            model='cube',
            scale=(1, 2, 1),
            pos=(2, 1, 0),
            color=(0.5, 0.5, 0.5, 1),  # GRAU
            unshaded=True  # Keine Shader
        )
        self.terrain_models['stone'] = stone
        print(f"  -> Grauer Stein erstellt bei (2, 1, 0)")
        
        # Ein gelber Block rechts
        sand = Entity(
            model='cube',
            scale=(2, 1, 2),
            pos=(-2, 0.5, 0),
            color=(1, 1, 0, 1),  # GELB
            unshaded=True  # Keine Shader
        )
        self.terrain_models['sand'] = sand
        print(f"  -> Gelber Sand erstellt bei (-2, 0.5, 0)")
    
    def update(self, player_pos):
        """Update (placeholder)"""
        pass
