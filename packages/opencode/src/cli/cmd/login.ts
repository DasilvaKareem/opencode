import { cmd } from "./cmd"
import { AuthLoginCommand } from "./auth"

export const LoginCommand = cmd({
  command: "login [url]",
  describe: "log in to a provider",
  builder: (yargs) =>
    yargs.positional("url", {
      describe: "Playscape auth provider",
      type: "string",
    }),
  async handler(args) {
    return AuthLoginCommand.handler(args)
  },
})
