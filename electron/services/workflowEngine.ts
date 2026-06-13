import path from "node:path";
import fs from "node:fs";
import { PDFDocument } from "pdf-lib";
import type {
  DocumentInfo,
  DuplexComputation,
  LearningAnswers,
  PageParity,
  PageOrder,
  PassPlan,
  PrinterInfo,
  PrinterProfile,
  WorkflowSummary
} from "../../src/shared/types";

function pageRange(pageCount: number, parity: "odd" | "even"): number[] {
  const pages: number[] = [];
  for (let page = 1; page <= pageCount; page += 1) {
    if ((parity === "odd" && page % 2 === 1) || (parity === "even" && page % 2 === 0)) {
      pages.push(page);
    }
  }
  return pages;
}

function plan(
  pass: 1 | 2,
  description: string,
  pages: number[],
  order: PageOrder,
  parity: PageParity,
  insertsBlankTrailingSheet = false
): PassPlan {
  return {
    pass,
    description,
    pages: order === "reverse" ? [...pages].reverse() : pages,
    order,
    parity,
    insertsBlankTrailingSheet
  };
}

function resolveWorkflow(profile?: PrinterProfile): {
  firstPassParity: PageParity;
  secondPassParity: PageParity;
  firstPassOrder: PageOrder;
  secondPassOrder: PageOrder;
} {
  if (!profile) {
    return {
      firstPassParity: "odd",
      secondPassParity: "even",
      firstPassOrder: "normal",
      secondPassOrder: "normal"
    };
  }

  if (!profile.firstPassParity || !profile.secondPassParity || !profile.firstPassOrder) {
    if (profile.reloadMethod === "same_stack") {
      return {
        firstPassParity: "even",
        secondPassParity: "odd",
        firstPassOrder: "normal",
        secondPassOrder: "normal"
      };
    }

    return {
      firstPassParity: "odd",
      secondPassParity: "even",
      firstPassOrder: "reverse",
      secondPassOrder: "normal"
    };
  }

  return {
    firstPassParity: profile.firstPassParity,
    secondPassParity: profile.secondPassParity,
    firstPassOrder: profile.firstPassOrder,
    secondPassOrder: profile.secondPassParity === "even" ? profile.evenPagesOrder : "normal"
  };
}

export class WorkflowEngine {
  async createDocument(filePath: string): Promise<DocumentInfo> {
    const basename = path.basename(filePath);
    const bytes = fs.readFileSync(filePath);
    const pdf = await PDFDocument.load(bytes);

    return {
      path: filePath,
      fileName: basename,
      pageCount: pdf.getPageCount()
    };
  }

  createWorkflow(document: DocumentInfo, printer: PrinterInfo, profile?: PrinterProfile): WorkflowSummary {
    const oddPages = pageRange(document.pageCount, "odd");
    const evenPages = pageRange(document.pageCount, "even");
    const { firstPassParity, secondPassParity, firstPassOrder, secondPassOrder } = resolveWorkflow(profile);
    const firstPassPages = firstPassParity === "odd" ? oddPages : evenPages;
    const secondPassPages = secondPassParity === "odd" ? oddPages : evenPages;
    const insertsBlankTrailingSheet = firstPassPages.length < secondPassPages.length;

    return {
      document,
      printer,
      profile,
      firstPass: plan(
        1,
        `Print ${firstPassParity} pages first.`,
        firstPassPages,
        firstPassOrder,
        firstPassParity,
        insertsBlankTrailingSheet
      ),
      secondPass: plan(2, `Reload the paper stack, then print ${secondPassParity} pages.`, secondPassPages, secondPassOrder, secondPassParity),
      requiresLearning: !profile,
      finalStackGoal: "page_1_on_top"
    };
  }

  createCalibrationWorkflow(document: DocumentInfo, printer: PrinterInfo): WorkflowSummary {
    const evenPages = pageRange(document.pageCount, "even");

    return {
      document,
      printer,
      firstPass: plan(1, "Print calibration even pages first.", evenPages, "normal", "even"),
      secondPass: plan(2, "Calibration second pass is not used.", [], "normal", "odd"),
      requiresLearning: true,
      finalStackGoal: "page_1_on_top"
    };
  }

  createSecondPassPlan(document: DocumentInfo, answers: LearningAnswers): PassPlan {
    const evenPages = pageRange(document.pageCount, "even");
    const order = answers.reloadMethod === "same_stack" ? "reverse" : "normal";

    return plan(
      2,
      "Reload the printed stack exactly as it came out, then print the remaining pages.",
      evenPages,
      order,
      "even"
    );
  }

  learnFromCalibration(observedTopSheetPage: string): DuplexComputation {
    const parsed = Number.parseInt(observedTopSheetPage, 10);
    const firstPassParity: PageParity = Number.isNaN(parsed) || parsed <= 2 ? "even" : "odd";
    const secondPassParity: PageParity = firstPassParity === "even" ? "odd" : "even";
    const firstPassOrder: PageOrder = firstPassParity === "even" ? "normal" : "reverse";
    const evenPagesOrder: PageOrder = secondPassParity === "even" ? "normal" : "reverse";

    return {
      outputFace: parsed % 2 === 0 ? "down" : "up",
      firstPassParity,
      secondPassParity,
      firstPassOrder,
      evenPagesOrder,
      reloadMethod: firstPassParity === "even" ? "same_stack" : "flip_long_edge"
    };
  }

  learnFromAnswers(printer: PrinterInfo, answers: LearningAnswers): DuplexComputation {
    const parsed = Number.parseInt(answers.observedTopSheetPage, 10);
    const evenPagesOrder: PageOrder = answers.reloadMethod === "same_stack" ? "reverse" : "normal";
    const firstPassParity: PageParity = answers.reloadMethod === "same_stack" ? "even" : "odd";
    const secondPassParity: PageParity = firstPassParity === "even" ? "odd" : "even";
    const firstPassOrder: PageOrder = firstPassParity === "even" ? "normal" : "reverse";

    return {
      outputFace: parsed % 2 === 0 ? "down" : "up",
      firstPassParity,
      secondPassParity,
      firstPassOrder,
      evenPagesOrder,
      reloadMethod: answers.reloadMethod
    };
  }
}
