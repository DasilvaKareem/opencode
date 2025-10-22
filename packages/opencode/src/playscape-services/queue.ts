import type { CompilationJob, CompilationRequest, CompilationStatus } from "./types"
import { ulid } from "ulid"

/**
 * Job queue for managing compilation tasks
 */
export class CompilationQueue {
  private jobs: Map<string, CompilationJob> = new Map()

  /**
   * Add a new compilation job to the queue
   */
  add(request: CompilationRequest): CompilationJob {
    const job: CompilationJob = {
      id: ulid(),
      request,
      status: "pending",
      retries: 0,
    }

    this.jobs.set(job.id, job)
    return job
  }

  /**
   * Get a job by ID
   */
  get(id: string): CompilationJob | undefined {
    return this.jobs.get(id)
  }

  /**
   * Update a job's properties
   */
  update(id: string, updates: Partial<CompilationJob>): void {
    const job = this.jobs.get(id)
    if (!job) return

    Object.assign(job, updates)
    this.jobs.set(id, job)
  }

  /**
   * Get jobs by status
   */
  getByStatus(status: CompilationStatus): CompilationJob[] {
    return Array.from(this.jobs.values()).filter((job) => job.status === status)
  }

  /**
   * Get next pending job
   */
  getNext(): CompilationJob | undefined {
    const pending = this.getByStatus("pending")
    return pending[0]
  }

  /**
   * Check if queue is complete (no pending or in-progress jobs)
   */
  isComplete(): boolean {
    const active = Array.from(this.jobs.values()).filter(
      (job) => job.status === "pending" || job.status === "compiling" || job.status === "uploading"
    )
    return active.length === 0
  }

  /**
   * Get queue statistics
   */
  getStats() {
    const all = Array.from(this.jobs.values())
    return {
      total: all.length,
      pending: all.filter((j) => j.status === "pending").length,
      compiling: all.filter((j) => j.status === "compiling").length,
      uploading: all.filter((j) => j.status === "uploading").length,
      completed: all.filter((j) => j.status === "completed").length,
      failed: all.filter((j) => j.status === "failed").length,
    }
  }

  /**
   * Clear all jobs
   */
  clear(): void {
    this.jobs.clear()
  }

  /**
   * Remove a job
   */
  remove(id: string): void {
    this.jobs.delete(id)
  }

  /**
   * Get all jobs
   */
  getAll(): CompilationJob[] {
    return Array.from(this.jobs.values())
  }
}
