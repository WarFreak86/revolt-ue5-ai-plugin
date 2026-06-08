# Example Prompt Library

These prompts are designed for local/offline use through Manual Command Mode, MCP clients, or local models.

## Inspect Current Level

Inspect the current Unreal level. Return the map name, actor count, selected actors if any, generated actors, and obvious safety issues. Use read-only commands only.

## Find All Weapon Data Assets

Find all weapon data assets under `/Game/RevoltGenerated`. Include asset path, class, and which template they belong to.

## Reduce Recoil By 15 Percent

Dry-run first: reduce vertical and horizontal recoil by 15 percent for generated zombie shooter weapon data assets. Do not apply changes until approved.

## Create Forest Biome

Create a generated forest biome asset from `examples/biomes/forest-biome.json`. Use dry-run first and require approval before creation.

## Create Desert Biome

Create a generated desert biome asset from `examples/biomes/desert-biome.json`. Use dry-run first and validate the biome after creation.

## Spawn Biome Content

Generate a terrain preview, then dry-run biome content spawning for the selected `ARevoltLandGenActor`. Apply only after approval.

## Audit Project

Run a read-only project audit. Include current level issues, Blueprint compile issues, generated asset placement issues, and a severity summary.

## Generate Fix Plan

Generate a read-only fix plan from the latest audit. Do not execute any fixes.

## Create Arena Shooter Template

Dry-run `create_arena_shooter_template`, review planned assets, then create the generated Arena Shooter template after approval.
