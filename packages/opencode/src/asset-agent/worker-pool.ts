import { writeFile, mkdir } from "fs/promises"
import { join } from "path"
import type { AssetJob, AssetResult, ProviderType } from "./types"
import { FalAIProvider } from "../provider/fal"
import { ElevenLabsProvider } from "../provider/elevenlabs"

/**
 * Worker Pool manages parallel execution of asset generation jobs
 * with rate limiting per provider
 */
export class WorkerPool {
  private falProvider = new FalAIProvider()
  private elevenLabsProvider = new ElevenLabsProvider()

  // Rate limits per provider (concurrent requests)
  private limits: Record<ProviderType, number> = {
    fal: 5, // 5 concurrent Fal AI requests
    elevenlabs: 3, // 3 concurrent ElevenLabs requests
  }

  // Track active workers per provider
  private activeWorkers = new Map<ProviderType, number>()

  // Assets output directory
  private outputDir = "./assets"

  constructor(outputDir?: string) {
    if (outputDir) {
      this.outputDir = outputDir
    }
    this.activeWorkers.set("fal", 0)
    this.activeWorkers.set("elevenlabs", 0)
  }

  /**
   * Execute a job (with rate limiting)
   */
  async execute(job: AssetJob): Promise<AssetResult> {
    const provider = job.provider

    // Wait for available slot
    await this.acquireSlot(provider)

    const startTime = Date.now()

    try {
      // Generate asset based on type
      const result = await this.generateAsset(job)

      // Save to disk
      const filePath = await this.saveAsset(result.data, job)

      const generationTime = Date.now() - startTime

      return {
        filePath,
        url: result.url,
        type: job.request.type,
        metadata: {
          provider: job.provider,
          cost: this.estimateCost(job),
          generationTime,
          seed: result.seed,
          size: result.size,
        },
      }
    } finally {
      this.releaseSlot(provider)
    }
  }

  /**
   * Get current active workers per provider
   */
  getActiveWorkers(): Record<ProviderType, number> {
    return {
      fal: this.activeWorkers.get("fal") || 0,
      elevenlabs: this.activeWorkers.get("elevenlabs") || 0,
    }
  }

  /**
   * Check if provider has available slots
   */
  hasAvailableSlot(provider: ProviderType): boolean {
    const active = this.activeWorkers.get(provider) || 0
    return active < this.limits[provider]
  }

  private async generateAsset(job: AssetJob): Promise<{ data: Blob | string; url: string; seed?: number; size?: number }> {
    const { type, description, parameters } = job.request

    switch (type) {
      case "sprite":
      case "texture": {
        const result = await this.falProvider.generate2DImage({
          prompt: description,
          aspectRatio: parameters?.aspectRatio,
          resolution: parameters?.resolution,
          seed: parameters?.seed,
        })
        // Download the image
        const response = await fetch(result.url)
        const blob = await response.blob()
        return {
          data: blob,
          url: result.url,
          seed: result.seed,
          size: blob.size,
        }
      }

      case "3d-model": {
        const result = await this.falProvider.generate3DModel({
          prompt: description,
          format: parameters?.format,
          quality: parameters?.quality,
          seed: parameters?.seed,
        })
        // Download the 3D model
        const response = await fetch(result.url)
        const blob = await response.blob()
        return {
          data: blob,
          url: result.url,
          size: blob.size,
        }
      }

      case "video": {
        const result = await this.falProvider.generateVideo({
          prompt: description,
          resolution: "720p",
          seed: parameters?.seed,
        })
        // Download the video
        const response = await fetch(result.url)
        const blob = await response.blob()
        return {
          data: blob,
          url: result.url,
          seed: result.seed,
          size: blob.size,
        }
      }

      case "sfx": {
        const blob = await this.elevenLabsProvider.generateSFX({
          description,
          duration: parameters?.duration,
        })
        return {
          data: blob,
          url: "", // ElevenLabs returns blob directly
          size: blob.size,
        }
      }

      case "music": {
        const blob = await this.elevenLabsProvider.generateMusic({
          prompt: description,
          duration: parameters?.duration,
          genre: parameters?.genre,
        })
        return {
          data: blob,
          url: "",
          size: blob.size,
        }
      }

      case "voice": {
        const blob = await this.elevenLabsProvider.generateVoice({
          text: description,
        })
        return {
          data: blob,
          url: "",
          size: blob.size,
        }
      }

      default:
        throw new Error(`Unsupported asset type: ${type}`)
    }
  }

  private async saveAsset(data: Blob | string, job: AssetJob): Promise<string> {
    const { name, type } = job.request

    // Determine subdirectory and extension
    const subdir = this.getSubdirectory(type)
    const extension = this.getExtension(type, job.request.parameters?.format)

    // Create directory if it doesn't exist
    const dir = join(this.outputDir, subdir)
    await mkdir(dir, { recursive: true })

    // Save file
    const filename = `${name}.${extension}`
    const filePath = join(dir, filename)

    if (data instanceof Blob) {
      const buffer = Buffer.from(await data.arrayBuffer())
      await writeFile(filePath, buffer)
    } else {
      await writeFile(filePath, data)
    }

    return filePath
  }

  private getSubdirectory(type: string): string {
    switch (type) {
      case "sprite":
        return "sprites"
      case "texture":
        return "textures"
      case "3d-model":
        return "models"
      case "video":
        return "videos"
      case "sfx":
        return "sfx"
      case "music":
        return "music"
      case "voice":
        return "voice"
      default:
        return "misc"
    }
  }

  private getExtension(type: string, format?: string): string {
    switch (type) {
      case "sprite":
      case "texture":
        return "png"
      case "3d-model":
        return format || "glb"
      case "video":
        return "mp4"
      case "sfx":
      case "music":
      case "voice":
        return "mp3"
      default:
        return "bin"
    }
  }

  private estimateCost(job: AssetJob): number {
    // Cost estimates in USD
    const costs: Record<string, number> = {
      sprite: 0.05,
      texture: 0.05,
      "3d-model": 0.4,
      video: 0.3,
      sfx: 0.01,
      music: 0.05,
      voice: 0.02,
    }
    return costs[job.request.type] || 0
  }

  private async acquireSlot(provider: ProviderType): Promise<void> {
    while (!this.hasAvailableSlot(provider)) {
      // Wait 100ms and check again
      await new Promise((resolve) => setTimeout(resolve, 100))
    }

    const current = this.activeWorkers.get(provider) || 0
    this.activeWorkers.set(provider, current + 1)
  }

  private releaseSlot(provider: ProviderType): void {
    const current = this.activeWorkers.get(provider) || 0
    this.activeWorkers.set(provider, Math.max(0, current - 1))
  }
}
