#!/usr/bin/env bun
import { AssetAgentController } from "./packages/opencode/src/asset-agent/controller"
import type { BatchAssetRequest } from "./packages/opencode/src/asset-agent/types"

console.log("🎬 Generating epic anime-style video intro...\n")

const controller = new AssetAgentController("./assets")

const request: BatchAssetRequest = {
  assets: [
    {
      name: "anime_warrior_intro",
      type: "video",
      description:
        "Cinematic anime-style video intro: A dramatic anime warrior character with flowing cloak standing on a cliff or rooftop, looking out over a sprawling futuristic fantasy city below. Camera angle from behind and side of the warrior. Epic atmospheric lighting with dramatic sunset or moody sky. Wind blowing through warrior's hair and cloak. City lights twinkling in the distance far below. High quality anime art style similar to Attack on Titan, Demon Slayer, or Sword Art Online. Smooth cinematic camera movement with slight pan. Professional anime intro quality with dramatic composition. Epic fantasy/sci-fi atmosphere. Heroic and atmospheric mood.",
      parameters: {
        aspectRatio: "16:9",
        seed: Math.floor(Math.random() * 1000000),
      },
    },
  ],
}

const result = await controller.generateBatch(request, (progress) => {
  console.log(progress)
})

console.log("\n✅ Video Generation Complete!\n")
console.log("📊 Summary:")
console.log(`   Success: ${result.success}`)
console.log(`   Failed: ${result.failed}`)
console.log(`   Total Cost: $${result.totalCost.toFixed(4)}`)
console.log(`   Total Time: ${result.totalTime.toFixed(2)}s`)

if (result.success > 0) {
  const videoAsset = result.assets["anime_warrior_intro"]
  console.log("\n🎥 Video Details:")
  console.log(`   File: ${videoAsset.filePath}`)
  console.log(`   URL: ${videoAsset.url}`)
  console.log(`   Size: ${(videoAsset.metadata.size! / 1024 / 1024).toFixed(2)} MB`)
  console.log(`   Generation Time: ${(videoAsset.metadata.generationTime / 1000).toFixed(2)}s`)
  console.log(`   Seed: ${videoAsset.metadata.seed}`)
}

if (result.failed > 0) {
  console.log("\n❌ Failures:")
  for (const [name, error] of Object.entries(result.failures)) {
    console.log(`   ${name}: ${error}`)
  }
}
