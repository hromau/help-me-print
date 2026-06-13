import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { PDFDocument, StandardFonts, rgb } from "pdf-lib";

export async function createPreparedPassPdf(
  sourcePdfPath: string,
  pages: number[],
  pass: 1 | 2,
  insertsBlankTrailingSheet = false
): Promise<string> {
  const sourceBytes = fs.readFileSync(sourcePdfPath);
  const sourcePdf = await PDFDocument.load(sourceBytes);
  const outputPdf = await PDFDocument.create();

  const zeroBasedIndexes = pages.map((pageNumber) => pageNumber - 1);
  const copiedPages = await outputPdf.copyPages(sourcePdf, zeroBasedIndexes);

  for (const page of copiedPages) {
    outputPdf.addPage(page);
  }

  if (insertsBlankTrailingSheet) {
    const referencePage = sourcePdf.getPage(Math.max(0, sourcePdf.getPageCount() - 1));
    const blankPage = outputPdf.addPage([referencePage.getWidth(), referencePage.getHeight()]);
    blankPage.drawRectangle({
      x: 0,
      y: 0,
      width: blankPage.getWidth(),
      height: blankPage.getHeight(),
      color: rgb(1, 1, 1)
    });
  }

  const preparedBytes = await outputPdf.save();
  const targetPath = path.join(
    os.tmpdir(),
    `duplexprint-pass-${pass}-${Date.now()}-${Math.random().toString(36).slice(2, 8)}.pdf`
  );

  fs.writeFileSync(targetPath, preparedBytes);
  return targetPath;
}

export async function createCalibrationPdf(): Promise<string> {
  const pdf = await PDFDocument.create();
  const font = await pdf.embedFont(StandardFonts.HelveticaBold);

  for (let pageNumber = 1; pageNumber <= 4; pageNumber += 1) {
    const page = pdf.addPage([595, 842]);
    const label = String(pageNumber);
    const fontSize = 280;
    const textWidth = font.widthOfTextAtSize(label, fontSize);

    page.drawText(label, {
      x: (page.getWidth() - textWidth) / 2,
      y: (page.getHeight() - fontSize) / 2,
      size: fontSize,
      font,
      color: rgb(0.1, 0.1, 0.1)
    });
  }

  const bytes = await pdf.save();
  const targetPath = path.join(
    os.tmpdir(),
    `duplexprint-calibration-${Date.now()}-${Math.random().toString(36).slice(2, 8)}.pdf`
  );

  fs.writeFileSync(targetPath, bytes);
  return targetPath;
}
