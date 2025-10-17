import z from "zod/v4"
import { Auth } from "./index"
import { NamedError } from "../util/error"

export namespace AuthPlayscapeSupabase {
  const SUPABASE_URL = process.env.NEXT_PUBLIC_SUPABASE_URL || ""
  const SUPABASE_ANON_KEY = process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY || ""

  interface SupabaseAuthResponse {
    access_token: string
    refresh_token: string
    expires_in: number
    user: {
      id: string
      email: string
    }
  }

  interface SupabaseErrorResponse {
    error: string
    error_description?: string
  }

  interface OAuthUrlResponse {
    url: string
    provider: string
  }

  export async function loginWithOAuth(provider: "google" | "discord") {
    const redirectUrl = `https://playscape.gg/auth/callback`

    const response = await fetch(`${SUPABASE_URL}/auth/v1/authorize?provider=${provider}`, {
      method: "GET",
      headers: {
        apikey: SUPABASE_ANON_KEY,
      },
    })

    if (!response.ok) {
      throw new AuthenticationError({
        message: `Failed to initiate ${provider} OAuth flow`,
      })
    }

    const authUrl = `${SUPABASE_URL}/auth/v1/authorize?provider=${provider}&redirect_to=${encodeURIComponent(redirectUrl)}`

    return {
      url: authUrl,
      provider,
    }
  }

  export async function handleOAuthCallback(code: string) {
    const response = await fetch(`${SUPABASE_URL}/auth/v1/token?grant_type=authorization_code`, {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
        apikey: SUPABASE_ANON_KEY,
      },
      body: JSON.stringify({
        auth_code: code,
      }),
    })

    if (!response.ok) {
      const error: SupabaseErrorResponse = await response.json()
      throw new AuthenticationError({
        message: error.error_description || error.error || "OAuth callback failed",
      })
    }

    const data: SupabaseAuthResponse = await response.json()

    await Auth.set("playscape-supabase", {
      type: "oauth",
      refresh: data.refresh_token,
      access: data.access_token,
      expires: Date.now() + data.expires_in * 1000,
    })

    return data.user
  }

  export async function exchangeSessionForToken(accessToken: string, refreshToken: string) {
    await Auth.set("playscape-supabase", {
      type: "oauth",
      refresh: refreshToken,
      access: accessToken,
      expires: Date.now() + 3600 * 1000,
    })

    return await getCurrentUser()
  }

  export async function loginWithEmail(email: string, password: string) {
    const response = await fetch(`${SUPABASE_URL}/auth/v1/token?grant_type=password`, {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
        apikey: SUPABASE_ANON_KEY,
      },
      body: JSON.stringify({
        email,
        password,
      }),
    })

    if (!response.ok) {
      const error: SupabaseErrorResponse = await response.json()
      throw new AuthenticationError({
        message: error.error_description || error.error || "Authentication failed",
      })
    }

    const data: SupabaseAuthResponse = await response.json()

    await Auth.set("playscape-supabase", {
      type: "oauth",
      refresh: data.refresh_token,
      access: data.access_token,
      expires: Date.now() + data.expires_in * 1000,
    })

    return data.user
  }

  export async function signUpWithEmail(email: string, password: string) {
    const response = await fetch(`${SUPABASE_URL}/auth/v1/signup`, {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
        apikey: SUPABASE_ANON_KEY,
      },
      body: JSON.stringify({
        email,
        password,
      }),
    })

    if (!response.ok) {
      const error: SupabaseErrorResponse = await response.json()
      throw new AuthenticationError({
        message: error.error_description || error.error || "Sign up failed",
      })
    }

    const data: SupabaseAuthResponse = await response.json()

    await Auth.set("playscape-supabase", {
      type: "oauth",
      refresh: data.refresh_token,
      access: data.access_token,
      expires: Date.now() + data.expires_in * 1000,
    })

    return data.user
  }

  export async function access() {
    const info = await Auth.get("playscape-supabase")
    if (!info || info.type !== "oauth") return

    if (info.access && info.expires > Date.now()) {
      return info.access
    }

    const response = await fetch(`${SUPABASE_URL}/auth/v1/token?grant_type=refresh_token`, {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
        apikey: SUPABASE_ANON_KEY,
      },
      body: JSON.stringify({
        refresh_token: info.refresh,
      }),
    })

    if (!response.ok) {
      throw new TokenRefreshError({
        message: "Failed to refresh access token",
      })
    }

    const data: SupabaseAuthResponse = await response.json()

    await Auth.set("playscape-supabase", {
      type: "oauth",
      refresh: data.refresh_token,
      access: data.access_token,
      expires: Date.now() + data.expires_in * 1000,
    })

    return data.access_token
  }

  export async function logout() {
    const token = await access()
    if (!token) return

    await fetch(`${SUPABASE_URL}/auth/v1/logout`, {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
        Authorization: `Bearer ${token}`,
        apikey: SUPABASE_ANON_KEY,
      },
    })

    await Auth.remove("playscape-supabase")
  }

  export async function getCurrentUser() {
    const token = await access()
    if (!token) return null

    const response = await fetch(`${SUPABASE_URL}/auth/v1/user`, {
      headers: {
        Authorization: `Bearer ${token}`,
        apikey: SUPABASE_ANON_KEY,
      },
    })

    if (!response.ok) return null

    return await response.json()
  }

  export const AuthenticationError = NamedError.create(
    "AuthenticationError",
    z.object({
      message: z.string(),
    }),
  )

  export const TokenRefreshError = NamedError.create(
    "TokenRefreshError",
    z.object({
      message: z.string(),
    }),
  )

  export const LogoutError = NamedError.create(
    "LogoutError",
    z.object({
      message: z.string(),
    }),
  )
}
