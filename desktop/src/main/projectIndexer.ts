import { createHash } from "node:crypto";
import { readdir, readFile, stat } from "node:fs/promises";
import { basename, extname, join, relative, resolve } from "node:path";
import { sendUnrealBridgeCommand } from "./bridgeClient.js";
import type { RuntimeDatabase } from "./database.js";
import { createModelProvider } from "./modelProviders.js";
import type { CompletionRequest, IndexStats, ModelProfile, RuntimeSettings } from "./types.js";

interface IndexCounts {
  filesIndexed: number;
  assetsIndexed: number;
  scenesIndexed: number;
}

const IGNORED_DIRECTORIES = new Set(["binaries", "deriveddatacache", "intermediate", "saved", ".git", ".vs", "node_modules"]);
const INDEXED_EXTENSIONS = new Set([".cpp", ".h", ".cs", ".uproject", ".uplugin", ".ini", ".json", ".md", ".py", ".ts", ".tsx"]);
const MAX_FILE_BYTES = 1024 * 1024;
const CHUNK_SIZE = 8000;

export class ProjectIndexer {
  private running = false;
  private stopRequested = false;

  constructor(private readonly database: RuntimeDatabase) { }

  isRunning(): boolean {
    return this.running;
  }

  requestStop(): void {
    this.stopRequested = true;
  }

  async start(projectRoot: string, settings: RuntimeSettings, activeProfile: ModelProfile | undefined): Promise<IndexStats> {
    if (this.running) {
      throw new Error("Project indexing is already running.");
    }

    const normalizedRoot = resolve(projectRoot);
    this.running = true;
    this.stopRequested = false;
    const run = this.database.startIndexRun(normalizedRoot);
    const counts: IndexCounts = { filesIndexed: 0, assetsIndexed: 0, scenesIndexed: 0 };

    try {
      await this.scanDirectory(normalizedRoot, normalizedRoot, run.id, activeProfile, counts);

      if (!this.stopRequested) {
        await this.indexUnrealContext(run.id, settings, activeProfile, counts);
      }

      const status = this.stopRequested ? "stopped" : "complete";
      const summary =
        status === "stopped"
          ? `Indexing stopped after ${counts.filesIndexed} files.`
          : `Indexed ${counts.filesIndexed} files, ${counts.assetsIndexed} assets, and ${counts.scenesIndexed} scenes locally.`;
      this.database.finishIndexRun(run.id, status, counts.filesIndexed, counts.assetsIndexed, counts.scenesIndexed, summary);
    } catch (error) {
      const message = error instanceof Error ? error.message : "Unknown indexing error.";
      this.database.finishIndexRun(run.id, "failed", counts.filesIndexed, counts.assetsIndexed, counts.scenesIndexed, "Indexing failed.", message);
      throw error;
    } finally {
      this.running = false;
    }

    return this.database.getIndexStats(this.running);
  }

  private async scanDirectory(root: string, directory: string, runId: string, activeProfile: ModelProfile | undefined, counts: IndexCounts): Promise<void> {
    if (this.stopRequested) {
      return;
    }

    const entries = await readdir(directory, { withFileTypes: true });
    for (const entry of entries) {
      if (this.stopRequested) {
        return;
      }

      const fullPath = join(directory, entry.name);
      if (entry.isDirectory()) {
        if (!IGNORED_DIRECTORIES.has(entry.name.toLowerCase())) {
          await this.scanDirectory(root, fullPath, runId, activeProfile, counts);
        }
        continue;
      }

      if (!entry.isFile() || !INDEXED_EXTENSIONS.has(extname(entry.name).toLowerCase())) {
        continue;
      }

      await this.indexFile(root, fullPath, runId, activeProfile);
      counts.filesIndexed += 1;

      if (counts.filesIndexed % 20 === 0) {
        this.database.updateIndexRunProgress(runId, counts.filesIndexed, counts.assetsIndexed, counts.scenesIndexed, `Indexed ${counts.filesIndexed} files locally.`);
        await new Promise((resolveTick) => setTimeout(resolveTick, 0));
      }
    }
  }

  private async indexFile(root: string, filePath: string, runId: string, activeProfile: ModelProfile | undefined): Promise<void> {
    const fileStat = await stat(filePath);
    if (fileStat.size > MAX_FILE_BYTES) {
      return;
    }

    const content = await readFile(filePath, "utf8");
    const chunks = chunkText(content);
    const summary = activeProfile ? await summarizeText(activeProfile, `Summarize this project file: ${relative(root, filePath)}`, content.slice(0, CHUNK_SIZE)) : "";
    const chunkRecords = [];

    for (let index = 0; index < chunks.length; index += 1) {
      const chunk = chunks[index];
      const chunkSummary =
        activeProfile && index === 0 ? await summarizeText(activeProfile, `Summarize this file chunk: ${relative(root, filePath)}`, chunk) : "";
      chunkRecords.push({
        chunkIndex: index,
        content: chunk,
        summary: chunkSummary,
        tokenEstimate: Math.ceil(chunk.length / 4)
      });
    }

    this.database.addIndexedFile(
      {
        runId,
        projectRoot: root,
        filePath,
        relativePath: relative(root, filePath),
        extension: extname(filePath).toLowerCase(),
        sizeBytes: fileStat.size,
        modifiedAt: fileStat.mtime.toISOString(),
        sha256: createHash("sha256").update(content).digest("hex"),
        summary
      },
      chunkRecords
    );
  }

  private async indexUnrealContext(runId: string, settings: RuntimeSettings, activeProfile: ModelProfile | undefined, counts: IndexCounts): Promise<void> {
    try {
      const assets = await sendUnrealBridgeCommand(settings, "find_assets", {
        path_filter: "/Game",
        max_results: 500
      });
      const assetRows = Array.isArray(assets) ? assets : extractArrayProperty(assets);
      for (const asset of assetRows) {
        const metadata = asset as Record<string, unknown>;
        const assetPath = String(metadata.path ?? metadata.object_path ?? metadata.package_name ?? "");
        const assetName = String(metadata.name ?? basename(assetPath) ?? "UnknownAsset");
        const assetClass = String(metadata.class ?? metadata.asset_class ?? metadata.type ?? "Unknown");
        const summary = activeProfile ? await summarizeText(activeProfile, `Summarize this Unreal asset metadata: ${assetName}`, JSON.stringify(metadata, null, 2)) : "";
        this.database.addAssetSummary(runId, assetPath, assetName, assetClass, metadata, summary);
        counts.assetsIndexed += 1;
      }
    } catch {
      this.database.updateIndexRunProgress(runId, counts.filesIndexed, counts.assetsIndexed, counts.scenesIndexed, "File index complete. Unreal asset metadata unavailable.");
    }

    try {
      const scene = await sendUnrealBridgeCommand(settings, "get_open_level_summary");
      const sceneName = String((scene as Record<string, unknown>).map_name ?? (scene as Record<string, unknown>).current_map_name ?? "OpenLevel");
      const summary = activeProfile ? await summarizeText(activeProfile, `Summarize this Unreal scene metadata: ${sceneName}`, JSON.stringify(scene, null, 2)) : "";
      this.database.addSceneSummary(runId, sceneName, scene, summary);
      counts.scenesIndexed += 1;
    } catch {
      this.database.updateIndexRunProgress(runId, counts.filesIndexed, counts.assetsIndexed, counts.scenesIndexed, "File index complete. Unreal scene metadata unavailable.");
    }
  }
}

function chunkText(content: string): string[] {
  const chunks: string[] = [];
  for (let index = 0; index < content.length; index += CHUNK_SIZE) {
    chunks.push(content.slice(index, index + CHUNK_SIZE));
  }
  return chunks.length > 0 ? chunks : [""];
}

async function summarizeText(profile: ModelProfile, label: string, text: string): Promise<string> {
  const provider = createModelProvider(profile);
  const request: CompletionRequest = {
    model: profile.modelName,
    temperature: Math.min(profile.temperature, 0.3),
    maxOutputTokens: Math.min(profile.maxOutputTokens, 160),
    systemPrompt: "You create concise local-only index summaries. Do not invent facts.",
    prompt: `${label}\n\n${text}`
  };
  const response = await provider.complete(request);
  return response.text.trim();
}

function extractArrayProperty(value: unknown): unknown[] {
  if (!value || typeof value !== "object") {
    return [];
  }

  for (const property of Object.values(value as Record<string, unknown>)) {
    if (Array.isArray(property)) {
      return property;
    }
  }

  return [];
}
