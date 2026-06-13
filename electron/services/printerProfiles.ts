import fs from "node:fs";
import path from "node:path";
import { app } from "electron";
import type { PrinterProfile } from "../../src/shared/types";

interface ProfileStore {
  profiles: Record<string, PrinterProfile>;
}

export class PrinterProfileService {
  private readonly filePath: string;

  constructor(filePath?: string) {
    this.filePath = filePath ?? path.join(app.getPath("userData"), "printer-profiles.json");
  }

  list(): PrinterProfile[] {
    return Object.values(this.readStore().profiles);
  }

  get(printerName: string): PrinterProfile | undefined {
    return this.readStore().profiles[printerName];
  }

  save(profile: PrinterProfile): void {
    const store = this.readStore();
    store.profiles[profile.printerName] = profile;
    fs.mkdirSync(path.dirname(this.filePath), { recursive: true });
    fs.writeFileSync(this.filePath, JSON.stringify(store, null, 2), "utf8");
  }

  private readStore(): ProfileStore {
    if (!fs.existsSync(this.filePath)) {
      return { profiles: {} };
    }

    try {
      return JSON.parse(fs.readFileSync(this.filePath, "utf8")) as ProfileStore;
    } catch {
      return { profiles: {} };
    }
  }
}
