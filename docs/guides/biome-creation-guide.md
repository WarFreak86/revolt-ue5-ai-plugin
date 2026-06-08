# Biome Creation Guide

Biome assets are generated data assets under `/Game/RevoltGenerated/Biomes`.

## Create

Use `create_biome_asset` with dry-run first:

```json
{
  "asset_path": "/Game/RevoltGenerated/Biomes/DA_ForestBiome",
  "biome": {
    "biome_name": "Forest",
    "seed": 12345
  },
  "dry_run": true,
  "permission_level": "asset_mutation"
}
```

Approve only after reviewing the preview.

## Validate

Use `validate_biome_asset` to check density, seed, slope, height, and terrain settings.

## Examples

See `examples/biomes` for forest, desert, urban ruins, and zombie arena biome JSON.
