import type { PassPlan } from "../../src/shared/types";
import fs from "node:fs";
import { execFile } from "node:child_process";
import { promisify } from "node:util";
import { createPreparedPassPdf } from "./pdfPreparation";

const execFileAsync = promisify(execFile);

export class PrintService {
  async printPass(pdfPath: string, printerName: string, passPlan: PassPlan): Promise<{ completed: boolean; message: string }> {
    const preparedPdfPath = await createPreparedPassPdf(
      pdfPath,
      passPlan.pages,
      passPlan.pass,
      passPlan.insertsBlankTrailingSheet
    );

    try {
      if (process.platform === "darwin" || process.platform === "linux") {
        await sendPdfToSystemPrinter(preparedPdfPath, printerName);
        return {
          completed: true,
          message: `Pass ${passPlan.pass} sent to printer. Pages: ${passPlan.pages.join(", ")}${passPlan.insertsBlankTrailingSheet ? " + 1 blank sheet" : ""}`
        };
      }

      return {
        completed: false,
        message: `Direct PDF printing is not implemented yet for platform ${process.platform}.`
      };
    } finally {
      fs.rmSync(preparedPdfPath, { force: true });
    }
  }
}

async function sendPdfToSystemPrinter(pdfPath: string, printerName: string): Promise<void> {
  const args = ["-d", printerName, pdfPath];
  await execFileAsync("/usr/bin/lp", args);
}
