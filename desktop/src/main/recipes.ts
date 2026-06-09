import type { RecipeCommandPlanStep, RecipeDefinition, RecipeResultPanel } from "./types.js";

const baseRecipeCatalog: RecipeDefinition[] = [
  {
    recipeId: "create_health_pickup",
    title: "Create Health Pickup",
    description: "Create a generated health pickup Blueprint asset structure and optionally place it in the current level.",
    beginnerDescription: "Creates an item the player can collect to restore health.",
    category: "Pickup",
    difficulty: "Easy",
    requiredPermissionLevel: "blueprint_mutation",
    outputSummary: "Generated health pickup Blueprint under /Game/RevoltGenerated/Pickups.",
    beginnerApprovalSummary: "This will create a generated Blueprint for a health pickup. It will not edit your existing assets.",
    formFields: [
      textField("blueprintName", "Blueprint name", "BP_HealthPickup"),
      numberField("healAmount", "Heal amount", 25),
      numberField("respawnTime", "Respawn time", 10),
      optionalAssetPathField("pickupMeshAssetPath", "Pickup mesh asset path, optional", ""),
      optionalAssetPathField("pickupSoundAssetPath", "Pickup sound asset path, optional", ""),
      checkboxField("placeInCurrentLevel", "Place in current level", false),
      vectorField("location", "Location, optional", [0, 0, 120]),
      assetPathField("generatedFolderPath", "Generated folder path", "/Game/RevoltGenerated/Pickups")
    ],
    generatedCommandPlan: [],
    supportsDryRun: true,
    requiresApproval: true
  },
  {
    recipeId: "create_basic_enemy",
    title: "Create Basic Enemy",
    description: "Plan a generated enemy Data Asset and starter enemy class.",
    beginnerDescription: "Creates the building blocks for a simple enemy.",
    category: "Enemy",
    difficulty: "Medium",
    requiredPermissionLevel: "blueprint_mutation",
    outputSummary: "Generated enemy Blueprint/Data Asset plan.",
    beginnerApprovalSummary: "This will plan generated enemy assets only. It will not edit your existing assets.",
    formFields: [
      textField("enemyName", "Enemy name", "BP_BasicEnemy"),
      actorClassField("enemyClass", "Enemy parent class", "Character"),
      numberField("health", "Health", 100),
      selectField("behavior", "Behavior", "Simple chase", ["Simple chase", "Stationary target", "Patrol placeholder"])
    ],
    generatedCommandPlan: [],
    supportsDryRun: true,
    requiresApproval: true
  },
  {
    recipeId: "create_simple_weapon",
    title: "Create Simple Weapon",
    description: "Plan a generated weapon Data Asset with beginner-friendly tuning.",
    beginnerDescription: "Sets up simple weapon values like damage, ammo, and fire rate.",
    category: "Weapon",
    difficulty: "Medium",
    requiredPermissionLevel: "asset_mutation",
    outputSummary: "Generated weapon data plan under /Game/RevoltGenerated.",
    beginnerApprovalSummary: "This will plan generated weapon data only. It will not edit your existing assets.",
    formFields: [
      textField("weaponName", "Weapon name", "DA_Weapon_Test"),
      assetPathField("assetPath", "Generated weapon asset path", "/Game/RevoltGenerated/Templates/ArenaShooter/Data/Weapons/DA_Weapon_Test"),
      numberField("damage", "Damage", 20),
      numberField("ammo", "Ammo", 30),
      numberField("fireRate", "Fire rate", 6)
    ],
    generatedCommandPlan: [],
    supportsDryRun: true,
    requiresApproval: true
  },
  {
    recipeId: "add_player_start",
    title: "Add PlayerStart",
    description: "Dry-run spawning a PlayerStart in the current level.",
    beginnerDescription: "Adds the spot where the player begins the level.",
    category: "Level",
    difficulty: "Easy",
    requiredPermissionLevel: "editor_mutation",
    outputSummary: "A PlayerStart spawn dry-run for the open level.",
    beginnerApprovalSummary: "This will add a generated PlayerStart to the current level only after approval.",
    formFields: [vectorField("location", "Start location", [0, 0, 120]), numberField("yaw", "Facing direction", 0)],
    generatedCommandPlan: [],
    supportsDryRun: true,
    requiresApproval: true
  },
  {
    recipeId: "create_main_menu",
    title: "Create Main Menu",
    description: "Plan a generated main menu Widget Blueprint.",
    beginnerDescription: "Creates a starter screen with Play and Quit buttons later.",
    category: "UI",
    difficulty: "Advanced",
    requiredPermissionLevel: "blueprint_mutation",
    outputSummary: "Generated main menu Widget Blueprint plan.",
    beginnerApprovalSummary: "This will plan a generated menu Blueprint only. It will not edit your existing assets.",
    formFields: [
      textField("menuName", "Menu Blueprint name", "WBP_MainMenu"),
      assetPathField("assetPath", "Generated menu path", "/Game/RevoltGenerated/Blueprints/WBP_MainMenu"),
      selectField("style", "Menu style", "Simple dark", ["Simple dark", "Sci-fi", "Horror", "Clean prototype"])
    ],
    generatedCommandPlan: [],
    supportsDryRun: true,
    requiresApproval: true
  },
  {
    recipeId: "create_zombie_arena",
    title: "Create Zombie Arena",
    description: "Plan generated zombie shooter template assets and a small test arena.",
    beginnerDescription: "Creates a starter zombie arena plan with enemies, weapon data, and a test wave.",
    category: "Enemy",
    difficulty: "Medium",
    requiredPermissionLevel: "asset_mutation",
    outputSummary: "Zombie arena generated template dry-run plan.",
    beginnerApprovalSummary: "This will plan generated zombie arena content only. It will not edit your existing assets.",
    formFields: [
      textField("arenaName", "Arena name", "L_ZombieArena_Test"),
      selectField("arenaSize", "Arena size", "Small arena", ["Tiny test room", "Small arena", "Open area"]),
      numberField("starterWaveCount", "Starter zombies", 6),
      checkboxField("includeTestMap", "Include generated test map plan", true)
    ],
    generatedCommandPlan: [],
    supportsDryRun: true,
    requiresApproval: true
  }
];

export const recipeCatalog: RecipeDefinition[] = baseRecipeCatalog.map((recipe) => ({
  ...recipe,
  generatedCommandPlan: buildRecipeCommandPlan(recipe.recipeId, defaultRecipeValues(recipe))
}));

export function getRecipe(recipeId: string): RecipeDefinition | undefined {
  const recipe = recipeCatalog.find((item) => item.recipeId === recipeId);
  return recipe ? withCommandPlan(recipe, defaultRecipeValues(recipe)) : undefined;
}

export function buildRecipe(recipeId: string, values: Record<string, unknown>): RecipeDefinition {
  const recipe = recipeCatalog.find((item) => item.recipeId === recipeId);
  if (!recipe) {
    throw new Error("Recipe does not exist.");
  }
  return withCommandPlan(recipe, { ...defaultRecipeValues(recipe), ...values });
}

function withCommandPlan(recipe: RecipeDefinition, values: Record<string, unknown>): RecipeDefinition {
  return {
    ...recipe,
    generatedCommandPlan: buildRecipeCommandPlan(recipe.recipeId, values)
  };
}

function buildRecipeCommandPlan(recipeId: string, values: Record<string, unknown>): RecipeCommandPlanStep[] {
  switch (recipeId) {
    case "add_player_start":
      return [
        {
          label: "Dry-run a PlayerStart actor spawn",
          command: "spawn_actor",
          params: {
            class: "PlayerStart",
            location: readVector(values.location, [0, 0, 120]),
            rotation: [0, Number(values.yaw ?? 0), 0],
            dry_run: true,
            approved: false,
            permission_level: "editor_mutation"
          },
          supported: true
        }
      ];
    case "create_zombie_arena":
      return [
        {
          label: "Preview generated zombie shooter template assets",
          command: "create_zombie_shooter_template",
          params: {
            template_name: String(values.arenaName ?? "L_ZombieArena_Test"),
            arena_size: String(values.arenaSize ?? "Small arena"),
            starter_wave_count: Number(values.starterWaveCount ?? 6),
            dry_run: true,
            approved: false,
            permission_level: "asset_mutation"
          },
          supported: false,
          reason: "Recipe foundation does not execute template generation yet; use Manual Command Mode after reviewing the plan."
        }
      ];
    case "create_health_pickup":
      return buildHealthPickupPlan(values);
    case "create_basic_enemy":
      return unsupportedPlan("create_blueprint_class", "Enemy Blueprint wiring is planned but not executed in the recipe foundation.", {
        blueprint_path: `/Game/RevoltGenerated/Blueprints/${values.enemyName ?? "BP_BasicEnemy"}`,
        parent_class: String(values.enemyClass ?? "Character"),
        health: Number(values.health ?? 100),
        behavior: String(values.behavior ?? "Simple chase")
      });
    case "create_simple_weapon":
      return unsupportedPlan("create_data_asset", "Weapon Data Asset creation is preview-only in the recipe foundation.", {
        asset_path: String(values.assetPath ?? `/Game/RevoltGenerated/Templates/ArenaShooter/Data/Weapons/${values.weaponName ?? "DA_Weapon_Test"}`),
        damage: Number(values.damage ?? 20),
        ammo: Number(values.ammo ?? 30),
        fire_rate: Number(values.fireRate ?? 6)
      });
    case "create_main_menu":
      return unsupportedPlan("create_blueprint_class", "Widget Blueprint menu creation is preview-only in the recipe foundation.", {
        blueprint_path: String(values.assetPath ?? `/Game/RevoltGenerated/Blueprints/${values.menuName ?? "WBP_MainMenu"}`),
        parent_class: "UserWidget",
        style: String(values.style ?? "Simple dark")
      });
    default:
      return [];
  }
}

export function buildRecipeResultPanel(recipe: RecipeDefinition, response: unknown, approved: boolean): RecipeResultPanel | undefined {
  if (recipe.recipeId !== "create_health_pickup") {
    return undefined;
  }

  const responses = Array.isArray(response) ? response : [];
  const blueprintCreated = approved && responses.some((item) => isOkResponseForCommand(item, "create_blueprint_class")) ? "Yes" : approved ? "Attempted" : "Not created during dry-run";
  const variablesAdded = approved && responses.some((item) => isOkResponseForCommand(item, "add_blueprint_variable")) ? "HealAmount and RespawnTime planned/applied" : "Planned only";
  const compileResponse = responses.find((item) => isOkResponseForCommand(item, "compile_blueprint")) as { response?: { result?: Record<string, unknown>; }; } | undefined;
  const compileResult = compileResponse?.response?.result;
  const compiledStatus = typeof compileResult?.after_status === "string" ? compileResult.after_status : approved ? "Check Unreal compile result details" : "Not compiled during dry-run";
  const placedInLevel = recipe.generatedCommandPlan.some((step) => step.command === "spawn_actor")
    ? approved && responses.some((item) => isOkResponseForCommand(item, "spawn_actor"))
      ? "Yes"
      : "Planned; apply after Blueprint creation if needed"
    : "No";

  return {
    blueprintCreated,
    variablesAdded,
    compiledStatus,
    placedInLevel,
    nextRecommendedStep: "Open the generated Blueprint and add overlap/gameplay graph logic to restore player health when collected.",
    missingBehaviorNote: "Complex Blueprint graph behavior is not implemented yet; this recipe creates the safe asset structure and tunable variables."
  };
}

function buildHealthPickupPlan(values: Record<string, unknown>): RecipeCommandPlanStep[] {
  const blueprintName = sanitizeAssetName(String(values.blueprintName ?? "BP_HealthPickup"));
  const generatedFolderPath = normalizeGeneratedFolder(String(values.generatedFolderPath ?? "/Game/RevoltGenerated/Pickups"));
  const blueprintPath = `${generatedFolderPath}/${blueprintName}`;
  const healAmount = Number(values.healAmount ?? 25);
  const respawnTime = Number(values.respawnTime ?? 10);
  const pickupMeshAssetPath = String(values.pickupMeshAssetPath ?? "").trim();
  const pickupSoundAssetPath = String(values.pickupSoundAssetPath ?? "").trim();
  const placeInCurrentLevel = values.placeInCurrentLevel === true;

  const steps: RecipeCommandPlanStep[] = [
    supportedStep("Create generated health pickup Blueprint", "create_blueprint_class", {
      asset_path: blueprintPath,
      parent_class: "Actor",
      dry_run: true,
      approved: false,
      permission_level: "blueprint_mutation"
    }),
    supportedStep("Add StaticMeshComponent placeholder", "add_component_to_blueprint", {
      blueprint_path: blueprintPath,
      component_class: "StaticMeshComponent",
      component_name: "PickupMesh",
      dry_run: true,
      approved: false,
      permission_level: "blueprint_mutation"
    }),
    supportedStep("Add HealAmount variable", "add_blueprint_variable", {
      blueprint_path: blueprintPath,
      variable_name: "HealAmount",
      variable_type: "float",
      dry_run: true,
      approved: false,
      permission_level: "blueprint_mutation"
    }),
    supportedStep("Set HealAmount default", "set_blueprint_default_value", {
      blueprint_path: blueprintPath,
      variable_name: "HealAmount",
      value: healAmount,
      dry_run: true,
      approved: false,
      permission_level: "blueprint_mutation"
    }),
    supportedStep("Add RespawnTime variable", "add_blueprint_variable", {
      blueprint_path: blueprintPath,
      variable_name: "RespawnTime",
      variable_type: "float",
      dry_run: true,
      approved: false,
      permission_level: "blueprint_mutation"
    }),
    supportedStep("Set RespawnTime default", "set_blueprint_default_value", {
      blueprint_path: blueprintPath,
      variable_name: "RespawnTime",
      value: respawnTime,
      dry_run: true,
      approved: false,
      permission_level: "blueprint_mutation"
    })
  ];

  if (pickupMeshAssetPath) {
    steps.push(
      supportedStep("Store optional pickup mesh asset path", "add_blueprint_variable", {
        blueprint_path: blueprintPath,
        variable_name: "PickupMeshAssetPath",
        variable_type: "string",
        dry_run: true,
        approved: false,
        permission_level: "blueprint_mutation"
      }),
      supportedStep("Set optional pickup mesh asset path", "set_blueprint_default_value", {
        blueprint_path: blueprintPath,
        variable_name: "PickupMeshAssetPath",
        value: pickupMeshAssetPath,
        dry_run: true,
        approved: false,
        permission_level: "blueprint_mutation"
      })
    );
  }

  if (pickupSoundAssetPath) {
    steps.push(
      supportedStep("Store optional pickup sound asset path", "add_blueprint_variable", {
        blueprint_path: blueprintPath,
        variable_name: "PickupSoundAssetPath",
        variable_type: "string",
        dry_run: true,
        approved: false,
        permission_level: "blueprint_mutation"
      }),
      supportedStep("Set optional pickup sound asset path", "set_blueprint_default_value", {
        blueprint_path: blueprintPath,
        variable_name: "PickupSoundAssetPath",
        value: pickupSoundAssetPath,
        dry_run: true,
        approved: false,
        permission_level: "blueprint_mutation"
      })
    );
  }

  steps.push(
    supportedStep("Compile health pickup Blueprint", "compile_blueprint", {
      blueprint_path: blueprintPath,
      dry_run: true,
      approved: false,
      permission_level: "blueprint_mutation"
    })
  );

  if (placeInCurrentLevel) {
    steps.push(
      supportedStep("Place generated pickup in current level", "spawn_actor", {
        class: blueprintPath,
        location: vectorObject(readVector(values.location, [0, 0, 120])),
        rotation: rotatorObject([0, 0, 0]),
        dry_run: true,
        approved: false,
        permission_level: "editor_mutation"
      })
    );
  }

  return steps;
}

function supportedStep(label: string, command: string, params: Record<string, unknown>): RecipeCommandPlanStep {
  return { label, command, params, supported: true };
}

function unsupportedPlan(command: string, reason: string, params: Record<string, unknown>): RecipeCommandPlanStep[] {
  return [
    {
      label: "Preview planned generated-content command",
      command,
      params: {
        ...params,
        dry_run: true,
        approved: false
      },
      supported: false,
      reason
    }
  ];
}

function defaultRecipeValues(recipe: RecipeDefinition): Record<string, unknown> {
  return Object.fromEntries(recipe.formFields.map((field) => [field.id, field.defaultValue]));
}

function textField(id: string, label: string, defaultValue: string) {
  return { id, label, type: "text" as const, required: true, defaultValue, helpText: "Use a clear generated-content name." };
}

function numberField(id: string, label: string, defaultValue: number) {
  return { id, label, type: "number" as const, required: true, defaultValue, helpText: "Use a small prototype-friendly value." };
}

function selectField(id: string, label: string, defaultValue: string, options: string[]) {
  return { id, label, type: "select" as const, required: true, defaultValue, helpText: "Choose one beginner-safe option.", options };
}

function checkboxField(id: string, label: string, defaultValue: boolean) {
  return { id, label, type: "checkbox" as const, required: false, defaultValue, helpText: "Optional recipe choice." };
}

function assetPathField(id: string, label: string, defaultValue: string) {
  return { id, label, type: "asset-path" as const, required: true, defaultValue, helpText: "Generated assets should stay under /Game/RevoltGenerated." };
}

function optionalAssetPathField(id: string, label: string, defaultValue: string) {
  return { id, label, type: "asset-path" as const, required: false, defaultValue, helpText: "Optional. Leave blank if you do not have an asset yet." };
}

function actorClassField(id: string, label: string, defaultValue: string) {
  return { id, label, type: "actor-class" as const, required: true, defaultValue, helpText: "Use a safe Unreal class such as Actor, Pawn, or Character." };
}

function vectorField(id: string, label: string, defaultValue: [number, number, number]) {
  return { id, label, type: "vector-location" as const, required: true, defaultValue, helpText: "X, Y, Z coordinates in the current level." };
}

function readVector(value: unknown, fallback: [number, number, number]): [number, number, number] {
  if (Array.isArray(value) && value.length === 3) {
    return [Number(value[0]), Number(value[1]), Number(value[2])];
  }
  return fallback;
}

function normalizeGeneratedFolder(folderPath: string): string {
  const normalized = folderPath.trim().replace(/\\/g, "/").replace(/\/$/, "") || "/Game/RevoltGenerated/Pickups";
  if (normalized !== "/Game/RevoltGenerated" && !normalized.startsWith("/Game/RevoltGenerated/")) {
    throw new Error("Generated folder path must be under /Game/RevoltGenerated.");
  }
  return normalized;
}

function sanitizeAssetName(assetName: string): string {
  const cleaned = assetName.trim().replace(/[^A-Za-z0-9_]/g, "_");
  if (!cleaned) {
    throw new Error("Blueprint name is required.");
  }
  return cleaned;
}

function isOkResponseForCommand(value: unknown, command: string): boolean {
  const item = value as { request?: { command?: string; }; ok?: boolean; } | undefined;
  return item?.request?.command === command && item.ok === true;
}

function vectorObject(value: [number, number, number]): { x: number; y: number; z: number; } {
  return { x: value[0], y: value[1], z: value[2] };
}

function rotatorObject(value: [number, number, number]): { pitch: number; yaw: number; roll: number; } {
  return { pitch: value[0], yaw: value[1], roll: value[2] };
}
