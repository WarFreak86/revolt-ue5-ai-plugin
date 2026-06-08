export type LogLevel = "debug" | "info" | "warn" | "error";

export interface ServerConfig {
  ueBridgeHost: string;
  ueBridgePort: number;
  requestTimeoutMs: number;
  logLevel: LogLevel;
}

function readIntEnv(name: string, fallback: number): number {
  const value = process.env[name];
  if (!value) {
    return fallback;
  }

  const parsed = Number.parseInt(value, 10);
  if (!Number.isFinite(parsed) || parsed <= 0) {
    throw new Error(`${name} must be a positive integer.`);
  }

  return parsed;
}

function readLogLevel(): LogLevel {
  const value = process.env.REVOLT_LOG_LEVEL ?? "info";
  if (value === "debug" || value === "info" || value === "warn" || value === "error") {
    return value;
  }

  throw new Error("REVOLT_LOG_LEVEL must be debug, info, warn, or error.");
}

export function loadConfig(): ServerConfig {
  const ueBridgeHost = process.env.REVOLT_UE_BRIDGE_HOST ?? "127.0.0.1";
  if (ueBridgeHost !== "127.0.0.1" && ueBridgeHost !== "localhost") {
    throw new Error("REVOLT_UE_BRIDGE_HOST must be 127.0.0.1 or localhost in Phase 6.");
  }

  return {
    ueBridgeHost,
    ueBridgePort: readIntEnv("REVOLT_UE_BRIDGE_PORT", 8765),
    requestTimeoutMs: readIntEnv("REVOLT_UE_BRIDGE_TIMEOUT_MS", 5000),
    logLevel: readLogLevel()
  };
}
