import { randomUUID } from "node:crypto";
import type { GamePrototypePlan, GamePrototypeWizardInput } from "./types.js";

export function createGamePrototypePlan(input: GamePrototypeWizardInput): GamePrototypePlan {
  validatePrototypeInput(input);

  const requiredSystems = unique([
    ...systemsForGameType(input.gameType),
    ...systemsForGameplayStyle(input.gameplayStyle),
    ...systemsForSetupStyle(input.setupStyle)
  ]);
  const suggestedGeneratedAssets = unique([
    ...assetsForGameType(input.gameType),
    ...assetsForGameplayStyle(input.gameplayStyle),
    ...assetsForSetupStyle(input.setupStyle)
  ]);
  const suggestedMapSetup = unique([...mapSetupForScope(input.scope), ...mapSetupForGameType(input.gameType)]);

  return {
    id: randomUUID(),
    createdAt: new Date().toISOString(),
    input,
    title: `${input.gameType} · ${input.scope}`,
    summary: `A safe dry-run plan for a ${input.scope.toLowerCase()} ${input.gameType.toLowerCase()} using ${input.gameplayStyle.toLowerCase()} and a ${input.setupStyle.toLowerCase()}.`,
    requiredSystems,
    suggestedGeneratedAssets,
    suggestedMapSetup,
    firstPlayableMilestone: milestoneFor(input),
    nextRecommendedAction: nextActionFor(input),
    safetyNotes: [
      "This is planning only; no Unreal command is sent.",
      "Any later project changes should use dry-run first and require approval.",
      "Generated assets should stay under /Game/RevoltGenerated."
    ],
    doesExecuteChanges: false
  };
}

function validatePrototypeInput(input: GamePrototypeWizardInput): void {
  if (!input.gameType || !input.scope || !input.gameplayStyle || !input.setupStyle) {
    throw new Error("Choose a game type, scope, gameplay style, and setup style before creating a plan.");
  }
}

function systemsForGameType(gameType: string): string[] {
  switch (gameType) {
    case "First-person shooter":
      return ["First-person player character", "Weapon data", "Health and damage", "Basic enemy target"];
    case "Third-person shooter":
      return ["Third-person player character", "Camera follow setup", "Weapon data", "Health and damage"];
    case "Horror game":
      return ["Player character", "Interaction system", "Objective tracker", "Lighting and atmosphere checklist"];
    case "Survival game":
      return ["Player character", "Inventory placeholder", "Pickup system", "Health and resource data"];
    case "Wave-based zombie game":
      return ["Player character", "Weapon data", "Zombie enemy data", "Wave spawner", "Health and damage"];
    case "Exploration game":
      return ["Player character", "Interaction system", "Objective tracker", "Collectible or clue data"];
    case "Platformer":
      return ["Platforming character", "Checkpoint system", "Pickup system", "Goal objective"];
    case "Top-down shooter":
      return ["Top-down player pawn", "Camera setup", "Weapon data", "Enemy spawner"];
    case "RPG prototype":
      return ["Player character", "Stats data", "Objective tracker", "NPC or enemy data placeholder"];
    default:
      return ["Player start", "Simple pawn or character", "Basic objective checklist"];
  }
}

function systemsForGameplayStyle(style: string): string[] {
  switch (style) {
    case "No combat":
      return ["Non-combat interaction flow", "Completion objective"];
    case "Guns":
      return ["Ammo and reload placeholder", "Weapon tuning data"];
    case "Melee":
      return ["Melee damage data", "Hit range validation"];
    case "Magic":
      return ["Ability data placeholder", "Cooldown and resource values"];
    case "Stealth":
      return ["AI perception notes", "Safe hiding route checklist"];
    case "Survival crafting":
      return ["Inventory placeholder", "Crafting recipe data placeholder"];
    case "Exploration/objectives":
      return ["Objective tracker", "Interactable waypoint checklist"];
    default:
      return [];
  }
}

function systemsForSetupStyle(style: string): string[] {
  if (style === "Advanced C++-friendly setup") {
    return ["Blueprint-friendly C++ base classes", "Data asset tuning workflow"];
  }
  if (style === "Data-driven generated setup") {
    return ["Generated Data Assets", "Template creation dry-run workflow"];
  }
  return ["Beginner Blueprint checklist", "Manual Command Mode workflow"];
}

function assetsForGameType(gameType: string): string[] {
  switch (gameType) {
    case "Wave-based zombie game":
      return [
        "/Game/RevoltGenerated/Templates/ZombieShooter/Data/Zombies/DA_Zombie_Shambler",
        "/Game/RevoltGenerated/Templates/ZombieShooter/Data/Weapons/DA_Weapon_Carbine",
        "/Game/RevoltGenerated/Templates/ZombieShooter/Data/Waves/DA_Zombie_Wave_Test"
      ];
    case "First-person shooter":
    case "Third-person shooter":
    case "Top-down shooter":
      return [
        "/Game/RevoltGenerated/Templates/ArenaShooter/Data/Weapons/DA_Weapon_Rifle",
        "/Game/RevoltGenerated/Templates/ArenaShooter/Data/Enemies/DA_Enemy_Basic",
        "/Game/RevoltGenerated/Templates/ArenaShooter/Data/Waves/DA_Wave_Test"
      ];
    case "Horror game":
      return ["/Game/RevoltGenerated/Blueprints/BP_HorrorObjective", "/Game/RevoltGenerated/Biomes/DA_DarkInteriorBiome"];
    case "Survival game":
      return ["/Game/RevoltGenerated/Blueprints/BP_SurvivalPickup", "/Game/RevoltGenerated/Data/DA_BasicCraftingRecipe"];
    case "Exploration game":
      return ["/Game/RevoltGenerated/Blueprints/BP_InteractableClue", "/Game/RevoltGenerated/Data/DA_ExplorationObjective"];
    case "Platformer":
      return ["/Game/RevoltGenerated/Blueprints/BP_Checkpoint", "/Game/RevoltGenerated/Blueprints/BP_Collectible"];
    case "RPG prototype":
      return ["/Game/RevoltGenerated/Data/DA_PlayerStats", "/Game/RevoltGenerated/Data/DA_StarterQuest"];
    default:
      return ["/Game/RevoltGenerated/Blueprints/BP_PrototypePawn", "/Game/RevoltGenerated/Data/DA_PrototypeObjective"];
  }
}

function assetsForGameplayStyle(style: string): string[] {
  switch (style) {
    case "Guns":
      return ["/Game/RevoltGenerated/Templates/ArenaShooter/Data/Weapons/DA_Weapon_Rifle"];
    case "Melee":
      return ["/Game/RevoltGenerated/Data/DA_MeleeDamage"];
    case "Magic":
      return ["/Game/RevoltGenerated/Data/DA_MagicAbility_Test"];
    case "Stealth":
      return ["/Game/RevoltGenerated/Data/DA_StealthPatrol_Test"];
    case "Survival crafting":
      return ["/Game/RevoltGenerated/Data/DA_CraftingRecipe_Test"];
    case "Exploration/objectives":
      return ["/Game/RevoltGenerated/Data/DA_Objective_Test"];
    default:
      return [];
  }
}

function assetsForSetupStyle(style: string): string[] {
  if (style === "Beginner-friendly Blueprint setup") {
    return ["/Game/RevoltGenerated/Blueprints/BP_BeginnerPrototypeController"];
  }
  return [];
}

function mapSetupForScope(scope: string): string[] {
  switch (scope) {
    case "Tiny test room":
      return ["One small room", "PlayerStart", "One goal marker", "Simple lighting"];
    case "Small arena":
      return ["Contained arena", "PlayerStart", "Enemy spawn points", "Pickup locations", "Exit or win objective"];
    case "Linear level":
      return ["Start area", "Two to three connected rooms", "Clear endpoint", "Checkpoint or objective markers"];
    case "Open area":
      return ["Bounded open play space", "Navigation landmarks", "Spawn zones", "Objective locations"];
    default:
      return ["Minimal test map", "PlayerStart", "One prototype objective"];
  }
}

function mapSetupForGameType(gameType: string): string[] {
  if (gameType === "Wave-based zombie game") {
    return ["Zombie spawn ring", "Weapon pickup point", "Extraction or survival timer marker"];
  }
  if (gameType === "Horror game") {
    return ["Safe starting corner", "Dark navigation path", "One tension set piece placeholder"];
  }
  if (gameType === "Platformer") {
    return ["Jump test lane", "Checkpoint marker", "Collectible path"];
  }
  return [];
}

function milestoneFor(input: GamePrototypeWizardInput): string {
  if (input.gameplayStyle === "No combat") {
    return "The player can spawn, move through the test space, interact with one objective, and finish the prototype loop.";
  }
  if (input.gameType === "Wave-based zombie game") {
    return "The player can spawn, pick up a weapon, survive one small zombie wave, and reach a simple success condition.";
  }
  return "The player can spawn, move, use the core gameplay interaction, and reach one clear win or test objective.";
}

function nextActionFor(input: GamePrototypeWizardInput): string {
  if (input.gameType === "Wave-based zombie game") {
    return "Open Manual Command Mode in Advanced Mode and dry-run create_zombie_shooter_template when you are ready to create generated assets.";
  }
  if (["First-person shooter", "Third-person shooter", "Top-down shooter"].includes(input.gameType)) {
    return "Open Manual Command Mode in Advanced Mode and dry-run create_arena_shooter_template when you are ready.";
  }
  return "Use the plan as a checklist first; later, create generated Blueprint/Data Asset pieces with dry-run and approval.";
}

function unique(values: string[]): string[] {
  return Array.from(new Set(values.filter((value) => value.trim().length > 0)));
}
