import type {
  ApprovalEvent,
  CommandHistoryEntry,
  DesktopAppState,
  GamePrototypePlan,
  GamePrototypeWizardInput,
  IndexStats,
  ManualCommandRequest,
  ManualCommandResult,
  ModelProfile,
  ModelProfileInput,
  ModelProviderType,
  PlayableFixItem,
  PlayableLevelWorkflowResult,
  ProviderDetectionResult,
  RecipeDefinition,
  RecipeRunRecord,
  RuntimeConfig,
  RuntimeSettings,
  StatusCard
} from "../main/types.js";
import { createDefaultDesktopAppState, mergeDesktopAppState } from "../shared/defaultState.js";

declare global {
  interface Window {
    revoltRuntime: {
      getInitialState: () => Promise<DesktopAppState>;
      checkUnrealBridge: () => Promise<{ status: StatusCard; history: CommandHistoryEntry[]; }>;
      testDefaultUnrealConnection: () => Promise<{ result: { ok: boolean; status: string; endpoint: string; message: string; detail: string; checkedAt: string; response: unknown; }; history: CommandHistoryEntry[]; }>;
      getMcpStatus: () => Promise<StatusCard>;
      getSettings: () => Promise<RuntimeSettings>;
      updateSettings: (settings: RuntimeSettings) => Promise<RuntimeSettings>;
      listHistory: () => Promise<CommandHistoryEntry[]>;
      clearHistory: () => Promise<CommandHistoryEntry[]>;
      exportHistory: () => Promise<{ exported: boolean; path: string; history?: CommandHistoryEntry[]; }>;
      listApprovals: () => Promise<ApprovalEvent[]>;
      checkPlayableLevel: () => Promise<{ workflow: PlayableLevelWorkflowResult; history: CommandHistoryEntry[]; }>;
      dryRunPlayableFixes: (fixIds: string[]) => Promise<{ workflow: PlayableLevelWorkflowResult; responses: unknown[]; history: CommandHistoryEntry[]; }>;
      createPrototypeDryRunPlan: (input: GamePrototypeWizardInput) => Promise<{ plan: GamePrototypePlan; history: CommandHistoryEntry[]; }>;
      getLatestPrototypePlan: () => Promise<GamePrototypePlan | null>;
      sendPrototypePlanToGuidedRecipe: (planId: string) => Promise<{ message: string; history: CommandHistoryEntry[]; }>;
      listRecipes: () => Promise<RecipeDefinition[]>;
      previewRecipe: (recipeId: string, values: Record<string, unknown>) => Promise<RecipeDefinition>;
      runRecipeDryRun: (
        recipeId: string,
        values: Record<string, unknown>
      ) => Promise<{ recipe: RecipeDefinition; run: RecipeRunRecord; history: CommandHistoryEntry[]; recentRecipeRuns: RecipeRunRecord[]; }>;
      applyApprovedRecipe: (
        recipeId: string,
        values: Record<string, unknown>
      ) => Promise<{ recipe: RecipeDefinition; run: RecipeRunRecord; history: CommandHistoryEntry[]; recentRecipeRuns: RecipeRunRecord[]; }>;
      listRecipeRuns: () => Promise<RecipeRunRecord[]>;
      runManualCommand: (request: ManualCommandRequest) => Promise<ManualCommandResult & { history: CommandHistoryEntry[]; }>;
      listModelProfiles: () => Promise<ModelProfile[]>;
      saveModelProfile: (input: ModelProfileInput) => Promise<ModelProfile>;
      deleteModelProfile: (id: string) => Promise<{ settings: RuntimeSettings; modelProfiles: ModelProfile[]; }>;
      setActiveModelProfile: (id: string) => Promise<RuntimeSettings>;
      detectModelProvider: (providerType: ModelProviderType, baseUrl: string) => Promise<ProviderDetectionResult>;
      duplicateModelProfile: (id: string) => Promise<{ profile: ModelProfile; modelProfiles: ModelProfile[]; }>;
      setModelProfileEnabled: (id: string, enabled: boolean) => Promise<{ settings: RuntimeSettings; modelProfiles: ModelProfile[]; }>;
      testModelProfile: (id: string) => Promise<{ result: { ok: boolean; message: string; status: string; responseText?: string; }; history: CommandHistoryEntry[]; modelProfiles: ModelProfile[]; }>;
      summarizeProjectWithActiveModel: () => Promise<{ completion: { text: string; }; history: CommandHistoryEntry[]; }>;
      chooseIndexRoot: () => Promise<string>;
      getIndexStats: () => Promise<IndexStats>;
      startIndexing: (projectRoot: string) => Promise<IndexStats>;
      stopIndexing: () => Promise<IndexStats>;
      clearLocalIndex: () => Promise<{ stats: IndexStats; history: CommandHistoryEntry[]; }>;
    };
  }
}

let state: DesktopAppState = createDefaultDesktopAppState();
let selectedRecipeId = "";
let selectedPlayableFixId = "";
let connectionStepIndex = 0;
let beginnerWorkflowPlan: BeginnerWorkflowPlan | null = null;

interface BeginnerWorkflowPlan {
  id: string;
  intent: string;
  title: string;
  beginnerSummary: string;
  command: string;
  dryRunParams: Record<string, unknown>;
  approvedParams: Record<string, unknown>;
  plannedChanges: string[];
  riskLevel: "Low" | "Medium" | "High" | "Dangerous";
  permissionLevel: string;
  status: "planned" | "dry_run_ready" | "dry_run_failed" | "applying" | "applied" | "failed";
  dryRunResult: ManualCommandResult | null;
  executionResult: ManualCommandResult | null;
  nextSteps: string[];
}

installRendererErrorBoundary();

const pages = document.querySelectorAll<HTMLElement>(".page");
const navButtons = document.querySelectorAll<HTMLButtonElement>(".nav");

for (const button of Array.from(navButtons)) {
  button.addEventListener("click", () => {
    const pageId = button.dataset.page ?? "dashboard";
    showPage(pageId);
  });
}

for (const id of ["experienceModeHeader", "experienceMode"]) {
  requireElement(id).addEventListener("change", async () => {
    await saveExperienceMode(getInputValue(id) as RuntimeSettings["experienceMode"]);
  });
}

function showPage(pageId: string): void {
  navButtons.forEach((item) => item.classList.toggle("active", item.dataset.page === pageId));
  pages.forEach((page) => page.classList.toggle("active", page.id === pageId));
}

async function saveExperienceMode(experienceMode: RuntimeSettings["experienceMode"]): Promise<void> {
  try {
    state.settings = await window.revoltRuntime.updateSettings({ ...state.settings, experienceMode });
  } catch (error) {
    state.settings = { ...state.settings, experienceMode };
    state.errorMessages = [...state.errorMessages, errorMessage(error, "Experience mode saved locally for this session only.")];
  }
  state.offlineStatus = getOfflineStatus();
  applyExperienceMode();
  renderStatusCard("card-offline", state.offlineStatus);
}

function applyExperienceMode(): void {
  const isBeginner = state.settings.experienceMode !== "advanced";
  setInputValue("experienceModeHeader", isBeginner ? "beginner" : "advanced");
  setInputValue("experienceMode", isBeginner ? "beginner" : "advanced");
  requireElement("mode-label").textContent = isBeginner ? "Beginner Mode" : "Advanced Mode";

  for (const element of Array.from(document.querySelectorAll<HTMLElement>("[data-mode-scope='advanced']"))) {
    const hidden = isBeginner;
    element.hidden = hidden;
    if (hidden && element.classList.contains("active")) {
      showPage("beginner-home");
    }
  }

  if (isBeginner && !Array.from(pages).some((page) => page.classList.contains("active"))) {
    showPage("beginner-home");
  }
}

function renderStatusCard(elementId: string, card: StatusCard): void {
  const element = requireElement(elementId);
  const safeCard = card ?? { state: "unknown" as const, label: "Status unavailable", detail: "This optional service is not connected." };
  element.innerHTML = `
    <span class="status-dot ${safeCard.state}"></span>
    <h3>${escapeHtml(safeCard.label)}</h3>
    <p>${escapeHtml(safeCard.detail)}</p>
  `;
}

function renderDetails(elementId: string, card: StatusCard): void {
  const element = requireElement(elementId);
  const safeCard = card ?? { state: "unknown" as const, label: "Status unavailable", detail: "This optional service is not connected." };
  element.innerHTML = `
    <div class="card">
      <span class="status-dot ${safeCard.state}"></span>
      <h3>${escapeHtml(safeCard.label)}</h3>
      <p>${escapeHtml(safeCard.detail)}</p>
    </div>
  `;
}

function renderSettings(settings: RuntimeSettings, runtimeConfig: RuntimeConfig): void {
  const safeSettings = mergeDesktopAppState({ settings }).settings;
  const safeConfig = runtimeConfig ?? { dangerousModeAvailable: false };
  setInputValue("ueBridgeHost", safeSettings.ueBridgeHost);
  setInputValue("ueBridgePort", String(safeSettings.ueBridgePort));
  setInputValue("mcpServerCommand", safeSettings.mcpServerCommand);
  setInputValue("mcpServerPath", safeSettings.mcpServerPath);
  setInputValue("defaultPermissionMode", safeSettings.defaultPermissionMode);
  setChecked("enableOnlineProviders", safeSettings.enableOnlineProviders);
  setChecked("offlineMode", safeSettings.offlineMode);
  setInputValue("experienceMode", safeSettings.experienceMode ?? "beginner");
  setInputValue("experienceModeHeader", safeSettings.experienceMode ?? "beginner");
  setInputValue("projectIndexRoot", safeSettings.projectIndexRoot);

  const dangerousOption = requireElement("dangerous-option") as HTMLOptionElement;
  dangerousOption.disabled = !safeConfig.dangerousModeAvailable;
  dangerousOption.textContent = safeConfig.dangerousModeAvailable ? "Dangerous" : "Dangerous (disabled by config)";
}

function renderHistory(entries: CommandHistoryEntry[]): void {
  const safeEntries = safeArray(entries);
  const body = requireElement("history-body");
  body.innerHTML = safeEntries.length
    ? safeEntries
      .map(
        (entry) => `
            <tr>
              <td>${escapeHtml(formatDate(entry.timestamp))}</td>
              <td>${escapeHtml(entry.commandName)}</td>
              <td>${escapeHtml(entry.target)}</td>
              <td>${escapeHtml(entry.status)}</td>
              <td>${escapeHtml(entry.resultSummary)}</td>
            </tr>
          `
      )
      .join("")
    : `<tr><td colspan="5">No command history yet.</td></tr>`;
}

function renderApprovals(entries: ApprovalEvent[]): void {
  const safeEntries = safeArray(entries);
  const body = requireElement("approvals-body");
  body.innerHTML = safeEntries.length
    ? safeEntries
      .map(
        (entry) => `
            <tr>
              <td>${escapeHtml(formatDate(entry.timestamp))}</td>
              <td>${escapeHtml(entry.commandName)}</td>
              <td>${escapeHtml(entry.target)}</td>
              <td>${escapeHtml(entry.decision)}</td>
            </tr>
          `
      )
      .join("")
    : `<tr><td colspan="4">No approval events recorded yet.</td></tr>`;
}

function renderAll(): void {
  state = mergeDesktopAppState(state);
  renderStatusCard("card-unreal", state.unrealStatus);
  renderStatusCard("card-mcp", state.mcpStatus);
  renderStatusCard("card-offline", state.offlineStatus);
  renderStatusCard("card-model", getModelStatusCard());
  renderStatusCard("card-model-detail", getModelDetailStatusCard());
  renderStatusCard("card-index", getIndexStatusCard());
  renderStatusCard("card-setup-checklist", getSetupChecklistStatusCard());
  renderSetupChecklistAction();
  renderBeginnerDashboard();
  renderDetails("unreal-details", state.unrealStatus);
  renderDetails("mcp-details", state.mcpStatus);
  renderSettings(state.settings, state.runtimeConfig);
  applyExperienceMode();
  renderHistory(state.history);
  renderApprovals(state.approvals);
  renderModelProfiles();
  renderIndexStats();
  renderPrototypePlan(state.latestPrototypePlan);
  renderPlayableWorkflow(state.playableWorkflow);
  renderRecipeCards();
  renderRecipeRuns();
  renderSelectedRecipe();
  renderConnectionWizard();
  renderBeginnerWorkflowPlan();
}

function renderBeginnerDashboard(): void {
  renderStatusCard("beginner-project-status", {
    state: state.unrealStatus.state === "online" ? "online" : "unknown",
    label: "Project connection",
    detail: state.unrealStatus.state === "online" ? "Unreal project is reachable through the local bridge." : "Connect Unreal to identify the active project."
  });
  renderStatusCard("beginner-unreal-status", {
    state: state.unrealStatus.state,
    label: "Unreal bridge",
    detail: state.unrealStatus.detail || state.unrealStatus.label
  });
  renderStatusCard("beginner-model-status", getModelStatusCard());
  renderStatusCard("beginner-manual-status", {
    state: "online",
    label: "Manual workflows",
    detail: "Always available, even with no local model configured."
  });

  if (state.unrealStatus.state === "online") {
    requireElement("beginner-next-action").textContent = "Run “Check my game for problems” to get a read-only health report.";
  } else {
    requireElement("beginner-next-action").textContent = "Start Unreal, open Tools > Revolt Bridge, then click Start Bridge.";
  }
}

const connectionSteps = [
  {
    title: "Step 1: Open your Unreal project",
    body: "Open the Unreal Engine project you want this tool to control. Keep Unreal open while using the desktop app.",
    buttons: [{ label: "I opened Unreal", action: "next" }]
  },
  {
    title: "Step 2: Make sure the plugin is enabled",
    body: "In Unreal, go to Edit → Plugins and search for RevoltEditorBridge. Make sure it is enabled. If Unreal asks you to restart, restart Unreal and come back here.",
    buttons: [
      { label: "Plugin is enabled", action: "next" },
      { label: "I need help finding it", action: "plugin-help" }
    ]
  },
  {
    title: "Step 3: Open the Revolt Bridge panel",
    body: "In Unreal, open Tools → Revolt Bridge. If you do not see Revolt Bridge under Tools, the plugin may not be enabled or Unreal may need to restart.",
    buttons: [{ label: "I opened Revolt Bridge", action: "next" }]
  },
  {
    title: "Step 4: Start the local bridge",
    body: "In the Revolt Bridge panel inside Unreal, click Start Bridge. This lets the desktop app talk to Unreal on your own computer. This uses localhost only by default. It does not require internet.",
    buttons: [{ label: "I clicked Start Bridge", action: "next" }]
  },
  {
    title: "Step 5: Test the connection",
    body: "Click the button below to check whether the desktop app can reach Unreal.",
    buttons: [{ label: "Test Connection", action: "test" }]
  }
];

function renderConnectionWizard(): void {
  const step = connectionSteps[Math.min(connectionStepIndex, connectionSteps.length - 1)];
  requireElement("connection-step-title").textContent = step.title;
  requireElement("connection-step-body").textContent = step.body;
  const extra = requireElement("connection-step-extra");
  extra.hidden = true;
  const buttons = requireElement("connection-step-buttons");
  buttons.innerHTML = step.buttons.map((button) => `<button data-connection-action="${escapeHtml(button.action)}" class="${button.action === "test" ? "primary" : ""}">${escapeHtml(button.label)}</button>`).join("");
  renderStatusCard("connection-status-card", connectionStatusCard());
  setConnectionFields(state.connectionState.unreal.host, state.connectionState.unreal.port, false);
  requireElement("connection-advanced-output").textContent = JSON.stringify(
    {
      endpoint: state.connectionState.unreal.endpoint,
      lastCheckedAt: state.connectionState.unreal.lastCheckedAt,
      lastError: state.connectionState.unreal.lastError,
      lastResponse: state.connectionState.unreal.lastResponse
    },
    null,
    2
  );

  for (const button of Array.from(document.querySelectorAll<HTMLButtonElement>("[data-connection-action]"))) {
    button.addEventListener("click", async () => {
      const action = button.dataset.connectionAction ?? "";
      if (action === "next") {
        connectionStepIndex = Math.min(connectionStepIndex + 1, connectionSteps.length - 1);
        renderConnectionWizard();
      } else if (action === "plugin-help") {
        extra.hidden = false;
        extra.innerHTML = `
          <ol>
            <li>In Unreal, click Edit.</li>
            <li>Click Plugins.</li>
            <li>Use the search box.</li>
            <li>Type RevoltEditorBridge.</li>
            <li>Check the Enabled box.</li>
            <li>Restart Unreal if asked.</li>
          </ol>
        `;
      } else if (action === "test") {
        await runUnrealConnectionTest("connect-unreal-wizard");
      }
    });
  }
}

function connectionStatusCard(): StatusCard {
  const unreal = state.connectionState.unreal;
  if (unreal.status === "connected") {
    return { state: "online", label: "Connected", detail: "You are ready to use Manual Command Mode or AI-assisted tools." };
  }
  if (unreal.status === "checking") {
    return { state: "warning", label: "Checking", detail: "Testing the local Unreal bridge now." };
  }
  if (unreal.status === "error") {
    return { state: "offline", label: "Error", detail: unreal.lastError ?? "The desktop app could not reach Unreal." };
  }
  return { state: "unknown", label: "Not Connected", detail: "Open Unreal → Tools → Revolt Bridge → Start Bridge → Test Connection." };
}

async function runUnrealConnectionTest(source: string): Promise<void> {
  const resultElement = document.getElementById("connection-result-message");
  state.connectionState.unreal = { ...state.connectionState.unreal, status: "checking", lastError: null };
  renderConnectionWizard();

  try {
    const response = await window.revoltRuntime.testDefaultUnrealConnection();
    applyUnrealConnectionResult(response.result);
    state.history = safeArray(response.history);

    if (response.result?.ok) {
      if (resultElement) {
        resultElement.innerHTML = `
          <strong>Connected to Unreal</strong><br />
          You are ready to use Manual Command Mode or AI-assisted tools.
          <div class="button-row">
            <button id="connection-open-manual">Open Manual Command Mode</button>
            <button id="connection-go-dashboard">Go to Dashboard</button>
            <button id="connection-run-first-test" class="primary">Run First Test</button>
          </div>
        `;
        requireElement("connection-open-manual").addEventListener("click", () => showPage("manual"));
        requireElement("connection-go-dashboard").addEventListener("click", () => showPage("dashboard"));
        requireElement("connection-run-first-test").addEventListener("click", () => {
          void runFirstUnrealConnectionTest();
        });
      }
      notify(`Unreal connection test succeeded from ${source}.`, "info");
    } else {
      const message = response.result?.message ?? "The desktop app could not reach Unreal.";
      const detail = response.result?.detail ?? "Most likely, Unreal is not open or Start Bridge has not been clicked.";
      if (resultElement) {
        resultElement.innerHTML = unrealConnectionFailureHtml(message, detail);
        requireElement("connection-try-again").addEventListener("click", () => {
          void runUnrealConnectionTest("connect-unreal-retry");
        });
        requireElement("connection-show-advanced").addEventListener("click", () => {
          requireElement("connection-advanced-output").scrollIntoView({ behavior: "smooth" });
        });
      }
      notify(`${message} ${detail}`, "warning");
    }
  } catch (error) {
    const message = errorMessage(error, "Unreal connection test failed before it could reach the local bridge.");
    state.unrealStatus = { state: "offline", label: "Unreal Test Failed", detail: message };
    state.connectionState.unreal = {
      ...state.connectionState.unreal,
      status: "error",
      endpoint: `http://${state.settings.ueBridgeHost}:${state.settings.ueBridgePort}`,
      lastCheckedAt: new Date().toISOString(),
      lastError: message
    };
    if (resultElement) {
      resultElement.innerHTML = unrealConnectionFailureHtml("Unreal connection test failed.", message);
    }
    notify(`Unreal connection test failed from ${source}: ${message}`, "error");
  } finally {
    renderAll();
  }
}

function unrealConnectionFailureHtml(message: string, detail: string): string {
  return `
    <strong>${escapeHtml(message)}</strong><br />
    ${escapeHtml(detail)}
    <ol>
      <li>Open your Unreal project.</li>
      <li>Go to Tools → Revolt Bridge.</li>
      <li>Click Start Bridge.</li>
      <li>Confirm the endpoint is http://127.0.0.1:8765.</li>
      <li>Come back here and click Test Connection again.</li>
    </ol>
    <div class="button-row">
      <button id="connection-try-again" class="primary">Try Again</button>
      <button id="connection-show-advanced">Show Advanced Details</button>
    </div>
  `;
}

function applyUnrealConnectionResult(result: { ok?: boolean; status?: string; endpoint?: string; message?: string; detail?: string; checkedAt?: string; response?: unknown; } | undefined): void {
  const ok = result?.ok === true;
  state.unrealStatus = ok
    ? { state: "online", label: "Connected to Unreal", detail: "You are ready to use Manual Command Mode or AI-assisted tools." }
    : { state: "offline", label: "Not Connected", detail: result?.detail ?? "Most likely, Unreal is not open or Start Bridge has not been clicked." };
  state.connectionState.unreal = {
    ...state.connectionState.unreal,
    status: ok ? "connected" : "error",
    endpoint: result?.endpoint ?? state.connectionState.unreal.endpoint,
    lastCheckedAt: result?.checkedAt ?? new Date().toISOString(),
    lastSuccessAt: ok ? result?.checkedAt ?? new Date().toISOString() : state.connectionState.unreal.lastSuccessAt,
    lastError: ok ? null : result?.message ?? "The desktop app could not reach Unreal.",
    lastResponse: result?.response ?? null
  };
}

async function runFirstUnrealConnectionTest(): Promise<void> {
  const message = requireElement("connection-result-message");
  try {
    const result = await window.revoltRuntime.runManualCommand({ command: "get_project_summary", params: {} });
    state.history = safeArray(result.history);
    message.textContent = result.ok ? "First test worked. Unreal returned a project summary." : `First test reached Unreal but returned: ${result.summary}`;
  } catch (error) {
    message.textContent = errorMessage(error, "First test could not run. The connection may have stopped.");
  }
}

requireElement("check-unreal").addEventListener("click", () => {
  void runUnrealConnectionTest("dashboard");
});
requireElement("check-unreal-secondary").addEventListener("click", () => {
  void runUnrealConnectionTest("unreal-connection-page");
});
for (const id of ["dashboard-connect-unreal", "beginner-connect-unreal", "open-connect-unreal"]) {
  requireElement(id).addEventListener("click", () => showPage("connect-unreal"));
}
bindSafeClick("clear-history", "Command history cleared.", async () => {
  state.history = safeArray(await window.revoltRuntime.clearHistory());
  renderHistory(state.history);
});

bindSafeClick("export-history", "Command history export finished.", async () => {
  const result = await window.revoltRuntime.exportHistory();
  if (result.history) {
    state.history = safeArray(result.history);
    renderHistory(state.history);
  }
});

requireElement("manualCommand").addEventListener("change", () => refreshManualJson());
for (const id of ["manualAssetPath", "manualActorPath", "manualFindPath", "manualFindName", "manualFindClass", "manualMaxResults", "manualBiomeJson", "manualDryRun", "manualApproved"]) {
  requireElement(id).addEventListener("input", () => refreshManualJson());
  requireElement(id).addEventListener("change", () => refreshManualJson());
}
bindSafeClick("refresh-manual-json", "Manual request JSON refreshed.", () => refreshManualJson());
bindSafeClick("run-manual-command", "Manual command finished.", async () => {
  const message = requireElement("manual-message");
  try {
    const request = readManualRequest();
    setInputValue("manualRequestJson", JSON.stringify(request, null, 2));
    const result = await window.revoltRuntime.runManualCommand(request);
    setInputValue("manualResponseJson", JSON.stringify(result.response, null, 2));
    state.history = safeArray(result.history);
    message.textContent = result.ok ? result.summary : `Bridge error: ${result.summary}`;
    renderHistory(state.history);
  } catch (error) {
    message.textContent = errorMessage(error, "Manual command failed.");
    notify(message.textContent, "warning");
  }
});

for (const button of Array.from(document.querySelectorAll<HTMLButtonElement>("[data-beginner-action]"))) {
  button.addEventListener("click", () => handleBeginnerAction(button.dataset.beginnerAction ?? ""));
}

requireElement("beginner-create-plan").addEventListener("click", () => {
  void createBeginnerIntentDryRunPlan();
});

requireElement("beginner-clear-plan").addEventListener("click", () => {
  beginnerWorkflowPlan = null;
  setInputValue("beginnerIntentInput", "");
  requireElement("beginner-action-result").textContent = "Plan cleared. Type a beginner request like “make a map” to create a new dry-run plan.";
  renderBeginnerWorkflowPlan();
});

requireElement("beginner-approve-execute").addEventListener("click", () => {
  void executeApprovedBeginnerWorkflow();
});

function handleBeginnerAction(action: string): void {
  if (action === "make_playable") {
    showPage("playable-workflow");
    requireElement("beginner-action-result").textContent = "Make This Level Playable opened. Run the read-only check first; no fixes run automatically.";
    return;
  }
  if (action === "new_prototype") {
    showPage("prototype-wizard");
    requireElement("beginner-action-result").textContent = "Prototype Wizard opened. Create a dry-run plan first; no assets will be changed.";
    return;
  }
  const recipeActions: Record<string, string> = {
    add_enemies: "create_basic_enemy",
    add_weapon: "create_simple_weapon",
    add_pickups: "create_health_pickup",
    create_menu: "create_main_menu"
  };
  if (recipeActions[action]) {
    selectedRecipeId = recipeActions[action];
    showPage("recipes");
    requireElement("beginner-action-result").textContent = "Recipe opened. Preview the command plan first; dry-run is the default.";
    renderSelectedRecipe();
    return;
  }

  const messages: Record<string, string> = {
    add_enemies: "Placeholder workflow: use generated enemy data and wave spawner tools after dry-run review. No enemies were spawned.",
    add_weapon: "Placeholder workflow: tune generated weapon Data Assets only after a dry-run preview. No weapon data changed.",
    add_pickups: "Placeholder workflow: plan generated health pickups first; later commands stay dry-run by default.",
    create_menu: "Placeholder workflow: main menu creation is planned for a later guided Blueprint workflow. No Blueprint changes ran.",
    create_biome: "Placeholder workflow: create generated biome JSON under /Game/RevoltGenerated/Biomes, preview it, then approve if safe.",
    audit_game: "Existing workflow: use Advanced Mode for the full Project Audit tools, or open Unreal’s Revolt Bridge tab and run Project Audit. No audit ran automatically here.",
    open_tutorials: "Local tutorials: start with docs/setup/offline-quickstart.md, docs/guides/manual-command-mode.md, docs/guides/project-auditing-guide.md, and docs/setup/local-model-setup.md."
  };
  requireElement("beginner-action-result").textContent = messages[action] ?? "Choose a guided action to see a safe next step.";
  if (action === "audit_game") {
    requireElement("beginner-health-summary").textContent = "Ready for a read-only audit when Unreal is connected.";
    requireElement("beginner-next-action").textContent = "Open Advanced Mode only if you want detailed audit controls.";
  } else if (action === "open_tutorials") {
    requireElement("beginner-next-action").textContent = "Read the local docs listed below; no internet required.";
  }
}

async function createBeginnerIntentDryRunPlan(): Promise<void> {
  const message = requireElement("beginner-action-result");
  const intent = getInputValue("beginnerIntentInput").trim();
  if (!intent) {
    message.textContent = "Type what you want first. Example: make a map.";
    notify("Type a beginner request first.", "warning");
    return;
  }

  const planned = planBeginnerIntent(intent);
  if (!planned) {
    message.textContent = "I can safely plan “make a map” right now. Other typed workflows will be added later.";
    notify("That beginner request is not supported yet.", "warning");
    return;
  }

  beginnerWorkflowPlan = planned;
  renderBeginnerWorkflowPlan();
  message.textContent = "Dry-run plan created. Testing the plan in Unreal without changing your project...";

  try {
    const result = await window.revoltRuntime.runManualCommand({ command: planned.command, params: planned.dryRunParams });
    state.history = safeArray(result.history);
    planned.dryRunResult = result;
    planned.status = result.ok ? "dry_run_ready" : "dry_run_failed";
    message.textContent = result.ok
      ? "Dry-run complete. Review the summary, then click Approve and Create Map if you want Unreal to apply it."
      : `Dry-run failed before changes were made: ${result.summary}`;
    renderHistory(state.history);
  } catch (error) {
    planned.status = "dry_run_failed";
    const detail = errorMessage(error, "Dry-run failed before changes were made.");
    message.textContent = detail;
    notify(detail, "warning");
  }

  renderBeginnerWorkflowPlan();
}

function planBeginnerIntent(intent: string): BeginnerWorkflowPlan | null {
  const normalized = intent.toLowerCase();
  if (/(make|create|build|generate).*(map|level|arena|room)|map|level|arena/.test(normalized)) {
    const dryRunParams = {
      dry_run: true,
      approved: false,
      permission_level: "editor_mutation",
      source_workflow: "beginner_make_map",
      beginner_intent: intent
    };
    return {
      id: `beginner-map-${Date.now()}`,
      intent,
      title: "Make a simple generated test map",
      beginnerSummary: "This will add a small generated arena layout to the currently open Unreal level. It will not run until you approve it.",
      command: "spawn_test_arena",
      dryRunParams,
      approvedParams: {
        ...dryRunParams,
        dry_run: false,
        approved: true
      },
      plannedChanges: [
        "Add a generated wave spawner actor to the current level.",
        "Add a generated pickup actor to the current level.",
        "Add a generated objective actor to the current level."
      ],
      riskLevel: "Low",
      permissionLevel: "SafeEdit / editor_mutation",
      status: "planned",
      dryRunResult: null,
      executionResult: null,
      nextSteps: [
        "Press Play in Unreal to inspect the generated layout.",
        "Run Make This Level Playable if the level still needs a PlayerStart or lighting.",
        "Use Undo in Unreal if you do not like the generated placement."
      ]
    };
  }

  if (normalized.includes("playable") || normalized.includes("player start") || normalized.includes("spawn point")) {
    showPage("playable-workflow");
    requireElement("beginner-action-result").textContent = "Make This Level Playable opened. Run the read-only check first; no fixes run automatically.";
    return null;
  }

  if (normalized.includes("prototype") || normalized.includes("game")) {
    showPage("prototype-wizard");
    requireElement("beginner-action-result").textContent = "Prototype Wizard opened. Create a dry-run plan first; no assets will be changed.";
    return null;
  }

  return null;
}

async function executeApprovedBeginnerWorkflow(): Promise<void> {
  const message = requireElement("beginner-action-result");
  if (!beginnerWorkflowPlan) {
    message.textContent = "Create a dry-run plan first.";
    notify("Create a dry-run plan first.", "warning");
    return;
  }
  if (beginnerWorkflowPlan.status !== "dry_run_ready") {
    message.textContent = "Review a successful dry-run before approving this workflow.";
    notify("A successful dry-run is required before approval.", "warning");
    return;
  }

  beginnerWorkflowPlan.status = "applying";
  renderBeginnerWorkflowPlan();
  message.textContent = "Applying approved map workflow in Unreal...";

  try {
    const result = await window.revoltRuntime.runManualCommand({ command: beginnerWorkflowPlan.command, params: beginnerWorkflowPlan.approvedParams });
    state.history = safeArray(result.history);
    beginnerWorkflowPlan.executionResult = result;
    beginnerWorkflowPlan.status = result.ok ? "applied" : "failed";
    message.textContent = result.ok
      ? "Map workflow applied in Unreal. Review the generated actors in the current level."
      : `Approved command reached Unreal but failed: ${result.summary}`;
    renderHistory(state.history);
    notify(message.textContent, result.ok ? "info" : "warning");
  } catch (error) {
    beginnerWorkflowPlan.status = "failed";
    const detail = errorMessage(error, "Approved map workflow failed.");
    message.textContent = detail;
    notify(detail, "error");
  }

  renderBeginnerWorkflowPlan();
}

function renderBeginnerWorkflowPlan(): void {
  const container = requireElement("beginner-workflow-plan");
  const technical = requireElement("beginner-workflow-technical");
  const approveButton = requireElement("beginner-approve-execute") as HTMLButtonElement;

  if (!beginnerWorkflowPlan) {
    container.hidden = true;
    technical.hidden = true;
    technical.textContent = "{}";
    approveButton.disabled = true;
    approveButton.textContent = "Approve and Create Map";
    return;
  }

  const plan = beginnerWorkflowPlan;
  container.hidden = false;
  approveButton.disabled = plan.status !== "dry_run_ready";
  approveButton.textContent = plan.status === "applied" ? "Map Created" : "Approve and Create Map";
  const dryRunSummary = plan.dryRunResult ? `${plan.dryRunResult.ok ? "Dry-run succeeded" : "Dry-run failed"}: ${plan.dryRunResult.summary}` : "Dry-run has not completed yet.";
  const executionSummary = plan.executionResult ? `${plan.executionResult.ok ? "Applied" : "Failed"}: ${plan.executionResult.summary}` : "Not applied yet.";
  container.innerHTML = `
    <h4>${escapeHtml(plan.title)}</h4>
    <p>${escapeHtml(plan.beginnerSummary)}</p>
    <p><strong>Status:</strong> ${escapeHtml(plan.status.replace(/_/g, " "))}</p>
    <p><strong>Risk:</strong> ${escapeHtml(plan.riskLevel)} · <strong>Permission:</strong> ${escapeHtml(plan.permissionLevel)}</p>
    ${renderPlanList("Planned changes", plan.plannedChanges)}
    <p><strong>Dry-run result:</strong> ${escapeHtml(dryRunSummary)}</p>
    <p><strong>Approved execution:</strong> ${escapeHtml(executionSummary)}</p>
    ${renderPlanList("Next steps", plan.nextSteps)}
  `;
  technical.hidden = false;
  technical.textContent = JSON.stringify(
    {
      intent: plan.intent,
      command: plan.command,
      dryRunParams: plan.dryRunParams,
      approvedParams: plan.approvedParams,
      dryRunResult: plan.dryRunResult?.response ?? null,
      executionResult: plan.executionResult?.response ?? null
    },
    null,
    2
  );
}

requireElement("run-playable-check").addEventListener("click", async () => {
  const message = requireElement("playable-message");
  try {
    message.textContent = "Checking the open level through the local Unreal bridge...";
    const result = await window.revoltRuntime.checkPlayableLevel();
    state.playableWorkflow = result?.workflow ?? null;
    state.history = safeArray(result.history);
    selectedPlayableFixId = safeArray(result?.workflow?.fixPlan).find((fix) => fix.dryRunAvailable)?.id ?? "";
    message.textContent = result?.workflow?.message ?? "Level check could not return a workflow. Confirm the Unreal bridge is running.";
    renderPlayableWorkflow(result?.workflow ?? null);
    renderHistory(state.history);
  } catch (error) {
    message.textContent = error instanceof Error ? error.message : "Unable to check the open level.";
  }
});

requireElement("dry-run-playable-selected").addEventListener("click", async () => {
  const message = requireElement("playable-message");
  try {
    if (!selectedPlayableFixId) {
      throw new Error("Select a dry-run capable fix first.");
    }
    message.textContent = "Sending selected fix as a dry-run preview...";
    const result = await window.revoltRuntime.dryRunPlayableFixes([selectedPlayableFixId]);
    state.playableWorkflow = result?.workflow ?? state.playableWorkflow;
    state.history = safeArray(result.history);
    showPlayableDryRunResponse(safeArray(result.responses));
    message.textContent = "Dry-run preview returned. Approval is still required before anything can apply.";
    renderPlayableWorkflow(result?.workflow ?? state.playableWorkflow);
    renderHistory(state.history);
  } catch (error) {
    message.textContent = error instanceof Error ? error.message : "Unable to dry-run the selected fix.";
  }
});

requireElement("dry-run-playable-all").addEventListener("click", async () => {
  const message = requireElement("playable-message");
  try {
    const safeFixIds = (state.playableWorkflow?.fixPlan ?? [])
      .filter((fix) => fix.selected && fix.dryRunAvailable && fix.riskLevel !== "Dangerous")
      .map((fix) => fix.id);
    if (safeFixIds.length === 0) {
      throw new Error("Run the level check first, then select at least one safe dry-run fix.");
    }
    message.textContent = "Sending all safe fixes as dry-run previews...";
    const result = await window.revoltRuntime.dryRunPlayableFixes(safeFixIds);
    state.playableWorkflow = result?.workflow ?? state.playableWorkflow;
    state.history = safeArray(result.history);
    showPlayableDryRunResponse(safeArray(result.responses));
    message.textContent = `Dry-run previews returned for ${safeArray(result.responses).length} fix(es). Approval is still required before applying.`;
    renderPlayableWorkflow(result?.workflow ?? state.playableWorkflow);
    renderHistory(state.history);
  } catch (error) {
    message.textContent = error instanceof Error ? error.message : "Unable to dry-run safe fixes.";
  }
});

requireElement("explain-playable-fix").addEventListener("click", () => {
  const fix = findSelectedPlayableFix();
  requireElement("playable-explanation").textContent = fix
    ? `${fix.label}: ${fix.beginnerExplanation}`
    : "Select a fix to see a beginner-friendly explanation.";
});

requireElement("toggle-playable-technical").addEventListener("click", () => {
  const technical = requireElement("playable-technical") as HTMLPreElement;
  technical.hidden = !technical.hidden;
  requireElement("playable-dryrun-response").hidden = technical.hidden;
  requireElement("toggle-playable-technical").textContent = technical.hidden ? "Show Technical Details" : "Hide Technical Details";
});

requireElement("preview-recipe").addEventListener("click", async () => {
  const message = requireElement("recipe-message");
  try {
    const recipe = await window.revoltRuntime.previewRecipe(requireSelectedRecipeId(), readRecipeValues());
    state.recipes = safeArray(state.recipes).map((item) => (item.recipeId === recipe.recipeId ? recipe : item));
    message.textContent = "Command plan preview refreshed. No command ran.";
    renderSelectedRecipe();
  } catch (error) {
    message.textContent = errorMessage(error, "Unable to preview recipe.");
    notify(message.textContent, "warning");
  }
});

requireElement("run-recipe-dry-run").addEventListener("click", async () => {
  const message = requireElement("recipe-message");
  try {
    const result = await window.revoltRuntime.runRecipeDryRun(requireSelectedRecipeId(), readRecipeValues());
    state.recipes = safeArray(state.recipes).map((item) => (item.recipeId === result.recipe?.recipeId ? result.recipe : item));
    state.recentRecipeRuns = safeArray(result.recentRecipeRuns);
    state.history = safeArray(result.history);
    message.textContent = result.run?.summary ?? "Recipe dry-run finished.";
    renderSelectedRecipe();
    renderRecipeRuns();
    renderRecipeResultPanel(result.run);
    renderHistory(state.history);
  } catch (error) {
    message.textContent = errorMessage(error, "Unable to run recipe dry-run.");
    notify(message.textContent, "warning");
  }
});

requireElement("apply-approved-recipe").addEventListener("click", async () => {
  const message = requireElement("recipe-message");
  try {
    if (!getChecked("recipe-approved")) {
      throw new Error("Review the dry-run plan and check the approval box before applying.");
    }
    const result = await window.revoltRuntime.applyApprovedRecipe(requireSelectedRecipeId(), readRecipeValues());
    state.recipes = safeArray(state.recipes).map((item) => (item.recipeId === result.recipe?.recipeId ? result.recipe : item));
    state.recentRecipeRuns = safeArray(result.recentRecipeRuns);
    state.history = safeArray(result.history);
    message.textContent = result.run?.summary ?? "Recipe apply finished.";
    setChecked("recipe-approved", false);
    renderSelectedRecipe();
    renderRecipeRuns();
    renderRecipeResultPanel(result.run);
    renderHistory(state.history);
  } catch (error) {
    message.textContent = errorMessage(error, "Unable to apply approved recipe.");
    notify(message.textContent, "warning");
  }
});

requireElement("prototype-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  const message = requireElement("prototype-message");
  try {
    const result = await window.revoltRuntime.createPrototypeDryRunPlan(readPrototypeInput());
    state.latestPrototypePlan = result.plan;
    state.history = safeArray(result.history);
    message.textContent = "Dry-run plan created and saved locally. No Unreal changes were made.";
    renderPrototypePlan(result.plan);
    renderHistory(state.history);
  } catch (error) {
    message.textContent = errorMessage(error, "Unable to create prototype plan.");
    notify(message.textContent, "warning");
  }
});

requireElement("send-prototype-recipe").addEventListener("click", async () => {
  const message = requireElement("prototype-message");
  try {
    if (!state.latestPrototypePlan) {
      throw new Error("Create a dry-run plan first.");
    }
    const result = await window.revoltRuntime.sendPrototypePlanToGuidedRecipe(state.latestPrototypePlan.id);
    state.history = safeArray(result.history);
    message.textContent = result.message ?? "Guided recipe handoff recorded locally.";
    renderHistory(state.history);
  } catch (error) {
    message.textContent = errorMessage(error, "Unable to send plan to Guided Recipe.");
    notify(message.textContent, "warning");
  }
});

requireElement("choose-index-root").addEventListener("click", async () => {
  const message = requireElement("index-status-message");
  try {
    const selected = await window.revoltRuntime.chooseIndexRoot();
    if (selected) {
      setInputValue("projectIndexRoot", selected);
    }
  } catch (error) {
    message.textContent = errorMessage(error, "Unable to open folder picker.");
  }
});

requireElement("start-indexing").addEventListener("click", async () => {
  const message = requireElement("index-status-message");
  try {
    state.indexStats = await window.revoltRuntime.startIndexing(getInputValue("projectIndexRoot"));
    state.settings.projectIndexRoot = getInputValue("projectIndexRoot");
    message.textContent = "Local indexing started. Summaries use the selected local model only when available.";
    renderAll();
  } catch (error) {
    message.textContent = error instanceof Error ? error.message : "Unable to start indexing.";
  }
});

requireElement("stop-indexing").addEventListener("click", async () => {
  const message = requireElement("index-status-message");
  try {
    state.indexStats = await window.revoltRuntime.stopIndexing();
    message.textContent = "Stop requested. Current file will finish first.";
    renderAll();
  } catch (error) {
    message.textContent = errorMessage(error, "Unable to stop indexing.");
  }
});

requireElement("clear-local-index").addEventListener("click", async () => {
  const message = requireElement("index-status-message");
  try {
    const result = await window.revoltRuntime.clearLocalIndex();
    state.indexStats = result?.stats ?? state.indexStats;
    state.history = safeArray(result?.history);
    message.textContent = "Local index cleared.";
    renderAll();
  } catch (error) {
    message.textContent = error instanceof Error ? error.message : "Unable to clear local index.";
  }
});

requireElement("modelProviderType").addEventListener("change", () => {
  const providerType = getInputValue("modelProviderType");
  if (!getInputValue("modelDisplayName")) {
    setInputValue("modelDisplayName", providerLabel(providerType));
  }
  if (!getInputValue("modelBaseUrl")) {
    setInputValue("modelBaseUrl", defaultBaseUrl(providerType));
  }
});

requireElement("wizardProviderType").addEventListener("change", () => {
  const providerType = getInputValue("wizardProviderType");
  setInputValue("wizardBaseUrl", defaultBaseUrl(providerType));
  setInputValue("wizardDisplayName", providerLabel(providerType));
  setInputValue("wizardClassification", providerType === "openai-compatible" ? "offline" : "offline");
  requireElement("wizard-message").textContent = `${providerLabel(providerType)} selected. Endpoint stays local by default.`;
});

requireElement("connection-reset-default").addEventListener("click", async () => {
  setConnectionFields("127.0.0.1", 8765);
  state.settings = await window.revoltRuntime.updateSettings({ ...state.settings, ueBridgeHost: "127.0.0.1", ueBridgePort: 8765 });
  state.connectionState.unreal = {
    ...state.connectionState.unreal,
    endpoint: "http://127.0.0.1:8765",
    host: "127.0.0.1",
    port: 8765,
    timeoutMs: 5000,
    lastError: null
  };
  requireElement("connection-result-message").textContent = "Default connection restored. Click Test Connection when Unreal is ready.";
  renderAll();
});

requireElement("connection-save-settings").addEventListener("click", async () => {
  const message = requireElement("connection-result-message");
  try {
    const endpoint = validateUnrealEndpointUrl(getInputValue("connectionEndpoint"), "Unreal endpoint");
    const parsed = new URL(endpoint);
    state.settings = await window.revoltRuntime.updateSettings({
      ...state.settings,
      ueBridgeHost: parsed.hostname,
      ueBridgePort: Number(parsed.port || 8765)
    });
    state.connectionState.unreal = {
      ...state.connectionState.unreal,
      endpoint,
      host: parsed.hostname,
      port: Number(parsed.port || 8765),
      timeoutMs: Number(getInputValue("connectionTimeoutMs")) || 5000
    };
    message.textContent = "Connection settings saved. Click Test Connection when Unreal is ready.";
    renderAll();
  } catch (error) {
    message.textContent = errorMessage(error, "The connection address is not valid.");
    notify(message.textContent, "warning");
  }
});

requireElement("wizardDetectedModels").addEventListener("change", () => {
  const selected = getInputValue("wizardDetectedModels");
  if (selected) {
    setInputValue("wizardModelName", selected);
  }
});

requireElement("wizard-detect").addEventListener("click", async () => {
  const message = requireElement("wizard-message");
  const output = requireElement("wizard-result");
  try {
    const result = await window.revoltRuntime.detectModelProvider(
      getInputValue("wizardProviderType") as ModelProviderType,
      validateLocalEndpointUrl(getInputValue("wizardBaseUrl"), "Wizard base URL")
    );
    output.textContent = JSON.stringify(result, null, 2);
    renderWizardDetectedModels(safeArray(result.models));
    const detectedModels = safeArray(result.models);
    if (detectedModels[0]) {
      setInputValue("wizardModelName", detectedModels[0]);
    }
    message.textContent = `${result.status ?? "Unknown error"}: ${result.message ?? "Endpoint did not return a usable response."}`;
  } catch (error) {
    message.textContent = errorMessage(error, "Unable to detect local endpoint.");
    notify(message.textContent, "warning");
  }
});

requireElement("wizard-test").addEventListener("click", async () => {
  const message = requireElement("wizard-message");
  const output = requireElement("wizard-result");
  try {
    const saved = await window.revoltRuntime.saveModelProfile(readWizardProfile());
    const response = await window.revoltRuntime.testModelProfile(saved.id);
    state.modelProfiles = safeArray(response.modelProfiles);
    state.history = safeArray(response.history);
    output.textContent = JSON.stringify(response.result, null, 2);
    message.textContent = `${response.result?.status ?? "Unknown error"}: ${response.result?.message ?? "No response from local model."}`;
    renderAll();
  } catch (error) {
    message.textContent = errorMessage(error, "Unable to test local model.");
    notify(message.textContent, "warning");
  }
});

requireElement("wizard-save").addEventListener("click", async () => {
  const message = requireElement("wizard-message");
  try {
    await window.revoltRuntime.saveModelProfile(readWizardProfile());
    state.modelProfiles = safeArray(await window.revoltRuntime.listModelProfiles());
    message.textContent = "Model profile saved locally in SQLite.";
    renderAll();
  } catch (error) {
    message.textContent = errorMessage(error, "Unable to save model profile.");
    notify(message.textContent, "warning");
  }
});

requireElement("wizard-save-active").addEventListener("click", async () => {
  const message = requireElement("wizard-message");
  try {
    const profile = await window.revoltRuntime.saveModelProfile(readWizardProfile());
    state.settings = await window.revoltRuntime.setActiveModelProfile(profile?.id ?? "");
    state.modelProfiles = safeArray(await window.revoltRuntime.listModelProfiles());
    state.activeModelProfile = safeArray(state.modelProfiles).find((item) => item.id === state.settings.activeModelProfileId) ?? null;
    state.offlineStatus = getOfflineStatus();
    message.textContent = `${profile.displayName ?? "Model profile"} saved and set as active.`;
    renderAll();
  } catch (error) {
    message.textContent = errorMessage(error, "Unable to save and activate model profile.");
    notify(message.textContent, "warning");
  }
});

requireElement("new-model-profile").addEventListener("click", () => clearModelForm());

requireElement("model-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  const message = requireElement("model-message");
  try {
    const saved = await window.revoltRuntime.saveModelProfile(readModelForm());
    state.modelProfiles = safeArray(await window.revoltRuntime.listModelProfiles());
    if (!state.settings.activeModelProfileId) {
      state.settings = await window.revoltRuntime.setActiveModelProfile(saved?.id ?? "");
    }
    state.activeModelProfile = safeArray(state.modelProfiles).find((item) => item.id === state.settings.activeModelProfileId) ?? null;
    clearModelForm();
    message.textContent = "Model profile saved locally.";
    renderAll();
  } catch (error) {
    message.textContent = errorMessage(error, "Unable to save model profile.");
    notify(message.textContent, "warning");
  }
});

requireElement("set-active-model").addEventListener("click", async () => {
  const message = requireElement("model-message");
  try {
    const selectedId = getInputValue("activeModelProfile");
    state.settings = await window.revoltRuntime.setActiveModelProfile(selectedId);
    state.activeModelProfile = safeArray(state.modelProfiles).find((profile) => profile.id === selectedId) ?? null;
    state.offlineStatus = getOfflineStatus();
    message.textContent = selectedId ? "Active model profile updated." : "Manual Command Mode selected.";
    renderAll();
  } catch (error) {
    message.textContent = errorMessage(error, "Unable to set active model profile.");
  }
});

requireElement("test-active-model").addEventListener("click", async () => {
  const message = requireElement("model-message");
  try {
    const selectedId = getInputValue("activeModelProfile");
    if (!selectedId) {
      throw new Error("Select a local model profile first.");
    }
    const response = await window.revoltRuntime.testModelProfile(selectedId);
    state.history = safeArray(response.history);
    state.modelProfiles = safeArray(response.modelProfiles);
    message.textContent = response.result?.message ?? "Local model test finished.";
    renderAll();
  } catch (error) {
    message.textContent = errorMessage(error, "Unable to test model profile.");
    notify(message.textContent, "warning");
  }
});

requireElement("summarize-project").addEventListener("click", async () => {
  const message = requireElement("model-message");
  const output = requireElement("model-summary-result");
  try {
    message.textContent = "Summarizing with selected local model...";
    const response = await window.revoltRuntime.summarizeProjectWithActiveModel();
    state.history = safeArray(response.history);
    output.textContent = response.completion?.text ?? "No local model response was returned.";
    message.textContent = "Project summary completed locally.";
    renderAll();
  } catch (error) {
    message.textContent = errorMessage(error, "Unable to summarize project.");
    notify(message.textContent, "warning");
  }
});

requireElement("settings-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  const message = requireElement("settings-message");
  try {
    const nextSettings: RuntimeSettings = {
      ueBridgeHost: getInputValue("ueBridgeHost"),
      ueBridgePort: Number(getInputValue("ueBridgePort")),
      mcpServerCommand: getInputValue("mcpServerCommand"),
      mcpServerPath: getInputValue("mcpServerPath"),
      defaultPermissionMode: getInputValue("defaultPermissionMode") as RuntimeSettings["defaultPermissionMode"],
      enableOnlineProviders: getChecked("enableOnlineProviders"),
      offlineMode: getChecked("offlineMode"),
      experienceMode: getInputValue("experienceMode") as RuntimeSettings["experienceMode"],
      activeModelProfileId: state.settings.activeModelProfileId,
      projectIndexRoot: getInputValue("projectIndexRoot")
    };
    state.settings = await window.revoltRuntime.updateSettings(nextSettings);
    state.mcpStatus = await window.revoltRuntime.getMcpStatus().catch((error) => ({
      state: "unknown" as const,
      label: "MCP status unavailable",
      detail: errorMessage(error, "MCP server is optional and not required for the UI.")
    }));
    state.offlineStatus = {
      ...getOfflineStatus()
    };
    message.textContent = "Settings saved locally.";
    renderAll();
  } catch (error) {
    message.textContent = errorMessage(error, "Unable to save settings.");
    notify(message.textContent, "warning");
  }
});

initializeRuntime().catch((error) => {
  const message = errorMessage(error, "Desktop runtime started with local defaults because initialization failed.");
  state = createDefaultDesktopAppState({ errorMessages: [message] });
  renderAll();
  showPage("beginner-home");
  renderStartupError(message);
});

async function initializeRuntime(): Promise<void> {
  state = createDefaultDesktopAppState();
  renderAll();
  showPage("beginner-home");
  applyExperienceMode();
  refreshManualJson();

  if (!window.revoltRuntime?.getInitialState) {
    throw new Error("Runtime preload API is unavailable. The UI is running with safe defaults.");
  }

  const initialState = await window.revoltRuntime.getInitialState();
  state = mergeDesktopAppState(initialState);
  renderAll();
  showPage(state.settings.experienceMode === "advanced" ? "dashboard" : "beginner-home");
  applyExperienceMode();
  refreshManualJson();
  setInterval(refreshIndexStats, 3000);
}

function installRendererErrorBoundary(): void {
  window.addEventListener("error", (event) => {
    const message = event.error instanceof Error ? event.error.message : event.message;
    state.errorMessages = [...state.errorMessages, message];
    renderStartupError(message);
    notify(message, "error");
  });

  window.addEventListener("unhandledrejection", (event) => {
    const message = event.reason instanceof Error ? event.reason.message : String(event.reason ?? "Unhandled UI action failed.");
    state.errorMessages = [...state.errorMessages, message];
    renderStartupError(message);
    notify(message, "error");
    event.preventDefault();
  });
}

async function refreshIndexStats(): Promise<void> {
  if (!state) {
    return;
  }
  try {
    state.indexStats = await window.revoltRuntime.getIndexStats();
    renderStatusCard("card-index", getIndexStatusCard());
    renderIndexStats();
  } catch (error) {
    state.errorMessages = [...state.errorMessages, errorMessage(error, "Unable to refresh index status.")];
  }
}

function renderModelProfiles(): void {
  const profiles = safeArray(state.modelProfiles);
  const activeSelect = requireElement("activeModelProfile") as HTMLSelectElement;
  activeSelect.innerHTML = [
    `<option value="">Manual Command Mode (no model)</option>`,
    ...profiles.map((profile) => {
      const selected = profile.id === state.settings.activeModelProfileId ? "selected" : "";
      return `<option value="${escapeHtml(profile.id)}" ${selected}>${escapeHtml(profile.displayName)} · ${escapeHtml(profile.classification)}</option>`;
    })
  ].join("");

  const body = requireElement("models-body");
  body.innerHTML = profiles.length
    ? profiles
      .map(
        (profile) => `
            <tr>
              <td>${escapeHtml(profile.displayName)}</td>
              <td>${escapeHtml(providerLabel(profile.providerType))}</td>
              <td>${escapeHtml(profile.baseUrl)}</td>
              <td>${escapeHtml(profile.modelName)}</td>
              <td>${escapeHtml(profile.classification)}</td>
              <td>${escapeHtml(profile.lastTestStatus || "Not tested")}</td>
              <td>
                <button data-edit-model="${escapeHtml(profile.id)}">Edit</button>
                <button data-duplicate-model="${escapeHtml(profile.id)}">Duplicate</button>
                <button data-toggle-model="${escapeHtml(profile.id)}">${profile.enabled ? "Disable" : "Enable"}</button>
                <button class="danger" data-delete-model="${escapeHtml(profile.id)}">Delete</button>
              </td>
            </tr>
          `
      )
      .join("")
    : `<tr><td colspan="7">No local model is configured. Manual Command Mode is still available.</td></tr>`;

  for (const button of Array.from(document.querySelectorAll<HTMLButtonElement>("[data-edit-model]"))) {
    button.addEventListener("click", () => {
      const profile = safeArray(state.modelProfiles).find((item) => item.id === button.dataset.editModel);
      if (profile) {
        fillModelForm(profile);
      }
    });
  }

  for (const button of Array.from(document.querySelectorAll<HTMLButtonElement>("[data-delete-model]"))) {
    button.addEventListener("click", async () => {
      try {
        const id = button.dataset.deleteModel;
        if (!id) {
          return;
        }
        const result = await window.revoltRuntime.deleteModelProfile(id);
        state.settings = result?.settings ?? state.settings;
        state.modelProfiles = safeArray(result?.modelProfiles);
        state.activeModelProfile = safeArray(state.modelProfiles).find((profile) => profile.id === state.settings.activeModelProfileId) ?? null;
        state.offlineStatus = getOfflineStatus();
        renderAll();
      } catch (error) {
        requireElement("model-message").textContent = errorMessage(error, "Unable to delete model profile.");
      }
    });
  }

  for (const button of Array.from(document.querySelectorAll<HTMLButtonElement>("[data-duplicate-model]"))) {
    button.addEventListener("click", async () => {
      try {
        const id = button.dataset.duplicateModel;
        if (!id) {
          return;
        }
        const result = await window.revoltRuntime.duplicateModelProfile(id);
        state.modelProfiles = safeArray(result?.modelProfiles);
        renderAll();
      } catch (error) {
        requireElement("model-message").textContent = errorMessage(error, "Unable to duplicate model profile.");
      }
    });
  }

  for (const button of Array.from(document.querySelectorAll<HTMLButtonElement>("[data-toggle-model]"))) {
    button.addEventListener("click", async () => {
      try {
        const id = button.dataset.toggleModel;
        const profile = safeArray(state.modelProfiles).find((item) => item.id === id);
        if (!id || !profile) {
          return;
        }
        const result = await window.revoltRuntime.setModelProfileEnabled(id, !profile.enabled);
        state.settings = result?.settings ?? state.settings;
        state.modelProfiles = safeArray(result?.modelProfiles);
        state.activeModelProfile = safeArray(state.modelProfiles).find((item) => item.id === state.settings.activeModelProfileId) ?? null;
        state.offlineStatus = getOfflineStatus();
        renderAll();
      } catch (error) {
        requireElement("model-message").textContent = errorMessage(error, "Unable to update model profile.");
      }
    });
  }
}

function renderIndexStats(): void {
  state = mergeDesktopAppState(state);
  setInputValue("projectIndexRoot", state.settings.projectIndexRoot);
  requireElement("last-index-time").textContent = state.indexStats.lastIndexTime ? formatDate(state.indexStats.lastIndexTime) : "Never";
  requireElement("indexed-file-count").textContent = String(state.indexStats.indexedFileCount);
  requireElement("indexed-asset-count").textContent = String(state.indexStats.indexedAssetCount);
  requireElement("indexed-scene-count").textContent = String(state.indexStats.indexedSceneCount);
  requireElement("index-status-message").textContent = state.indexStats.currentRunSummary;
}

function refreshManualJson(): void {
  try {
    setInputValue("manualRequestJson", JSON.stringify(readManualRequest(), null, 2));
  } catch (error) {
    setInputValue("manualRequestJson", error instanceof Error ? error.message : "Unable to build request.");
  }
}

function readManualRequest(): ManualCommandRequest {
  const command = getInputValue("manualCommand");
  return {
    command,
    params: buildManualParams(command)
  };
}

function buildManualParams(command: string): Record<string, unknown> {
  const params: Record<string, unknown> = {};
  const assetPath = getInputValue("manualAssetPath").trim();
  const actorPath = getInputValue("manualActorPath").trim();
  const maxResults = Number(getInputValue("manualMaxResults")) || 50;

  if (command === "find_assets") {
    params.path = getInputValue("manualFindPath").trim() || "/Game";
    params.name_substring = getInputValue("manualFindName").trim();
    params.class_filter = getInputValue("manualFindClass").trim();
    params.max_results = maxResults;
  } else if (command === "create_biome_asset") {
    if (assetPath) {
      params.asset_path = assetPath;
    }
    params.biome = parseOptionalJson(getInputValue("manualBiomeJson"), "Biome JSON");
  } else if (command === "validate_biome_asset") {
    if (assetPath) {
      params.asset_path = assetPath;
    } else {
      params.biome = parseOptionalJson(getInputValue("manualBiomeJson"), "Biome JSON");
    }
  } else if (command === "landgen_generate_preview" || command === "landgen_spawn_biome_content" || command === "landgen_clear_generated_content") {
    if (actorPath) {
      params.actor = actorPath;
    }
  } else if (command === "compile_blueprint") {
    if (assetPath) {
      params.blueprint_path = assetPath;
    }
  }

  if (isManualMutationCommand(command)) {
    params.dry_run = getChecked("manualDryRun");
    params.approved = getChecked("manualApproved");
    params.permission_level = defaultManualPermission(command);
  }

  return params;
}

function parseOptionalJson(value: string, label: string): unknown {
  const trimmed = value.trim();
  if (!trimmed) {
    return {};
  }
  try {
    return JSON.parse(trimmed);
  } catch {
    throw new Error(`${label} must be valid JSON.`);
  }
}

function isManualMutationCommand(command: string): boolean {
  return [
    "create_biome_asset",
    "landgen_generate_preview",
    "landgen_spawn_biome_content",
    "landgen_clear_generated_content",
    "compile_blueprint",
    "save_generated_assets"
  ].includes(command);
}

function defaultManualPermission(command: string): string {
  if (command === "create_biome_asset" || command === "save_generated_assets") {
    return "asset_mutation";
  }
  if (command === "compile_blueprint") {
    return "blueprint_mutation";
  }
  return "editor_mutation";
}

function readPrototypeInput(): GamePrototypeWizardInput {
  return {
    gameType: getInputValue("prototypeGameType") as GamePrototypeWizardInput["gameType"],
    scope: getInputValue("prototypeScope") as GamePrototypeWizardInput["scope"],
    gameplayStyle: getInputValue("prototypeGameplayStyle") as GamePrototypeWizardInput["gameplayStyle"],
    setupStyle: getInputValue("prototypeSetupStyle") as GamePrototypeWizardInput["setupStyle"]
  };
}

function renderPrototypePlan(plan: GamePrototypePlan | null): void {
  const output = requireElement("prototype-plan-output");
  if (!plan) {
    output.className = "prototype-plan-empty";
    output.textContent = "Create a dry-run plan to see beginner-friendly next steps.";
    return;
  }

  output.className = "prototype-plan";
  output.innerHTML = `
    <p class="eyebrow">Saved ${escapeHtml(formatDate(plan.createdAt))}</p>
    <h3>${escapeHtml(plan.title)}</h3>
    <p>${escapeHtml(plan.summary)}</p>
    ${renderPlanList("Required systems", safeArray(plan.requiredSystems))}
    ${renderPlanList("Suggested generated assets", safeArray(plan.suggestedGeneratedAssets))}
    ${renderPlanList("Suggested map setup", safeArray(plan.suggestedMapSetup))}
    <h4>First playable milestone</h4>
    <p>${escapeHtml(plan.firstPlayableMilestone)}</p>
    <h4>Next recommended action</h4>
    <p>${escapeHtml(plan.nextRecommendedAction)}</p>
    ${renderPlanList("Safety notes", safeArray(plan.safetyNotes))}
    <pre>${escapeHtml(JSON.stringify(plan, null, 2))}</pre>
  `;
}

function renderPlayableWorkflow(workflow: PlayableLevelWorkflowResult | null): void {
  const summary = requireElement("playable-summary");
  const fixList = requireElement("playable-fix-list");
  const technical = requireElement("playable-technical");

  if (!workflow) {
    summary.innerHTML = `
      ${renderPlayableSummaryItem("Player can spawn", "Unknown", "Run a read-only level check first.")}
      ${renderPlayableSummaryItem("Player can move", "Unknown", "Run a read-only level check first.")}
      ${renderPlayableSummaryItem("Level has lighting", "Unknown", "Run a read-only level check first.")}
      ${renderPlayableSummaryItem("Enemies can move", "Unknown", "Run a read-only level check first.")}
      ${renderPlayableSummaryItem("Objective exists", "Unknown", "Run a read-only level check first.")}
    `;
    fixList.innerHTML = `<p class="message">No fix plan yet. Run the level check to generate beginner-friendly suggestions.</p>`;
    technical.textContent = "Technical details are hidden in Beginner Mode.";
    return;
  }

  const summaryItems = [
    renderPlayableSummaryItem(
      "Player can spawn",
      workflow.summary.playerCanSpawn,
      workflow.summary.playerCanSpawn === "No"
        ? "Player cannot spawn: no PlayerStart was detected."
        : "A player spawn point appears to exist."
    ),
    renderPlayableSummaryItem(
      "Player can move",
      workflow.summary.playerCanMove,
      workflow.summary.playerCanMove === "Unknown"
        ? "Movement setup could not be fully verified."
        : "A player pawn, character, or GameMode was detected."
    ),
    renderPlayableSummaryItem(
      "Level has lighting",
      workflow.summary.levelHasLighting,
      workflow.summary.levelHasLighting === "No"
        ? "No basic lighting actor was detected."
        : "Lighting appears to be present."
    ),
    renderPlayableSummaryItem(
      "Enemies can move",
      workflow.summary.enemiesCanMove,
      workflow.summary.enemiesCanMove === "Unknown"
        ? "No obvious AI movement requirement was detected."
        : "AI movement readiness was checked against NavMesh data."
    ),
    renderPlayableSummaryItem(
      "Objective exists",
      workflow.summary.objectiveExists,
      workflow.summary.objectiveExists === "No"
        ? "No simple objective marker was detected."
        : "An objective-like actor or tag was detected."
    )
  ];
  summary.innerHTML = summaryItems.join("");

  const fixPlan = safeArray(workflow.fixPlan);
  fixList.innerHTML = fixPlan.length
    ? fixPlan.map((fix) => renderPlayableFixCard(fix)).join("")
    : `<p class="message">This level already passes the beginner checks. No safe fixes are suggested.</p>`;

  technical.textContent = JSON.stringify(
    {
      checkedAt: workflow.checkedAt,
      mapName: workflow.mapName,
      readOnly: workflow.readOnly,
      summary: workflow.summary,
      fixPlan,
      technicalDetails: workflow.technicalDetails
    },
    null,
    2
  );

  for (const button of Array.from(document.querySelectorAll<HTMLButtonElement>("[data-select-playable-fix]"))) {
    button.addEventListener("click", () => {
      selectedPlayableFixId = button.dataset.selectPlayableFix ?? "";
      renderPlayableWorkflow(state.playableWorkflow);
      const fix = findSelectedPlayableFix();
      requireElement("playable-explanation").textContent = fix
        ? `${fix.label}: ${fix.beginnerExplanation}`
        : "Select a fix to see a beginner-friendly explanation.";
    });
  }

  const selected = findSelectedPlayableFix();
  requireElement("playable-explanation").textContent = selected
    ? `${selected.label}: ${selected.beginnerExplanation}`
    : "Select a fix to see a beginner explanation.";
}

function renderPlayableSummaryItem(label: string, value: string, detail: string): string {
  return `
    <div class="playable-summary-item">
      <strong>${escapeHtml(label)}: ${escapeHtml(value)}</strong>
      <span>${escapeHtml(detail)}</span>
    </div>
  `;
}

function renderPlayableFixCard(fix: PlayableFixItem): string {
  const selected = fix.id === selectedPlayableFixId ? " selected" : "";
  const metadata = [
    `Command: ${escapeHtml(fix.technicalCommand)}`,
    `Risk: ${escapeHtml(fix.riskLevel)}`,
    `Generated assets only: ${fix.modifiesGeneratedAssetsOnly ? "Yes" : "No"}`,
    `Dry-run: ${fix.dryRunAvailable ? "Yes" : "No"}`
  ].join(" · ");

  return `
    <article class="playable-fix-card${selected}">
      <strong>${escapeHtml(fix.label)}</strong>
      <span>${escapeHtml(fix.beginnerExplanation)}</span>
      <small>${metadata}</small>
      <button data-select-playable-fix="${escapeHtml(fix.id)}">${selected ? "Selected" : "Select Fix"}</button>
    </article>
  `;
}

function findSelectedPlayableFix(): PlayableFixItem | undefined {
  return safeArray(state.playableWorkflow?.fixPlan).find((fix) => fix.id === selectedPlayableFixId);
}

function showPlayableDryRunResponse(responses: unknown[]): void {
  const response = requireElement("playable-dryrun-response");
  response.hidden = (requireElement("playable-technical") as HTMLPreElement).hidden;
  response.textContent = JSON.stringify(
    {
      dryRun: true,
      requiresApproval: true,
      responses
    },
    null,
    2
  );
}

function renderPlanList(title: string, values: string[]): string {
  return `
    <h4>${escapeHtml(title)}</h4>
    <ul>
      ${values.map((value) => `<li>${escapeHtml(value)}</li>`).join("")}
    </ul>
  `;
}

function renderRecipeCards(): void {
  const container = requireElement("recipe-cards");
  const recipes = safeArray(state.recipes);
  container.innerHTML = recipes.length
    ? recipes
      .map(
        (recipe) => `
        <article class="recipe-card">
          <strong>${escapeHtml(recipe.title)}</strong>
          <span>${escapeHtml(recipe.beginnerDescription)}</span>
          <small>${escapeHtml(recipe.category)} · ${escapeHtml(recipe.difficulty)} · ${escapeHtml(recipe.requiredPermissionLevel)}</small>
          <button data-open-recipe="${escapeHtml(recipe.recipeId)}">Open Recipe</button>
        </article>
      `
      )
      .join("")
    : `<p class="message">No recipes are loaded yet. Manual Command Mode is still available.</p>`;

  for (const button of Array.from(document.querySelectorAll<HTMLButtonElement>("[data-open-recipe]"))) {
    button.addEventListener("click", () => {
      selectedRecipeId = button.dataset.openRecipe ?? "";
      showPage("recipes");
      renderSelectedRecipe();
    });
  }

  if (!selectedRecipeId && recipes[0]) {
    selectedRecipeId = recipes[0].recipeId;
  }
}

function renderSelectedRecipe(): void {
  const recipe = safeArray(state.recipes).find((item) => item.recipeId === selectedRecipeId);
  const form = requireElement("recipe-form");
  if (!recipe) {
    requireElement("recipe-detail-title").textContent = "Choose a recipe";
    requireElement("recipe-detail-description").textContent = "Open a recipe card to see its guided form and generated command plan.";
    form.innerHTML = "";
    requireElement("recipe-command-plan").textContent = "Select a recipe to preview the command plan.";
    return;
  }

  requireElement("recipe-detail-title").textContent = recipe.title;
  requireElement("recipe-detail-description").textContent = recipe.description;
  requireElement("recipe-approval-summary").textContent = recipe.beginnerApprovalSummary;
  form.innerHTML = safeArray(recipe.formFields).map((field) => renderRecipeField(recipe.recipeId, field)).join("");
  requireElement("recipe-command-plan").textContent = JSON.stringify(
    {
      recipeId: recipe.recipeId,
      supportsDryRun: recipe.supportsDryRun,
      requiresApproval: recipe.requiresApproval,
      outputSummary: recipe.outputSummary,
      generatedCommandPlan: recipe.generatedCommandPlan
    },
    null,
    2
  );
  const latestRun = safeArray(state.recentRecipeRuns).find((run) => run.recipeId === recipe.recipeId);
  renderRecipeResultPanel(latestRun);
}

function renderRecipeField(recipeId: string, field: RecipeDefinition["formFields"][number]): string {
  const elementId = recipeFieldElementId(recipeId, field.id);
  if (field.type === "select") {
    return `
      <label>${escapeHtml(field.label)}
        <select id="${escapeHtml(elementId)}">
          ${(field.options ?? []).map((option) => `<option ${option === field.defaultValue ? "selected" : ""}>${escapeHtml(option)}</option>`).join("")}
        </select>
        <small>${escapeHtml(field.helpText)}</small>
      </label>
    `;
  }
  if (field.type === "checkbox") {
    return `
      <label class="check">
        <input id="${escapeHtml(elementId)}" type="checkbox" ${field.defaultValue === true ? "checked" : ""} />
        ${escapeHtml(field.label)}
      </label>
    `;
  }
  if (field.type === "vector-location") {
    const value = Array.isArray(field.defaultValue) ? field.defaultValue : [0, 0, 0];
    return `
      <label>${escapeHtml(field.label)}
        <div class="vector-fields">
          <input id="${escapeHtml(elementId)}-x" type="number" value="${escapeHtml(String(value[0]))}" placeholder="X" />
          <input id="${escapeHtml(elementId)}-y" type="number" value="${escapeHtml(String(value[1]))}" placeholder="Y" />
          <input id="${escapeHtml(elementId)}-z" type="number" value="${escapeHtml(String(value[2]))}" placeholder="Z" />
        </div>
        <small>${escapeHtml(field.helpText)}</small>
      </label>
    `;
  }
  const inputType = field.type === "number" ? "number" : "text";
  return `
    <label>${escapeHtml(field.label)}
      <input id="${escapeHtml(elementId)}" type="${inputType}" value="${escapeHtml(String(field.defaultValue))}" />
      <small>${escapeHtml(field.helpText)}</small>
    </label>
  `;
}

function renderRecipeRuns(): void {
  const body = requireElement("recipe-runs-body");
  const runs = safeArray(state.recentRecipeRuns);
  body.innerHTML = runs.length
    ? runs
      .map(
        (run) => `
            <tr>
              <td>${escapeHtml(formatDate(run.timestamp))}</td>
              <td>${escapeHtml(run.recipeTitle)}</td>
              <td>${escapeHtml(run.status)}</td>
              <td>${escapeHtml(run.summary)}</td>
            </tr>
          `
      )
      .join("")
    : `<tr><td colspan="4">No recipe runs yet.</td></tr>`;
}

function renderRecipeResultPanel(run: RecipeRunRecord | undefined): void {
  const panel = requireElement("recipe-result-panel");
  if (!run?.resultPanel) {
    panel.innerHTML = `
      <h3>Post-Run Result</h3>
      <p class="message">Run a recipe dry-run or approved apply to see results.</p>
    `;
    return;
  }

  panel.innerHTML = `
    <h3>Post-Run Result</h3>
    <dl class="result-list">
      <dt>Blueprint created</dt>
      <dd>${escapeHtml(run.resultPanel.blueprintCreated)}</dd>
      <dt>Variables added</dt>
      <dd>${escapeHtml(run.resultPanel.variablesAdded)}</dd>
      <dt>Compiled successfully or failed</dt>
      <dd>${escapeHtml(run.resultPanel.compiledStatus)}</dd>
      <dt>Placed in level</dt>
      <dd>${escapeHtml(run.resultPanel.placedInLevel)}</dd>
      <dt>Next recommended step</dt>
      <dd>${escapeHtml(run.resultPanel.nextRecommendedStep)}</dd>
    </dl>
    <p class="message">${escapeHtml(run.resultPanel.missingBehaviorNote)}</p>
  `;
}

function readRecipeValues(): Record<string, unknown> {
  const recipe = safeArray(state.recipes).find((item) => item.recipeId === requireSelectedRecipeId());
  if (!recipe) {
    return {};
  }
  const values: Record<string, unknown> = {};
  for (const field of safeArray(recipe.formFields)) {
    const elementId = recipeFieldElementId(recipe.recipeId, field.id);
    if (field.type === "checkbox") {
      values[field.id] = getChecked(elementId);
    } else if (field.type === "number") {
      values[field.id] = Number(getInputValue(elementId));
    } else if (field.type === "vector-location") {
      values[field.id] = [Number(getInputValue(`${elementId}-x`)), Number(getInputValue(`${elementId}-y`)), Number(getInputValue(`${elementId}-z`))];
    } else {
      values[field.id] = getInputValue(elementId);
    }
  }
  return values;
}

function requireSelectedRecipeId(): string {
  if (!selectedRecipeId) {
    throw new Error("Choose a recipe first.");
  }
  return selectedRecipeId;
}

function recipeFieldElementId(recipeId: string, fieldId: string): string {
  return `recipe-${recipeId}-${fieldId}`;
}

function readModelForm(): ModelProfileInput {
  const id = getInputValue("modelProfileId");
  const baseUrl = validateLocalEndpointUrl(getInputValue("modelBaseUrl"), "Model base URL");
  return {
    ...(id ? { id } : {}),
    providerType: getInputValue("modelProviderType") as ModelProfileInput["providerType"],
    displayName: getInputValue("modelDisplayName"),
    baseUrl,
    modelName: getInputValue("modelName"),
    contextLength: Number(getInputValue("modelContextLength")),
    temperature: Number(getInputValue("modelTemperature")),
    maxOutputTokens: Number(getInputValue("modelMaxOutputTokens")),
    classification: getInputValue("modelClassification") as ModelProfileInput["classification"],
    enabled: true
  };
}

function readWizardProfile(): ModelProfileInput {
  const providerType = getInputValue("wizardProviderType") as ModelProfileInput["providerType"];
  const baseUrl = validateLocalEndpointUrl(getInputValue("wizardBaseUrl"), "Wizard base URL");
  return {
    providerType,
    displayName: getInputValue("wizardDisplayName").trim() || providerLabel(providerType),
    baseUrl,
    modelName: getInputValue("wizardModelName"),
    contextLength: Number(getInputValue("wizardContextLength")),
    temperature: Number(getInputValue("wizardTemperature")),
    maxOutputTokens: Number(getInputValue("wizardMaxOutputTokens")),
    classification: getInputValue("wizardClassification") as ModelProfileInput["classification"],
    supportsStreaming: providerType !== "openai-compatible",
    supportsTools: false,
    enabled: true
  };
}

function renderWizardDetectedModels(models: string[]): void {
  const select = requireElement("wizardDetectedModels") as HTMLSelectElement;
  const safeModels = safeArray(models);
  select.innerHTML = safeModels.length
    ? safeModels.map((model) => `<option value="${escapeHtml(model)}">${escapeHtml(model)}</option>`).join("")
    : `<option value="">No model list available; enter manually</option>`;
}

function fillModelForm(profile: ModelProfile): void {
  setInputValue("modelProfileId", profile.id);
  setInputValue("modelProviderType", profile.providerType);
  setInputValue("modelDisplayName", profile.displayName);
  setInputValue("modelBaseUrl", profile.baseUrl);
  setInputValue("modelName", profile.modelName);
  setInputValue("modelContextLength", String(profile.contextLength));
  setInputValue("modelTemperature", String(profile.temperature));
  setInputValue("modelMaxOutputTokens", String(profile.maxOutputTokens));
  setInputValue("modelClassification", profile.classification);
}

function clearModelForm(): void {
  setInputValue("modelProfileId", "");
  setInputValue("modelProviderType", "ollama");
  setInputValue("modelDisplayName", "Ollama");
  setInputValue("modelBaseUrl", "http://127.0.0.1:11434");
  setInputValue("modelName", "");
  setInputValue("modelContextLength", "4096");
  setInputValue("modelTemperature", "0.2");
  setInputValue("modelMaxOutputTokens", "512");
  setInputValue("modelClassification", "offline");
}

function getModelStatusCard(): StatusCard {
  state = mergeDesktopAppState(state);
  const active = safeArray(state.modelProfiles).find((profile) => profile.id === state.settings.activeModelProfileId);
  if (!active) {
    return {
      state: "unknown",
      label: "Manual Command Mode",
      detail: "No local model is configured. Manual Command Mode is still available."
    };
  }
  return {
    state: active.enabled ? "online" : "warning",
    label: active.displayName,
    detail: `${providerLabel(active.providerType)} · ${active.classification} · ${active.lastTestStatus || "Not tested"}`
  };
}

function getModelDetailStatusCard(): StatusCard {
  state = mergeDesktopAppState(state);
  const active = safeArray(state.modelProfiles).find((profile) => profile.id === state.settings.activeModelProfileId);
  if (!active) {
    return {
      state: "unknown",
      label: "Active model profile",
      detail: "No local model is configured. Manual Command Mode is still available."
    };
  }

  return {
    state: active.lastTestStatus === "Connected" ? "online" : "warning",
    label: "Active model profile",
    detail: `${active.displayName} · ${providerLabel(active.providerType)} · ${active.classification} · ${active.lastTestStatus || "Not tested"}`
  };
}

function getIndexStatusCard(): StatusCard {
  state = mergeDesktopAppState(state);
  if (state.indexStats.isRunning) {
    return { state: "warning", label: "Indexing running", detail: state.indexStats.currentRunSummary };
  }

  return {
    state: state.indexStats.indexedFileCount > 0 ? "online" : "unknown",
    label: state.indexStats.indexedFileCount > 0 ? "Local index ready" : "No local index",
    detail: `${state.indexStats.indexedFileCount} files · ${state.indexStats.indexedAssetCount} assets`
  };
}

function getSetupChecklistStatusCard(): StatusCard {
  state = mergeDesktopAppState(state);
  const unrealReady = state.unrealStatus.state === "online";
  const modelReady = safeArray(state.modelProfiles).some((profile) => profile.id === state.settings.activeModelProfileId);
  const mcpReady = state.mcpStatus.state === "online";
  const readyCount = [unrealReady, modelReady, mcpReady].filter(Boolean).length;
  return {
    state: unrealReady ? "online" : "warning",
    label: `Setup checklist: ${readyCount}/3 optional services ready`,
    detail: [
      unrealReady ? "Unreal connected" : "Unreal bridge optional: start it from Tools > Revolt Bridge",
      modelReady ? "Local model selected" : "Local model optional: Manual Command Mode is available",
      mcpReady ? "MCP build found" : "MCP optional: configure/build only when needed"
    ].join(" · ")
  };
}

function renderSetupChecklistAction(): void {
  const card = requireElement("card-setup-checklist");
  if (card.querySelector("[data-open-connect-unreal]")) {
    return;
  }
  const button = document.createElement("button");
  button.textContent = "Connect to Unreal";
  button.dataset.openConnectUnreal = "true";
  button.className = "primary";
  button.addEventListener("click", () => showPage("connect-unreal"));
  card.appendChild(button);
}

function getOfflineStatus(): StatusCard {
  state = mergeDesktopAppState(state);
  return {
    state: state.settings.offlineMode ? "online" : "warning",
    label: state.settings.offlineMode ? "Offline mode enabled" : "Offline mode disabled",
    detail: state.settings.activeModelProfileId ? "Using selected local model profile." : "Manual Command Mode: no model profile selected."
  };
}

function providerLabel(providerType: string): string {
  switch (providerType) {
    case "ollama":
      return "Ollama";
    case "lmstudio":
      return "LM Studio";
    case "llamacpp":
      return "llama.cpp server";
    case "openai-compatible":
      return "Local OpenAI-compatible endpoint";
    default:
      return providerType;
  }
}

function defaultBaseUrl(providerType: string): string {
  switch (providerType) {
    case "ollama":
      return "http://127.0.0.1:11434";
    case "lmstudio":
      return "http://127.0.0.1:1234";
    case "llamacpp":
      return "http://127.0.0.1:8080";
    case "openai-compatible":
      return "http://127.0.0.1:8000";
    default:
      return "http://127.0.0.1:11434";
  }
}

function validateLocalEndpointUrl(value: string, label: string): string {
  let parsed: URL;
  try {
    parsed = new URL(value);
  } catch {
    throw new Error(`${label} must be a valid local URL, such as http://127.0.0.1:11434.`);
  }

  const hostname = parsed.hostname.toLowerCase();
  const isLocal =
    hostname === "localhost" ||
    hostname === "127.0.0.1" ||
    hostname === "::1" ||
    hostname.endsWith(".local") ||
    /^10\.\d{1,3}\.\d{1,3}\.\d{1,3}$/.test(hostname) ||
    /^192\.168\.\d{1,3}\.\d{1,3}$/.test(hostname) ||
    /^172\.(1[6-9]|2\d|3[01])\.\d{1,3}\.\d{1,3}$/.test(hostname);

  if (!isLocal && !state.settings.enableOnlineProviders) {
    throw new Error(`${label} must be localhost or a private LAN address unless Online Providers are explicitly enabled.`);
  }

  return parsed.toString().replace(/\/$/, "");
}

function validateUnrealEndpointUrl(value: string, label: string): string {
  let parsed: URL;
  try {
    parsed = new URL(value);
  } catch {
    throw new Error(`${label} must be a valid URL. Use http://127.0.0.1:8765 unless you changed it in Unreal.`);
  }
  const hostname = parsed.hostname.toLowerCase();
  if (hostname !== "127.0.0.1" && hostname !== "localhost" && !state.settings.enableOnlineProviders) {
    throw new Error(`${label} must use localhost or 127.0.0.1 by default.`);
  }
  return parsed.toString().replace(/\/$/, "");
}

function setConnectionFields(host: string, port: number, updateEndpoint = true): void {
  const safeHost = host || "127.0.0.1";
  const safePort = Number(port) || 8765;
  setInputValue("connectionHost", safeHost);
  setInputValue("connectionPort", String(safePort));
  setInputValue("connectionTimeoutMs", String(state.connectionState.unreal.timeoutMs || 5000));
  if (updateEndpoint) {
    setInputValue("connectionEndpoint", `http://${safeHost}:${safePort}`);
  } else if (!getInputValue("connectionEndpoint")) {
    setInputValue("connectionEndpoint", `http://${safeHost}:${safePort}`);
  }
}

function requireElement(id: string): HTMLElement {
  const element = document.getElementById(id);
  if (!element) {
    console.warn(`[RevoltDesktop] Missing element: ${id}`);
    const fallback = document.createElement("div");
    fallback.id = id;
    fallback.hidden = true;
    document.body.appendChild(fallback);
    notify(`A UI section was missing and has been safely skipped: ${id}`, "warning");
    return fallback;
  }
  return element;
}

function setInputValue(id: string, value: string): void {
  (requireElement(id) as HTMLInputElement | HTMLSelectElement).value = value;
}

function getInputValue(id: string): string {
  return (requireElement(id) as HTMLInputElement | HTMLSelectElement).value;
}

function setChecked(id: string, value: boolean): void {
  (requireElement(id) as HTMLInputElement).checked = value;
}

function getChecked(id: string): boolean {
  return (requireElement(id) as HTMLInputElement).checked;
}

function safeArray<T>(value: T[] | null | undefined): T[] {
  return Array.isArray(value) ? value : [];
}

function bindSafeClick(id: string, successMessage: string, action: () => void | Promise<void>): void {
  requireElement(id).addEventListener("click", () => {
    void runUiAction(successMessage, action);
  });
}

async function runUiAction(successMessage: string, action: () => void | Promise<void>): Promise<void> {
  try {
    await action();
    if (successMessage) {
      notify(successMessage, "info");
    }
  } catch (error) {
    notify(errorMessage(error, "That action is unavailable right now."), "warning");
  }
}

function notify(message: string, level: "info" | "warning" | "error" = "info"): void {
  const stack = document.getElementById("notification-stack");
  if (!stack || !message.trim()) {
    return;
  }
  const item = document.createElement("div");
  item.className = `notification ${level}`;
  item.textContent = message;
  stack.prepend(item);
  window.setTimeout(() => item.remove(), 5200);
}

function formatDate(timestamp: string): string {
  if (!timestamp) {
    return "Never";
  }
  const date = new Date(timestamp);
  return Number.isNaN(date.getTime()) ? "Unknown" : date.toLocaleString();
}

function escapeHtml(value: unknown): string {
  return String(value ?? "").replace(/[&<>"']/g, (character) => {
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

function errorMessage(error: unknown, fallback: string): string {
  const message = error instanceof Error && error.message ? error.message : "";
  if (message.toLowerCase().includes("cannot read properties of undefined")) {
    return fallback;
  }
  return message || fallback;
}

function renderStartupError(message: string): void {
  const element = document.getElementById("beginner-action-result") ?? document.getElementById("settings-message");
  if (element) {
    element.textContent = message;
  }
}
