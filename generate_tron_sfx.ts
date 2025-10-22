import { ElevenLabsProvider } from "./packages/opencode/src/provider/elevenlabs"
import { writeFileSync } from "fs"
import { join } from "path"

const provider = new ElevenLabsProvider()
const outputDir = "/Users/kareemdasilva/code/opencode"

const sounds = [
  {
    filename: "bike_engine.mp3",
    prompt:
      "Continuous electronic synthetic engine hum sound, futuristic and smooth high-tech light cycle idling sound, Tron style cyberpunk, loopable ambient hum with slight modulation",
    duration: 3,
    purpose: "Continuous engine hum for light cycle (loopable)",
  },
  {
    filename: "bike_boost.mp3",
    prompt:
      "Sharp powerful electronic boost acceleration whoosh sound, high energy surge when light cycle speeds up, Tron style futuristic cyberpunk with rising pitch and intensity",
    duration: 2,
    purpose: "Boost/acceleration sound effect",
  },
  {
    filename: "bike_turn.mp3",
    prompt:
      "Quick sharp electronic chirp beep sound for 90-degree turn, very short snappy Tron style futuristic sound effect, digital blip",
    duration: 0.5,
    purpose: "Turn indicator sound (90-degree turns)",
  },
  {
    filename: "bike_explosion.mp3",
    prompt:
      "Dramatic electronic explosion destruction sound, digital synthetic crash with heavy impact, Tron style light cycle destruction, cyberpunk game over sound",
    duration: 2,
    purpose: "Crash/explosion sound when bike is destroyed",
  },
  {
    filename: "menu_select.mp3",
    prompt:
      "Clean futuristic UI blip beep for menu navigation, short Tron style cyberpunk interface sound, smooth digital click",
    duration: 0.3,
    purpose: "Menu selection/navigation sound",
  },
]

console.log("🎵 Generating Tron-style sound effects...\n")

const results: Array<{
  filename: string
  filepath?: string
  duration?: number
  purpose: string
  status: string
  error?: string
}> = []

for (const sound of sounds) {
  try {
    console.log(`Generating ${sound.filename}...`)

    const blob = await provider.generateSFX({
      description: sound.prompt,
      duration: sound.duration,
    })

    // Convert blob to buffer and save
    const arrayBuffer = await blob.arrayBuffer()
    const buffer = Buffer.from(arrayBuffer)

    const filepath = join(outputDir, sound.filename)
    writeFileSync(filepath, buffer)

    results.push({
      filename: sound.filename,
      filepath: filepath,
      duration: sound.duration,
      purpose: sound.purpose,
      status: "✓",
    })

    console.log(`  ✓ Saved to ${filepath}`)
  } catch (error: any) {
    results.push({
      filename: sound.filename,
      purpose: sound.purpose,
      status: "✗",
      error: error.message,
    })
    console.log(`  ✗ Error: ${error.message}`)
  }
}

console.log("\n📊 Generation Summary:\n")
console.log("┌─────────────────────────────────────────────────────────────────────────┐")
results.forEach((r) => {
  console.log(`│ ${r.status} ${r.filename.padEnd(25)} │`)
  if (r.filepath) console.log(`│   Path: ${r.filepath.padEnd(62)} │`)
  if (r.duration) console.log(`│   Duration: ${r.duration}s`.padEnd(74) + "│")
  console.log(`│   Purpose: ${r.purpose.padEnd(61)} │`)
  if (r.error) console.log(`│   Error: ${r.error.padEnd(62)} │`)
  console.log("├─────────────────────────────────────────────────────────────────────────┤")
})
console.log("└─────────────────────────────────────────────────────────────────────────┘")

console.log("\n✅ All Tron-style sound effects generated!\n")
console.log("Generated files:")
results.forEach((r) => {
  if (r.filepath) {
    console.log(`  • ${r.filepath}`)
    console.log(`    ${r.purpose}`)
  }
})
