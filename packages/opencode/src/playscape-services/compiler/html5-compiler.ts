import { join, dirname, basename } from "path"
import { existsSync, statSync, readdirSync, readFileSync, writeFileSync, mkdirSync } from "fs"
import type { CompilationRequest, CompiledOutput, HTML5CompilerOptions } from "../types"

/**
 * HTML5 Game Compiler
 * Bundles HTML5 games into a playable format
 */
export class HTML5Compiler {
  private options: HTML5CompilerOptions

  constructor(options: HTML5CompilerOptions = {}) {
    this.options = {
      minify: options.minify ?? true,
      sourceMaps: options.sourceMaps ?? false,
      target: options.target ?? "es2020",
    }
  }

  /**
   * Compile HTML5 game
   */
  async compile(request: CompilationRequest): Promise<CompiledOutput> {
    const { sourcePath, outputDir = "/tmp/playscape-builds" } = request

    // Validate source path
    if (!existsSync(sourcePath)) {
      throw new Error(`Source path not found: ${sourcePath}`)
    }

    // Determine if source is a file or directory
    const stats = statSync(sourcePath)
    const isDirectory = stats.isDirectory()

    // Find index.html
    let htmlPath: string
    if (isDirectory) {
      htmlPath = join(sourcePath, "index.html")
      if (!existsSync(htmlPath)) {
        throw new Error(`index.html not found in ${sourcePath}`)
      }
    } else if (sourcePath.endsWith(".html")) {
      htmlPath = sourcePath
    } else {
      throw new Error("Source must be a directory with index.html or an HTML file")
    }

    // Create output directory
    const buildId = `html5-${Date.now()}`
    const outputPath = join(outputDir, buildId)
    if (!existsSync(outputPath)) {
      mkdirSync(outputPath, { recursive: true })
    }

    // Read HTML content
    let htmlContent = readFileSync(htmlPath, "utf-8")

    // Inline scripts and styles if in same directory
    const baseDir = dirname(htmlPath)
    htmlContent = this.inlineAssets(htmlContent, baseDir)

    // Minify if requested
    if (this.options.minify) {
      htmlContent = this.minifyHTML(htmlContent)
    }

    // Write bundled HTML
    const outputHtmlPath = join(outputPath, "game.html")
    writeFileSync(outputHtmlPath, htmlContent)

    // Create a minimal JS loader (empty for HTML5 games)
    const jsContent = `// HTML5 Game Loader
console.log('Playscape HTML5 Game loaded');
`
    const outputJsPath = join(outputPath, "game.js")
    writeFileSync(outputJsPath, jsContent)

    // Create a dummy WASM file (HTML5 games don't use WASM, but the uploader expects it)
    const wasmContent = new Uint8Array([0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00]) // WASM magic number
    const outputWasmPath = join(outputPath, "game.wasm")
    writeFileSync(outputWasmPath, wasmContent)

    // Gather additional assets
    const assets: string[] = []
    if (isDirectory) {
      const files = this.gatherAssets(sourcePath, outputPath)
      assets.push(...files)
    }

    // Calculate total size
    const fileSize = this.calculateSize(outputPath)

    return {
      wasmFile: outputWasmPath,
      jsFile: outputJsPath,
      htmlFile: outputHtmlPath,
      assets,
      metadata: {
        gameType: "html5",
        compiledAt: new Date().toISOString(),
        fileSize,
        optimized: this.options.minify ?? true,
      },
    }
  }

  /**
   * Inline scripts and styles from external files
   */
  private inlineAssets(html: string, baseDir: string): string {
    // Inline external scripts
    html = html.replace(
      /<script\s+src=["']([^"']+)["'][^>]*><\/script>/g,
      (match, src) => {
        // Skip external URLs
        if (src.startsWith("http://") || src.startsWith("https://") || src.startsWith("//")) {
          return match
        }

        const scriptPath = join(baseDir, src)
        if (existsSync(scriptPath)) {
          const scriptContent = readFileSync(scriptPath, "utf-8")
          return `<script>${scriptContent}</script>`
        }
        return match
      }
    )

    // Inline external stylesheets
    html = html.replace(
      /<link\s+rel=["']stylesheet["']\s+href=["']([^"']+)["'][^>]*>/g,
      (match, href) => {
        // Skip external URLs
        if (href.startsWith("http://") || href.startsWith("https://") || href.startsWith("//")) {
          return match
        }

        const cssPath = join(baseDir, href)
        if (existsSync(cssPath)) {
          const cssContent = readFileSync(cssPath, "utf-8")
          return `<style>${cssContent}</style>`
        }
        return match
      }
    )

    return html
  }

  /**
   * Simple HTML minification
   */
  private minifyHTML(html: string): string {
    return html
      .replace(/<!--[\s\S]*?-->/g, "") // Remove comments
      .replace(/\s+/g, " ") // Collapse whitespace
      .replace(/>\s+</g, "><") // Remove space between tags
      .trim()
  }

  /**
   * Gather additional asset files (images, audio, etc.)
   */
  private gatherAssets(sourceDir: string, outputDir: string): string[] {
    const assets: string[] = []
    const assetExtensions = [".png", ".jpg", ".jpeg", ".gif", ".svg", ".mp3", ".wav", ".ogg", ".json"]

    const traverse = (dir: string) => {
      const files = readdirSync(dir)
      for (const file of files) {
        const fullPath = join(dir, file)
        const stat = statSync(fullPath)

        if (stat.isDirectory()) {
          traverse(fullPath)
        } else {
          const ext = file.substring(file.lastIndexOf("."))
          if (assetExtensions.includes(ext.toLowerCase())) {
            assets.push(fullPath)
          }
        }
      }
    }

    traverse(sourceDir)
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
