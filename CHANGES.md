# CHANGES

## Survival-Meilenstein: Interaktion, Ressourcen, Bauen, Container, Campfire, Save

### Neu/erweitert
- Interaktion bleibt Line-Trace-basiert und zeigt jetzt `E`-Prompts plus Feedback bei blockierten Aktionen.
- World-Pickups nutzen weiter `AItemPickupActor` und legen Items ins bestehende `UInventoryComponent`; bei vollem Inventar bleibt der Pickup liegen.
- Ressourcenquellen wurden von Einzel-Output auf datengetriebene Loot-Tabellen, Lebenspunkte, Werkzeugtypen, Bare-Hand-Fallback und Depleted-State erweitert.
- Werkzeuge besitzen nun ToolType, Effizienz, Harvest-Schaden, Haltbarkeitsverlust und Brennstoffdaten.
- Neues modulares Bausystem unter `Source/SurvivalWorldUE5/Public|Private/Building` mit Build-Catalog, Ghost Preview, Rotation, Valid/Invalid-State, Materialkosten und Snap für Wände an Fundamente.
- Neue Buildables: Fundament, Wand, Tuer, Dach, Lagerkiste, Lagerfeuer, Werkbank, Schlafsack.
- Lagerkiste und Lagerfeuer besitzen eigene Inventare; das HUD kann Items zwischen Spielerinventar und offenem World-Inventory transferieren.
- Lagerfeuer verbraucht Brennstoff-Items ueber deren Itemdaten, kocht rohes Fleisch zu gekochtem Fleisch und kocht schmutziges Wasser zu sauberem Wasser ab.
- SaveGame speichert nun Spielerwerte, Inventar/Hotbar, Ressourcen, gebaute Objekte, Container-Inhalte und Campfire-Zustand.
- Debug-Execs im PlayerController: `GiveItem`, `SpawnResource`, `SaveSurvival`, `LoadSurvival`, `SelectBuildPart`.
- GameMode spawnt in der aktuellen Testumgebung einen kleinen Milestone-Parcours mit Pickups und Baum/Felsen/Busch.

### Pruefung
- `SurvivalWorldUE5Editor Mac Development`: erfolgreich.
- Headless Automation `Automation RunTests SurvivalWorld`: 7 Tests, alle erfolgreich.

## Angepasste Dateien
- `Source/SurvivalWorldUE5/Public/UI/SurvivalHUDWidget.h`
- `Source/SurvivalWorldUE5/Private/UI/SurvivalHUDWidget.cpp`
- `Source/SurvivalWorldUE5/Public/Crafting/CraftingComponent.h`
- `Source/SurvivalWorldUE5/Private/Crafting/CraftingComponent.cpp`
- `PLAN.md`
- `ASSET_PROMPTS.md`

## Neu erstellte Asset-Struktur
- `Content/UI/Survival/Textures/Panels`
- `Content/UI/Survival/Textures/Slots`
- `Content/UI/Survival/Textures/HUD`
- `Content/UI/Survival/Textures/Icons`
- `Content/UI/Survival/Textures/Tooltips`
- `Content/UI/Survival/Materials`
- `Content/UI/Survival/Widgets`
- `Content/UI/Survival/Fonts`
- `Content/UI/Survival/Data`

## Umgesetzte UI-Aenderungen
- Survival-HUD neu ausgerichtet: Statuswerte unten links, Ausdauer unten mittig, Hotbar unten mittig, Minimap/Kompass oben rechts.
- Inventory-Overlay auf dunkle, realistische Panel-/Slot-Optik umgestellt.
- Inventory-Stapel werden als Slots mit Hover- und Selected-Zustand angezeigt.
- Tooltips zeigen vorhandene Itemdaten: Name, Kategorie, Bestand und Beschreibung.
- Gewicht/Zustand werden bewusst als fehlende Itemdaten markiert, weil diese Felder aktuell nicht in `FItemDef` existieren.
- Crafting-UI mit Rezeptliste, Rezeptdetails, Zutatenstatus und separatem Craft-Button ueberarbeitet.
- Crafting-Logik bleibt bei `UCraftingComponent::CanCraft` und `CraftRecipe`; Zutaten werden weiterhin nur ueber die bestehende Logik entfernt.
- Hotbar nutzt vorhandene Inventar-Stapel als visuelle Quickslot-Darstellung, ohne neues slotbasiertes Inventory-System zu erfinden.
- Item-Icons werden zunaechst als einheitliche prozedurale Slate-Piktogramme aus echten ItemIds/Kategorien gezeichnet.
- `UCraftingComponent::GetItemDefinition` wurde ergaenzt, damit UI und Blueprints vorhandene Itembeschreibungen sauber abrufen koennen.

## Neu erstellte Assets
- Keine importierten `.uasset`-Texturen oder Materialien. Ohne Unreal-Editor-Importpipeline waeren direkte Binary-Assets riskant und schnell gebrochene Referenzen.
- `ASSET_PROMPTS.md` enthaelt wiederverwendbare Prompts fuer Inventory-Panel, Slot-Varianten, Crafting-Panel, Tooltip, HUD-Symbole, Minimap-/Kompass-Rahmen, Hotbar und aktuelle Fallback-Item-Icons.

## Bekannte Einschraenkungen
- Kein echtes Drag & Drop: Das vorhandene Inventory speichert aggregierte Counts, keine Slots.
- Keine Equipment- oder Container-/Loot-Fenster-Logik gefunden; die UI zeigt Ausruestung/Quickslots nur aus vorhandenen Stacks.
- Keine echten `UTexture2D` Item-Icons vorhanden; Icon-Zuordnung zu DataAssets ist vorbereitet, aber nicht importiert.
- Keine Haltbarkeit, Gewicht oder Seltenheit in der aktuellen Itemstruktur; die UI erfindet diese Werte nicht.
- Visuelle Pruefung im Editor-Viewport wurde nicht automatisiert, aber der C++ Editor-Build wurde erfolgreich ausgefuehrt.

## Testanleitung im Spiel
1. Projekt in Unreal Editor 5.7 oeffnen.
2. Level starten.
3. `Tab` druecken: Inventory/Crafting-Overlay oeffnet sich.
4. Mit der Maus ueber Slots fahren: Hover-Zustand und Tooltip pruefen.
5. Items anklicken: Selected-Zustand pruefen.
6. Crafting-Rezept anklicken: Detailansicht pruefen.
7. Bei vorhandenem Material `Craften` klicken: Stack-Verbrauch und Output im Inventar pruefen.
8. `M` druecken: Karten-Overlay pruefen.
9. Sprinten: Ausdauer unten mittig beobachten.
10. Hunger/Durst/Gesundheit ueber laengere Laufzeit oder Debugwerte pruefen.

## Durchgefuehrte Pruefung
- `SurvivalWorldUE5Editor Mac Development` via UnrealBuildTool: erfolgreich.
