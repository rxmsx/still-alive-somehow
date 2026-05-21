# Open World Content Layout

Binary `.umap` and `.uasset` files are created in the Unreal Editor after the engine install completes.

- `Maps/`: `L_OpenWorld_Prototype.umap`, World Partition cells, HLOD data.
- `PCG/`: biome/resource/cave entrance PCG graphs.
- `Data/`: item, biome, resource-node, recipe, and ore-vein Data Assets.
- `Caves/`: cave and mineshaft Level Instances.
- `Resources/`: Blueprint children of `AResourceNodeActor`.

The C++ code already exposes the core item, resource, biome, ore, inventory, survival-stat, seed, and save types these assets should use.
