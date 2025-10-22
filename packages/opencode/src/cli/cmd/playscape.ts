import { cmd } from "./cmd"
import * as prompts from "@clack/prompts"
import { UI } from "../ui"
import { existsSync, statSync, readdirSync } from "fs"
import { join, extname } from "path"
import type { GameType } from "../../playscape-services/types"

export const PlayscapeCommand = cmd({
  command: "compile <path>",
  describe: "compile a game to WASM and upload to Playscape",
  builder: (yargs) =>
    yargs
      .positional("path", {
        describe: "path to game source (directory or file)",
        type: "string",
        demandOption: true,
      })
      .option("type", {
        describe: "game type (html5 or raylib-c)",
        type: "string",
        choices: ["html5", "raylib-c"],
      })
      .option("game-id", {
        describe: "associate with existing game ID",
        type: "string",
      })
      .option("optimize", {
        describe: "enable optimizations",
        type: "boolean",
        default: true,
      })
      .option("backend-url", {
        describe: "backend URL (for development)",
        type: "string",
      }),
  async handler(args) {
    UI.empty()
    prompts.intro("🎮 Playscape Game Compilation")

    const sourcePath = args.path as string

    // Validate source path
    if (!existsSync(sourcePath)) {
      prompts.log.error(`Source path not found: ${sourcePath}`)
      prompts.outro("Failed")
      return
    }

    // Auto-detect game type if not provided
    let gameType = args.type as GameType | undefined
    if (!gameType) {
      const detected = detectGameType(sourcePath)
      if (!detected) {
        prompts.log.error("Could not auto-detect game type. Please specify with --type")
        prompts.outro("Failed")
        return
      }
      gameType = detected
      prompts.log.info(`Auto-detected game type: ${gameType}`)
    }

    // Validate authentication
    const { AuthPlayscapeSupabase } = await import("../../auth/playscape-supabase")
    const token = await AuthPlayscapeSupabase.access()
    if (!token) {
      prompts.log.error("Not authenticated. Please run 'playscape auth login' first")
      prompts.outro("Failed")
      return
    }

    const user = await AuthPlayscapeSupabase.getCurrentUser()
    if (user) {
      prompts.log.info(`Authenticated as: ${user.email}`)
    }

    // Import controller
    const { PlayscapeServiceController } = await import("../../playscape-services/controller")
    const controller = new PlayscapeServiceController(args.backendUrl)

    // Create compilation request
    const request = {
      sourcePath,
      gameType,
      gameId: args.gameId,
      optimize: args.optimize,
    }

    // Start compilation with progress tracking
    const spinner = prompts.spinner()
    spinner.start("Preparing compilation...")

    let lastProgress = 0

    try {
      const result = await controller.compile(request, (update) => {
        // Update spinner with progress
        if (update.progress !== lastProgress) {
          const progressBar = createProgressBar(update.progress)
          spinner.message(`${progressBar} ${update.message}`)
          lastProgress = update.progress
        }
      })

      if (result.success) {
        spinner.stop("Compilation complete!")

        prompts.log.success("Game compiled and uploaded successfully!")

        if (result.playableUrl) {
          prompts.log.info(`Playable URL: ${result.playableUrl}`)
        }

        if (result.uploadResult) {
          prompts.log.info(`WASM URL: ${result.uploadResult.url}`)
          prompts.log.info(`Size: ${formatBytes(result.uploadResult.size)}`)
        }

        prompts.log.info(`Total time: ${(result.totalTime / 1000).toFixed(2)}s`)

        prompts.outro("Success!")
      } else {
        spinner.stop("Compilation failed", 1)
        prompts.log.error(result.error || "Unknown error")
        prompts.outro("Failed")
      }
    } catch (error: any) {
      spinner.stop("Compilation failed", 1)
      prompts.log.error(error.message || "Unknown error")
      prompts.outro("Failed")
    }
  },
})

/**
 * Auto-detect game type from source path
 */
function detectGameType(sourcePath: string): GameType | undefined {
  const stats = statSync(sourcePath)

  if (stats.isFile()) {
    const ext = extname(sourcePath)
    if (ext === ".html") return "html5"
    if (ext === ".c") return "raylib-c"
    return undefined
  }

  // It's a directory - check contents
  const files = readdirSync(sourcePath)

  // Check for index.html
  if (files.includes("index.html")) {
    return "html5"
  }

  // Check for .c files
  const hasCFiles = files.some((f) => f.endsWith(".c"))
  if (hasCFiles) {
    return "raylib-c"
  }

  return undefined
}

/**
 * Create a visual progress bar
 */
function createProgressBar(progress: number, width: number = 20): string {
  const filled = Math.round((progress / 100) * width)
  const empty = width - filled
  return `[${"█".repeat(filled)}${" ".repeat(empty)}] ${progress}%`
}

/**
 * Format bytes to human-readable size
 */
function formatBytes(bytes: number): string {
  if (bytes === 0) return "0 B"
  const k = 1024
  const sizes = ["B", "KB", "MB", "GB"]
  const i = Math.floor(Math.log(bytes) / Math.log(k))
  return `${(bytes / Math.pow(k, i)).toFixed(2)} ${sizes[i]}`
}
