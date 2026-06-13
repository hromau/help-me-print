import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { afterEach, describe, expect, it } from "vitest";
import { PDFDocument } from "pdf-lib";
import { createPreparedPassPdf } from "../electron/services/pdfPreparation";

const tempFiles: string[] = [];

async function createSourcePdf(pageCount: number): Promise<string> {
  const pdf = await PDFDocument.create();
  for (let index = 0; index < pageCount; index += 1) {
    pdf.addPage([595, 842]);
  }

  const bytes = await pdf.save();
  const filePath = path.join(
    os.tmpdir(),
    `duplexprint-source-${Date.now()}-${Math.random().toString(36).slice(2, 8)}.pdf`
  );
  fs.writeFileSync(filePath, bytes);
  tempFiles.push(filePath);
  return filePath;
}

afterEach(() => {
  for (const filePath of tempFiles.splice(0)) {
    fs.rmSync(filePath, { force: true });
  }
});

describe("createPreparedPassPdf", () => {
  it("creates a reduced PDF with only requested odd pages in requested order", async () => {
    const sourcePdfPath = await createSourcePdf(8);

    const preparedPath = await createPreparedPassPdf(sourcePdfPath, [1, 3, 5, 7], 1);
    tempFiles.push(preparedPath);

    const preparedPdf = await PDFDocument.load(fs.readFileSync(preparedPath));

    expect(preparedPdf.getPageCount()).toBe(4);
  });

  it("creates a reduced PDF for reverse-ordered even pages", async () => {
    const sourcePdfPath = await createSourcePdf(8);

    const preparedPath = await createPreparedPassPdf(sourcePdfPath, [8, 6, 4, 2], 2);
    tempFiles.push(preparedPath);

    const preparedPdf = await PDFDocument.load(fs.readFileSync(preparedPath));

    expect(preparedPdf.getPageCount()).toBe(4);
  });

  it("appends a blank trailing sheet when requested", async () => {
    const sourcePdfPath = await createSourcePdf(5);

    const preparedPath = await createPreparedPassPdf(sourcePdfPath, [2, 4], 1, true);
    tempFiles.push(preparedPath);

    const preparedPdf = await PDFDocument.load(fs.readFileSync(preparedPath));

    expect(preparedPdf.getPageCount()).toBe(3);
  });
});
