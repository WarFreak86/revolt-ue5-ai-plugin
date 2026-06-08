import type { PlayableAnswer, PlayableFixItem, PlayableLevelSummary, PlayableLevelWorkflowResult } from "./types.js";

export function buildPlayableLevelWorkflow(openLevelSummary: unknown, auditResult: unknown): PlayableLevelWorkflowResult {
  const audit = asRecord(auditResult);
  const openLevel = asRecord(openLevelSummary);
  const mapName = String(audit.map_name ?? openLevel.current_map_name ?? "Unknown level");
  const hasAiActors = readBool(audit.has_likely_ai_actor);
  const hasNavMesh = readBool(audit.has_navmesh_bounds_volume);
  const hasPlayerStart = readBool(audit.has_player_start);
  const hasPlayablePawn = readBool(audit.has_playable_pawn_or_character);
  const hasLighting = readBool(audit.has_basic_lighting);
  const hasObjective = readBool(audit.has_objective);
  const hasGameMode = readBool(audit.has_game_mode);
  const hasCamera = readBool(audit.has_camera_setup);

  const summary: PlayableLevelSummary = {
    playerCanSpawn: hasPlayerStart ? "Yes" : "No",
    playerCanMove: hasPlayablePawn || hasGameMode ? "Yes" : "Unknown",
    levelHasLighting: hasLighting ? "Yes" : "No",
    enemiesCanMove: hasAiActors ? (hasNavMesh ? "Yes" : "No") : "Unknown",
    objectiveExists: hasObjective ? "Yes" : "No"
  };

  return {
    checkedAt: new Date().toISOString(),
    ok: true,
    mapName,
    readOnly: true,
    summary,
    fixPlan: buildFixPlan(summary, { hasAiActors, hasGameMode, hasCamera, hasPlayablePawn }),
    technicalDetails: { openLevelSummary, auditResult },
    message: "Read-only level check complete. No project changes were made."
  };
}

function buildFixPlan(summary: PlayableLevelSummary, context: { hasAiActors: boolean; hasGameMode: boolean; hasCamera: boolean; hasPlayablePawn: boolean }): PlayableFixItem[] {
  const fixes: PlayableFixItem[] = [];
  if (summary.playerCanSpawn === "No") {
    fixes.push({
      id: "add_player_spawn",
      label: "Add player spawn point",
      beginnerExplanation: "Adds a safe starting point so the player has somewhere to appear when the level begins.",
      technicalCommand: "spawn_actor",
      params: dryRunParams("spawn_actor", { class: "PlayerStart", location: vectorObject([0, 0, 120]), rotation: rotatorObject([0, 0, 0]), permission_level: "editor_mutation" }),
      riskLevel: "Low",
      modifiesGeneratedAssetsOnly: false,
      dryRunAvailable: true,
      selected: true
    });
  }

  if (!context.hasPlayablePawn) {
    fixes.push({
      id: "add_basic_player_character",
      label: "Add basic player character",
      beginnerExplanation: "Creates a generated Character Blueprint that can become the player's controllable character later.",
      technicalCommand: "create_blueprint_class",
      params: dryRunParams("create_blueprint_class", {
        asset_path: "/Game/RevoltGenerated/Blueprints/BP_RevoltPlayableCharacter",
        parent_class: "Character",
        permission_level: "blueprint_mutation"
      }),
      riskLevel: "Low",
      modifiesGeneratedAssetsOnly: true,
      dryRunAvailable: true,
      selected: true
    });
  }

  if (!context.hasGameMode) {
    fixes.push({
      id: "add_default_game_mode",
      label: "Add default GameMode",
      beginnerExplanation: "A GameMode tells Unreal which player rules and pawn to use. Automatic world-setting edits are not supported yet, so this is a guided manual step for now.",
      technicalCommand: "manual_review",
      params: { reason: "World/project settings edits are intentionally not automated in Beginner Mode yet." },
      riskLevel: "Medium",
      modifiesGeneratedAssetsOnly: false,
      dryRunAvailable: false,
      selected: false
    });
  }

  if (summary.levelHasLighting === "No") {
    fixes.push({
      id: "add_basic_lighting",
      label: "Add basic lighting",
      beginnerExplanation: "Adds one generated light so the level is visible during playtesting.",
      technicalCommand: "spawn_actor",
      params: dryRunParams("spawn_actor", { class: "PointLight", location: vectorObject([0, 0, 300]), rotation: rotatorObject([0, 0, 0]), permission_level: "editor_mutation" }),
      riskLevel: "Low",
      modifiesGeneratedAssetsOnly: false,
      dryRunAvailable: true,
      selected: true
    });
  }

  if (context.hasAiActors && summary.enemiesCanMove === "No") {
    fixes.push({
      id: "add_enemy_walking_area",
      label: "Add enemy walking area",
      beginnerExplanation: "Adds a NavMeshBoundsVolume so AI enemies have a navigation area to walk on.",
      technicalCommand: "spawn_actor",
      params: dryRunParams("spawn_actor", { class: "NavMeshBoundsVolume", location: vectorObject([0, 0, 0]), scale: vectorObject([10, 10, 2]), permission_level: "editor_mutation" }),
      riskLevel: "Low",
      modifiesGeneratedAssetsOnly: false,
      dryRunAvailable: true,
      selected: true
    });
  }

  if (summary.objectiveExists === "No") {
    fixes.push({
      id: "add_simple_objective_marker",
      label: "Add simple objective marker",
      beginnerExplanation: "Adds a generated marker actor that can stand in for a goal while you prototype.",
      technicalCommand: "spawn_actor",
      params: dryRunParams("spawn_actor", { class: "TargetPoint", location: vectorObject([400, 0, 120]), rotation: rotatorObject([0, 0, 0]), permission_level: "editor_mutation" }),
      riskLevel: "Low",
      modifiesGeneratedAssetsOnly: false,
      dryRunAvailable: true,
      selected: true
    });
  }

  if (!context.hasCamera && !context.hasPlayablePawn) {
    fixes.push({
      id: "add_camera_setup_note",
      label: "Review camera/player setup",
      beginnerExplanation: "No obvious camera or player character was detected. Create the basic player character first, then test the camera in Play mode.",
      technicalCommand: "manual_review",
      params: { reason: "Camera setup depends on the chosen player pawn and is not automated in this workflow." },
      riskLevel: "Low",
      modifiesGeneratedAssetsOnly: true,
      dryRunAvailable: false,
      selected: false
    });
  }

  return fixes;
}

function dryRunParams(command: string, params: Record<string, unknown>): Record<string, unknown> {
  return { ...params, dry_run: true, approved: false, source_workflow: "make_level_playable", command };
}

function readBool(value: unknown): boolean {
  return value === true;
}

function asRecord(value: unknown): Record<string, unknown> {
  return value && typeof value === "object" ? (value as Record<string, unknown>) : {};
}

function vectorObject(value: [number, number, number]): { x: number; y: number; z: number } {
  return { x: value[0], y: value[1], z: value[2] };
}

function rotatorObject(value: [number, number, number]): { pitch: number; yaw: number; roll: number } {
  return { pitch: value[0], yaw: value[1], roll: value[2] };
}
