import type { CompilationJob, ProgressUpdate } from "./types"

/**
 * Progress tracker for compilation jobs
 */
export class ProgressTracker {
  private jobs: Map<string, CompilationJob> = new Map()
  private listeners: Array<(update: ProgressUpdate) => void> = []
  private startTime: number = 0

  /**
   * Initialize tracker with jobs
   */
  initialize(jobs: CompilationJob[]): void {
    this.jobs.clear()
    jobs.forEach((job) => this.jobs.set(job.id, job))
    this.startTime = Date.now()
  }

  /**
   * Update a job's status
   */
  updateJob(id: string, status: CompilationJob["status"], error?: string): void {
    const job = this.jobs.get(id)
    if (!job) return

    job.status = status
    if (error) job.error = error

    if (status === "compiling") {
      job.startTime = Date.now()
    } else if (status === "completed" || status === "failed") {
      job.endTime = Date.now()
    }

    this.jobs.set(id, job)
    this.notifyListeners()
  }

  /**
   * Subscribe to progress updates
   */
  onProgress(callback: (update: ProgressUpdate) => void): void {
    this.listeners.push(callback)
  }

  /**
   * Notify all listeners of progress change
   */
  private notifyListeners(): void {
    const update = this.getProgressUpdate()
    this.listeners.forEach((listener) => listener(update))
  }

  /**
   * Get current progress update
   */
  getProgressUpdate(): ProgressUpdate {
    const jobs = Array.from(this.jobs.values())
    const total = jobs.length
    const completed = jobs.filter((j) => j.status === "completed").length
    const failed = jobs.filter((j) => j.status === "failed").length
    const compiling = jobs.filter((j) => j.status === "compiling").length
    const uploading = jobs.filter((j) => j.status === "uploading").length

    let stage: ProgressUpdate["stage"] = "validating"
    let progress = 0
    let message = "Preparing..."

    if (failed > 0 && completed + failed === total) {
      stage = "failed"
      progress = 100
      message = `Failed: ${failed} job(s) failed`
    } else if (completed === total) {
      stage = "completed"
      progress = 100
      message = "Compilation complete!"
    } else if (uploading > 0) {
      stage = "uploading"
      progress = 85 + ((completed / total) * 15)
      message = `Uploading to Playscape...`
    } else if (compiling > 0) {
      stage = "compiling"
      progress = 10 + ((completed / total) * 60)
      message = `Compiling to WASM...`
    } else {
      stage = "validating"
      progress = 5
      message = "Validating game files..."
    }

    const eta = this.calculateETA(completed, total)

    return {
      stage,
      progress: Math.round(progress),
      message,
      eta,
    }
  }

  /**
   * Calculate estimated time remaining
   */
  private calculateETA(completed: number, total: number): number | undefined {
    if (completed === 0) return undefined

    const elapsed = Date.now() - this.startTime
    const avgTimePerJob = elapsed / completed
    const remaining = total - completed
    return Math.round((avgTimePerJob * remaining) / 1000) // Convert to seconds
  }

  /**
   * Get summary statistics
   */
  getSummary() {
    const jobs = Array.from(this.jobs.values())
    return {
      total: jobs.length,
      completed: jobs.filter((j) => j.status === "completed").length,
      failed: jobs.filter((j) => j.status === "failed").length,
      pending: jobs.filter((j) => j.status === "pending").length,
      compiling: jobs.filter((j) => j.status === "compiling").length,
      uploading: jobs.filter((j) => j.status === "uploading").length,
    }
  }

  /**
   * Get progress string for display
   */
  getProgressString(): string {
    const update = this.getProgressUpdate()
    const summary = this.getSummary()

    let str = `[${update.progress}%] ${update.message}`

    if (update.eta) {
      str += ` (ETA: ${update.eta}s)`
    }

    str += ` - ${summary.completed}/${summary.total} complete`

    if (summary.failed > 0) {
      str += `, ${summary.failed} failed`
    }

    return str
  }

  /**
   * Clear all progress data
   */
  clear(): void {
    this.jobs.clear()
    this.listeners = []
    this.startTime = 0
  }
}
