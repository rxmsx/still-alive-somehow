# 🌍 Survival World - 3D Open-World Survival Game

Ein ambitioniertes Lernprojekt: First-Person Open-World Survival Game in Python mit Ursina 3D Engine.

## 🎮 Starten

```bash
cd /Users/ramsiado/lernen/game
python main_game.py
```

## 🕹️ Steuerung

| Taste | Aktion |
|-------|--------|
| **W/A/S/D** | Bewegung (vorwärts/links/rückwärts/rechts) |
| **SPACE** | Springen |
| **SHIFT** | Sprint (verbraucht Stamina & Hunger) |
| **Maus** | Umschauen (First-Person Look-Around) |
| **ESC** | Pausieren |
| **F3** | Debug Info anzeigen |
| **I** | Inventar (später) |

## 🎯 Aktuelle Features (Phase 1)

✅ **First-Person Kamera** mit Maus-Look
✅ **Terrain Generation** mit Noise-basierter Map
✅ **Survival Stats** - Health, Hunger, Stamina
✅ **Physik** - Gravität, Sprünge, Bewegung
✅ **Tag/Nacht Zyklus** mit Beleuchtung
✅ **HUD** - Status-Anzeigen
✅ **Inventar System** (Basis)

## 🏗️ Projektstruktur

```
game/
├── main_game.py      # Hauptprogramm - Hier startest du
├── config_3d.py      # Einstellungen & Konstanten
├── world.py          # Terrain & World Management
├── player.py         # First-Person Spieler
├── ui.py             # HUD & Inventar
├── server.py         # Multiplayer Server (später)
└── client_3d.py      # Multiplayer Client (später)
```

## 📚 Was du hier lernst

- **3D Grafik & Ursina Engine** - Wie 3D-Spiele funktionieren
- **Terrain Generation** - Prozedurale Map-Erstellung
- **First-Person Physik** - Kamera, Bewegung, Gravität
- **Survival Mechanics** - Health/Hunger/Stamina Management
- **Game State Management** - Pausieren, Stats, Zeit
- **HUD/UI-System** - Spieler-Feedback
- **Multiplayer Architektur** - Vorbereitung für Phase 4

## 🚀 Nächste Schritte

### Phase 1 Verbesserungen
- [ ] Bessere Terrain-Grafik (Texturen statt Farben)
- [ ] Mehrere Biome (Wald, Berge, Wüste, Wasser)
- [ ] Resource-Knoten (Holz, Stein, etc.)
- [ ] Sound-System

### Phase 2 (Crafting & Building)
- [ ] Crafting-System
- [ ] Gegner/Feinde
- [ ] Einfache Gebäude
- [ ] Nahrung finden

### Phase 3 (Expansion)
- [ ] Komplexere Structures
- [ ] Bessere KI für Gegner
- [ ] Wetterbedingungen
- [ ] Save/Load System

### Phase 4 (Multiplayer)
- [ ] Mehrspieler über Netzwerk
- [ ] Andere Spieler sehen
- [ ] Gemeinsam bauen
- [ ] Synchronisierung

## ⚙️ System-Anforderungen

- Python 3.8+
- Pygame/Ursina/Panda3D
- 4GB RAM empfohlen
- Dedizierte Grafikkarte empfohlen

## 🎓 Wichtige Konzepte zum Lernen

1. **Objektorientierte Programmierung (OOP)** - Klassen für World, Player, etc.
2. **Game Loops** - Hauptupdate-Schleife
3. **3D Vektoren & Matrizen** - Position, Rotation, Skalierung
4. **Terrain Generation** - Noise-basierte prozedurale Erzeugung
5. **Physics Simulation** - Gravität, Kollisionen
6. **State Management** - Spielmodus, Pause, etc.

---

**Status**: 🟡 In Entwicklung - Phase 1 Foundation läuft!

Fehler? Fragen? Gib mir Bescheid! 💻
