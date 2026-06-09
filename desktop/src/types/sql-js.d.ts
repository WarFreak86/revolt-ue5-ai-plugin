declare module "sql.js" {
  export interface Statement {
    step(): boolean;
    getAsObject(): Record<string, unknown>;
    free(): void;
  }

  export class Database {
    constructor(data?: Uint8Array | Buffer);
    run(sql: string, params?: Array<string | number | null>): void;
    prepare(sql: string, params?: Array<string | number | null>): Statement;
    export(): Uint8Array;
  }

  export interface SqlJsStatic {
    Database: typeof Database;
  }

  export default function initSqlJs(options?: { locateFile?: (fileName: string) => string; }): Promise<SqlJsStatic>;
}
