"""
UI/HUD System für Survival Game
"""

from ursina import *
from config_3d import *

class HUD:
    """Head-Up Display für Spieler-Stats"""
    
    def __init__(self):
        self.health_text = Text(text='', position=(-0.45, 0.45), scale=2)
        self.hunger_text = Text(text='', position=(-0.45, 0.40), scale=2)
        self.stamina_text = Text(text='', position=(-0.45, 0.35), scale=2)
        self.time_text = Text(text='', position=(0.35, 0.45), scale=2)
        
        # Crosshair
        self.crosshair = Entity(
            model='quad',
            scale=(0.02, 0.02),
            color=color.white,
            parent=camera
        )
    
    def update(self, player, time_of_day):
        """Updated HUD Anzeige"""
        # Health Bar
        health_pct = (player.health / PLAYER_MAX_HEALTH) * 100
        health_color = color.green if health_pct > 50 else color.yellow if health_pct > 25 else color.red
        health_bar = "#" * int(health_pct // 10) + "-" * (10 - int(health_pct // 10))
        self.health_text.text = f'Health: {health_pct:.0f}% [{health_bar}]'
        self.health_text.color = health_color
        
        # Hunger Bar
        hunger_pct = (player.hunger / PLAYER_MAX_HUNGER) * 100
        hunger_color = color.green if hunger_pct > 50 else color.yellow if hunger_pct > 25 else color.red
        hunger_bar = "#" * int(hunger_pct // 10) + "-" * (10 - int(hunger_pct // 10))
        self.hunger_text.text = f'Hunger: {hunger_pct:.0f}% [{hunger_bar}]'
        self.hunger_text.color = hunger_color
        
        # Stamina Bar
        stamina_pct = (player.stamina / PLAYER_MAX_STAMINA) * 100
        stamina_bar = "#" * int(stamina_pct // 10) + "-" * (10 - int(stamina_pct // 10))
        self.stamina_text.text = f'Stamina: {stamina_pct:.0f}% [{stamina_bar}]'
        self.stamina_text.color = color.blue
        
        # Time of Day
        hours = int((time_of_day / DAY_NIGHT_CYCLE) * 24)
        minutes = int(((time_of_day / DAY_NIGHT_CYCLE) * 24 - hours) * 60)
        self.time_text.text = f'Time: {hours:02d}:{minutes:02d}'


class Inventory:
    """Einfaches Inventar-System"""
    
    def __init__(self, max_slots=20):
        self.items = {}
        self.max_slots = max_slots
        self.ui_text = Text(text='Inventory: Leer', position=(-0.45, -0.40), scale=1.8)
    
    def add_item(self, item_name, amount=1):
        """Fügt Item zu Inventar hinzu"""
        if item_name in self.items:
            self.items[item_name] += amount
        else:
            if len(self.items) < self.max_slots:
                self.items[item_name] = amount
            else:
                return False
        return True
    
    def remove_item(self, item_name, amount=1):
        """Entfernt Item aus Inventar"""
        if item_name in self.items:
            self.items[item_name] -= amount
            if self.items[item_name] <= 0:
                del self.items[item_name]
            return True
        return False
    
    def has_item(self, item_name, amount=1):
        """Prüft ob Item vorhanden ist"""
        return self.items.get(item_name, 0) >= amount
    
    def update_ui(self):
        """Updated Inventar UI"""
        if not self.items:
            self.ui_text.text = "Inventory: Leer"
        else:
            inv_text = "Inventory:\n"
            for item, count in list(self.items.items())[:3]:  # Zeige nur erste 3
                inv_text += f"  {item}: {count}\n"
            self.ui_text.text = inv_text
