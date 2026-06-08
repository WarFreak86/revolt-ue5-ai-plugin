import { z } from "zod";

export const emptySchema = z.object({}).strict();

export const vectorSchema = z.object({
  x: z.number().finite(),
  y: z.number().finite(),
  z: z.number().finite()
}).strict();

export const rotationSchema = z.object({
  pitch: z.number().finite(),
  yaw: z.number().finite(),
  roll: z.number().finite()
}).strict();

export const findAssetsSchema = z.object({
  path: z.string().default("/Game"),
  classFilter: z.string().optional(),
  typeFilter: z.string().optional(),
  nameSubstring: z.string().optional(),
  maxResults: z.number().int().positive().max(500).default(50)
}).strict();

export const setActorTransformSchema = z.object({
  actor: z.string().optional(),
  actorPath: z.string().optional(),
  actorName: z.string().optional(),
  location: vectorSchema.optional(),
  rotation: rotationSchema.optional(),
  scale: vectorSchema.optional(),
  dryRun: z.boolean().default(true),
  approved: z.boolean().default(false)
}).strict();

export const setActorPropertySchema = z.object({
  actor: z.string().optional(),
  actorPath: z.string().optional(),
  actorName: z.string().optional(),
  property: z.string().min(1),
  value: z.union([z.string(), z.number(), z.boolean()]),
  dryRun: z.boolean().default(true),
  approved: z.boolean().default(false)
}).strict();

export const spawnActorSchema = z.object({
  className: z.string().min(1),
  location: vectorSchema.optional(),
  rotation: rotationSchema.optional(),
  scale: vectorSchema.optional(),
  dryRun: z.boolean().default(true),
  approved: z.boolean().default(false)
}).strict();

export const createBlueprintClassSchema = z.object({
  assetPath: z.string().min(1),
  parentClass: z.enum(["Actor", "Pawn", "Character", "ActorComponent", "SceneComponent", "UserWidget"]),
  dryRun: z.boolean().default(true),
  approved: z.boolean().default(false)
}).strict();

export const compileBlueprintSchema = z.object({
  blueprint: z.string().min(1),
  dryRun: z.boolean().default(true),
  approved: z.boolean().default(false)
}).strict();

export type EmptyInput = z.infer<typeof emptySchema>;
export type FindAssetsInput = z.infer<typeof findAssetsSchema>;
export type SetActorTransformInput = z.infer<typeof setActorTransformSchema>;
export type SetActorPropertyInput = z.infer<typeof setActorPropertySchema>;
export type SpawnActorInput = z.infer<typeof spawnActorSchema>;
export type CreateBlueprintClassInput = z.infer<typeof createBlueprintClassSchema>;
export type CompileBlueprintInput = z.infer<typeof compileBlueprintSchema>;
