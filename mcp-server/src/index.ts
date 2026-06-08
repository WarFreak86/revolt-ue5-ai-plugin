#!/usr/bin/env node

import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import { z } from "zod";
import { loadConfig } from "./config.js";
import { Logger } from "./logger.js";
import { bridgeResponseToMcp } from "./mcpResponse.js";
import {
  compileBlueprintSchema,
  createBlueprintClassSchema,
  emptySchema,
  findAssetsSchema,
  setActorPropertySchema,
  setActorTransformSchema,
  spawnActorSchema
} from "./schemas.js";
import {
  mapCompileBlueprint,
  mapCreateBlueprintClass,
  mapFindAssets,
  mapSetActorProperty,
  mapSetActorTransform,
  mapSpawnActor
} from "./toolMapping.js";
import { UnrealBridgeClient } from "./ueBridgeClient.js";

const config = loadConfig();
const logger = new Logger(config);
const bridgeClient = new UnrealBridgeClient(config);

const server = new McpServer({
  name: "revolt-editor-bridge",
  version: "0.1.0"
});

function registerTool<Input extends z.ZodRawShape>(
  name: string,
  description: string,
  schema: z.ZodObject<Input>,
  handler: (input: z.infer<z.ZodObject<Input>>) => Promise<ReturnType<typeof bridgeResponseToMcp>>
): void {
  const callback = async (input: z.infer<z.ZodObject<Input>>) => {
    const parsed = schema.parse(input);
    return handler(parsed);
  };
  server.registerTool(name, { description, inputSchema: schema.shape }, callback as never);
}

registerTool("unreal_ping", "Ping the local Unreal bridge.", emptySchema, async () => {
  return bridgeResponseToMcp(await bridgeClient.command("ping"));
});

registerTool("unreal_get_project_summary", "Get a read-only Unreal project summary.", emptySchema, async () => {
  return bridgeResponseToMcp(await bridgeClient.command("get_project_summary"));
});

registerTool("unreal_get_open_level_summary", "Get a read-only summary of the open editor level.", emptySchema, async () => {
  return bridgeResponseToMcp(await bridgeClient.command("get_open_level_summary"));
});

registerTool("unreal_get_selected_actors", "Get read-only details for selected actors.", emptySchema, async () => {
  return bridgeResponseToMcp(await bridgeClient.command("get_selected_actors"));
});

registerTool("unreal_find_assets", "Find Unreal assets using read-only filters.", findAssetsSchema, async (input) => {
  return bridgeResponseToMcp(await bridgeClient.command("find_assets", mapFindAssets(input)));
});

registerTool("unreal_set_actor_transform", "Preview or apply a safe actor transform mutation.", setActorTransformSchema, async (input) => {
  return bridgeResponseToMcp(await bridgeClient.command("set_actor_transform", mapSetActorTransform(input)));
});

registerTool("unreal_set_actor_property", "Preview or apply a safe actor property mutation.", setActorPropertySchema, async (input) => {
  return bridgeResponseToMcp(await bridgeClient.command("set_actor_property", mapSetActorProperty(input)));
});

registerTool("unreal_spawn_actor", "Preview or apply a safe actor spawn mutation.", spawnActorSchema, async (input) => {
  return bridgeResponseToMcp(await bridgeClient.command("spawn_actor", mapSpawnActor(input)));
});

registerTool("unreal_create_blueprint_class", "Preview or create a generated Blueprint class.", createBlueprintClassSchema, async (input) => {
  return bridgeResponseToMcp(await bridgeClient.command("create_blueprint_class", mapCreateBlueprintClass(input)));
});

registerTool("unreal_compile_blueprint", "Preview or compile a Blueprint.", compileBlueprintSchema, async (input) => {
  return bridgeResponseToMcp(await bridgeClient.command("compile_blueprint", mapCompileBlueprint(input)));
});

const transport = new StdioServerTransport();
await server.connect(transport);
logger.info(`MCP server started for Unreal bridge ${config.ueBridgeHost}:${config.ueBridgePort}`);
