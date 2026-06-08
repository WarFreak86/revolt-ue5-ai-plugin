import { app } from "electron";
import { existsSync, readFileSync } from "node:fs";
import { join, resolve } from "node:path";
import type { RuntimeConfig, RuntimeSettings } from "./types.js";
import { defaultRuntimeSettings } from "../shared/defaultState.js";

export const defaultSettings: RuntimeSettings = defaultRuntimeSettings;

export function loadRuntimeConfig(): RuntimeConfig {
  const candidates = [
    resolve(process.cwd(), "config", "runtime.json"),
    resolve(app.getAppPath(), "config", "runtime.json")
  ];

  for (const candidate of candidates) {
    if (!existsSync(candidate)) {
      continue;
    }

    try {
      const parsed = JSON.parse(readFileSync(candidate, "utf8")) as Partial<RuntimeConfig>;
      return { dangerousModeAvailable: parsed.dangerousModeAvailable === true };
    } catch {
      return { dangerousModeAvailable: false };
    }
  }

  return { dangerousModeAvailable: false };
}

export function getDatabasePath(): string {
  return join(app.getPath("userData"), "revolt-desktop.sqlite");
}

export function normalizeLocalHost(host: string): string {
  const normalized = host.trim().toLowerCase();
  if (normalized !== "127.0.0.1" && normalized !== "localhost") {
    throw new Error("UE bridge host must be 127.0.0.1 or localhost in offline mode.");
  }
  return normalized;
}
