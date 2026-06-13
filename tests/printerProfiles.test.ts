import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { vi } from "vitest";
import { afterEach, describe, expect, it } from "vitest";
import type { PrinterProfile } from "../src/shared/types";

const tempDirs: string[] = [];

vi.mock("electron", () => ({
  app: {
    getPath: () => os.tmpdir()
  }
}));

const { PrinterProfileService } = await import("../electron/services/printerProfiles");

function makeService(): { service: PrinterProfileService; filePath: string } {
  const dir = fs.mkdtempSync(path.join(os.tmpdir(), "duplexprint-profiles-"));
  tempDirs.push(dir);
  const filePath = path.join(dir, "printer-profiles.json");
  return {
    service: new PrinterProfileService(filePath),
    filePath
  };
}

afterEach(() => {
  for (const dir of tempDirs.splice(0)) {
    fs.rmSync(dir, { recursive: true, force: true });
  }
});

describe("PrinterProfileService", () => {
  it("returns an empty list when no store exists", () => {
    const { service } = makeService();

    expect(service.list()).toEqual([]);
    expect(service.get("Unknown printer")).toBeUndefined();
  });

  it("saves and retrieves a profile by printer name", () => {
    const { service, filePath } = makeService();
    const profile: PrinterProfile = {
      printerName: "HP Smart Tank 580",
      manufacturer: "HP",
      model: "Smart Tank 580",
      outputFace: "up",
      firstPassParity: "even",
      secondPassParity: "odd",
      firstPassOrder: "normal",
      evenPagesOrder: "reverse",
      reloadMethod: "same_stack",
      confidence: 80,
      learnedAt: "2026-06-09T00:00:00.000Z",
      source: "local"
    };

    service.save(profile);

    expect(fs.existsSync(filePath)).toBe(true);
    expect(service.get(profile.printerName)).toEqual(profile);
    expect(service.list()).toEqual([profile]);
  });

  it("returns an empty store when the JSON file is corrupted", () => {
    const { service, filePath } = makeService();
    fs.writeFileSync(filePath, "{ this is not valid json", "utf8");

    expect(service.list()).toEqual([]);
    expect(service.get("HP Smart Tank 580")).toBeUndefined();
  });
});
