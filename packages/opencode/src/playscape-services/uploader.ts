import { readFileSync, statSync } from "fs"
import { basename } from "path"
import { AuthPlayscapeSupabase } from "../auth/playscape-supabase"
import type { CompiledOutput, UploadResult } from "./types"

/**
 * Uploader for compiled games to Playscape R2 storage
 */
export class GameUploader {
  private backendUrl: string

  constructor(backendUrl?: string) {
    this.backendUrl = backendUrl || process.env.PLAYSCAPE_BACKEND_URL || "https://playscape.gg"
  }

  /**
   * Upload compiled game files to Playscape
   */
  async upload(output: CompiledOutput, gameId?: string): Promise<UploadResult> {
    // Get access token
    const accessToken = await AuthPlayscapeSupabase.access()
    if (!accessToken) {
      throw new Error("Not authenticated. Please run 'opencode auth login playscape' first")
    }

    // Upload WASM file (required)
    const wasmResult = await this.uploadFile(
      output.wasmFile,
      accessToken,
      gameId
    )

    // Upload additional files if they exist
    const jsResult = await this.uploadFile(
      output.jsFile,
      accessToken,
      gameId
    )

    const htmlResult = await this.uploadFile(
      output.htmlFile,
      accessToken,
      gameId
    )

    let dataResult: any = null
    if (output.dataFile) {
      dataResult = await this.uploadFile(
        output.dataFile,
        accessToken,
        gameId
      )
    }

    // Return combined result
    return {
      success: true,
      url: wasmResult.url,
      key: wasmResult.key,
      fileName: wasmResult.fileName,
      size: wasmResult.size,
      gameId: wasmResult.gameId,
      htmlUrl: htmlResult.url,
      jsUrl: jsResult.url,
      dataUrl: dataResult?.url,
    }
  }

  /**
   * Upload a single file to the backend
   */
  private async uploadFile(
    filePath: string,
    accessToken: string,
    gameId?: string
  ): Promise<any> {
    const fileBuffer = readFileSync(filePath)
    const fileName = basename(filePath)
    const fileSize = statSync(filePath).size

    // Create form data
    const formData = new FormData()
    const blob = new Blob([new Uint8Array(fileBuffer)], {
      type: this.getContentType(fileName),
    })
    formData.append("file", blob, fileName)

    if (gameId) {
      formData.append("gameId", gameId)
    }

    // Upload to backend
    const response = await fetch(`${this.backendUrl}/api/cli/upload-wasm`, {
      method: "POST",
      headers: {
        Authorization: `Bearer ${accessToken}`,
      },
      body: formData,
    })

    if (!response.ok) {
      const errorData = await response.json().catch(() => ({}))
      throw new Error(
        `Upload failed for ${fileName}: ${errorData.error || response.statusText}`
      )
    }

    const result = await response.json()
    return result
  }

  /**
   * Get content type based on file extension
   */
  private getContentType(fileName: string): string {
    const ext = fileName.substring(fileName.lastIndexOf("."))
    switch (ext) {
      case ".wasm":
        return "application/wasm"
      case ".js":
        return "application/javascript"
      case ".html":
        return "text/html"
      case ".data":
        return "application/octet-stream"
      default:
        return "application/octet-stream"
    }
  }

  /**
   * Upload with retry logic
   */
  async uploadWithRetry(
    output: CompiledOutput,
    gameId?: string,
    maxRetries: number = 3
  ): Promise<UploadResult> {
    let lastError: Error | null = null

    for (let attempt = 1; attempt <= maxRetries; attempt++) {
      try {
        return await this.upload(output, gameId)
      } catch (error) {
        lastError = error as Error
        console.error(`Upload attempt ${attempt}/${maxRetries} failed:`, error)

        if (attempt < maxRetries) {
          // Exponential backoff
          const delay = Math.pow(2, attempt) * 1000
          await new Promise((resolve) => setTimeout(resolve, delay))
        }
      }
    }

    throw new Error(
      `Upload failed after ${maxRetries} attempts: ${lastError?.message}`
    )
  }

  /**
   * Get playable URL for uploaded game
   */
  getPlayableUrl(uploadResult: UploadResult): string {
    // Return the HTML URL if available, otherwise construct from WASM URL
    if (uploadResult.htmlUrl) {
      return uploadResult.htmlUrl
    }

    // Construct HTML URL from WASM URL by replacing extension
    return uploadResult.url.replace(/\.wasm$/, ".html")
  }
}
