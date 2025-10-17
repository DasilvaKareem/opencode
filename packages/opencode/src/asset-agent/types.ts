/**
 * Asset types supported by the Asset Agent
 */
export type AssetType = "sprite" | "texture" | "3d-model" | "sfx" | "music" | "voice" | "video"

/**
 * Provider types
 */
export type ProviderType = "fal" | "elevenlabs"

/**
 * Job status
 */
export type JobStatus = "pending" | "in_progress" | "completed" | "failed"

/**
 * Asset request from Main Agent
 */
export interface AssetRequest {
  name: string
  type: AssetType
  description: string
  parameters?: {
    // Image/Sprite specific
    aspectRatio?: "1:1" | "16:9" | "9:16" | "3:4" | "4:3"
    resolution?: "1K" | "2K"
    // 3D Model specific
    format?: "glb" | "usdz" | "fbx" | "obj" | "stl"
    quality?: "high" | "medium" | "low"
    // Audio specific
    duration?: number
    genre?: string
    // Common
    seed?: number
  }
  dependencies?: string[] // Other asset names this depends on
}

/**
 * Internal job representation
 */
export interface AssetJob {
  id: string
  request: AssetRequest
  provider: ProviderType
  priority: number
  estimatedTime: number
  status: JobStatus
  retries: number
  error?: string
  result?: AssetResult
  startTime?: number
  endTime?: number
}

/**
 * Asset generation result
 */
export interface AssetResult {
  filePath: string
  url: string
  type: AssetType
  metadata: {
    provider: ProviderType
    cost: number
    generationTime: number
    seed?: number
    size?: number
  }
}

/**
 * Batch generation request
 */
export interface BatchAssetRequest {
  assets: AssetRequest[]
  budget?: number
  maxTime?: number
}

/**
 * Batch generation result
 */
export interface BatchAssetResult {
  success: number
  failed: number
  totalCost: number
  totalTime: number
  assets: Record<string, AssetResult>
  failures: Record<string, string>
  manifest: AssetManifest
}

/**
 * Asset manifest for project
 */
export interface AssetManifest {
  created: string
  totalAssets: number
  byType: Record<AssetType, number>
  totalCost: number
  totalTime: number
}

/**
 * Progress update
 */
export interface ProgressUpdate {
  total: number
  completed: number
  failed: number
  inProgress: number
  currentJobs: Array<{
    id: string
    name: string
    type: AssetType
    progress: number
    status: JobStatus
  }>
  estimatedTimeRemaining: number
}
