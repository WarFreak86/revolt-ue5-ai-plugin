import type { LogLevel, ServerConfig } from "./config.js";

const logRanks: Record<LogLevel, number> = {
  debug: 0,
  info: 1,
  warn: 2,
  error: 3
};

export class Logger {
  constructor(private readonly config: ServerConfig) {}

  debug(message: string): void {
    this.write("debug", message);
  }

  info(message: string): void {
    this.write("info", message);
  }

  warn(message: string): void {
    this.write("warn", message);
  }

  error(message: string): void {
    this.write("error", message);
  }

  private write(level: LogLevel, message: string): void {
    if (logRanks[level] < logRanks[this.config.logLevel]) {
      return;
    }

    process.stderr.write(`[revolt-mcp:${level}] ${message}\n`);
  }
}
