export namespace Flag {
  export const PLAYSCAPE_AUTO_SHARE = truthy("PLAYSCAPE_AUTO_SHARE")
  export const PLAYSCAPE_CONFIG = process.env["PLAYSCAPE_CONFIG"]
  export const PLAYSCAPE_CONFIG_CONTENT = process.env["PLAYSCAPE_CONFIG_CONTENT"]
  export const PLAYSCAPE_DISABLE_AUTOUPDATE = truthy("PLAYSCAPE_DISABLE_AUTOUPDATE")
  export const PLAYSCAPE_DISABLE_PRUNE = truthy("PLAYSCAPE_DISABLE_PRUNE")
  export const PLAYSCAPE_PERMISSION = process.env["PLAYSCAPE_PERMISSION"]
  export const PLAYSCAPE_DISABLE_DEFAULT_PLUGINS = truthy("PLAYSCAPE_DISABLE_DEFAULT_PLUGINS")
  export const PLAYSCAPE_DISABLE_LSP_DOWNLOAD = truthy("PLAYSCAPE_DISABLE_LSP_DOWNLOAD")
  export const PLAYSCAPE_ENABLE_EXPERIMENTAL_MODELS = truthy("PLAYSCAPE_ENABLE_EXPERIMENTAL_MODELS")
  export const PLAYSCAPE_DISABLE_AUTOCOMPACT = truthy("PLAYSCAPE_DISABLE_AUTOCOMPACT")

  // Experimental
  export const PLAYSCAPE_EXPERIMENTAL_WATCHER = truthy("PLAYSCAPE_EXPERIMENTAL_WATCHER")
  export const PLAYSCAPE_EXPERIMENTAL_NO_BOOTSTRAP = truthy("PLAYSCAPE_EXPERIMENTAL_NO_BOOTSTRAP")

  // Asset Generation API Keys
  export const FAL_KEY = process.env["FAL_KEY"]
  export const ELEVENLABS_API_KEY = process.env["ELEVENLABS_API_KEY"]

  function truthy(key: string) {
    const value = process.env[key]?.toLowerCase()
    return value === "true" || value === "1"
  }
}
