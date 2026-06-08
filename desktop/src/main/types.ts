export type PermissionMode = "ReadOnly" | "SafeEdit" | "ProjectEdit" | "Dangerous";
export type ModelProviderType = "ollama" | "lmstudio" | "llamacpp" | "openai-compatible";
export type ModelProviderClassification = "offline" | "local-network" | "online";
export type ExperienceMode = "beginner" | "advanced";
export type PrototypeGameType =
  | "First-person shooter"
  | "Third-person shooter"
  | "Horror game"
  | "Survival game"
  | "Wave-based zombie game"
  | "Exploration game"
  | "Platformer"
  | "Top-down shooter"
  | "RPG prototype"
  | "Blank project";
export type PrototypeScope = "Tiny test room" | "Small arena" | "Linear level" | "Open area" | "Prototype only";
export type PrototypeGameplayStyle = "No combat" | "Guns" | "Melee" | "Magic" | "Stealth" | "Survival crafting" | "Exploration/objectives";
export type PrototypeSetupStyle = "Beginner-friendly Blueprint setup" | "Data-driven generated setup" | "Advanced C++-friendly setup";
export type RecipeCategory = "Player" | "Enemy" | "Weapon" | "Pickup" | "UI" | "Level" | "Biome" | "Objective" | "Utility";
export type RecipeDifficulty = "Easy" | "Medium" | "Advanced";
export type RecipeFieldType = "text" | "number" | "select" | "checkbox" | "asset-path" | "actor-class" | "vector-location";

export interface RuntimeSettings {
  ueBridgeHost: string;
  ueBridgePort: number;
  mcpServerCommand: string;
  mcpServerPath: string;
  defaultPermissionMode: PermissionMode;
  enableOnlineProviders: boolean;
  offlineMode: boolean;
  experienceMode: ExperienceMode;
  activeModelProfileId: string;
  projectIndexRoot: string;
}

export type UnrealConnectionTestStatus = "connected" | "unreachable" | "invalid_response" | "timeout" | "invalid_endpoint" | "unknown_error";

export interface UnrealConnectionState {
  status: "disconnected" | "checking" | "connected" | "error";
  endpoint: string;
  host: string;
  port: number;
  timeoutMs: number;
  lastCheckedAt: string | null;
  lastSuccessAt: string | null;
  lastError: string | null;
  lastResponse: unknown | null;
}

export interface UnrealConnectionTestResult {
  status: UnrealConnectionTestStatus;
  ok: boolean;
  endpoint: string;
  message: string;
  detail: string;
  checkedAt: string;
  response: unknown | null;
}

export interface ConnectionServiceState {
  status: string;
  endpoint: string | null;
  lastCheckedAt: string | null;
  lastError: string | null;
}

export interface DesktopConnectionState {
  unreal: UnrealConnectionState;
  mcp: ConnectionServiceState;
  model: ConnectionServiceState & {
    activeProfile: ModelProfile | null;
  };
}

export interface RuntimeConfig {
  dangerousModeAvailable: boolean;
}

export interface StatusCard {
  state: "online" | "offline" | "unknown" | "warning";
  label: string;
  detail: string;
}

export interface CommandHistoryEntry {
  id: string;
  timestamp: string;
  commandName: string;
  target: string;
  status: string;
  resultSummary: string;
}

export interface CommandHistoryExportEntry extends CommandHistoryEntry {
  payload: unknown;
}

export interface ManualCommandRequest {
  command: string;
  params: Record<string, unknown>;
}

export interface ManualCommandResult {
  request: {
    id: string;
    command: string;
    params: Record<string, unknown>;
  };
  response: unknown;
  ok: boolean;
  summary: string;
}

export interface ApprovalEvent {
  id: string;
  timestamp: string;
  commandId: string;
  commandName: string;
  target: string;
  decision: string;
}

export interface CompletionRequest {
  prompt: string;
  systemPrompt?: string;
  model: string;
  temperature: number;
  maxOutputTokens: number;
}

export interface CompletionResponse {
  text: string;
  providerId: string;
  model: string;
  finishReason?: string;
}

export interface ModelProvider {
  id: string;
  name: string;
  classification: ModelProviderClassification;
  supportsStreaming: boolean;
  supportsTools: boolean;
  complete(request: CompletionRequest): Promise<CompletionResponse>;
}

export interface ModelProfile {
  id: string;
  providerType: ModelProviderType;
  displayName: string;
  baseUrl: string;
  modelName: string;
  contextLength: number;
  temperature: number;
  maxOutputTokens: number;
  classification: ModelProviderClassification;
  supportsStreaming: boolean;
  supportsTools: boolean;
  enabled: boolean;
  lastTestedAt: string;
  lastTestStatus: string;
  createdAt: string;
  updatedAt: string;
}

export interface ModelProfileInput {
  id?: string;
  providerType: ModelProviderType;
  displayName: string;
  baseUrl: string;
  modelName: string;
  contextLength: number;
  temperature: number;
  maxOutputTokens: number;
  classification: ModelProviderClassification;
  supportsStreaming?: boolean;
  supportsTools?: boolean;
  enabled?: boolean;
}

export interface GamePrototypeWizardInput {
  gameType: PrototypeGameType;
  scope: PrototypeScope;
  gameplayStyle: PrototypeGameplayStyle;
  setupStyle: PrototypeSetupStyle;
}

export interface GamePrototypePlan {
  id: string;
  createdAt: string;
  input: GamePrototypeWizardInput;
  title: string;
  summary: string;
  requiredSystems: string[];
  suggestedGeneratedAssets: string[];
  suggestedMapSetup: string[];
  firstPlayableMilestone: string;
  nextRecommendedAction: string;
  safetyNotes: string[];
  doesExecuteChanges: false;
}

export interface RecipeFormField {
  id: string;
  label: string;
  type: RecipeFieldType;
  required: boolean;
  defaultValue: string | number | boolean | [number, number, number];
  helpText: string;
  options?: string[];
}

export interface RecipeCommandPlanStep {
  label: string;
  command: string;
  params: Record<string, unknown>;
  supported: boolean;
  reason?: string;
}

export interface RecipeResultPanel {
  blueprintCreated: string;
  variablesAdded: string;
  compiledStatus: string;
  placedInLevel: string;
  nextRecommendedStep: string;
  missingBehaviorNote: string;
}

export interface RecipeDefinition {
  recipeId: string;
  title: string;
  description: string;
  beginnerDescription: string;
  category: RecipeCategory;
  difficulty: RecipeDifficulty;
  requiredPermissionLevel: string;
  outputSummary: string;
  beginnerApprovalSummary: string;
  formFields: RecipeFormField[];
  generatedCommandPlan: RecipeCommandPlanStep[];
  supportsDryRun: boolean;
  requiresApproval: boolean;
}

export interface RecipeRunRecord {
  id: string;
  recipeId: string;
  recipeTitle: string;
  timestamp: string;
  status: string;
  dryRun: boolean;
  approved: boolean;
  summary: string;
  commandPlan: RecipeCommandPlanStep[];
  response: unknown;
  resultPanel?: RecipeResultPanel;
}

export type PlayableAnswer = "Yes" | "No" | "Unknown";

export interface PlayableLevelSummary {
  playerCanSpawn: PlayableAnswer;
  playerCanMove: PlayableAnswer;
  levelHasLighting: PlayableAnswer;
  enemiesCanMove: PlayableAnswer;
  objectiveExists: PlayableAnswer;
}

export interface PlayableFixItem {
  id: string;
  label: string;
  beginnerExplanation: string;
  technicalCommand: string;
  params: Record<string, unknown>;
  riskLevel: "Low" | "Medium" | "High" | "Dangerous";
  modifiesGeneratedAssetsOnly: boolean;
  dryRunAvailable: boolean;
  selected: boolean;
}

export interface PlayableLevelWorkflowResult {
  checkedAt: string;
  ok: boolean;
  mapName: string;
  readOnly: true;
  summary: PlayableLevelSummary;
  fixPlan: PlayableFixItem[];
  technicalDetails: unknown;
  message: string;
}

export interface ProviderDetectionResult {
  providerType: ModelProviderType;
  baseUrl: string;
  classification: ModelProviderClassification;
  status: "Connected" | "Not running" | "Endpoint unreachable" | "Model missing" | "Invalid response format" | "Timeout" | "Unknown error";
  message: string;
  models: string[];
}

export interface IndexStats {
  isRunning: boolean;
  lastIndexTime: string;
  indexedFileCount: number;
  indexedAssetCount: number;
  indexedSceneCount: number;
  currentRunStatus: string;
  currentRunSummary: string;
}

export interface IndexRunRecord {
  id: string;
  projectRoot: string;
  startedAt: string;
  finishedAt: string;
  status: string;
  filesIndexed: number;
  assetsIndexed: number;
  scenesIndexed: number;
  summary: string;
  errorMessage: string;
}

export interface IndexedFileRecord {
  id: string;
  runId: string;
  projectRoot: string;
  filePath: string;
  relativePath: string;
  extension: string;
  sizeBytes: number;
  modifiedAt: string;
  sha256: string;
  summary: string;
  indexedAt: string;
}

export interface DesktopAppState {
  settings: RuntimeSettings;
  runtimeConfig: RuntimeConfig;
  unrealStatus: StatusCard;
  mcpStatus: StatusCard;
  offlineStatus: StatusCard;
  history: CommandHistoryEntry[];
  approvals: ApprovalEvent[];
  modelProfiles: ModelProfile[];
  activeModelProfile: ModelProfile | null;
  indexStats: IndexStats;
  latestPrototypePlan: GamePrototypePlan | null;
  playableWorkflow: PlayableLevelWorkflowResult | null;
  recipes: RecipeDefinition[];
  recentRecipeRuns: RecipeRunRecord[];
  connectionState: DesktopConnectionState;
  currentProjectSummary: Record<string, unknown> | null;
  auditResults: unknown | null;
  tutorials: string[];
  errorMessages: string[];
}
