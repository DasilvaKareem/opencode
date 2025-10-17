import { AssetJobQueue } from "./queue"
import { WorkerPool } from "./worker-pool"
import { ProgressTracker } from "./progress"
import type { BatchAssetRequest, BatchAssetResult, AssetManifest, AssetJob } from "./types"

/**
 * Asset Agent Controller - Main orchestrator for batch asset generation
 */
export class AssetAgentController {
  private queue: AssetJobQueue
  private pool: WorkerPool
  private tracker: ProgressTracker
  private maxRetries = 3

  constructor(outputDir?: string) {
    this.queue = new AssetJobQueue()
    this.pool = new WorkerPool(outputDir)
    this.tracker = new ProgressTracker()
  }

  /**
   * Generate a batch of assets with real-time progress updates
   */
  async generateBatch(request: BatchAssetRequest, onProgress?: (progress: string) => void): Promise<BatchAssetResult> {
    const startTime = Date.now()

    // Initialize queue and tracker
    this.queue.addBatch(request.assets)
    const allJobs = request.assets.map((_, i) => this.queue.getJobsByStatus("pending")[i])
    this.tracker.initialize(allJobs)

    // Subscribe to progress updates
    if (onProgress) {
      this.tracker.onProgress(() => {
        const progressStr = this.tracker.getProgressString()
        onProgress(progressStr)
      })
    }

    // Process jobs in parallel
    const results = await this.processQueue()

    // Generate summary
    const totalTime = (Date.now() - startTime) / 1000
    const totalCost = this.calculateTotalCost(results.assets)

    // Build result
    const batchResult: BatchAssetResult = {
      success: this.tracker.getSummary().completed,
      failed: this.tracker.getSummary().failed,
      totalCost,
      totalTime,
      assets: results.assets,
      failures: results.failures,
      manifest: this.generateManifest(results.assets, totalCost, totalTime),
    }

    return batchResult
  }

  /**
   * Process jobs from queue with parallel execution
   */
  private async processQueue(): Promise<{
    assets: Record<string, any>
    failures: Record<string, string>
  }> {
    const assets: Record<string, any> = {}
    const failures: Record<string, string> = {}
    const workers: Promise<void>[] = []

    // Keep processing until queue is complete
    while (!this.queue.isComplete()) {
      // Get next job that's ready to process
      const job = this.queue.getNext()

      if (!job) {
        // No jobs ready (waiting on dependencies), wait a bit
        await new Promise((resolve) => setTimeout(resolve, 100))
        continue
      }

      // Check if provider has available slot
      if (!this.pool.hasAvailableSlot(job.provider)) {
        // Put job back in queue
        this.queue.updateJob(job.id, { status: "pending" })
        await new Promise((resolve) => setTimeout(resolve, 100))
        continue
      }

      // Start processing job
      const worker = this.processJob(job, assets, failures)
      workers.push(worker)

      // Limit total concurrent workers
      if (workers.length >= 8) {
        await Promise.race(workers)
        // Clean up completed workers
        for (let i = workers.length - 1; i >= 0; i--) {
          const settled = await Promise.race([
            workers[i].then(() => true),
            Promise.resolve(false),
          ])
          if (settled) {
            workers.splice(i, 1)
          }
        }
      }
    }

    // Wait for remaining workers to complete
    await Promise.all(workers)

    return { assets, failures }
  }

  /**
   * Process a single job with retry logic
   */
  private async processJob(
    job: AssetJob,
    assets: Record<string, any>,
    failures: Record<string, string>
  ): Promise<void> {
    const jobName = job.request.name

    try {
      // Update status to in_progress
      this.queue.updateJob(job.id, { status: "in_progress" })
      this.tracker.updateJob(job.id, "in_progress")

      // Execute job
      const result = await this.pool.execute(job)

      // Update status to completed
      this.queue.updateJob(job.id, { status: "completed", result })
      this.tracker.updateJob(job.id, "completed")

      // Store result
      assets[jobName] = result
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : String(error)

      // Retry logic
      if (job.retries < this.maxRetries) {
        this.queue.updateJob(job.id, {
          status: "pending",
          retries: job.retries + 1,
        })
        this.tracker.updateJob(job.id, "pending")

        // Wait before retry
        await new Promise((resolve) => setTimeout(resolve, 1000 * (job.retries + 1)))

        // Re-process
        return this.processJob(job, assets, failures)
      } else {
        // Max retries exceeded
        this.queue.updateJob(job.id, { status: "failed", error: errorMessage })
        this.tracker.updateJob(job.id, "failed", errorMessage)

        // Store failure
        failures[jobName] = errorMessage
      }
    }
  }

  /**
   * Calculate total cost from results
   */
  private calculateTotalCost(assets: Record<string, any>): number {
    return Object.values(assets).reduce((sum, asset) => {
      return sum + (asset.metadata?.cost || 0)
    }, 0)
  }

  /**
   * Generate asset manifest
   */
  private generateManifest(assets: Record<string, any>, totalCost: number, totalTime: number): AssetManifest {
    const byType: Record<string, number> = {}

    Object.values(assets).forEach((asset: any) => {
      const type = asset.type
      byType[type] = (byType[type] || 0) + 1
    })

    return {
      created: new Date().toISOString(),
      totalAssets: Object.keys(assets).length,
      byType,
      totalCost,
      totalTime,
    }
  }

  /**
   * Get current worker statistics
   */
  getWorkerStats() {
    return {
      active: this.pool.getActiveWorkers(),
      queue: this.queue.getStats(),
    }
  }
}
