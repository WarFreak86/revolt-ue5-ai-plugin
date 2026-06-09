import type { CompletionRequest, CompletionResponse, ModelProfile, ModelProvider, ModelProviderClassification, ModelProviderType, ProviderDetectionResult } from "./types.js";

export interface ProviderTestResult {
  ok: boolean;
  message: string;
  status: ProviderDetectionResult["status"];
  responseText?: string;
}

const LOCAL_HOSTS = new Set(["localhost", "127.0.0.1", "::1"]);
export const SAFE_LOCAL_MODEL_TEST_PROMPT = "Reply with the word READY and a one-sentence description of your coding capability.";

export function createModelProvider(profile: ModelProfile): ModelProvider {
  validateOfflineProfile(profile);

  const provider: Omit<ModelProvider, "complete"> = {
    id: profile.providerType,
    name: getProviderName(profile.providerType),
    classification: profile.classification,
    supportsStreaming: profile.providerType !== "openai-compatible",
    supportsTools: false
  };

  return {
    ...provider,
    complete: (request) => completeWithProvider(profile, request)
  };
}

export async function testModelProfile(profile: ModelProfile): Promise<ProviderTestResult> {
  try {
    const provider = createModelProvider(profile);
    const response = await provider.complete({
      model: profile.modelName,
      prompt: SAFE_LOCAL_MODEL_TEST_PROMPT,
      temperature: 0,
      maxOutputTokens: 32
    });
    const trimmed = response.text.trim();
    return {
      ok: trimmed.length > 0,
      status: trimmed.length > 0 ? "Connected" : "Invalid response format",
      message: trimmed.length > 0 ? `Connected to ${provider.name}. Safe local test prompt completed.` : `${provider.name} returned an empty response.`,
      responseText: trimmed
    };
  } catch (error) {
    const message = error instanceof Error ? error.message : "Model connection failed.";
    return { ok: false, status: classifyProviderError(message), message };
  }
}

export async function detectProviderEndpoint(providerType: ModelProviderType, baseUrl: string): Promise<ProviderDetectionResult> {
  try {
    parseLocalUrl(baseUrl);
    const models = await fetchModelList(providerType, baseUrl);
    return {
      providerType,
      baseUrl,
      classification: "offline",
      status: "Connected",
      message: models.length > 0 ? `Connected. Found ${models.length} model(s).` : "Connected. Model list was empty; enter a model name manually.",
      models
    };
  } catch (error) {
    const message = error instanceof Error ? error.message : "Unknown local model detection error.";
    return {
      providerType,
      baseUrl,
      classification: "offline",
      status: classifyProviderError(message),
      message,
      models: []
    };
  }
}

export async function summarizeProjectWithModel(profile: ModelProfile, projectSummary: unknown): Promise<CompletionResponse> {
  const provider = createModelProvider(profile);
  return provider.complete({
    model: profile.modelName,
    temperature: profile.temperature,
    maxOutputTokens: profile.maxOutputTokens,
    systemPrompt: "You summarize Unreal Engine project metadata for a local offline desktop tool. Be concise and do not invent facts.",
    prompt: `Summarize this Unreal project summary in plain language:\n\n${JSON.stringify(projectSummary, null, 2)}`
  });
}

function validateOfflineProfile(profile: ModelProfile): void {
  if (profile.classification === "online") {
    throw new Error("Online providers are not available in Phase 8.");
  }

  const parsed = parseLocalUrl(profile.baseUrl);
  const hostname = parsed.hostname.toLowerCase();
  const isLocal = isAllowedLocalOrPrivateHost(hostname);

  if (!isLocal) {
    throw new Error("Model provider URL must be localhost, 127.0.0.1, ::1, .local, or a private LAN address.");
  }
}

async function completeWithProvider(profile: ModelProfile, request: CompletionRequest): Promise<CompletionResponse> {
  switch (profile.providerType) {
    case "ollama":
      return completeOllama(profile, request);
    case "lmstudio":
    case "openai-compatible":
      return completeOpenAiCompatible(profile, request);
    case "llamacpp":
      return completeLlamaCpp(profile, request);
    default:
      throw new Error("Unsupported model provider.");
  }
}

async function completeOllama(profile: ModelProfile, request: CompletionRequest): Promise<CompletionResponse> {
  const json = await postJson<Record<string, unknown>>(`${profile.baseUrl}/api/generate`, {
    model: request.model,
    prompt: combinePrompts(request),
    stream: false,
    options: {
      temperature: request.temperature,
      num_predict: request.maxOutputTokens,
      num_ctx: profile.contextLength
    }
  });

  return {
    text: typeof json.response === "string" ? json.response : "",
    providerId: profile.providerType,
    model: request.model,
    finishReason: typeof json.done_reason === "string" ? json.done_reason : undefined
  };
}

async function completeOpenAiCompatible(profile: ModelProfile, request: CompletionRequest): Promise<CompletionResponse> {
  const json = await postJson<{ choices?: Array<{ message?: { content?: string; }; finish_reason?: string; }>; }>(`${profile.baseUrl}/v1/chat/completions`, {
    model: request.model,
    messages: [
      ...(request.systemPrompt ? [{ role: "system", content: request.systemPrompt }] : []),
      { role: "user", content: request.prompt }
    ],
    temperature: request.temperature,
    max_tokens: request.maxOutputTokens,
    stream: false
  });
  const choice = json.choices?.[0];

  return {
    text: choice?.message?.content ?? "",
    providerId: profile.providerType,
    model: request.model,
    finishReason: choice?.finish_reason
  };
}

async function completeLlamaCpp(profile: ModelProfile, request: CompletionRequest): Promise<CompletionResponse> {
  const json = await postJson<Record<string, unknown>>(`${profile.baseUrl}/completion`, {
    prompt: combinePrompts(request),
    temperature: request.temperature,
    n_predict: request.maxOutputTokens,
    stream: false
  });

  return {
    text: typeof json.content === "string" ? json.content : "",
    providerId: profile.providerType,
    model: request.model
  };
}

async function postJson<T>(url: string, body: unknown): Promise<T> {
  const parsed = parseLocalUrl(url);
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 15000);

  try {
    const response = await fetch(parsed.toString(), {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(body),
      signal: controller.signal
    });

    const text = await response.text();
    const json = text.length > 0 ? JSON.parse(text) : {};

    if (!response.ok) {
      throw new Error(`Local model endpoint returned HTTP ${response.status}: ${text.slice(0, 180)}`);
    }

    return json as T;
  } finally {
    clearTimeout(timeout);
  }
}

function parseLocalUrl(url: string): URL {
  const parsed = new URL(url);
  const hostname = parsed.hostname.toLowerCase();
  if (!isAllowedLocalOrPrivateHost(hostname)) {
    throw new Error("Refusing non-local model endpoint.");
  }
  return parsed;
}

async function fetchModelList(providerType: ModelProviderType, baseUrl: string): Promise<string[]> {
  if (providerType === "ollama") {
    const json = await getJson<{ models?: Array<{ name?: string; }>; }>(`${baseUrl}/api/tags`);
    if (!Array.isArray(json.models)) {
      throw new Error("Invalid response format from Ollama model list.");
    }
    return json.models.map((model) => model.name).filter((name): name is string => typeof name === "string" && name.length > 0);
  }

  const json = await getJson<{ data?: Array<{ id?: string; }>; }>(`${baseUrl}/v1/models`);
  if (!Array.isArray(json.data)) {
    throw new Error("Invalid response format from local OpenAI-compatible model list.");
  }
  return json.data.map((model) => model.id).filter((id): id is string => typeof id === "string" && id.length > 0);
}

async function getJson<T>(url: string): Promise<T> {
  const parsed = parseLocalUrl(url);
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 5000);

  try {
    const response = await fetch(parsed.toString(), { method: "GET", signal: controller.signal });
    const text = await response.text();
    if (!response.ok) {
      throw new Error(`Endpoint returned HTTP ${response.status}: ${text.slice(0, 120)}`);
    }
    return (text.length > 0 ? JSON.parse(text) : {}) as T;
  } finally {
    clearTimeout(timeout);
  }
}

function classifyProviderError(message: string): ProviderDetectionResult["status"] {
  const lower = message.toLowerCase();
  if (lower.includes("abort") || lower.includes("timeout")) {
    return "Timeout";
  }
  if (lower.includes("econnrefused") || lower.includes("not running") || lower.includes("fetch failed")) {
    return "Not running";
  }
  if (lower.includes("model") && (lower.includes("not found") || lower.includes("missing"))) {
    return "Model missing";
  }
  if (lower.includes("json") || lower.includes("invalid response")) {
    return "Invalid response format";
  }
  if (lower.includes("refusing") || lower.includes("url")) {
    return "Endpoint unreachable";
  }
  return "Unknown error";
}

function isAllowedLocalOrPrivateHost(hostname: string): boolean {
  const normalized = hostname.toLowerCase();
  if (LOCAL_HOSTS.has(normalized) || normalized.endsWith(".local")) {
    return true;
  }
  if (/^10\.\d{1,3}\.\d{1,3}\.\d{1,3}$/.test(normalized) || /^192\.168\.\d{1,3}\.\d{1,3}$/.test(normalized)) {
    return true;
  }
  const match = /^172\.(\d{1,2})\.\d{1,3}\.\d{1,3}$/.exec(normalized);
  return match ? Number(match[1]) >= 16 && Number(match[1]) <= 31 : false;
}

function combinePrompts(request: CompletionRequest): string {
  return request.systemPrompt ? `${request.systemPrompt}\n\n${request.prompt}` : request.prompt;
}

function getProviderName(providerType: ModelProfile["providerType"]): string {
  switch (providerType) {
    case "ollama":
      return "Ollama";
    case "lmstudio":
      return "LM Studio";
    case "llamacpp":
      return "llama.cpp server";
    case "openai-compatible":
      return "Local OpenAI-compatible endpoint";
  }
}

export function defaultClassificationForProvider(providerType: ModelProfile["providerType"]): ModelProviderClassification {
  return providerType === "openai-compatible" ? "offline" : "offline";
}
