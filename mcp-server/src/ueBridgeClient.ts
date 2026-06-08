import crypto from "node:crypto";
import type { ServerConfig } from "./config.js";

export interface UnrealBridgeRequest {
  id: string;
  command: string;
  params: Record<string, unknown>;
}

export interface UnrealBridgeError {
  code: string;
  message: string;
}

export type UnrealBridgeResponse =
  | { id: string; ok: true; result: unknown }
  | { id: string; ok: false; error: UnrealBridgeError };

export class UnrealBridgeClient {
  constructor(private readonly config: ServerConfig) {}

  async command(command: string, params: Record<string, unknown> = {}): Promise<UnrealBridgeResponse> {
    const request: UnrealBridgeRequest = {
      id: crypto.randomUUID(),
      command,
      params
    };

    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), this.config.requestTimeoutMs);

    try {
      const response = await fetch(`http://${this.config.ueBridgeHost}:${this.config.ueBridgePort}/`, {
        method: "POST",
        headers: {
          "Content-Type": "application/json"
        },
        body: JSON.stringify(request),
        signal: controller.signal
      });

      const text = await response.text();
      if (!response.ok) {
        return {
          id: request.id,
          ok: false,
          error: {
            code: "HTTP_ERROR",
            message: `Unreal bridge returned HTTP ${response.status}: ${text}`
          }
        };
      }

      return JSON.parse(text) as UnrealBridgeResponse;
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      return {
        id: request.id,
        ok: false,
        error: {
          code: "BRIDGE_REQUEST_FAILED",
          message
        }
      };
    } finally {
      clearTimeout(timeout);
    }
  }
}
