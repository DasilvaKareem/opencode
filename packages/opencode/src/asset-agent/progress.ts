import { EventEmitter } from "events"
import type { AssetJob, ProgressUpdate, JobStatus } from "./types"

/**
 * Progress Tracker for real-time asset generation updates
 */
export class ProgressTracker extends EventEmitter {
  private jobs = new Map<string, AssetJob>()
  private startTime: number = 0

  /**
   * Initialize tracker with jobs
   */
  initialize(jobs: AssetJob[]): void {
    this.jobs.clear()
    jobs.forEach((job) => this.jobs.set(job.id, job))
    this.startTime = Date.now()
    this.emitUpdate()
  }

  /**
   * Update job progress
   */
  updateJob(jobId: string, status: JobStatus, error?: string): void {
    const job = this.jobs.get(jobId)
    if (!job) return

    job.status = status

    if (status === "in_progress" && !job.startTime) {
      job.startTime = Date.now()
    }

    if (status === "completed" || status === "failed") {
      job.endTime = Date.now()
      if (error) {
        job.error = error
      }
    }

    this.emitUpdate()
  }

  /**
   * Get current progress snapshot
   */
  getProgress(): ProgressUpdate {
    const jobs = Array.from(this.jobs.values())

    const total = jobs.length
    const completed = jobs.filter((j) => j.status === "completed").length
    const failed = jobs.filter((j) => j.status === "failed").length
    const inProgress = jobs.filter((j) => j.status === "in_progress").length

    const currentJobs = jobs
      .filter((j) => j.status === "in_progress")
      .map((j) => ({
        id: j.id,
        name: j.request.name,
        type: j.request.type,
        progress: this.calculateJobProgress(j),
        status: j.status,
      }))

    const estimatedTimeRemaining = this.estimateTimeRemaining(jobs)

    return {
      total,
      completed,
      failed,
      inProgress,
      currentJobs,
      estimatedTimeRemaining,
    }
  }

  /**
   * Get formatted progress string for display
   */
  getProgressString(): string {
    const progress = this.getProgress()
    const percentage = Math.round((progress.completed / progress.total) * 100)

    let str = `Asset Generation Progress: ${progress.completed}/${progress.total} (${percentage}%)\n`

    if (progress.inProgress > 0) {
      str += `\n⏳ In Progress (${progress.inProgress}):\n`
      progress.currentJobs.forEach((job) => {
        str += `  • ${job.name} (${job.type})\n`
      })
    }

    if (progress.failed > 0) {
      str += `\n❌ Failed: ${progress.failed}\n`
    }

    if (progress.estimatedTimeRemaining > 0) {
      const mins = Math.floor(progress.estimatedTimeRemaining / 60)
      const secs = Math.round(progress.estimatedTimeRemaining % 60)
      str += `\n⏱️  Estimated time remaining: ${mins}m ${secs}s\n`
    }

    return str
  }

  /**
   * Get summary statistics
   */
  getSummary() {
    const jobs = Array.from(this.jobs.values())
    const completed = jobs.filter((j) => j.status === "completed")
    const failed = jobs.filter((j) => j.status === "failed")

    const totalTime = Date.now() - this.startTime
    const avgTimePerJob = completed.length > 0 ? totalTime / completed.length : 0

    return {
      totalJobs: jobs.length,
      completed: completed.length,
      failed: failed.length,
      totalTime: totalTime / 1000, // seconds
      avgTimePerJob: avgTimePerJob / 1000, // seconds
      successRate: jobs.length > 0 ? (completed.length / jobs.length) * 100 : 0,
    }
  }

  private calculateJobProgress(job: AssetJob): number {
    if (job.status === "completed") return 100
    if (job.status === "failed") return 0
    if (job.status === "pending") return 0

    // For in-progress jobs, estimate based on time elapsed
    if (!job.startTime) return 0

    const elapsed = Date.now() - job.startTime
    const estimated = job.estimatedTime * 1000 // convert to ms

    return Math.min(95, Math.round((elapsed / estimated) * 100))
  }

  private estimateTimeRemaining(jobs: AssetJob[]): number {
    const pending = jobs.filter((j) => j.status === "pending")
    const inProgress = jobs.filter((j) => j.status === "in_progress")

    // Estimate based on pending jobs
    const pendingTime = pending.reduce((sum, job) => sum + job.estimatedTime, 0)

    // Add remaining time for in-progress jobs
    const inProgressTime = inProgress.reduce((sum, job) => {
      if (!job.startTime) return sum + job.estimatedTime

      const elapsed = (Date.now() - job.startTime) / 1000
      const remaining = Math.max(0, job.estimatedTime - elapsed)
      return sum + remaining
    }, 0)

    // Account for parallelization (rough estimate)
    // Assume average of 4 concurrent jobs
    const avgConcurrency = 4
    return (pendingTime + inProgressTime) / avgConcurrency
  }

  private emitUpdate(): void {
    const progress = this.getProgress()
    this.emit("progress", progress)
  }

  /**
   * Subscribe to progress updates
   */
  onProgress(callback: (progress: ProgressUpdate) => void): void {
    this.on("progress", callback)
  }

  /**
   * Unsubscribe from progress updates
   */
  offProgress(callback: (progress: ProgressUpdate) => void): void {
    this.off("progress", callback)
  }
}
