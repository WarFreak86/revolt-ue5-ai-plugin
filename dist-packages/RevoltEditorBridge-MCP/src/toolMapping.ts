import type {
  CompileBlueprintInput,
  CreateBlueprintClassInput,
  FindAssetsInput,
  SetActorPropertyInput,
  SetActorTransformInput,
  SpawnActorInput
} from "./schemas.js";

function approvedDryRun(dryRun: boolean, approved: boolean): boolean {
  return approved ? false : dryRun;
}

export function mapFindAssets(input: FindAssetsInput): Record<string, unknown> {
  return {
    path: input.path,
    class_filter: input.classFilter,
    type_filter: input.typeFilter,
    name_substring: input.nameSubstring,
    max_results: input.maxResults
  };
}

export function mapSetActorTransform(input: SetActorTransformInput): Record<string, unknown> {
  return {
    permission_level: "editor_mutation",
    actor: input.actor,
    actor_path: input.actorPath,
    actor_name: input.actorName,
    location: input.location,
    rotation: input.rotation,
    scale: input.scale,
    dry_run: approvedDryRun(input.dryRun, input.approved),
    approved: input.approved
  };
}

export function mapSetActorProperty(input: SetActorPropertyInput): Record<string, unknown> {
  return {
    permission_level: "editor_mutation",
    actor: input.actor,
    actor_path: input.actorPath,
    actor_name: input.actorName,
    property: input.property,
    value: input.value,
    dry_run: approvedDryRun(input.dryRun, input.approved),
    approved: input.approved
  };
}

export function mapSpawnActor(input: SpawnActorInput): Record<string, unknown> {
  return {
    permission_level: "editor_mutation",
    class: input.className,
    location: input.location,
    rotation: input.rotation,
    scale: input.scale,
    dry_run: approvedDryRun(input.dryRun, input.approved),
    approved: input.approved
  };
}

export function mapCreateBlueprintClass(input: CreateBlueprintClassInput): Record<string, unknown> {
  return {
    permission_level: "blueprint_mutation",
    asset_path: input.assetPath,
    parent_class: input.parentClass,
    dry_run: approvedDryRun(input.dryRun, input.approved),
    approved: input.approved
  };
}

export function mapCompileBlueprint(input: CompileBlueprintInput): Record<string, unknown> {
  return {
    permission_level: "blueprint_mutation",
    blueprint: input.blueprint,
    dry_run: approvedDryRun(input.dryRun, input.approved),
    approved: input.approved
  };
}
