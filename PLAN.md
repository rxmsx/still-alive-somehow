# PLAN

## Gefundene Projektstruktur
- Unreal Engine Projekt: `SurvivalWorldUE5.uproject`, EngineAssociation `5.7`.
- Runtime-Modul: `Source/SurvivalWorldUE5`.
- Aktivierte Plugins: `EnhancedInput`, `PCG`, `PythonScriptPlugin`, `EditorScriptingUtilities`.
- UI: C++/UMG ueber `USurvivalHUDWidget`, aber die sichtbare UI wird direkt in `NativePaint` per Slate gezeichnet. Keine Widget-Blueprints, keine CommonUI-Nutzung und keine Slate-Style-Assets im Content gefunden.
- Daten: `USurvivalItemCatalog : UDataAsset` ist vorhanden. DataTables wurden nicht gefunden. Im Content gibt es aktuell kein ItemCatalog-Asset und keine UI/Icon-Texturen.
- Inventory: `UInventoryComponent` verwaltet aggregierte Item-Counts pro `ItemId`; keine echte slotbasierte Datenstruktur, kein Equipment-/Container-System und kein Drag-and-drop-System gefunden.
- Crafting: `UCraftingComponent` nutzt `FCraftingRecipe` aus ItemCatalog oder zwei C++-Fallback-Rezepte (`StoneBlade`, `Stick`).
- HUD: `USurvivalStatsComponent` liefert echte Gameplay-Werte fuer Gesundheit, Ausdauer, Hunger und Durst. `UWorldMapSubsystem`/Marker sind fuer Map/Minimap vorhanden.

## Relevante UI-/Inventory-/Crafting-Dateien
- `Source/SurvivalWorldUE5/Public/UI/SurvivalHUDWidget.h`
- `Source/SurvivalWorldUE5/Private/UI/SurvivalHUDWidget.cpp`
- `Source/SurvivalWorldUE5/Public/UI/SurvivalHUD.h`
- `Source/SurvivalWorldUE5/Private/UI/SurvivalHUD.cpp`
- `Source/SurvivalWorldUE5/Public/Items/SurvivalItemTypes.h`
- `Source/SurvivalWorldUE5/Public/Items/InventoryComponent.h`
- `Source/SurvivalWorldUE5/Private/Items/InventoryComponent.cpp`
- `Source/SurvivalWorldUE5/Public/Crafting/CraftingComponent.h`
- `Source/SurvivalWorldUE5/Private/Crafting/CraftingComponent.cpp`
- `Source/SurvivalWorldUE5/Public/Survival/SurvivalStatsComponent.h`
- `Source/SurvivalWorldUE5/Private/Survival/SurvivalStatsComponent.cpp`
- `Source/SurvivalWorldUE5/Public/Player/SurvivalPlayerController.h`
- `Source/SurvivalWorldUE5/Private/Player/SurvivalPlayerController.cpp`
- `Source/SurvivalWorldUE5/Public/Map/WorldMapSubsystem.h`
- `Source/SurvivalWorldUE5/Private/Map/WorldMapSubsystem.cpp`

## Geplante Umsetzung
- Bestehende Gameplay-Logik beibehalten und die UI direkt an vorhandene Components binden.
- `USurvivalHUDWidget` zu einer realistischeren Survival-UI umbauen: dunkle Metall-/Stoff-Flaechen, klare Slot-Rahmen, Hover/Selected-Zustaende, Tooltips und kompakte HUD-Meter.
- Inventory weiterhin aus den vorhandenen aggregierten Stacks darstellen; keine neue slotbasierte Speicherlogik erfinden.
- Crafting mit Rezeptliste, Rezeptdetails, Zutatenstatus und Craft-Button neu zeichnen; Crafting-Aufrufe bleiben bei `UCraftingComponent::CraftRecipe`.
- HUD-Layout anpassen: Status unten links, Ausdauer unten mittig, Hotbar unten mittig, Minimap/Kompass oben rechts.
- Item-Icons als einheitliche prozedurale Slate-Piktogramme pro tatsaechlicher ItemId/Category zeichnen, da keine importierten `UTexture2D` Assets vorhanden sind.
- `ASSET_PROMPTS.md` mit wiederverwendbaren Prompts fuer spaetere echte Textur-/Icon-Generierung erstellen. Direkte Bildgenerierung wird hier nicht als Projektasset eingebunden, weil ohne Unreal-Import keine verlaesslichen `.uasset`-Referenzen erzeugt werden koennen.
- `CHANGES.md` mit geaenderten Dateien, offenen Punkten, Grenzen und Testanleitung erstellen.

## Risiken und Annahmen
- Das bestehende Inventory ist nicht slotbasiert. Sichtbare Slots/Hotbar sind UI-Darstellung, keine neue Inventarsemantik.
- Drag & Drop, Equipment-Slots und Container/Loot-Fenster sind nicht als Gameplay-System vorhanden. Ich bereite sichtbare UI-Zustaende vor, erfinde aber keine unverbundene Gameplay-Logik.
- Ohne Unreal Editor/Asset-Import koennen keine echten `.uasset` Texturen, Materials, Fonts oder Widget-Blueprints sicher erzeugt werden.
- Itemgewicht, Haltbarkeit und Seltenheit sind in der aktuellen Item-Struktur nicht vorhanden. Tooltips zeigen nur vorhandene Daten und dokumentieren fehlende Datenpunkte.
- Visuelle QA im echten UE-Viewport ist nur moeglich, wenn ein Editor-/Build-Lauf in dieser Umgebung funktioniert.
