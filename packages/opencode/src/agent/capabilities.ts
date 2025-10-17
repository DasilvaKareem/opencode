import { Filesystem } from "../util/filesystem"
import path from "path"
import { Instance } from "../project/instance"

export namespace Capabilities {
  export interface AssetTypeCapability {
    type: string
    description: string
    provider: string
    parameters: string[]
    supportedFormats?: string[]
  }

  export interface AgentCapabilities {
    agent: string
    version: string
    capabilities: {
      assetTypes?: AssetTypeCapability[]
      features?: string[]
      limitations?: string[]
      [key: string]: any
    }
    examples?: Array<{
      task: string
      capability: string
      supported: boolean
    }>
  }

  /**
   * Load capabilities manifest for an agent
   */
  export async function load(agentName: string): Promise<AgentCapabilities | null> {
    const capabilitiesPath = path.join(
      Instance.directory,
      ".opencode",
      "agent",
      `${agentName}.capabilities.json`
    )

    try {
      const file = Bun.file(capabilitiesPath)
      if (!(await file.exists())) {
        return null
      }
      const content = await file.json()
      return content as AgentCapabilities
    } catch (error) {
      console.error(`Failed to load capabilities for agent ${agentName}:`, error)
      return null
    }
  }

  /**
   * Format capabilities as a system prompt section
   */
  export function formatAsPrompt(capabilities: AgentCapabilities): string {
    const sections: string[] = []

    sections.push(`# Agent Capabilities\n`)
    sections.push(`You are the ${capabilities.agent} agent (v${capabilities.version}).\n`)

    // Asset types
    if (capabilities.capabilities.assetTypes && capabilities.capabilities.assetTypes.length > 0) {
      sections.push(`## Supported Asset Types\n`)
      for (const assetType of capabilities.capabilities.assetTypes) {
        sections.push(`- **${assetType.type}**: ${assetType.description}`)
        sections.push(`  - Provider: ${assetType.provider}`)
        if (assetType.supportedFormats) {
          sections.push(`  - Formats: ${assetType.supportedFormats.join(", ")}`)
        }
        sections.push(`  - Parameters: ${assetType.parameters.join(", ")}`)
      }
      sections.push("")
    }

    // Features
    if (capabilities.capabilities.features && capabilities.capabilities.features.length > 0) {
      sections.push(`## Features\n`)
      for (const feature of capabilities.capabilities.features) {
        sections.push(`- ${feature}`)
      }
      sections.push("")
    }

    // Limitations
    if (capabilities.capabilities.limitations && capabilities.capabilities.limitations.length > 0) {
      sections.push(`## Limitations\n`)
      for (const limitation of capabilities.capabilities.limitations) {
        sections.push(`- ${limitation}`)
      }
      sections.push("")
    }

    // Examples
    if (capabilities.examples && capabilities.examples.length > 0) {
      sections.push(`## Example Capabilities\n`)
      for (const example of capabilities.examples) {
        const status = example.supported ? "✓" : "✗"
        sections.push(`${status} ${example.task} (${example.capability})`)
      }
      sections.push("")
    }

    sections.push(
      `IMPORTANT: You have immediate knowledge of these capabilities. Do not search the codebase to discover what you can do - you already know from this manifest.`
    )

    return sections.join("\n")
  }

  /**
   * Check if an agent can perform a specific capability
   */
  export async function canPerform(
    agentName: string,
    capabilityType: string,
    specificCapability?: string
  ): Promise<boolean> {
    const capabilities = await load(agentName)
    if (!capabilities) return false

    // Check asset types
    if (capabilities.capabilities.assetTypes) {
      const hasAssetType = capabilities.capabilities.assetTypes.some(
        (at) => at.type === specificCapability
      )
      if (hasAssetType) return true
    }

    // Check examples
    if (capabilities.examples) {
      const hasExample = capabilities.examples.some(
        (ex) =>
          ex.capability === specificCapability ||
          ex.task.toLowerCase().includes(specificCapability?.toLowerCase() || "")
      )
      if (hasExample) return true
    }

    return false
  }
}
