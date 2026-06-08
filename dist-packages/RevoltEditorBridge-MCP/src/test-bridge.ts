#!/usr/bin/env node

import { loadConfig } from "./config.js";
import { UnrealBridgeClient } from "./ueBridgeClient.js";

const config = loadConfig();
const client = new UnrealBridgeClient(config);
const response = await client.command("ping");

process.stdout.write(`${JSON.stringify(response, null, 2)}\n`);

if (!response.ok) {
  process.exitCode = 1;
}
