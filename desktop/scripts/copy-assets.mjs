import { copyFileSync, mkdirSync } from "node:fs";
import { join } from "node:path";

const rendererOut = join("dist", "renderer");
mkdirSync(rendererOut, { recursive: true });

for (const file of ["index.html", "styles.css"]) {
  copyFileSync(join("src", "renderer", file), join(rendererOut, file));
}
