/**
 * Game types supported by playscape-services
 */
export type GameType = "html5" | "raylib-c"

/**
 * Compilation status
 */
export type CompilationStatus = "pending" | "compiling" | "uploading" | "completed" | "failed"

/**
 * Request to compile a game
 */
export interface CompilationRequest {
  sourcePath: string
  gameType: GameType
  gameId?: string // Optional game ID for organization
  optimize?: boolean
  outputDir?: string
}

/**
 * Compiled game output files
 */
export interface CompiledOutput {
  wasmFile: string // Path to .wasm
  jsFile: string // Path to .js loader
  htmlFile: string // Path to playable .html
  dataFile?: string // Path to .data (raylib assets)
  assets?: string[] // Additional asset files
  metadata: {
    gameType: GameType
    compiledAt: string
    fileSize: number
    optimized: boolean
  }
}

/**
 * Internal job representation
 */
export interface CompilationJob {
  id: string
  request: CompilationRequest
  status: CompilationStatus
  retries: number
  error?: string
  outputFiles?: CompiledOutput
  startTime?: number
  endTime?: number
}

/**
 * Upload result from R2
 */
export interface UploadResult {
  success: boolean
  url: string // Public R2 URL for WASM file
  key: string // R2 key
  fileName: string
  size: number
  gameId?: string
  htmlUrl?: string // Public URL for game.html
  jsUrl?: string // Public URL for game.js
  dataUrl?: string // Public URL for game.data
}

/**
 * Complete compilation result
 */
export interface CompilationResult {
  success: boolean
  compiledOutput?: CompiledOutput
  uploadResult?: UploadResult
  error?: string
  totalTime: number
  playableUrl?: string
}

/**
 * Progress update callback
 */
export interface ProgressUpdate {
  stage: "validating" | "compiling" | "optimizing" | "uploading" | "completed" | "failed"
  progress: number // 0-100
  message: string
  eta?: number // Estimated time remaining in seconds
}

/**
 * Compiler options for HTML5
 */
export interface HTML5CompilerOptions {
  minify?: boolean
  sourceMaps?: boolean
  target?: "es2015" | "es2020" | "esnext"
}

/**
 * Compiler options for Raylib
 */
export interface RaylibCompilerOptions {
  optimization?: "O0" | "O1" | "O2" | "O3" | "Os"
  embedAssets?: boolean
  memorySize?: number // Initial memory in MB
  allowMemoryGrowth?: boolean
}
