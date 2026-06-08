import { normalizeLocalHost } from "./config.js";
import type { ManualCommandResult, RuntimeSettings, StatusCard, UnrealConnectionTestResult } from "./types.js";
import { DEFAULT_UNREAL_HOST, DEFAULT_UNREAL_PORT, DEFAULT_UNREAL_TIMEOUT_MS } from "../shared/defaultState.js";

interface BridgeResponse {
  id?: string;
  ok?: boolean;
  result?: unknown;
  error?: { code?: string; message?: string };
}

export async function checkUnrealBridge(settings: RuntimeSettings): Promise<StatusCard> {
  const host = normalizeLocalHost(settings.ueBridgeHost);
  const endpoint = `http://${host}:${settings.ueBridgePort}/`;
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 2500);

  try {
    const response = await fetch(endpoint, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ id: "desktop-ping", command: "ping", params: {} }),
      signal: controller.signal
    });
    const json = (await response.json()) as BridgeResponse;

    if (response.ok && json.ok === true) {
      return { state: "online", label: "Unreal bridge online", detail: endpoint };
    }

    return {
      state: "warning",
      label: "Unreal bridge responded with an error",
      detail: json.error?.message ?? `HTTP ${response.status}`
    };
  } catch (error) {
    const message = error instanceof Error ? error.message : "Unknown connection error";
    return { state: "offline", label: "Unreal bridge offline", detail: `${endpoint} (${message})` };
  } finally {
    clearTimeout(timeout);
  }
}

export async function testDefaultUnrealConnection(settings?: Partial<RuntimeSettings>): Promise<UnrealConnectionTestResult> {
  const checkedAt = new Date().toISOString();
  const host = settings?.ueBridgeHost || DEFAULT_UNREAL_HOST;
  const port = Number(settings?.ueBridgePort) || DEFAULT_UNREAL_PORT;
  let endpoint = "";

  try {
    const parsedEndpoint = validateLocalUnrealEndpoint(`http://${host}:${port}`);
    endpoint = parsedEndpoint.toString().replace(/\/$/, "");
  } catch {
    return {
      status: "invalid_endpoint",
      ok: false,
      endpoint: `http://${host}:${port}`,
      message: "The connection address is not valid.",
      detail: "Use the default address unless you know you changed it inside Unreal.",
      checkedAt,
      response: null
    };
  }

  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), DEFAULT_UNREAL_TIMEOUT_MS);

  try {
    const response = await fetch(`${endpoint}/`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ id: "desktop-default-ping", command: "ping", params: {} }),
      signal: controller.signal
    });
    const text = await response.text();
    let json: BridgeResponse;
    try {
      json = (text ? JSON.parse(text) : {}) as BridgeResponse;
    } catch {
      return {
        status: "invalid_response",
        ok: false,
        endpoint,
        message: "The Unreal plugin answered, but the response was not understood.",
        detail: "The plugin may need to restart. Close and reopen Unreal, then click Start Bridge again.",
        checkedAt,
        response: text
      };
    }

    if (response.ok && json.ok === true) {
      return {
        status: "connected",
        ok: true,
        endpoint,
        message: "Connected to Unreal",
        detail: "You are ready to use Manual Command Mode or AI-assisted tools.",
        checkedAt,
        response: json
      };
    }

    return {
      status: "invalid_response",
      ok: false,
      endpoint,
      message: "The Unreal plugin does not appear to be running.",
      detail: json.error?.message ?? "Open Unreal, go to Edit → Plugins, enable RevoltEditorBridge, then restart Unreal.",
      checkedAt,
      response: json
    };
  } catch (error) {
    const message = error instanceof Error ? error.message.toLowerCase() : "";
    const isTimeout = message.includes("abort") || message.includes("timeout");
    return {
      status: isTimeout ? "timeout" : "unreachable",
      ok: false,
      endpoint,
      message: isTimeout ? "The desktop app could not connect before the timeout." : "The desktop app could not reach Unreal.",
      detail: "Most likely, Unreal is not open or the Start Bridge button has not been clicked inside Unreal.",
      checkedAt,
      response: null
    };
  } finally {
    clearTimeout(timeout);
  }
}

export async function sendUnrealBridgeCommand(settings: RuntimeSettings, command: string, params: Record<string, unknown> = {}): Promise<unknown> {
  const host = normalizeLocalHost(settings.ueBridgeHost);
  const endpoint = `http://${host}:${settings.ueBridgePort}/`;
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 5000);

  try {
    const response = await fetch(endpoint, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ id: `desktop-${command}`, command, params }),
      signal: controller.signal
    });
    const json = (await response.json()) as BridgeResponse;

    if (!response.ok || json.ok !== true) {
      throw new Error(json.error?.message ?? `Unreal bridge returned HTTP ${response.status}`);
    }

    return json.result ?? {};
  } finally {
    clearTimeout(timeout);
  }
}

export async function sendUnrealBridgeManualCommand(
  settings: RuntimeSettings,
  command: string,
  params: Record<string, unknown> = {}
): Promise<ManualCommandResult> {
  const host = normalizeLocalHost(settings.ueBridgeHost);
  const endpoint = `http://${host}:${settings.ueBridgePort}/`;
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 5000);
  const request = {
    id: `manual-${Date.now()}`,
    command,
    params
  };

  try {
    const response = await fetch(endpoint, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(request),
      signal: controller.signal
    });
    const json = (await response.json()) as BridgeResponse;
    const ok = response.ok && json.ok === true;
    const summary = ok
      ? summarizeBridgeResult(command, json.result)
      : json.error?.message ?? `Unreal bridge returned HTTP ${response.status}`;

    return {
      request,
      response: json,
      ok,
      summary
    };
  } catch (error) {
    const message = error instanceof Error ? error.message : "Unknown bridge error";
    return {
      request,
      response: {
        id: request.id,
        ok: false,
        error: {
          code: "LOCAL_BRIDGE_ERROR",
          message
        }
      },
      ok: false,
      summary: message
    };
  } finally {
    clearTimeout(timeout);
  }
}

function summarizeBridgeResult(command: string, result: unknown): string {
  if (result && typeof result === "object") {
    const record = result as Record<string, unknown>;
    if (typeof record.count === "number") {
      return `${command} returned ${record.count} item(s).`;
    }
    if (typeof record.issue_count === "number") {
      return `${command} returned ${record.issue_count} issue(s).`;
    }
    if (typeof record.project_name === "string") {
      return `Project: ${record.project_name}`;
    }
    if (typeof record.current_map_name === "string") {
      return `Map: ${record.current_map_name}`;
    }
    if (typeof record.status === "string") {
      return String(record.status);
    }
  }

  return `${command} completed.`;
}

function validateLocalUnrealEndpoint(endpoint: string): URL {
  const parsed = new URL(endpoint);
  const host = parsed.hostname.toLowerCase();
  if (host !== "127.0.0.1" && host !== "localhost") {
    throw new Error("Unreal connection must use localhost or 127.0.0.1 by default.");
  }
  return parsed;
}
