import { Flag } from "../../flag/flag"

const ELEVENLABS_API_KEY = process.env.ELEVENLABS_API_KEY || Flag.ELEVENLABS_API_KEY || ""
const ELEVENLABS_API_BASE = "https://api.elevenlabs.io/v1"

export interface ElevenLabsSFXRequest {
  description: string
  duration?: number // seconds
  promptInfluence?: number // 0-1
}

export interface ElevenLabsMusicRequest {
  prompt: string
  duration?: number // seconds
  genre?: string
}

export interface ElevenLabsVoiceRequest {
  text: string
  voiceId?: string
  modelId?: string
}

export class ElevenLabsProvider {
  private apiKey: string

  constructor(apiKey?: string) {
    this.apiKey = apiKey || ELEVENLABS_API_KEY
  }

  /**
   * Generate sound effects
   */
  async generateSFX(request: ElevenLabsSFXRequest): Promise<Blob> {
    const response = await fetch(`${ELEVENLABS_API_BASE}/sound-generation`, {
      method: "POST",
      headers: {
        "xi-api-key": this.apiKey,
        "Content-Type": "application/json",
      },
      body: JSON.stringify({
        text: request.description,
        duration_seconds: request.duration || 1.0,
        prompt_influence: request.promptInfluence || 0.3,
      }),
    })

    if (!response.ok) {
      const error = await response.text()
      throw new Error(`ElevenLabs SFX generation failed: ${error}`)
    }

    return await response.blob()
  }

  /**
   * Generate music
   * Note: This endpoint may vary based on ElevenLabs' API
   */
  async generateMusic(request: ElevenLabsMusicRequest): Promise<Blob> {
    // ElevenLabs music generation - adjust endpoint as needed
    const description = request.genre
      ? `${request.prompt}, ${request.genre} style`
      : request.prompt

    const response = await fetch(`${ELEVENLABS_API_BASE}/sound-generation`, {
      method: "POST",
      headers: {
        "xi-api-key": this.apiKey,
        "Content-Type": "application/json",
      },
      body: JSON.stringify({
        text: description,
        duration_seconds: request.duration || 30.0,
        prompt_influence: 0.5,
      }),
    })

    if (!response.ok) {
      const error = await response.text()
      throw new Error(`ElevenLabs music generation failed: ${error}`)
    }

    return await response.blob()
  }

  /**
   * Generate speech from text
   */
  async generateVoice(request: ElevenLabsVoiceRequest): Promise<Blob> {
    const voiceId = request.voiceId || "21m00Tcm4TlvDq8ikWAM" // Default voice
    const modelId = request.modelId || "eleven_monolingual_v1"

    const response = await fetch(`${ELEVENLABS_API_BASE}/text-to-speech/${voiceId}`, {
      method: "POST",
      headers: {
        "xi-api-key": this.apiKey,
        "Content-Type": "application/json",
      },
      body: JSON.stringify({
        text: request.text,
        model_id: modelId,
        voice_settings: {
          stability: 0.5,
          similarity_boost: 0.75,
        },
      }),
    })

    if (!response.ok) {
      const error = await response.text()
      throw new Error(`ElevenLabs voice generation failed: ${error}`)
    }

    return await response.blob()
  }

  /**
   * Check health of ElevenLabs service
   */
  async checkHealth(): Promise<boolean> {
    try {
      const response = await fetch(`${ELEVENLABS_API_BASE}/user`, {
        headers: {
          "xi-api-key": this.apiKey,
        },
      })
      return response.ok
    } catch (error) {
      return false
    }
  }
}
