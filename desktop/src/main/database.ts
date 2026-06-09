import initSqlJs, { type Database, type SqlJsStatic } from "sql.js";
import { existsSync, mkdirSync, readFileSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { randomUUID } from "node:crypto";
import { app } from "electron";
import { defaultSettings, getDatabasePath, normalizeLocalHost } from "./config.js";
import type {
  ApprovalEvent,
  CommandHistoryEntry,
  IndexedFileRecord,
  IndexRunRecord,
  IndexStats,
  ModelProfile,
  ModelProfileInput,
  ModelProviderType,
  CommandHistoryExportEntry,
  GamePrototypePlan,
  GamePrototypeWizardInput,
  RecipeCommandPlanStep,
  RecipeResultPanel,
  RecipeRunRecord,
  RuntimeConfig,
  RuntimeSettings
} from "./types.js";

export class RuntimeDatabase {
  private sqlite?: SqlJsStatic;
  private database?: Database;
  private readonly databasePath: string;

  constructor(private readonly runtimeConfig: RuntimeConfig) {
    this.databasePath = getDatabasePath();
  }

  async initialize(): Promise<void> {
    this.sqlite = await initSqlJs({
      locateFile: (fileName: string) => join(process.cwd(), "node_modules", "sql.js", "dist", fileName)
    });

    mkdirSync(dirname(this.databasePath), { recursive: true });
    const bytes = existsSync(this.databasePath) ? readFileSync(this.databasePath) : undefined;
    this.database = bytes ? new this.sqlite.Database(bytes) : new this.sqlite.Database();
    this.migrate();
    this.seedDefaults();
    this.save();
  }

  getSettings(): RuntimeSettings {
    const settings = { ...defaultSettings };
    const rows = this.query<{ key: string; value_json: string; }>("SELECT key, value_json FROM settings");
    for (const row of rows) {
      if (row.key in settings) {
        try {
          (settings as Record<string, unknown>)[row.key] = JSON.parse(row.value_json);
        } catch {
          // Keep the safe default if a persisted setting was partially written or corrupted.
        }
      }
    }
    return settings;
  }

  updateSettings(nextSettings: Partial<RuntimeSettings>): RuntimeSettings {
    const mergedSettings: RuntimeSettings = { ...defaultSettings, ...this.getSettings(), ...nextSettings };
    if (!this.runtimeConfig.dangerousModeAvailable && mergedSettings.defaultPermissionMode === "Dangerous") {
      throw new Error("Dangerous permission mode is disabled by config/runtime.json.");
    }

    const normalized: RuntimeSettings = {
      ...mergedSettings,
      ueBridgeHost: normalizeLocalHost(mergedSettings.ueBridgeHost),
      ueBridgePort: Number(mergedSettings.ueBridgePort),
      enableOnlineProviders: mergedSettings.enableOnlineProviders === true,
      offlineMode: mergedSettings.offlineMode !== false,
      experienceMode: mergedSettings.experienceMode === "advanced" ? "advanced" : "beginner",
      activeModelProfileId: typeof mergedSettings.activeModelProfileId === "string" ? mergedSettings.activeModelProfileId : "",
      projectIndexRoot: typeof mergedSettings.projectIndexRoot === "string" ? mergedSettings.projectIndexRoot : ""
    };

    if (!Number.isInteger(normalized.ueBridgePort) || normalized.ueBridgePort < 1 || normalized.ueBridgePort > 65535) {
      throw new Error("UE bridge port must be between 1 and 65535.");
    }

    for (const [key, value] of Object.entries(normalized)) {
      this.execute("INSERT OR REPLACE INTO settings (key, value_json, updated_at) VALUES (?, ?, ?)", [
        key,
        JSON.stringify(value),
        new Date().toISOString()
      ]);
    }

    this.save();
    return this.getSettings();
  }

  addHistory(commandName: string, target: string, status: string, resultSummary: string, payload: unknown = {}): void {
    this.execute(
      "INSERT INTO command_history (id, timestamp, command_name, target, status, result_summary, payload_json) VALUES (?, ?, ?, ?, ?, ?, ?)",
      [randomUUID(), new Date().toISOString(), commandName, target, status, resultSummary, JSON.stringify(payload)]
    );
    this.save();
  }

  listHistory(): CommandHistoryEntry[] {
    return this.query<{
      id: string;
      timestamp: string;
      command_name: string;
      target: string;
      status: string;
      result_summary: string;
    }>("SELECT id, timestamp, command_name, target, status, result_summary FROM command_history ORDER BY timestamp DESC LIMIT 200").map((row) => ({
      id: row.id,
      timestamp: row.timestamp,
      commandName: row.command_name,
      target: row.target,
      status: row.status,
      resultSummary: row.result_summary
    }));
  }

  listHistoryForExport(): CommandHistoryExportEntry[] {
    return this.query<{
      id: string;
      timestamp: string;
      command_name: string;
      target: string;
      status: string;
      result_summary: string;
      payload_json: string;
    }>("SELECT id, timestamp, command_name, target, status, result_summary, payload_json FROM command_history ORDER BY timestamp DESC").map((row) => ({
      id: row.id,
      timestamp: row.timestamp,
      commandName: row.command_name,
      target: row.target,
      status: row.status,
      resultSummary: row.result_summary,
      payload: JSON.parse(row.payload_json || "{}")
    }));
  }

  clearHistory(): void {
    this.execute("DELETE FROM command_history");
    this.save();
  }

  listApprovalEvents(): ApprovalEvent[] {
    return this.query<{
      id: string;
      timestamp: string;
      command_id: string;
      command_name: string;
      target: string;
      decision: string;
    }>("SELECT id, timestamp, command_id, command_name, target, decision FROM approval_events ORDER BY timestamp DESC LIMIT 200").map((row) => ({
      id: row.id,
      timestamp: row.timestamp,
      commandId: row.command_id,
      commandName: row.command_name,
      target: row.target,
      decision: row.decision
    }));
  }

  listModelProfiles(): ModelProfile[] {
    return this.query<{
      id: string;
      provider_type: string;
      display_name: string;
      base_url: string;
      model_name: string;
      context_length: number;
      temperature: number;
      max_output_tokens: number;
      classification: string;
      supports_streaming: number;
      supports_tools: number;
      enabled: number;
      last_tested_at: string;
      last_test_status: string;
      created_at: string;
      updated_at: string;
    }>(
      "SELECT id, provider_type, display_name, base_url, model_name, context_length, temperature, " +
      "max_output_tokens, classification, supports_streaming, supports_tools, enabled, " +
      "last_tested_at, last_test_status, created_at, updated_at FROM model_profiles " +
      "ORDER BY display_name ASC"
    ).map((row) => ({
      id: row.id,
      providerType: row.provider_type as ModelProfile["providerType"],
      displayName: row.display_name,
      baseUrl: row.base_url,
      modelName: row.model_name,
      contextLength: row.context_length,
      temperature: row.temperature,
      maxOutputTokens: row.max_output_tokens,
      classification: row.classification as ModelProfile["classification"],
      supportsStreaming: row.supports_streaming === 1,
      supportsTools: row.supports_tools === 1,
      enabled: row.enabled !== 0,
      lastTestedAt: row.last_tested_at ?? "",
      lastTestStatus: row.last_test_status ?? "Not tested",
      createdAt: row.created_at,
      updatedAt: row.updated_at
    }));
  }

  getModelProfile(id: string): ModelProfile | undefined {
    return this.listModelProfiles().find((profile) => profile.id === id);
  }

  saveModelProfile(input: ModelProfileInput): ModelProfile {
    this.validateModelProfile(input);
    const id = input.id?.trim() || randomUUID();
    const existing = this.getModelProfile(id);
    const now = new Date().toISOString();
    const providerCapabilities = defaultCapabilitiesForProvider(input.providerType);
    this.execute(
      `INSERT OR REPLACE INTO model_profiles
      (id, name, provider_type, display_name, base_url, model_name, context_length, temperature, max_output_tokens, classification, supports_streaming, supports_tools, enabled, config_json, created_at, updated_at)
      VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`,
      [
        id,
        input.displayName.trim(),
        input.providerType,
        input.displayName.trim(),
        this.normalizeLocalBaseUrl(input.baseUrl),
        input.modelName.trim(),
        Number(input.contextLength),
        Number(input.temperature),
        Number(input.maxOutputTokens),
        input.classification,
        (input.supportsStreaming ?? providerCapabilities.supportsStreaming) ? 1 : 0,
        (input.supportsTools ?? providerCapabilities.supportsTools) ? 1 : 0,
        input.enabled === false ? 0 : 1,
        JSON.stringify({ phase: 8, setupWizard: true }),
        existing?.createdAt ?? now,
        now
      ]
    );
    this.save();
    return this.getModelProfile(id)!;
  }

  duplicateModelProfile(id: string): ModelProfile {
    const profile = this.getModelProfile(id);
    if (!profile) {
      throw new Error("Model profile does not exist.");
    }

    return this.saveModelProfile({
      providerType: profile.providerType,
      displayName: `${profile.displayName} Copy`,
      baseUrl: profile.baseUrl,
      modelName: profile.modelName,
      contextLength: profile.contextLength,
      temperature: profile.temperature,
      maxOutputTokens: profile.maxOutputTokens,
      classification: profile.classification,
      supportsStreaming: profile.supportsStreaming,
      supportsTools: profile.supportsTools,
      enabled: profile.enabled
    });
  }

  setModelProfileEnabled(id: string, enabled: boolean): ModelProfile {
    if (!this.getModelProfile(id)) {
      throw new Error("Model profile does not exist.");
    }
    this.execute("UPDATE model_profiles SET enabled = ?, updated_at = ? WHERE id = ?", [enabled ? 1 : 0, new Date().toISOString(), id]);
    if (!enabled) {
      const settings = this.getSettings();
      if (settings.activeModelProfileId === id) {
        this.updateSettings({ ...settings, activeModelProfileId: "" });
      } else {
        this.save();
      }
    } else {
      this.save();
    }
    return this.getModelProfile(id)!;
  }

  updateModelProfileTestStatus(id: string, status: string): ModelProfile | undefined {
    if (!this.getModelProfile(id)) {
      return undefined;
    }
    this.execute("UPDATE model_profiles SET last_tested_at = ?, last_test_status = ?, updated_at = ? WHERE id = ?", [
      new Date().toISOString(),
      status,
      new Date().toISOString(),
      id
    ]);
    this.save();
    return this.getModelProfile(id);
  }

  deleteModelProfile(id: string): void {
    this.execute("DELETE FROM model_profiles WHERE id = ?", [id]);
    const settings = this.getSettings();
    if (settings.activeModelProfileId === id) {
      this.updateSettings({ ...settings, activeModelProfileId: "" });
    } else {
      this.save();
    }
  }

  startIndexRun(projectRoot: string): IndexRunRecord {
    const id = randomUUID();
    const now = new Date().toISOString();
    this.execute(
      `INSERT INTO index_runs
      (id, project_root, started_at, finished_at, status, files_indexed, assets_indexed, scenes_indexed, summary, error_message)
      VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`,
      [id, projectRoot, now, "", "running", 0, 0, 0, "Indexing started.", ""]
    );
    this.save();
    return this.getIndexRun(id)!;
  }

  finishIndexRun(id: string, status: string, filesIndexed: number, assetsIndexed: number, scenesIndexed: number, summary: string, errorMessage = ""): void {
    this.execute(
      `UPDATE index_runs
      SET finished_at = ?, status = ?, files_indexed = ?, assets_indexed = ?, scenes_indexed = ?, summary = ?, error_message = ?
      WHERE id = ?`,
      [new Date().toISOString(), status, filesIndexed, assetsIndexed, scenesIndexed, summary, errorMessage, id]
    );
    this.save();
  }

  updateIndexRunProgress(id: string, filesIndexed: number, assetsIndexed: number, scenesIndexed: number, summary: string): void {
    this.execute(
      "UPDATE index_runs SET files_indexed = ?, assets_indexed = ?, scenes_indexed = ?, summary = ? WHERE id = ?",
      [filesIndexed, assetsIndexed, scenesIndexed, summary, id]
    );
    this.save();
  }

  addIndexedFile(input: Omit<IndexedFileRecord, "id" | "indexedAt">, chunks: Array<{ chunkIndex: number; content: string; summary: string; tokenEstimate: number; }>): void {
    const fileId = randomUUID();
    const indexedAt = new Date().toISOString();
    this.execute(
      `INSERT INTO indexed_files
      (id, run_id, project_root, file_path, relative_path, extension, size_bytes, modified_at, sha256, summary, indexed_at)
      VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`,
      [
        fileId,
        input.runId,
        input.projectRoot,
        input.filePath,
        input.relativePath,
        input.extension,
        input.sizeBytes,
        input.modifiedAt,
        input.sha256,
        input.summary,
        indexedAt
      ]
    );

    for (const chunk of chunks) {
      this.execute(
        `INSERT INTO file_chunks
        (id, file_id, run_id, chunk_index, content, summary, token_estimate, indexed_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)`,
        [randomUUID(), fileId, input.runId, chunk.chunkIndex, chunk.content, chunk.summary, chunk.tokenEstimate, indexedAt]
      );
    }
  }

  addAssetSummary(runId: string, assetPath: string, assetName: string, assetClass: string, metadata: unknown, summary: string): void {
    this.execute(
      `INSERT INTO asset_summaries
      (id, run_id, asset_path, asset_name, asset_class, metadata_json, summary, indexed_at)
      VALUES (?, ?, ?, ?, ?, ?, ?, ?)`,
      [randomUUID(), runId, assetPath, assetName, assetClass, JSON.stringify(metadata), summary, new Date().toISOString()]
    );
  }

  addSceneSummary(runId: string, sceneName: string, metadata: unknown, summary: string): void {
    this.execute(
      `INSERT INTO scene_summaries
      (id, run_id, scene_name, metadata_json, summary, indexed_at)
      VALUES (?, ?, ?, ?, ?, ?)`,
      [randomUUID(), runId, sceneName, JSON.stringify(metadata), summary, new Date().toISOString()]
    );
  }

  getIndexStats(isRunning = false): IndexStats {
    const fileCount = this.query<{ count: number; }>("SELECT COUNT(*) AS count FROM indexed_files")[0]?.count ?? 0;
    const assetCount = this.query<{ count: number; }>("SELECT COUNT(*) AS count FROM asset_summaries")[0]?.count ?? 0;
    const sceneCount = this.query<{ count: number; }>("SELECT COUNT(*) AS count FROM scene_summaries")[0]?.count ?? 0;
    const latestRun = this.query<{
      finished_at: string;
      started_at: string;
      status: string;
      summary: string;
    }>("SELECT finished_at, started_at, status, summary FROM index_runs ORDER BY started_at DESC LIMIT 1")[0];

    return {
      isRunning,
      lastIndexTime: latestRun?.finished_at || latestRun?.started_at || "",
      indexedFileCount: fileCount,
      indexedAssetCount: assetCount,
      indexedSceneCount: sceneCount,
      currentRunStatus: latestRun?.status ?? "not_started",
      currentRunSummary: latestRun?.summary ?? "No local index has been created yet."
    };
  }

  clearLocalIndex(): void {
    this.execute("DELETE FROM file_chunks");
    this.execute("DELETE FROM indexed_files");
    this.execute("DELETE FROM asset_summaries");
    this.execute("DELETE FROM scene_summaries");
    this.execute("DELETE FROM index_runs");
    this.save();
  }

  savePrototypeWizardPlan(input: GamePrototypeWizardInput, plan: GamePrototypePlan): GamePrototypePlan {
    this.execute(
      "INSERT INTO prototype_wizard_plans (id, created_at, input_json, plan_json) VALUES (?, ?, ?, ?)",
      [plan.id, plan.createdAt, JSON.stringify(input), JSON.stringify(plan)]
    );
    this.save();
    return plan;
  }

  getLatestPrototypeWizardPlan(): GamePrototypePlan | undefined {
    const row = this.query<{ plan_json: string; }>("SELECT plan_json FROM prototype_wizard_plans ORDER BY created_at DESC LIMIT 1")[0];
    return row ? (JSON.parse(row.plan_json) as GamePrototypePlan) : undefined;
  }

  addRecipeRun(input: {
    recipeId: string;
    recipeTitle: string;
    status: string;
    dryRun: boolean;
    approved: boolean;
    summary: string;
    commandPlan: RecipeCommandPlanStep[];
    response: unknown;
    resultPanel?: RecipeResultPanel;
  }): RecipeRunRecord {
    const record: RecipeRunRecord = {
      id: randomUUID(),
      recipeId: input.recipeId,
      recipeTitle: input.recipeTitle,
      timestamp: new Date().toISOString(),
      status: input.status,
      dryRun: input.dryRun,
      approved: input.approved,
      summary: input.summary,
      commandPlan: input.commandPlan,
      response: input.response,
      resultPanel: input.resultPanel
    };
    this.execute(
      `INSERT INTO recipe_runs
      (id, recipe_id, recipe_title, timestamp, status, dry_run, approved, summary, command_plan_json, response_json, result_panel_json)
      VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`,
      [
        record.id,
        record.recipeId,
        record.recipeTitle,
        record.timestamp,
        record.status,
        record.dryRun ? 1 : 0,
        record.approved ? 1 : 0,
        record.summary,
        JSON.stringify(record.commandPlan),
        JSON.stringify(record.response),
        JSON.stringify(record.resultPanel ?? null)
      ]
    );
    this.save();
    return record;
  }

  listRecentRecipeRuns(): RecipeRunRecord[] {
    return this.query<{
      id: string;
      recipe_id: string;
      recipe_title: string;
      timestamp: string;
      status: string;
      dry_run: number;
      approved: number;
      summary: string;
      command_plan_json: string;
      response_json: string;
      result_panel_json: string;
    }>(
      "SELECT id, recipe_id, recipe_title, timestamp, status, dry_run, approved, summary, command_plan_json, response_json, result_panel_json FROM recipe_runs ORDER BY timestamp DESC LIMIT 50"
    ).map((row) => ({
      id: row.id,
      recipeId: row.recipe_id,
      recipeTitle: row.recipe_title,
      timestamp: row.timestamp,
      status: row.status,
      dryRun: row.dry_run === 1,
      approved: row.approved === 1,
      summary: row.summary,
      commandPlan: JSON.parse(row.command_plan_json || "[]") as RecipeCommandPlanStep[],
      response: JSON.parse(row.response_json || "{}"),
      resultPanel: JSON.parse(row.result_panel_json || "null") as RecipeResultPanel | undefined
    }));
  }

  private migrate(): void {
    this.execute(`
      CREATE TABLE IF NOT EXISTS projects (
        id TEXT PRIMARY KEY,
        name TEXT NOT NULL,
        path TEXT NOT NULL,
        created_at TEXT NOT NULL,
        updated_at TEXT NOT NULL
      );
    `);
    this.execute(`
      CREATE TABLE IF NOT EXISTS sessions (
        id TEXT PRIMARY KEY,
        project_id TEXT,
        started_at TEXT NOT NULL,
        ended_at TEXT,
        metadata_json TEXT NOT NULL
      );
    `);
    this.execute(`
      CREATE TABLE IF NOT EXISTS command_history (
        id TEXT PRIMARY KEY,
        timestamp TEXT NOT NULL,
        command_name TEXT NOT NULL,
        target TEXT NOT NULL,
        status TEXT NOT NULL,
        result_summary TEXT NOT NULL,
        payload_json TEXT NOT NULL
      );
    `);
    this.execute(`
      CREATE TABLE IF NOT EXISTS approval_events (
        id TEXT PRIMARY KEY,
        timestamp TEXT NOT NULL,
        command_id TEXT NOT NULL,
        command_name TEXT NOT NULL,
        target TEXT NOT NULL,
        decision TEXT NOT NULL,
        details_json TEXT NOT NULL
      );
    `);
    this.execute(`
      CREATE TABLE IF NOT EXISTS settings (
        key TEXT PRIMARY KEY,
        value_json TEXT NOT NULL,
        updated_at TEXT NOT NULL
      );
    `);
    this.execute(`
      CREATE TABLE IF NOT EXISTS prototype_wizard_plans (
        id TEXT PRIMARY KEY,
        created_at TEXT NOT NULL,
        input_json TEXT NOT NULL,
        plan_json TEXT NOT NULL
      );
    `);
    this.execute(`
      CREATE TABLE IF NOT EXISTS recipe_runs (
        id TEXT PRIMARY KEY,
        recipe_id TEXT NOT NULL,
        recipe_title TEXT NOT NULL,
        timestamp TEXT NOT NULL,
        status TEXT NOT NULL,
        dry_run INTEGER NOT NULL,
        approved INTEGER NOT NULL,
        summary TEXT NOT NULL,
        command_plan_json TEXT NOT NULL,
        response_json TEXT NOT NULL,
        result_panel_json TEXT
      );
    `);
    this.addColumnIfMissing("recipe_runs", "result_panel_json", "TEXT");
    this.execute(`
      CREATE TABLE IF NOT EXISTS model_profiles (
        id TEXT PRIMARY KEY,
        name TEXT,
        display_name TEXT,
        provider_type TEXT NOT NULL,
        base_url TEXT,
        model_name TEXT,
        context_length INTEGER DEFAULT 4096,
        temperature REAL DEFAULT 0.2,
        max_output_tokens INTEGER DEFAULT 512,
        classification TEXT DEFAULT 'offline',
        supports_streaming INTEGER NOT NULL DEFAULT 0,
        supports_tools INTEGER NOT NULL DEFAULT 0,
        last_tested_at TEXT,
        last_test_status TEXT,
        config_json TEXT NOT NULL,
        enabled INTEGER NOT NULL DEFAULT 0,
        created_at TEXT NOT NULL,
        updated_at TEXT
      );
    `);
    this.addColumnIfMissing("model_profiles", "display_name", "TEXT");
    this.addColumnIfMissing("model_profiles", "base_url", "TEXT");
    this.addColumnIfMissing("model_profiles", "model_name", "TEXT");
    this.addColumnIfMissing("model_profiles", "context_length", "INTEGER DEFAULT 4096");
    this.addColumnIfMissing("model_profiles", "temperature", "REAL DEFAULT 0.2");
    this.addColumnIfMissing("model_profiles", "max_output_tokens", "INTEGER DEFAULT 512");
    this.addColumnIfMissing("model_profiles", "classification", "TEXT DEFAULT 'offline'");
    this.addColumnIfMissing("model_profiles", "supports_streaming", "INTEGER NOT NULL DEFAULT 0");
    this.addColumnIfMissing("model_profiles", "supports_tools", "INTEGER NOT NULL DEFAULT 0");
    this.addColumnIfMissing("model_profiles", "last_tested_at", "TEXT");
    this.addColumnIfMissing("model_profiles", "last_test_status", "TEXT");
    this.addColumnIfMissing("model_profiles", "updated_at", "TEXT");
    this.execute(`
      CREATE TABLE IF NOT EXISTS index_runs (
        id TEXT PRIMARY KEY,
        project_root TEXT NOT NULL,
        started_at TEXT NOT NULL,
        finished_at TEXT,
        status TEXT NOT NULL,
        files_indexed INTEGER NOT NULL DEFAULT 0,
        assets_indexed INTEGER NOT NULL DEFAULT 0,
        scenes_indexed INTEGER NOT NULL DEFAULT 0,
        summary TEXT NOT NULL,
        error_message TEXT
      );
    `);
    this.execute(`
      CREATE TABLE IF NOT EXISTS indexed_files (
        id TEXT PRIMARY KEY,
        run_id TEXT NOT NULL,
        project_root TEXT NOT NULL,
        file_path TEXT NOT NULL,
        relative_path TEXT NOT NULL,
        extension TEXT NOT NULL,
        size_bytes INTEGER NOT NULL,
        modified_at TEXT NOT NULL,
        sha256 TEXT NOT NULL,
        summary TEXT NOT NULL,
        indexed_at TEXT NOT NULL
      );
    `);
    this.execute(`
      CREATE TABLE IF NOT EXISTS file_chunks (
        id TEXT PRIMARY KEY,
        file_id TEXT NOT NULL,
        run_id TEXT NOT NULL,
        chunk_index INTEGER NOT NULL,
        content TEXT NOT NULL,
        summary TEXT NOT NULL,
        token_estimate INTEGER NOT NULL,
        indexed_at TEXT NOT NULL
      );
    `);
    this.execute(`
      CREATE TABLE IF NOT EXISTS asset_summaries (
        id TEXT PRIMARY KEY,
        run_id TEXT NOT NULL,
        asset_path TEXT NOT NULL,
        asset_name TEXT NOT NULL,
        asset_class TEXT NOT NULL,
        metadata_json TEXT NOT NULL,
        summary TEXT NOT NULL,
        indexed_at TEXT NOT NULL
      );
    `);
    this.execute(`
      CREATE TABLE IF NOT EXISTS scene_summaries (
        id TEXT PRIMARY KEY,
        run_id TEXT NOT NULL,
        scene_name TEXT NOT NULL,
        metadata_json TEXT NOT NULL,
        summary TEXT NOT NULL,
        indexed_at TEXT NOT NULL
      );
    `);
  }

  private seedDefaults(): void {
    const count = this.query<{ count: number; }>("SELECT COUNT(*) AS count FROM settings")[0]?.count ?? 0;
    if (count > 0) {
      return;
    }

    const mcpPath = join(app.getAppPath(), "..", "mcp-server", "dist", "index.js");
    this.updateSettings({ ...defaultSettings, mcpServerPath: mcpPath });
  }

  private execute(sql: string, params: Array<string | number | null> = []): void {
    this.requireDatabase().run(sql, params);
  }

  private query<T>(sql: string, params: Array<string | number | null> = []): T[] {
    const statement = this.requireDatabase().prepare(sql, params);
    const rows: T[] = [];
    try {
      while (statement.step()) {
        rows.push(statement.getAsObject() as T);
      }
    } finally {
      statement.free();
    }
    return rows;
  }

  private getIndexRun(id: string): IndexRunRecord | undefined {
    return this.query<{
      id: string;
      project_root: string;
      started_at: string;
      finished_at: string;
      status: string;
      files_indexed: number;
      assets_indexed: number;
      scenes_indexed: number;
      summary: string;
      error_message: string;
    }>("SELECT id, project_root, started_at, finished_at, status, files_indexed, assets_indexed, scenes_indexed, summary, error_message FROM index_runs WHERE id = ?", [
      id
    ]).map((row) => ({
      id: row.id,
      projectRoot: row.project_root,
      startedAt: row.started_at,
      finishedAt: row.finished_at,
      status: row.status,
      filesIndexed: row.files_indexed,
      assetsIndexed: row.assets_indexed,
      scenesIndexed: row.scenes_indexed,
      summary: row.summary,
      errorMessage: row.error_message
    }))[0];
  }

  private addColumnIfMissing(tableName: string, columnName: string, definition: string): void {
    const columns = this.query<{ name: string; }>(`PRAGMA table_info(${tableName})`);
    if (!columns.some((column) => column.name === columnName)) {
      this.execute(`ALTER TABLE ${tableName} ADD COLUMN ${columnName} ${definition}`);
    }
  }

  private validateModelProfile(input: ModelProfileInput): void {
    const allowedProviderTypes = new Set(["ollama", "lmstudio", "llamacpp", "openai-compatible"]);
    if (!allowedProviderTypes.has(input.providerType)) {
      throw new Error("Unsupported local model provider type.");
    }
    if (input.classification === "online") {
      throw new Error("Online model providers are not available in this offline setup wizard phase.");
    }
    if (!input.displayName.trim()) {
      throw new Error("Model profile display name is required.");
    }
    if (!input.modelName.trim()) {
      throw new Error("Model name is required.");
    }
    if (!Number.isInteger(Number(input.contextLength)) || Number(input.contextLength) < 1) {
      throw new Error("Context length must be positive.");
    }
    if (!Number.isFinite(Number(input.temperature)) || Number(input.temperature) < 0 || Number(input.temperature) > 2) {
      throw new Error("Temperature must be between 0 and 2.");
    }
    if (!Number.isInteger(Number(input.maxOutputTokens)) || Number(input.maxOutputTokens) < 1) {
      throw new Error("Max output tokens must be positive.");
    }
    this.normalizeLocalBaseUrl(input.baseUrl);
  }

  private normalizeLocalBaseUrl(baseUrl: string): string {
    let parsed: URL;
    try {
      parsed = new URL(baseUrl.trim());
    } catch {
      throw new Error("Base URL must be a valid local HTTP URL.");
    }

    if (parsed.protocol !== "http:" && parsed.protocol !== "https:") {
      throw new Error("Base URL must use HTTP or HTTPS.");
    }

    if (!isAllowedLocalOrPrivateHost(parsed.hostname) && !this.getSettings().enableOnlineProviders) {
      throw new Error("External model URLs are blocked unless Enable Online Providers is explicitly enabled. Use localhost, 127.0.0.1, ::1, .local, or a private LAN address.");
    }

    return parsed.toString().replace(/\/$/, "");
  }

  private save(): void {
    const bytes = this.requireDatabase().export();
    writeFileSync(this.databasePath, Buffer.from(bytes));
  }

  private requireDatabase(): Database {
    if (!this.database) {
      throw new Error("Runtime database has not been initialized.");
    }
    return this.database;
  }
}

function defaultCapabilitiesForProvider(providerType: ModelProviderType): { supportsStreaming: boolean; supportsTools: boolean; } {
  return {
    supportsStreaming: providerType !== "openai-compatible",
    supportsTools: false
  };
}

function isAllowedLocalOrPrivateHost(hostname: string): boolean {
  const normalized = hostname.toLowerCase();
  if (normalized === "localhost" || normalized === "127.0.0.1" || normalized === "::1" || normalized.endsWith(".local")) {
    return true;
  }

  if (/^10\.\d{1,3}\.\d{1,3}\.\d{1,3}$/.test(normalized) || /^192\.168\.\d{1,3}\.\d{1,3}$/.test(normalized)) {
    return true;
  }

  const match = /^172\.(\d{1,2})\.\d{1,3}\.\d{1,3}$/.exec(normalized);
  return match ? Number(match[1]) >= 16 && Number(match[1]) <= 31 : false;
}
