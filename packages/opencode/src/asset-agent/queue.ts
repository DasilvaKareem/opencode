import type { AssetJob, AssetRequest, AssetType, JobStatus } from "./types"

/**
 * Priority Queue for managing asset generation jobs
 */
export class AssetJobQueue {
  private jobs = new Map<string, AssetJob>()
  private pendingQueue: AssetJob[] = []
  private completedJobs = new Map<string, AssetJob>()
  private failedJobs = new Map<string, AssetJob>()

  /**
   * Add a batch of asset requests to the queue
   */
  addBatch(requests: AssetRequest[]): void {
    const jobs = requests.map((req) => this.createJob(req))

    // Sort by priority (higher priority = processed first)
    const sorted = this.sortByPriority(jobs)

    sorted.forEach((job) => {
      this.jobs.set(job.id, job)
      this.pendingQueue.push(job)
    })
  }

  /**
   * Get next job to process
   */
  getNext(): AssetJob | undefined {
    // Check dependencies first
    for (let i = 0; i < this.pendingQueue.length; i++) {
      const job = this.pendingQueue[i]

      if (this.areDependenciesMet(job)) {
        this.pendingQueue.splice(i, 1)
        return job
      }
    }

    return undefined
  }

  /**
   * Update job status
   */
  updateJob(jobId: string, updates: Partial<AssetJob>): void {
    const job = this.jobs.get(jobId)
    if (!job) return

    Object.assign(job, updates)

    // Move to appropriate collection
    if (updates.status === "completed") {
      this.completedJobs.set(jobId, job)
    } else if (updates.status === "failed") {
      this.failedJobs.set(jobId, job)
    }
  }

  /**
   * Check if all jobs are complete
   */
  isComplete(): boolean {
    return this.pendingQueue.length === 0 && this.getInProgressCount() === 0
  }

  /**
   * Get queue statistics
   */
  getStats() {
    return {
      total: this.jobs.size,
      pending: this.pendingQueue.length,
      inProgress: this.getInProgressCount(),
      completed: this.completedJobs.size,
      failed: this.failedJobs.size,
    }
  }

  /**
   * Get all jobs by status
   */
  getJobsByStatus(status: JobStatus): AssetJob[] {
    return Array.from(this.jobs.values()).filter((job) => job.status === status)
  }

  /**
   * Get job by ID
   */
  getJob(jobId: string): AssetJob | undefined {
    return this.jobs.get(jobId)
  }

  /**
   * Get all completed jobs
   */
  getCompleted(): AssetJob[] {
    return Array.from(this.completedJobs.values())
  }

  /**
   * Get all failed jobs
   */
  getFailed(): AssetJob[] {
    return Array.from(this.failedJobs.values())
  }

  /**
   * Clear the queue
   */
  clear(): void {
    this.jobs.clear()
    this.pendingQueue = []
    this.completedJobs.clear()
    this.failedJobs.clear()
  }

  private createJob(request: AssetRequest): AssetJob {
    const provider = this.selectProvider(request.type)
    const estimatedTime = this.estimateTime(request.type)
    const priority = this.calculatePriority(request.type, estimatedTime)

    return {
      id: this.generateId(),
      request,
      provider,
      priority,
      estimatedTime,
      status: "pending",
      retries: 0,
    }
  }

  private selectProvider(type: AssetType): "fal" | "elevenlabs" {
    switch (type) {
      case "sprite":
      case "texture":
      case "3d-model":
      case "video":
        return "fal"
      case "sfx":
      case "music":
      case "voice":
        return "elevenlabs"
    }
  }

  private estimateTime(type: AssetType): number {
    // Estimated time in seconds
    const estimates: Record<AssetType, number> = {
      sprite: 8,
      texture: 8,
      "3d-model": 20,
      video: 60,
      sfx: 5,
      music: 30,
      voice: 3,
    }
    return estimates[type] || 10
  }

  private calculatePriority(type: AssetType, estimatedTime: number): number {
    // Priority calculation:
    // - Faster jobs get higher priority (process quick wins first)
    // - Dependencies are handled separately
    // Higher number = higher priority
    const basePriority = 100 - estimatedTime

    // Boost priority for fast assets
    if (estimatedTime <= 5) {
      return basePriority + 50
    }
    if (estimatedTime <= 10) {
      return basePriority + 25
    }

    return basePriority
  }

  private sortByPriority(jobs: AssetJob[]): AssetJob[] {
    return jobs.sort((a, b) => {
      // Check dependencies first
      const aHasDeps = (a.request.dependencies?.length || 0) > 0
      const bHasDeps = (b.request.dependencies?.length || 0) > 0

      if (aHasDeps && !bHasDeps) return 1
      if (!aHasDeps && bHasDeps) return -1

      // Then sort by priority
      return b.priority - a.priority
    })
  }

  private areDependenciesMet(job: AssetJob): boolean {
    if (!job.request.dependencies || job.request.dependencies.length === 0) {
      return true
    }

    // Check if all dependencies are completed
    return job.request.dependencies.every((depName) => {
      const depJob = Array.from(this.jobs.values()).find((j) => j.request.name === depName)
      return depJob?.status === "completed"
    })
  }

  private getInProgressCount(): number {
    return Array.from(this.jobs.values()).filter((job) => job.status === "in_progress").length
  }

  private generateId(): string {
    return `job_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`
  }
}
