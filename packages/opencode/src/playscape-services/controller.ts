import { CompilationQueue } from "./queue"
import { WorkerPool } from "./worker-pool"
import { ProgressTracker } from "./progress"
import { GameUploader } from "./uploader"
import type {
  CompilationRequest,
  CompilationResult,
  CompilationJob,
  ProgressUpdate,
} from "./types"

/**
 * Main controller for Playscape compilation service
 * Orchestrates compilation, upload, and progress tracking
 */
export class PlayscapeServiceController {
  private queue: CompilationQueue
  private pool: WorkerPool
  private tracker: ProgressTracker
  private uploader: GameUploader
  private maxRetries = 3

  constructor(backendUrl?: string) {
    this.queue = new CompilationQueue()
    this.pool = new WorkerPool()
    this.tracker = new ProgressTracker()
    this.uploader = new GameUploader(backendUrl)
  }

  /**
   * Compile a game and upload to Playscape
   */
  async compile(
    request: CompilationRequest,
    onProgress?: (update: ProgressUpdate) => void
  ): Promise<CompilationResult> {
    const startTime = Date.now()

    try {
      // 1. Validate request
      if (onProgress) {
        onProgress({
          stage: "validating",
          progress: 5,
          message: "Validating game files...",
        })
      }

      this.validateRequest(request)

      // 2. Add job to queue
      const job = this.queue.add(request)
      this.tracker.initialize([job])

      // Subscribe to progress updates
      if (onProgress) {
        this.tracker.onProgress((update) => {
          onProgress(update)
        })
      }

      // 3. Compile game
      if (onProgress) {
        onProgress({
          stage: "compiling",
          progress: 10,
          message: `Compiling ${request.gameType} game...`,
        })
      }

      const compiledOutput = await this.compileWithRetry(job)

      // 4. Upload to Playscape
      if (onProgress) {
        onProgress({
          stage: "uploading",
          progress: 85,
          message: "Uploading to Playscape...",
        })
      }

      this.queue.update(job.id, { status: "uploading" })
      this.tracker.updateJob(job.id, "uploading")

      const uploadResult = await this.uploader.uploadWithRetry(
        compiledOutput,
        request.gameId
      )

      // 5. Complete
      this.queue.update(job.id, { status: "completed" })
      this.tracker.updateJob(job.id, "completed")

      const totalTime = Date.now() - startTime

      if (onProgress) {
        onProgress({
          stage: "completed",
          progress: 100,
          message: "Compilation complete!",
        })
      }

      return {
        success: true,
        compiledOutput,
        uploadResult,
        totalTime,
        playableUrl: this.uploader.getPlayableUrl(uploadResult),
      }
    } catch (error) {
      const totalTime = Date.now() - startTime
      const errorMessage = error instanceof Error ? error.message : String(error)

      if (onProgress) {
        onProgress({
          stage: "failed",
          progress: 0,
          message: `Compilation failed: ${errorMessage}`,
        })
      }

      return {
        success: false,
        error: errorMessage,
        totalTime,
      }
    }
  }

  /**
   * Validate compilation request
   */
  private validateRequest(request: CompilationRequest): void {
    if (!request.sourcePath) {
      throw new Error("Source path is required")
    }

    if (!request.gameType) {
      throw new Error("Game type is required")
    }

    if (!["html5", "raylib-c"].includes(request.gameType)) {
      throw new Error(`Invalid game type: ${request.gameType}`)
    }
  }

  /**
   * Compile with retry logic
   */
  private async compileWithRetry(job: CompilationJob) {
    let lastError: Error | null = null

    for (let attempt = 0; attempt <= this.maxRetries; attempt++) {
      try {
        // Wait for available worker slot
        if (!this.pool.hasAvailableSlot()) {
          await this.pool.waitForSlot()
        }

        // Update status
        this.queue.update(job.id, { status: "compiling", retries: attempt })
        this.tracker.updateJob(job.id, "compiling")

        // Execute compilation
        const result = await this.pool.execute(job)

        // Store result
        this.queue.update(job.id, { outputFiles: result })

        return result
      } catch (error) {
        lastError = error as Error

        if (attempt < this.maxRetries) {
          // Wait before retry (exponential backoff)
          const delay = Math.pow(2, attempt) * 1000
          await new Promise((resolve) => setTimeout(resolve, delay))
        }
      }
    }

    // Max retries exceeded
    this.queue.update(job.id, {
      status: "failed",
      error: lastError?.message || "Unknown error",
    })
    this.tracker.updateJob(job.id, "failed", lastError?.message)

    throw lastError || new Error("Compilation failed after max retries")
  }

  /**
   * Get compilation statistics
   */
  getStats() {
    return {
      queue: this.queue.getStats(),
      activeWorkers: this.pool.getActiveCount(),
      progress: this.tracker.getSummary(),
    }
  }

  /**
   * Clear all state
   */
  clear() {
    this.queue.clear()
    this.pool.clear()
    this.tracker.clear()
  }
}
