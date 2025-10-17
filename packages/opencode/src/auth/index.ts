import path from "path"
import { Global } from "../global"
import fs from "fs/promises"
import z from "zod/v4"

export namespace Auth {
  export const Oauth = z
    .object({
      type: z.literal("oauth"),
      refresh: z.string(),
      access: z.string(),
      expires: z.number(),
    })
    .meta({ ref: "OAuth" })

  export const Api = z
    .object({
      type: z.literal("api"),
      key: z.string(),
    })
    .meta({ ref: "ApiAuth" })

  export const WellKnown = z
    .object({
      type: z.literal("wellknown"),
      key: z.string(),
      token: z.string(),
    })
    .meta({ ref: "WellKnownAuth" })

  export const Info = z.discriminatedUnion("type", [Oauth, Api, WellKnown]).meta({ ref: "Auth" })
  export type Info = z.infer<typeof Info>

  const filepath = path.join(Global.Path.data, "auth.json")

  export async function get(providerID: string) {
    const file = Bun.file(filepath)
    return file
      .json()
      .catch(() => ({}))
      .then((x) => x[providerID] as Info | undefined)
  }

  export async function all(): Promise<Record<string, Info>> {
    const file = Bun.file(filepath)
    return file.json().catch(() => ({}))
  }

  export async function set(key: string, info: Info) {
    const file = Bun.file(filepath)
    const data = await all()
    await Bun.write(file, JSON.stringify({ ...data, [key]: info }, null, 2))
    await fs.chmod(file.name!, 0o600)
  }

  export async function remove(key: string) {
    const file = Bun.file(filepath)
    const data = await all()
    delete data[key]
    await Bun.write(file, JSON.stringify(data, null, 2))
    await fs.chmod(file.name!, 0o600)
  }

  export async function ensureCredentials(providerID: string): Promise<boolean> {
    // Check if credentials already exist
    const existing = await get(providerID)
    if (existing) return true

    // Check if env var is set
    const { ModelsDev } = await import("../provider/models")
    const database = await ModelsDev.get()
    const provider = database[providerID]
    if (provider?.env.some((envVar) => process.env[envVar])) return true

    // No credentials found, need to prompt user to login
    return false
  }

  export async function promptLogin(providerID?: string): Promise<void> {
    const prompts = await import("@clack/prompts")
    const { ModelsDev } = await import("../provider/models")
    const { Plugin } = await import("../plugin")
    const { UI } = await import("../cli/ui")
    const { pipe, values, sortBy, map } = await import("remeda")

    await ModelsDev.refresh().catch(() => {})
    const providers = await ModelsDev.get()

    // If providerID is specified, try to login directly
    if (providerID) {
      const provider = providers[providerID]
      if (!provider) {
        throw new Error(`Provider ${providerID} not found`)
      }

      prompts.intro(`Login required for ${provider.name || providerID}`)

      // Check if there's a plugin with auth
      const plugin = await Plugin.list().then((x) => x.find((x) => x.auth?.provider === providerID))
      if (plugin?.auth) {
        let index = 0
        if (plugin.auth.methods.length > 1) {
          const method = await prompts.select({
            message: "Login method",
            options: plugin.auth.methods.map((x, index) => ({
              label: x.label,
              value: index.toString(),
            })),
          })
          if (prompts.isCancel(method)) throw new UI.CancelledError()
          index = parseInt(method)
        }
        const method = plugin.auth.methods[index]
        if (method.type === "oauth") {
          const authorize = await method.authorize()
          if (authorize.url) {
            prompts.log.info("Go to: " + authorize.url)
          }
          if (authorize.method === "auto") {
            if (authorize.instructions) {
              prompts.log.info(authorize.instructions)
            }
            const spinner = prompts.spinner()
            spinner.start("Waiting for authorization...")
            const result = await authorize.callback()
            if (result.type === "failed") {
              spinner.stop("Failed to authorize", 1)
              throw new Error("Authorization failed")
            }
            if (result.type === "success") {
              if ("refresh" in result) {
                await set(providerID, {
                  type: "oauth",
                  refresh: result.refresh,
                  access: result.access,
                  expires: result.expires,
                })
              }
              if ("key" in result) {
                await set(providerID, {
                  type: "api",
                  key: result.key,
                })
              }
              spinner.stop("Login successful")
            }
          }
          if (authorize.method === "code") {
            const code = await prompts.text({
              message: "Paste the authorization code here: ",
              validate: (x) => (x && x.length > 0 ? undefined : "Required"),
            })
            if (prompts.isCancel(code)) throw new UI.CancelledError()
            const result = await authorize.callback(code)
            if (result.type === "failed") {
              throw new Error("Authorization failed")
            }
            if (result.type === "success") {
              if ("refresh" in result) {
                await set(providerID, {
                  type: "oauth",
                  refresh: result.refresh,
                  access: result.access,
                  expires: result.expires,
                })
              }
              if ("key" in result) {
                await set(providerID, {
                  type: "api",
                  key: result.key,
                })
              }
              prompts.log.success("Login successful")
            }
          }
          prompts.outro("Done")
          return
        }
      }

      // Handle special providers
      if (providerID === "amazon-bedrock") {
        prompts.log.info(
          "Amazon bedrock can be configured with standard AWS environment variables like AWS_BEARER_TOKEN_BEDROCK, AWS_PROFILE or AWS_ACCESS_KEY_ID",
        )
        prompts.outro("Done")
        return
      }

      if (providerID === "google-vertex") {
        prompts.log.info(
          "Google Cloud Vertex AI uses Application Default Credentials. Set GOOGLE_APPLICATION_CREDENTIALS or run 'gcloud auth application-default login'. Optionally set GOOGLE_CLOUD_PROJECT and GOOGLE_CLOUD_LOCATION (or VERTEX_LOCATION)",
        )
        prompts.outro("Done")
        return
      }

      if (providerID === "opencode") {
        prompts.log.info("Create an api key at https://playscape.ai/auth")
      }

      if (providerID === "playscape-supabase") {
        const { AuthPlayscapeSupabase } = await import("./playscape-supabase")

        const authMethod = await prompts.select({
          message: "Select authentication method",
          options: [
            { label: "Google OAuth", value: "google" },
            { label: "Discord OAuth", value: "discord" },
            { label: "Email & Password", value: "email" },
          ],
        })

        if (prompts.isCancel(authMethod)) throw new UI.CancelledError()

        if (authMethod === "google" || authMethod === "discord") {
          const oauthResult = await AuthPlayscapeSupabase.loginWithOAuth(authMethod)
          prompts.log.info("Go to: " + oauthResult.url)

          const code = await prompts.text({
            message: "Paste the authorization code from the callback URL: ",
            validate: (x) => (x && x.length > 0 ? undefined : "Required"),
          })

          if (prompts.isCancel(code)) throw new UI.CancelledError()

          try {
            const user = await AuthPlayscapeSupabase.handleOAuthCallback(code)
            prompts.log.success(`Logged in as ${user.email}`)
            prompts.outro("Done")
            return
          } catch (error: any) {
            prompts.log.error(error.message || "OAuth authentication failed")
            prompts.outro("Done")
            return
          }
        }

        if (authMethod === "email") {
          const email = await prompts.text({
            message: "Email address",
            validate: (x) => (x && x.includes("@") ? undefined : "Invalid email"),
          })

          if (prompts.isCancel(email)) throw new UI.CancelledError()

          const password = await prompts.password({
            message: "Password",
            validate: (x) => (x && x.length > 0 ? undefined : "Required"),
          })

          if (prompts.isCancel(password)) throw new UI.CancelledError()

          try {
            const user = await AuthPlayscapeSupabase.loginWithEmail(email, password)
            prompts.log.success(`Logged in as ${user.email}`)
            prompts.outro("Done")
            return
          } catch (error: any) {
            prompts.log.error(error.message || "Email/password authentication failed")
            prompts.outro("Done")
            return
          }
        }
      }

      if (providerID === "vercel") {
        prompts.log.info("You can create an api key at https://vercel.link/ai-gateway-token")
      }

      // Default API key prompt
      const key = await prompts.password({
        message: `Enter your API key for ${provider.name || providerID}`,
        validate: (x) => (x && x.length > 0 ? undefined : "Required"),
      })
      if (prompts.isCancel(key)) throw new UI.CancelledError()
      await set(providerID, {
        type: "api",
        key,
      })
      prompts.outro("Done")
      return
    }

    // If no providerID specified, show provider selection
    prompts.intro("Login required")
    const priority: Record<string, number> = {
      opencode: 0,
      anthropic: 1,
      "github-copilot": 2,
      openai: 3,
      google: 4,
      openrouter: 5,
      vercel: 6,
    }
    const provider = await prompts.autocomplete({
      message: "Select provider",
      maxItems: 8,
      options: pipe(
        providers,
        values(),
        sortBy(
          (x) => priority[x.id] ?? 99,
          (x) => x.name ?? x.id,
        ),
        map((x) => ({
          label: x.name,
          value: x.id,
          hint: priority[x.id] <= 1 ? "recommended" : undefined,
        })),
      ),
    })

    if (prompts.isCancel(provider)) throw new UI.CancelledError()

    // Recursively call with the selected provider
    await promptLogin(provider as string)
  }
}
