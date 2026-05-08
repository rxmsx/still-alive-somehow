# Godot 3D Survival Game

## Struktur

```
game_3d_godot/
├── player.gd        # First-Person Player Controller
├── world.gd         # Terrain & Welt
├── hud.gd           # User Interface
└── project.godot    # Godot Projekt Config
```

## Setup

1. **Godot öffnen:**
   ```bash
   godot
   ```

2. **Projekt öffnen:**
   - "Open" → `/Users/ramsiado/lernen/game_3d_godot`

3. **Main Scene erstellen:**
   - Neue Scene: "3D Scene" → "Node3D" 
   - Speichern als: `main.tscn`

4. **Struktur im Scene Tree:**
   ```
   MainScene (Node3D)
   ├── Player (CharacterBody3D) 
   │   ├── CollisionShape3D
   │   └── Camera3D
   ├── World (Node3D)
   │   ├── Terrain
   │   ├── Trees
   │   └── Rocks
   └── HUD (CanvasLayer)
       └── VBoxContainer
           ├── HealthLabel
           ├── HungerLabel
           └── StaminaLabel
   ```

5. **Scripts zuweisen:**
   - Player → `player.gd`
   - World → `world.gd`
   - HUD → `hud.gd`

6. **Spielen:**
   - F5 oder Play Button

## Steuerung

- **WASD** - Bewegung
- **SHIFT** - Sprint
- **SPACE** - Jump
- **Maus** - Umschauen
- **ESC** - Beenden

## Features

✅ First-Person Player  
✅ Survival Stats (Health/Hunger/Stamina)  
✅ Terrain Rendering  
✅ HUD Display  
⏳ Gegner (in Arbeit)  
⏳ Crafting System (in Arbeit)  

---

**Status:** Alpha - Grundstruktur fertig, wird erweitert
