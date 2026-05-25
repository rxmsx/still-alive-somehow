# PLAN

## Gefundene Projektstruktur
- Unreal Engine Projekt: `SurvivalWorldUE5.uproject`, EngineAssociation `5.7`.
- Runtime-Modul: `Source/SurvivalWorldUE5`.
- Aktivierte Plugins laut `.uproject`: `EnhancedInput`, `PCG`, `PythonScriptPlugin`, `EditorScriptingUtilities`.
- UI-Technik: C++ `UUserWidget` mit direktem Slate-Painting in `USurvivalHUDWidget`; keine gefundenen Widget-Blueprints, keine CommonUI-Nutzung, keine vorhandenen UMG-Blueprint-Assets.
- Item-/Rezeptdaten: C++ `USurvivalItemCatalog : UDataAsset` ist vorhanden; aktuell existiert zusaetzlich `Content/OpenWorld/Data/ResourceCatalog.json` als dokumentierter Katalog mit 41 Items und 7 Rezepten. Die referenzierten Icon-/Mesh-Assets aus dem JSON sind im Content-Ordner nicht vorhanden.
- Gameplay-Systeme: Inventory, Crafting, Survival Stats, Player Controller, Map Marker/World Map und Interactable-Interface sind in C++ vorhanden.

## Relevante UI-, Inventory-, Crafting- und HUD-Dateien
- `Source/SurvivalWorldUE5/Public/Items/SurvivalItemTypes.h`
- `Source/SurvivalWorldUE5/Private/Items/SurvivalItemTypes.cpp`
- `Source/SurvivalWorldUE5/Public/Items/InventoryComponent.h`
- `Source/SurvivalWorldUE5/Private/Items/InventoryComponent.cpp`
- `Source/SurvivalWorldUE5/Public/Crafting/CraftingComponent.h`
- `Source/SurvivalWorldUE5/Private/Crafting/CraftingComponent.cpp`
- `Source/SurvivalWorldUE5/Public/UI/SurvivalHUD.h`
- `Source/SurvivalWorldUE5/Private/UI/SurvivalHUD.cpp`
- `Source/SurvivalWorldUE5/Public/UI/SurvivalHUDWidget.h`
- `Source/SurvivalWorldUE5/Private/UI/SurvivalHUDWidget.cpp`
- `Source/SurvivalWorldUE5/Public/Survival/SurvivalStatsComponent.h`
- `Source/SurvivalWorldUE5/Private/Survival/SurvivalStatsComponent.cpp`
- `Source/SurvivalWorldUE5/Public/Player/SurvivalCharacter.h`
- `Source/SurvivalWorldUE5/Private/Player/SurvivalCharacter.cpp`
- `Source/SurvivalWorldUE5/Public/Player/SurvivalPlayerController.h`
- `Source/SurvivalWorldUE5/Private/Player/SurvivalPlayerController.cpp`
- `Source/SurvivalWorldUE5/Public/Map/WorldMapSubsystem.h`
- `Source/SurvivalWorldUE5/Private/Map/WorldMapSubsystem.cpp`
- `Content/OpenWorld/Data/ResourceCatalog.json`

## Geplante Umsetzung
- Bestehende Gameplay-Logik beibehalten: `UInventoryComponent`, `UCraftingComponent`, `USurvivalStatsComponent` und Controller-State bleiben die Datenquelle.
- Das vorhandene Slate-gezeichnete `USurvivalHUDWidget` weiter ausbauen, statt eine isolierte Demo oder neue unverbundene Widget-Blueprints zu erstellen.
- Inventory-Overlay visuell auf ein dunkles Survival-Panel mit Metall-/Stoff-/Leder-Anmutung, Slot-Grid, Hover-/Selected-Zustaenden, Stack-Zahlen, Gewichtsanzeige und volleren Tooltips umstellen.
- Crafting-UI im selben Widget als Werkbank-/Blueprint-artige Detailansicht ausbauen: Rezeptliste, Zutaten mit vorhanden/benoetigt, Ergebnis, dezenter Crafting-Button und fehlende Zutaten.
- HUD kompakt und immersiv halten: Gesundheit, Ausdauer, Hunger und Durst aus `USurvivalStatsComponent`; Hotbar/Quickslot-Leiste aus vorhandenen Inventory-Stapeln; vorhandene Map-/Marker-Daten nur visuell rahmen, kein neues Mapping-System.
- Item-Icons: Da echte UE-Texture-Assets nicht vorhanden sind und automatische `.uasset`-Erstellung hier riskant ist, werden im Widget zunaechst konsistente prozedurale Item-Piktogramme aus den tatsaechlichen Item-IDs gezeichnet. Parallel wird `ASSET_PROMPTS.md` mit wiederverwendbaren Prompts fuer spaetere KI-generierte Icons und UI-Texturen erstellt.
- Ordnerstruktur `Content/UI/Survival/...` wird angelegt, aber nur fuer nachvollziehbare Dokumentations-/Prompt-Dateien genutzt, solange keine importierten UE-Assets erzeugt werden koennen.
- Abschlussdokumentation in `CHANGES.md` mit geaenderten Dateien, offenen Punkten und Testanleitung.

## Risiken und Annahmen
- Der Worktree enthielt bereits uncommitted Aenderungen in Gameplay-, Save-, Survival- und UI-Dateien. Diese werden als bestehende Projektarbeit behandelt und nicht zurueckgesetzt.
- Ohne Unreal Editor/Asset-Import im laufenden Prozess koennen keine verlaesslichen `.uasset`-Texturen, Materialien oder Widget-Blueprints erzeugt werden. Deshalb wird die visuelle Umsetzung hauptsaechlich ueber Slate-Zeichnung und dokumentierte Asset-Prompts umgesetzt.
- Drag-and-drop-Slots sind im vorhandenen System nicht als echte Inventory-Slots implementiert; das Inventory ist aktuell ein ItemId->Count Bestand. Ich werde die vorhandene Klick-/Craft-/Consume-Funktionalitaet erhalten und sichtbare Hover-/Selection-/Slot-Zustaende vorbereiten, aber kein neues slotbasiertes Inventar erfinden, wenn es die Datenstruktur nicht traegt.
- Equipment-Slots und Container/Loot-Fenster wurden in der Codebasis nicht als eigene Systeme gefunden. Es werden nur visuelle/strukturelle Bereiche vorbereitet, ohne Gameplay-Daten zu erfinden.
- Die im JSON referenzierten `/Game/OpenWorld/UI/Icons/...` Assets fehlen. Icon-Zuordnung zu echten `UTexture2D` Assets kann erst nach Import/Erzeugung dieser Texturen erfolgen.
- Vollstaendige visuelle QA im UE-Viewport kann nur erfolgen, wenn der Unreal Editor oder ein headless Build/Automation-Lauf in dieser Umgebung verfuegbar ist.
