import type { CallToolResult } from "@modelcontextprotocol/sdk/types.js";
import type { UnrealBridgeResponse } from "./ueBridgeClient.js";

export function bridgeResponseToMcp(response: UnrealBridgeResponse): CallToolResult {
  return {
    isError: !response.ok,
    content: [
      {
        type: "text",
        text: JSON.stringify(response, null, 2)
      }
    ]
  };
}
