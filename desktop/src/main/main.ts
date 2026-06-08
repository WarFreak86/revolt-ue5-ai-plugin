import { app, BrowserWindow, dialog, ipcMain } from "electron";
import { existsSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { checkUnrealBridge, sendUnrealBridgeCommand, sendUnrealBridgeManualCommand, testDefaultUnrealConnection } from "./bridgeClient.js";
import { loadRuntimeConfig } from "./config.js";
import { RuntimeDatabase } from "./database.js";
import { detectProviderEndpoint, testModelProfile, summarizeProjectWithModel } from "./modelProviders.js";
import { ProjectIndexer } from "./projectIndexer.js";
import { buildPlayableLevelWorkflow } from "./playableWorkflow.js";
import { createGamePrototypePlan } from "./prototypePlanner.js";
import { buildRecipe, buildRecipeResultPanel, recipeCatalog } from "./recipes.js";
import { createDefaultConnectionState, createDefaultDesktopAppState } from "../shared/defaultState.js";
import type { GamePrototypeWizardInput, ManualCommandRequest, ModelProfileInput, ModelProviderType, PlayableFixItem, RuntimeSettings, StatusCard } from "./types.js";

let mainWindow: BrowserWindow | undefined;
let database: RuntimeDatabase;
let projectIndexer: ProjectIndexer;
const mainDir = dirname(fileURLToPath(import.meta.url));
const distDir = dirname(mainDir);

const mutatingManualCommands = new Set([
  "create_biome_asset",
  "spawn_actor",
  "create_zombie_shooter_template",
  "create_blueprint_class",
  "add_blueprint_variable",
  "set_blueprint_default_value",
  "add_component_to_blueprint",
  "landgen_generate_preview",
  "landgen_spawn_biome_content",
  "landgen_clear_generated_content",
  "compile_blueprint",
  "save_generated_assets"
]);

async function createWindow(): Promise<void> {
  mainWindow = new BrowserWindow({
    width: 1180,
    height: 760,
    minWidth: 980,
    minHeight: 640,
    title: "Revolt Desktop Runtime",
    webPreferences: {
      preload: join(distDir, "preload.js"),
      contextIsolation: true,
      nodeIntegration: false
    }
  });

  mainWindow.webContents.on("did-fail-load", (_event, errorCode, errorDescription, validatedUrl) => {
    console.error(`[RevoltDesktop] Renderer failed to load ${validatedUrl}: ${errorCode} ${errorDescription}`);
  });
  mainWindow.webContents.on("render-process-gone", (_event, details) => {
    console.error("[RevoltDesktop] Renderer process exited unexpectedly.", details);
  });

  const devServerUrl = process.env.REVOLT_RENDERER_DEV_URL || process.env.VITE_DEV_SERVER_URL;
  try {
    if (!app.isPackaged && devServerUrl) {
      await mainWindow.loadURL(devServerUrl);
      return;
    }

    const rendererPath = join(distDir, "renderer", "index.html");
    if (!existsSync(rendererPath)) {
      throw new Error(`Built renderer was not found at ${rendererPath}. Run npm run build.`);
    }
    await mainWindow.loadFile(rendererPath);
  } catch (error) {
    const message = error instanceof Error ? error.message : "Unknown renderer loading error.";
    console.error("[RevoltDesktop] Unable to load renderer.", error);
    await mainWindow.loadURL(
      `data:text/html;charset=utf-8,${encodeURIComponent(`
        <html>
          <body style="font-family: sans-serif; background: #0d1117; color: #e7edf5; padding: 32px;">
            <h1>Revolt Runtime could not load the interface</h1>
            <p>${escapeHtmlForDataUrl(message)}</p>
            <p>Try running <code>npm run build</code> from the desktop folder, then launch again.</p>
          </body>
        </html>
      `)}`
    );
  }
}

function getMcpStatus(settings: RuntimeSettings): StatusCard {
  if (!settings.mcpServerPath.trim()) {
    return { state: "unknown", label: "MCP server path not configured", detail: "Set the path in Settings." };
  }

  if (existsSync(settings.mcpServerPath)) {
    return { state: "online", label: "MCP server build found", detail: settings.mcpServerPath };
  }

  return { state: "offline", label: "MCP server not built yet", detail: settings.mcpServerPath };
}

function registerIpc(): void {
  ipcMain.handle("runtime:getInitialState", async () => {
    const settings = database.getSettings();
    const modelProfiles = database.listModelProfiles();
    const activeModelProfile = modelProfiles.find((profile) => profile.id === settings.activeModelProfileId) ?? null;
    return createDefaultDesktopAppState({
      settings,
      runtimeConfig: loadRuntimeConfig(),
      unrealStatus: { state: "unknown", label: "Unreal bridge not checked yet", detail: "Use Check Connection. The app works without Unreal running." },
      mcpStatus: getMcpStatus(settings),
      history: database.listHistory(),
      approvals: database.listApprovalEvents(),
      modelProfiles,
      activeModelProfile,
      indexStats: database.getIndexStats(projectIndexer?.isRunning() ?? false),
      latestPrototypePlan: database.getLatestPrototypeWizardPlan() ?? null,
      playableWorkflow: null,
      recipes: recipeCatalog,
      recentRecipeRuns: database.listRecentRecipeRuns(),
      connectionState: createDefaultConnectionState(settings),
      offlineStatus: {
        state: settings.offlineMode ? "online" : "warning",
        label: settings.offlineMode ? "Offline mode enabled" : "Offline mode disabled",
        detail: settings.activeModelProfileId
          ? "Using selected local model profile."
          : "Manual Command Mode: no model profile selected."
      }
    });
  });

  ipcMain.handle("runtime:checkUnrealBridge", async () => {
    const settings = database.getSettings();
    const status = await checkUnrealBridge(settings);
    database.addHistory("ping", `${settings.ueBridgeHost}:${settings.ueBridgePort}`, status.state, status.label, status);
    return { status, history: database.listHistory() };
  });

  ipcMain.handle("runtime:testDefaultUnrealConnection", async () => {
    const settings = database.getSettings();
    const result = await testDefaultUnrealConnection(settings);
    database.addHistory("test_unreal_connection", result.endpoint, result.ok ? "connected" : result.status, result.message, result);
    return { result, history: database.listHistory() };
  });

  ipcMain.handle("runtime:getMcpStatus", () => getMcpStatus(database.getSettings()));
  ipcMain.handle("settings:get", () => database.getSettings());
  ipcMain.handle("settings:update", (_event, settings: RuntimeSettings) => database.updateSettings(settings));
  ipcMain.handle("history:list", () => database.listHistory());
  ipcMain.handle("history:clear", () => {
    database.clearHistory();
    return database.listHistory();
  });
  ipcMain.handle("history:export", async () => {
    const result = await dialog.showSaveDialog(mainWindow!, {
      title: "Export command history",
      defaultPath: `revolt-command-history-${new Date().toISOString().replace(/[:.]/g, "-")}.json`,
      filters: [{ name: "JSON", extensions: ["json"] }]
    });
    if (result.canceled || !result.filePath) {
      return { exported: false, path: "" };
    }

    writeFileSync(result.filePath, JSON.stringify(database.listHistoryForExport(), null, 2), "utf8");
    database.addHistory("export_command_history", result.filePath, "completed", "Command history exported locally.");
    return { exported: true, path: result.filePath, history: database.listHistory() };
  });
  ipcMain.handle("approvals:list", () => database.listApprovalEvents());
  ipcMain.handle("playable:checkLevel", async () => {
    const settings = database.getSettings();
    const openLevelSummary = await sendUnrealBridgeCommand(settings, "get_open_level_summary");
    const auditResult = await sendUnrealBridgeCommand(settings, "audit_current_level");
    const workflow = buildPlayableLevelWorkflow(openLevelSummary, auditResult);
    database.addHistory("make_level_playable_check", workflow.mapName, "read_only", workflow.message, workflow);
    return { workflow, history: database.listHistory() };
  });
  ipcMain.handle("playable:dryRunFixes", async (_event, fixIds: string[]) => {
    const settings = database.getSettings();
    const openLevelSummary = await sendUnrealBridgeCommand(settings, "get_open_level_summary");
    const auditResult = await sendUnrealBridgeCommand(settings, "audit_current_level");
    const workflow = buildPlayableLevelWorkflow(openLevelSummary, auditResult);
    const requestedIds = new Set(fixIds);
    const fixes = workflow.fixPlan.filter((fix) => requestedIds.has(fix.id) && fix.dryRunAvailable);
    if (fixes.length === 0) {
      throw new Error("No dry-run capable fixes were selected.");
    }

    const responses = [];
    for (const fix of fixes) {
      responses.push(await dryRunPlayableFix(settings, fix));
    }
    const ok = responses.every((response) => response.ok);
    const summary = ok ? `Dry-run generated for ${responses.length} playable-level fix(es).` : "One or more playable-level dry-runs returned an error.";
    database.addHistory("make_level_playable_dry_run", workflow.mapName, ok ? "dry_run" : "failed", summary, { fixes, responses });
    return { workflow, responses, history: database.listHistory() };
  });
  ipcMain.handle("prototype:createDryRunPlan", (_event, input: GamePrototypeWizardInput) => {
    const plan = database.savePrototypeWizardPlan(input, createGamePrototypePlan(input));
    database.addHistory("create_game_prototype_plan", plan.title, "planned", "Dry-run prototype plan created locally.", {
      planId: plan.id,
      doesExecuteChanges: plan.doesExecuteChanges
    });
    return { plan, history: database.listHistory() };
  });
  ipcMain.handle("prototype:getLatestPlan", () => database.getLatestPrototypeWizardPlan() ?? null);
  ipcMain.handle("prototype:sendToGuidedRecipe", (_event, planId: string) => {
    const plan = database.getLatestPrototypeWizardPlan();
    if (!plan || plan.id !== planId) {
      throw new Error("Create a dry-run plan before sending it to a guided recipe.");
    }
    database.addHistory("send_plan_to_guided_recipe", plan.title, "placeholder", "Guided Recipe handoff placeholder recorded locally.", {
      planId,
      doesExecuteChanges: false
    });
    return { message: "Guided Recipe is not implemented yet. Your plan was saved locally; no Unreal changes were made.", history: database.listHistory() };
  });
  ipcMain.handle("recipes:list", () => recipeCatalog);
  ipcMain.handle("recipes:preview", (_event, recipeId: string, values: Record<string, unknown>) => buildRecipe(recipeId, values));
  ipcMain.handle("recipes:runs", () => database.listRecentRecipeRuns());
  ipcMain.handle("recipes:runDryRun", async (_event, recipeId: string, values: Record<string, unknown>) => {
    const recipe = buildRecipe(recipeId, values);
    const supportedSteps = recipe.generatedCommandPlan.filter((step) => step.supported);
    if (supportedSteps.length === 0 || recipe.recipeId === "create_health_pickup") {
      const run = database.addRecipeRun({
        recipeId: recipe.recipeId,
        recipeTitle: recipe.title,
        status: "dry_run",
        dryRun: true,
        approved: false,
        summary:
          recipe.recipeId === "create_health_pickup"
            ? "Health Pickup dry-run plan generated locally. Approval is required before Blueprint assets are created."
            : "Recipe preview saved locally. Unsupported placeholder commands were not executed.",
        commandPlan: recipe.generatedCommandPlan,
        response: { ok: true, previewOnly: true },
        resultPanel: buildRecipeResultPanel(recipe, [], false)
      });
      database.addHistory(recipe.recipeId === "create_health_pickup" ? "recipe_dry_run" : "recipe_preview_only", recipe.title, run.status, run.summary, run);
      return { recipe, run, history: database.listHistory(), recentRecipeRuns: database.listRecentRecipeRuns() };
    }

    const settings = database.getSettings();
    const responses = [];
    for (const step of supportedSteps) {
      const params = {
        ...step.params,
        dry_run: true,
        approved: false,
        permission_level: step.params.permission_level ?? recipe.requiredPermissionLevel
      };
      responses.push(await sendUnrealBridgeManualCommand(settings, step.command, params));
    }
    const ok = responses.every((response) => response.ok);
    const summary = ok
      ? "Recipe dry-run sent through the local Unreal bridge approval workflow."
      : "Recipe dry-run reached the local Unreal bridge but returned an error.";
    const run = database.addRecipeRun({
      recipeId: recipe.recipeId,
      recipeTitle: recipe.title,
      status: ok ? "dry_run" : "failed",
      dryRun: true,
      approved: false,
      summary,
      commandPlan: recipe.generatedCommandPlan,
      response: responses,
      resultPanel: buildRecipeResultPanel(recipe, responses, false)
    });
    database.addHistory("recipe_dry_run", recipe.title, ok ? "dry_run" : "failed", summary, { run, responses });
    return { recipe, run, history: database.listHistory(), recentRecipeRuns: database.listRecentRecipeRuns() };
  });
  ipcMain.handle("recipes:applyApproved", async (_event, recipeId: string, values: Record<string, unknown>) => {
    const recipe = buildRecipe(recipeId, values);
    if (recipe.recipeId !== "create_health_pickup") {
      throw new Error("Only Create Health Pickup supports approved recipe application in this phase.");
    }

    const settings = database.getSettings();
    const responses = [];
    for (const step of recipe.generatedCommandPlan.filter((item) => item.supported)) {
      const params = {
        ...step.params,
        dry_run: false,
        approved: true,
        permission_level: step.params.permission_level ?? recipe.requiredPermissionLevel
      };
      responses.push(await sendUnrealBridgeManualCommand(settings, step.command, params));
      if (!responses[responses.length - 1]?.ok) {
        break;
      }
    }

    const ok = responses.every((response) => response.ok);
    const summary = ok
      ? "Health Pickup recipe applied after approval. Generated Blueprint asset structure was created."
      : "Health Pickup recipe stopped after a bridge error. Review the response details.";
    const resultPanel = buildRecipeResultPanel(recipe, responses, true);
    const run = database.addRecipeRun({
      recipeId: recipe.recipeId,
      recipeTitle: recipe.title,
      status: ok ? "applied" : "failed",
      dryRun: false,
      approved: true,
      summary,
      commandPlan: recipe.generatedCommandPlan,
      response: responses,
      resultPanel
    });
    database.addHistory("recipe_apply_approved", recipe.title, ok ? "applied" : "failed", summary, { run, responses });
    return { recipe, run, history: database.listHistory(), recentRecipeRuns: database.listRecentRecipeRuns() };
  });
  ipcMain.handle("manual:runCommand", async (_event, request: ManualCommandRequest) => {
    const command = request.command.trim();
    if (!command) {
      throw new Error("Select a manual command first.");
    }

    const params = { ...(request.params ?? {}) };
    if (mutatingManualCommands.has(command)) {
      params.permission_level = typeof params.permission_level === "string" ? params.permission_level : defaultPermissionForManualCommand(command);
      params.dry_run = params.approved === true ? params.dry_run === true : true;
      params.approved = params.approved === true;
    }

    const settings = database.getSettings();
    const result = await sendUnrealBridgeManualCommand(settings, command, params);
    database.addHistory(command, manualCommandTarget(command, params), result.ok ? "completed" : "failed", result.summary, result);
    return { ...result, history: database.listHistory() };
  });
  ipcMain.handle("models:list", () => database.listModelProfiles());
  ipcMain.handle("models:save", (_event, input: ModelProfileInput) => database.saveModelProfile(input));
  ipcMain.handle("models:detect", (_event, providerType: ModelProviderType, baseUrl: string) => detectProviderEndpoint(providerType, baseUrl));
  ipcMain.handle("models:delete", (_event, id: string) => {
    database.deleteModelProfile(id);
    return { settings: database.getSettings(), modelProfiles: database.listModelProfiles() };
  });
  ipcMain.handle("models:duplicate", (_event, id: string) => {
    const profile = database.duplicateModelProfile(id);
    return { profile, modelProfiles: database.listModelProfiles() };
  });
  ipcMain.handle("models:setEnabled", (_event, id: string, enabled: boolean) => {
    database.setModelProfileEnabled(id, enabled);
    return { settings: database.getSettings(), modelProfiles: database.listModelProfiles() };
  });
  ipcMain.handle("models:setActive", (_event, id: string) => {
    const settings = database.getSettings();
    const profile = id ? database.getModelProfile(id) : undefined;
    if (id && !profile) {
      throw new Error("Selected model profile does not exist.");
    }
    if (profile && !profile.enabled) {
      throw new Error("Enable this profile before setting it active.");
    }
    return database.updateSettings({ ...settings, activeModelProfileId: id });
  });
  ipcMain.handle("models:test", async (_event, id: string) => {
    const profile = database.getModelProfile(id);
    if (!profile) {
      throw new Error("Model profile does not exist.");
    }
    const result = await testModelProfile(profile);
    database.updateModelProfileTestStatus(id, result.status);
    database.addHistory("test_model_connection", profile.displayName, result.ok ? "success" : "failed", result.message, result);
    return { result, history: database.listHistory(), modelProfiles: database.listModelProfiles() };
  });
  ipcMain.handle("models:summarizeProject", async () => {
    const settings = database.getSettings();
    if (!settings.activeModelProfileId) {
      throw new Error("Manual Command Mode is active. Select a local model profile before summarizing.");
    }
    const profile = database.getModelProfile(settings.activeModelProfileId);
    if (!profile) {
      throw new Error("Active model profile was not found.");
    }

    const projectSummary = await sendUnrealBridgeCommand(settings, "get_project_summary");
    const completion = await summarizeProjectWithModel(profile, projectSummary);
    const resultSummary = completion.text.trim().slice(0, 180) || "Local model returned an empty summary.";
    database.addHistory("summarize_project", profile.displayName, "completed", resultSummary, {
      provider: profile.providerType,
      classification: profile.classification
    });
    return { completion, history: database.listHistory() };
  });
  ipcMain.handle("index:chooseRoot", async () => {
    const result = await dialog.showOpenDialog(mainWindow!, {
      title: "Select project folder to index locally",
      properties: ["openDirectory"]
    });
    return result.canceled ? "" : result.filePaths[0] ?? "";
  });
  ipcMain.handle("index:stats", () => database.getIndexStats(projectIndexer.isRunning()));
  ipcMain.handle("index:start", (_event, projectRoot: string) => {
    if (!projectRoot.trim()) {
      throw new Error("Select a project folder before indexing.");
    }
    if (projectIndexer.isRunning()) {
      throw new Error("Project indexing is already running.");
    }

    const settings = database.updateSettings({ ...database.getSettings(), projectIndexRoot: projectRoot });
    const profile = settings.activeModelProfileId ? database.getModelProfile(settings.activeModelProfileId) : undefined;
    database.addHistory("start_project_index", projectRoot, "running", profile ? "Local indexing started with selected local model summaries." : "Local metadata-only indexing started.");
    void projectIndexer
      .start(projectRoot, settings, profile)
      .then((stats) => {
        database.addHistory("project_index", projectRoot, stats.currentRunStatus, stats.currentRunSummary);
      })
      .catch((error) => {
        const message = error instanceof Error ? error.message : "Project indexing failed.";
        database.addHistory("project_index", projectRoot, "failed", message);
      });
    return database.getIndexStats(true);
  });
  ipcMain.handle("index:stop", () => {
    projectIndexer.requestStop();
    return database.getIndexStats(projectIndexer.isRunning());
  });
  ipcMain.handle("index:clear", () => {
    if (projectIndexer.isRunning()) {
      throw new Error("Stop indexing before clearing the local index.");
    }
    database.clearLocalIndex();
    database.addHistory("clear_local_index", "local_index", "completed", "Local project index cleared.");
    return { stats: database.getIndexStats(false), history: database.listHistory() };
  });
}

function dryRunPlayableFix(settings: RuntimeSettings, fix: PlayableFixItem) {
  return sendUnrealBridgeManualCommand(settings, fix.technicalCommand, {
    ...fix.params,
    dry_run: true,
    approved: false
  });
}

function defaultPermissionForManualCommand(command: string): string {
  if (command === "create_biome_asset" || command === "save_generated_assets") {
    return "asset_mutation";
  }
  if (command === "compile_blueprint") {
    return "blueprint_mutation";
  }
  return "editor_mutation";
}

function manualCommandTarget(command: string, params: Record<string, unknown>): string {
  for (const key of ["asset_path", "blueprint_path", "actor", "actor_path", "path", "name"]) {
    const value = params[key];
    if (typeof value === "string" && value.trim()) {
      return value;
    }
  }
  return command;
}

function escapeHtmlForDataUrl(value: string): string {
  return value.replace(/[&<>"']/g, (character) => {
    switch (character) {
      case "&":
        return "&amp;";
      case "<":
        return "&lt;";
      case ">":
        return "&gt;";
      case "\"":
        return "&quot;";
      default:
        return "&#039;";
    }
  });
}

app.whenReady().then(async () => {
  database = new RuntimeDatabase(loadRuntimeConfig());
  await database.initialize();
  projectIndexer = new ProjectIndexer(database);
  registerIpc();
  await createWindow();
});

app.on("window-all-closed", () => {
  if (process.platform !== "darwin") {
    app.quit();
  }
});

app.on("activate", async () => {
  if (BrowserWindow.getAllWindows().length === 0) {
    await createWindow();
  }
});
