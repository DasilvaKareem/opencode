import { LanguageModelV1, type LanguageModelV1StreamPart } from "@ai-sdk/provider"
import { AuthPlayscapeSupabase } from "../auth/playscape-supabase"

// Playscape backend - ALL users use this (no local API keys needed)
const PROXY_URL = process.env.PLAYSCAPE_PROXY_URL || "https://playscape.gg/api/ai/generateCli"

/**
 * Custom proxy model that routes requests through the Playscape backend
 * instead of making direct API calls. This allows users to authenticate
 * with Playscape and use backend-provided API keys.
 */
export function createProxyModel(modelId: string): LanguageModelV1 {
  return {
    specificationVersion: "v1",
    provider: "playscape-proxy",
    modelId,
    defaultObjectGenerationMode: "json",

    async doGenerate(options) {
      const token = await AuthPlayscapeSupabase.access()
      if (!token) {
        throw new Error("Not authenticated. Please run: playscape auth login")
      }

      const response = await fetch(PROXY_URL, {
        method: "POST",
        headers: {
          "Authorization": `Bearer ${token}`,
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          model: modelId,
          messages: options.prompt,
          temperature: options.temperature,
          topP: options.topP,
          maxTokens: options.maxTokens,
          abortSignal: options.abortSignal,
        }),
      })

      if (!response.ok) {
        const error = await response.json().catch(() => ({}))
        throw new Error(`Proxy request failed: ${error.error || response.statusText}`)
      }

      const data = await response.json()

      return {
        text: data.text,
        usage: data.usage,
        finishReason: data.finishReason || "stop",
        rawCall: { rawPrompt: options.prompt, rawSettings: {} },
      }
    },

    async doStream(options) {
      const token = await AuthPlayscapeSupabase.access()
      if (!token) {
        throw new Error("Not authenticated. Please run: playscape auth login")
      }

      const response = await fetch(PROXY_URL, {
        method: "POST",
        headers: {
          "Authorization": `Bearer ${token}`,
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          model: modelId,
          messages: options.prompt,
          temperature: options.temperature,
          topP: options.topP,
          maxTokens: options.maxTokens,
          stream: true,
          tools: options.tools,
          toolChoice: options.toolChoice,
        }),
        signal: options.abortSignal,
      })

      if (!response.ok) {
        const error = await response.json().catch(() => ({}))
        throw new Error(`Proxy request failed: ${error.error || response.statusText}`)
      }

      if (!response.body) {
        throw new Error("Response body is null")
      }

      return {
        stream: createStreamFromResponse(response.body),
        rawCall: { rawPrompt: options.prompt, rawSettings: {} },
      }
    },
  }
}

async function* createStreamFromResponse(
  body: ReadableStream<Uint8Array>
): AsyncGenerator<LanguageModelV1StreamPart> {
  const reader = body.getReader()
  const decoder = new TextDecoder()
  let buffer = ""

  try {
    while (true) {
      const { done, value } = await reader.read()
      if (done) break

      buffer += decoder.decode(value, { stream: true })
      const lines = buffer.split("\n")
      buffer = lines.pop() || ""

      for (const line of lines) {
        if (!line.trim() || line.startsWith(":")) continue

        if (line.startsWith("data: ")) {
          const data = line.slice(6)
          if (data === "[DONE]") continue

          try {
            const parsed = JSON.parse(data)

            // Handle text delta
            if (parsed.type === "text-delta" || parsed.textDelta) {
              yield {
                type: "text-delta",
                textDelta: parsed.textDelta || parsed.delta,
              }
            }

            // Handle tool calls
            if (parsed.type === "tool-call-delta" || parsed.toolCallDelta) {
              yield {
                type: "tool-call-delta",
                toolCallType: "function",
                toolCallId: parsed.toolCallId,
                toolName: parsed.toolName,
                argsTextDelta: parsed.argsTextDelta || "",
              }
            }

            // Handle finish
            if (parsed.type === "finish" || parsed.finishReason) {
              yield {
                type: "finish",
                finishReason: parsed.finishReason || "stop",
                usage: parsed.usage || { promptTokens: 0, completionTokens: 0 },
              }
            }
          } catch (e) {
            console.error("Failed to parse SSE data:", data, e)
          }
        }
      }
    }
  } finally {
    reader.releaseLock()
  }
}
