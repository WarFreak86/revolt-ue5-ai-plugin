import type { DesktopAppState, DesktopConnectionState, IndexStats, RuntimeConfig, RuntimeSettings, StatusCard, UnrealConnectionState } from "../main/types.js";

export const DEFAULT_UNREAL_HOST = "127.0.0.1";
export const DEFAULT_UNREAL_PORT = 8765;
export const DEFAULT_UNREAL_TIMEOUT_MS = 5000;

export const defaultRuntimeSettings: RuntimeSettings = {
  ueBridgeHost: DEFAULT_UNREAL_HOST,
  ueBridgePort: DEFAULT_UNREAL_PORT,
  mcpServerCommand: "node",
  mcpServerPath: "../mcp-server/dist/index.js",
  defaultPermissionMode: "ReadOnly",
  enableOnlineProviders: false,
  offlineMode: true,
  experienceMode: "beginner",
  activeModelProfileId: "",
  projectIndexRoot: ""
};

export const defaultRuntimeConfig: RuntimeConfig = {
  dangerousModeAvailable: false
};

export function createDefaultUnrealStatus(): StatusCard {
  return {
    state: "unknown",
    label: "Not Connected",
    detail: "Open Unreal, then start the Revolt Bridge panel when you are ready."
  };
}

export function createDefaultMcpStatus(): StatusCard {
  return {
    state: "unknown",
    label: "MCP server not running",
    detail: "MCP is optional for the desktop UI."
  };
}

export function createDefaultOfflineStatus(): StatusCard {
  return {
    state: "online",
    label: "Offline mode enabled",
    detail: "Manual Command Mode works without Unreal, MCP, or a local model."
  };
}

export function createDefaultIndexStats(): IndexStats {
  return {
    isRunning: false,
    lastIndexTime: "",
    indexedFileCount: 0,
    indexedAssetCount: 0,
    indexedSceneCount: 0,
    currentRunStatus: "not_started",
    currentRunSummary: "No local index has been created yet."
  };
}

export function createDefaultUnrealConnection(settings: RuntimeSettings = defaultRuntimeSettings): UnrealConnectionState {
  const host = settings.ueBridgeHost || DEFAULT_UNREAL_HOST;
  const port = Number(settings.ueBridgePort) || DEFAULT_UNREAL_PORT;
  return {
    status: "disconnected",
    endpoint: `http://${host}:${port}`,
    host,
    port,
    timeoutMs: DEFAULT_UNREAL_TIMEOUT_MS,
    lastCheckedAt: null,
    lastSuccessAt: null,
    lastError: null,
    lastResponse: null
  };
}

export function createDefaultConnectionState(settings: RuntimeSettings = defaultRuntimeSettings): DesktopConnectionState {
  return {
    unreal: createDefaultUnrealConnection(settings),
    mcp: {
      status: "not_running",
      endpoint: settings.mcpServerPath || null,
      lastCheckedAt: null,
      lastError: null
    },
    model: {
      status: "not_configured",
      endpoint: null,
      activeProfile: null,
      lastCheckedAt: null,
      lastError: null
    }
  };
}

export function mergeConnectionState(input: Partial<DesktopConnectionState> | null | undefined, settings: RuntimeSettings = defaultRuntimeSettings): DesktopConnectionState {
  const defaults = createDefaultConnectionState(settings);
  const unreal = input?.unreal ?? {};
  const mcp = input?.mcp ?? {};
  const model = input?.model ?? {};

  return {
    unreal: {
      ...defaults.unreal,
      ...unreal
    },
    mcp: {
      ...defaults.mcp,
      ...mcp
    },
    model: {
      ...defaults.model,
      ...model
    }
  };
}

export function createDefaultDesktopAppState(overrides: Partial<DesktopAppState> = {}): DesktopAppState {
  const settings = { ...defaultRuntimeSettings, ...(overrides.settings ?? {}) };
  const modelProfiles = Array.isArray(overrides.modelProfiles) ? overrides.modelProfiles : [];
  const activeModelProfile = overrides.activeModelProfile ?? modelProfiles.find((profile) => profile.id === settings.activeModelProfileId) ?? null;

  return {
    settings,
    runtimeConfig: overrides.runtimeConfig ?? defaultRuntimeConfig,
    unrealStatus: overrides.unrealStatus ?? createDefaultUnrealStatus(),
    mcpStatus: overrides.mcpStatus ?? createDefaultMcpStatus(),
    offlineStatus: overrides.offlineStatus ?? createDefaultOfflineStatus(),
    history: Array.isArray(overrides.history) ? overrides.history : [],
    approvals: Array.isArray(overrides.approvals) ? overrides.approvals : [],
    modelProfiles,
    activeModelProfile,
    indexStats: overrides.indexStats ?? createDefaultIndexStats(),
    latestPrototypePlan: overrides.latestPrototypePlan ?? null,
    playableWorkflow: overrides.playableWorkflow ?? null,
    recipes: Array.isArray(overrides.recipes) ? overrides.recipes : [],
    recentRecipeRuns: Array.isArray(overrides.recentRecipeRuns) ? overrides.recentRecipeRuns : [],
    connectionState: mergeConnectionState(overrides.connectionState, settings),
    currentProjectSummary: overrides.currentProjectSummary ?? null,
    auditResults: overrides.auditResults ?? null,
    tutorials: Array.isArray(overrides.tutorials) ? overrides.tutorials : [
      "docs/setup/offline-quickstart.md",
      "docs/setup/unreal-plugin-setup.md",
      "docs/setup/local-model-setup.md",
      "docs/guides/manual-command-mode.md",
      "docs/troubleshooting.md"
    ],
    errorMessages: Array.isArray(overrides.errorMessages) ? overrides.errorMessages : []
  };
}

export function mergeDesktopAppState(input: Partial<DesktopAppState> | null | undefined): DesktopAppState {
  return createDefaultDesktopAppState(input ?? {});
}
