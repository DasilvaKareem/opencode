import { fal } from "@fal-ai/client"
import { Flag } from "../../flag/flag"

// Configure Fal AI
fal.config({
  credentials: process.env.FAL_KEY || Flag.FAL_KEY || "",
})

export interface FalImageRequest {
  prompt: string
  aspectRatio?: "1:1" | "16:9" | "9:16" | "3:4" | "4:3"
  resolution?: "1K" | "2K"
  numImages?: number
  seed?: number
}

export interface Fal3DRequest {
  prompt?: string
  imageUrl?: string
  format?: "glb" | "usdz" | "fbx" | "obj" | "stl"
  material?: "PBR" | "Shaded"
  quality?: "high" | "medium" | "low" | "extra-low"
  seed?: number
}

export interface FalVideoRequest {
  prompt: string
  resolution?: "480p" | "580p" | "720p"
  aspectRatio?: "16:9" | "9:16" | "1:1"
  seed?: number
}

export class FalAIProvider {
  /**
   * Generate a 2D image using Imagen 4
   */
  async generate2DImage(request: FalImageRequest): Promise<{ url: string; seed: number }> {
    const result = await fal.subscribe("fal-ai/imagen4/preview/fast", {
      input: {
        prompt: request.prompt,
        aspect_ratio: request.aspectRatio || "1:1",
        resolution: request.resolution || "1K",
        num_images: request.numImages || 1,
        seed: request.seed,
      },
      logs: false,
    })

    return {
      url: result.data.images[0].url,
      seed: result.data.seed,
    }
  }

  /**
   * Generate a 3D model using Hyper3D Rodin
   */
  async generate3DModel(request: Fal3DRequest): Promise<{ url: string }> {
    const input: any = {
      geometry_file_format: request.format || "glb",
      material: request.material || "PBR",
      quality: request.quality || "medium",
      seed: request.seed,
    }

    // Either text-to-3D or image-to-3D
    if (request.prompt) {
      input.prompt = request.prompt
    }
    if (request.imageUrl) {
      input.input_image_urls = request.imageUrl
    }

    const result = await fal.subscribe("fal-ai/hyper3d/rodin", {
      input,
      logs: false,
    })

    // Result contains the 3D model URL
    return {
      url: (result.data as any).geometry_file?.url || (result.data as any).model_url?.url || "",
    }
  }

  /**
   * Generate a video using Wan Text-to-Video Turbo
   */
  async generateVideo(request: FalVideoRequest): Promise<{ url: string; seed: number }> {
    const result = await fal.subscribe("fal-ai/wan/v2.2-a14b/text-to-video/turbo", {
      input: {
        prompt: request.prompt,
        resolution: request.resolution || "720p",
        aspect_ratio: request.aspectRatio || "16:9",
        seed: request.seed,
      },
      logs: false,
    })

    return {
      url: result.data.video.url,
      seed: result.data.seed || 0,
    }
  }

  /**
   * Check health of Fal AI service
   */
  async checkHealth(): Promise<boolean> {
    try {
      // Simple health check with a minimal request
      await fal.subscribe("fal-ai/imagen4/preview/fast", {
        input: {
          prompt: "test",
          num_images: 1,
        },
        logs: false,
      })
      return true
    } catch (error) {
      return false
    }
  }
}
