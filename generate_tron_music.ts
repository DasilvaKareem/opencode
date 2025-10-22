import { AssetAgentController } from "./packages/opencode/src/asset-agent/controller"
import type { BatchAssetRequest } from "./packages/opencode/src/asset-agent/types"

/**
 * Generate retro synthwave/electronic background music for Tron light cycle game
 */
async function generateTronMusic() {
  console.log("🎵 Generating Tron synthwave music...")

  // Create asset controller with output to current directory
  const controller = new AssetAgentController("/Users/kareemdasilva/code/opencode")

  // Define the music generation request
  const request: BatchAssetRequest = {
    assets: [
      {
        name: "tron_bgm",
        type: "music",
        description:
          "Retro 80s synthwave electronic music with pulsing basslines and futuristic arpeggiators. High energy EDM with classic Tron soundtrack vibes. Seamless loop-friendly continuous gameplay background music. Neon digital atmosphere with driving synth rhythms and cyberpunk aesthetic.",
        parameters: {
          duration: 30, // 30 seconds (maximum allowed, designed for seamless looping)
          genre: "synthwave electronic EDM 80s retro cyberpunk",
        },
      },
    ],
  }

  try {
    // Generate with progress updates
    const result = await controller.generateBatch(request, (progress) => {
      console.log(progress)
    })

    // Check results
    if (result.success > 0) {
      const asset = result.assets["tron_bgm"]
      console.log("\n✅ Music generation successful!")
      console.log(`📁 File path: ${asset.filePath}`)
      console.log(`💰 Cost: $${result.totalCost.toFixed(4)}`)
      console.log(`⏱️  Time: ${result.totalTime.toFixed(2)}s`)
      console.log(`📊 File size: ${(asset.metadata.size / 1024 / 1024).toFixed(2)} MB`)

      return asset.filePath
    } else {
      console.error("\n❌ Music generation failed:")
      console.error(result.failures["tron_bgm"])
      throw new Error(result.failures["tron_bgm"])
    }
  } catch (error) {
    console.error("\n❌ Error generating music:")
    console.error(error)
    throw error
  }
}

// Run the generation
generateTronMusic()
  .then((filePath) => {
    console.log(`\n🎉 Complete! Music saved to: ${filePath}`)
    process.exit(0)
  })
  .catch((error) => {
    console.error("\n💥 Generation failed:", error)
    process.exit(1)
  })
