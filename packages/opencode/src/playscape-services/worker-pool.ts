import { HTML5Compiler } from "./compiler/html5-compiler"
import { RaylibCompiler } from "./compiler/raylib-compiler"
import type { CompilationJob, CompiledOutput } from "./types"

/**
 * Worker pool for parallel compilation
 */
export class WorkerPool {
  private html5Compiler: HTML5Compiler
  private raylibCompiler: RaylibCompiler
  private activeWorkers: Map<string, Promise<CompiledOutput>> = new Map()
  private maxConcurrent: number

  constructor(maxConcurrent: number = 4) {
    this.maxConcurrent = maxConcurrent
    this.html5Compiler = new HTML5Compiler()
    this.raylibCompiler = new RaylibCompiler()
  }

  /**
   * Execute a compilation job
   */
  async execute(job: CompilationJob): Promise<CompiledOutput> {
    const { request } = job

    // Create compilation task
    const task = this.compileGame(job)

    // Track active worker
    this.activeWorkers.set(job.id, task)

    try {
      const result = await task
      return result
    } finally {
      // Remove from active workers
      this.activeWorkers.delete(job.id)
    }
  }

  /**
   * Compile a game based on its type
   */
  private async compileGame(job: CompilationJob): Promise<CompiledOutput> {
    const { request } = job

    switch (request.gameType) {
      case "html5":
        return await this.html5Compiler.compile(request)

      case "raylib-c":
        return await this.raylibCompiler.compile(request)

      default:
        throw new Error(`Unsupported game type: ${request.gameType}`)
    }
  }

  /**
   * Check if pool has available slots
   */
  hasAvailableSlot(): boolean {
    return this.activeWorkers.size < this.maxConcurrent
  }

  /**
   * Get number of active workers
   */
  getActiveCount(): number {
    return this.activeWorkers.size
  }

  /**
   * Get active worker IDs
   */
  getActiveWorkers(): string[] {
    return Array.from(this.activeWorkers.keys())
  }

  /**
   * Wait for a worker slot to become available
   */
  async waitForSlot(): Promise<void> {
    while (!this.hasAvailableSlot()) {
      await Promise.race(Array.from(this.activeWorkers.values()))
    }
  }

  /**
   * Wait for all workers to complete
   */
  async waitForAll(): Promise<void> {
    await Promise.all(Array.from(this.activeWorkers.values()))
  }

  /**
   * Clear all workers
   */
  clear(): void {
    this.activeWorkers.clear()
  }
}
