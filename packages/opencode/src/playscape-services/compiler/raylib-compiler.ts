import { join, dirname, basename, extname } from "path"
import { existsSync, statSync, readdirSync, mkdirSync, copyFileSync } from "fs"
import { spawn } from "child_process"
import type { CompilationRequest, CompiledOutput, RaylibCompilerOptions } from "../types"

/**
 * Raylib C Game Compiler
 * Compiles Raylib C games to WASM using Emscripten
 */
export class RaylibCompiler {
  private options: RaylibCompilerOptions

  constructor(options: RaylibCompilerOptions = {}) {
    this.options = {
      optimization: options.optimization ?? "O2",
      embedAssets: options.embedAssets ?? true,
      memorySize: options.memorySize ?? 64,
      allowMemoryGrowth: options.allowMemoryGrowth ?? true,
    }
  }

  /**
   * Compile Raylib game to WASM
   */
  async compile(request: CompilationRequest): Promise<CompiledOutput> {
    const { sourcePath, outputDir = "/tmp/playscape-builds" } = request

    // Validate Emscripten installation
    await this.validateEmscripten()

    // Validate source path
    if (!existsSync(sourcePath)) {
      throw new Error(`Source path not found: ${sourcePath}`)
    }

    // Determine if source is a file or directory
    const stats = statSync(sourcePath)
    const isDirectory = stats.isDirectory()

    // Find C source files
    let sourceFiles: string[]
    let baseDir: string

    if (isDirectory) {
      baseDir = sourcePath
      sourceFiles = this.findSourceFiles(sourcePath)
      if (sourceFiles.length === 0) {
        throw new Error(`No .c source files found in ${sourcePath}`)
      }
    } else if (sourcePath.endsWith(".c")) {
      baseDir = dirname(sourcePath)
      sourceFiles = [sourcePath]
    } else {
      throw new Error("Source must be a directory with .c files or a .c file")
    }

    // Create output directory
    const buildId = `raylib-${Date.now()}`
    const outputPath = join(outputDir, buildId)
    if (!existsSync(outputPath)) {
      mkdirSync(outputPath, { recursive: true })
    }

    // Find assets directory if it exists
    const assetsDir = join(baseDir, "assets")
    const hasAssets = existsSync(assetsDir) && statSync(assetsDir).isDirectory()

    // Build Emscripten command
    const emccArgs = this.buildEmccCommand(sourceFiles, outputPath, hasAssets ? assetsDir : undefined)

    // Run compilation
    await this.runEmscripten(emccArgs)

    // Verify output files
    const wasmPath = join(outputPath, "game.wasm")
    const jsPath = join(outputPath, "game.js")
    const htmlPath = join(outputPath, "game.html")
    const dataPath = join(outputPath, "game.data")

    if (!existsSync(wasmPath)) {
      throw new Error("Compilation failed: game.wasm not generated")
    }

    // Calculate total size
    const fileSize = this.calculateSize(outputPath)

    return {
      wasmFile: wasmPath,
      jsFile: jsPath,
      htmlFile: htmlPath,
      dataFile: hasAssets && existsSync(dataPath) ? dataPath : undefined,
      assets: hasAssets ? this.gatherAssets(assetsDir) : [],
      metadata: {
        gameType: "raylib-c",
        compiledAt: new Date().toISOString(),
        fileSize,
        optimized: this.options.optimization !== "O0",
      },
    }
  }

  /**
   * Validate Emscripten is installed
   */
  private async validateEmscripten(): Promise<void> {
    return new Promise((resolve, reject) => {
      const emcc = spawn("emcc", ["--version"])

      emcc.on("error", (err) => {
        reject(
          new Error(
            "Emscripten not found. Please install Emscripten SDK: https://emscripten.org/docs/getting_started/downloads.html",
          ),
        )
      })

      emcc.on("close", (code) => {
        if (code === 0) {
          resolve()
        } else {
          reject(new Error("Emscripten validation failed"))
        }
      })
    })
  }

  /**
   * Find all .c source files in directory
   */
  private findSourceFiles(dir: string): string[] {
    const sources: string[] = []

    const traverse = (path: string) => {
      const files = readdirSync(path)
      for (const file of files) {
        const fullPath = join(path, file)
        const stat = statSync(fullPath)

        if (stat.isDirectory() && file !== "assets") {
          traverse(fullPath)
        } else if (file.endsWith(".c")) {
          sources.push(fullPath)
        }
      }
    }

    traverse(dir)
    return sources
  }

  /**
   * Build Emscripten compilation command
   */
  private buildEmccCommand(sourceFiles: string[], outputPath: string, assetsDir?: string): string[] {
    const args = [
      ...sourceFiles,
      "-o",
      join(outputPath, "game.html"),
      `-${this.options.optimization}`,
      "-s",
      "USE_GLFW=3",
      "-s",
      "FULL_ES2=1",
      "-s",
      "ALLOW_MEMORY_GROWTH=1",
      "-s",
      `INITIAL_MEMORY=${this.options.memorySize! * 1024 * 1024}`,
      "-s",
      "EXPORTED_RUNTIME_METHODS=['ccall','cwrap']",
      "-s",
      "EXPORTED_FUNCTIONS=['_main']",
      // Raylib flags
      "-DPLATFORM_WEB",
      // Include raylib headers and library for web
      "-I/tmp/raylib/src",
      "-L/tmp/raylib/src",
      "/tmp/raylib/src/libraylib.web.a",
    ]

    // Embed assets if directory exists
    if (assetsDir && this.options.embedAssets) {
      args.push("--preload-file", `${assetsDir}@/assets`)
    }

    // Add shell file for better web compatibility
    args.push("--shell-file", this.getShellTemplate())

    return args
  }

  /**
   * Get HTML shell template path (or use default)
   */
  private getShellTemplate(): string {
    // Try to find Emscripten's shell_minimal.html
    const emsdkPath = process.env.EMSDK
    if (emsdkPath) {
      const shellPath = join(emsdkPath, "upstream", "emscripten", "src", "shell_minimal.html")
      if (existsSync(shellPath)) {
        return shellPath
      }
    }

    // Try homebrew installation
    const brewShellPath = "/opt/homebrew/Cellar/emscripten/4.0.17/libexec/src/shell_minimal.html"
    if (existsSync(brewShellPath)) {
      return brewShellPath
    }

    // Fallback to relative path (might work in some setups)
    return "shell_minimal.html"
  }

  /**
   * Run Emscripten compiler
   */
  private async runEmscripten(args: string[]): Promise<void> {
    return new Promise((resolve, reject) => {
      const emcc = spawn("emcc", args, {
        stdio: ["ignore", "pipe", "pipe"],
      })

      let stdout = ""
      let stderr = ""

      emcc.stdout?.on("data", (data) => {
        stdout += data.toString()
      })

      emcc.stderr?.on("data", (data) => {
        stderr += data.toString()
      })

      emcc.on("close", (code) => {
        if (code === 0) {
          resolve()
        } else {
          reject(new Error(`Emscripten compilation failed (exit code ${code}):\n${stderr || stdout}`))
        }
      })

      emcc.on("error", (err) => {
        reject(new Error(`Failed to run emcc: ${err.message}`))
      })
    })
  }

  /**
   * Gather asset files
   */
  private gatherAssets(assetsDir: string): string[] {
    const assets: string[] = []

    const traverse = (dir: string) => {
      const files = readdirSync(dir)
      for (const file of files) {
        const fullPath = join(dir, file)
        const stat = statSync(fullPath)

        if (stat.isDirectory()) {
          traverse(fullPath)
        } else {
          assets.push(fullPath)
        }
      }
    }

    traverse(assetsDir)
    return assets
  }

  /**
   * Calculate total size of output directory
   */
  private calculateSize(dir: string): number {
    let totalSize = 0

    const traverse = (path: string) => {
      const stat = statSync(path)
      if (stat.isDirectory()) {
        const files = readdirSync(path)
        files.forEach((file) => traverse(join(path, file)))
      } else {
        totalSize += stat.size
      }
    }

    traverse(dir)
    return totalSize
  }
}
