import { describe, expect, it } from "vitest";
import type { DocumentInfo, PrinterInfo, PrinterProfile } from "../src/shared/types";
import { WorkflowEngine } from "../electron/services/workflowEngine";

const printer: PrinterInfo = {
  id: "hp-580",
  name: "HP Smart Tank 580",
  deviceName: "HP_Smart_Tank_580",
  manufacturer: "HP",
  model: "Smart Tank 580",
  hasLocalProfile: false
};

const document: DocumentInfo = {
  path: "/tmp/sample.pdf",
  fileName: "sample.pdf",
  pageCount: 8
};

describe("WorkflowEngine", () => {
  it("creates odd pages first and normal even-page order when profile is missing", () => {
    const engine = new WorkflowEngine();

    const workflow = engine.createWorkflow(document, printer);

    expect(workflow.requiresLearning).toBe(true);
    expect(workflow.firstPass.pages).toEqual([1, 3, 5, 7]);
    expect(workflow.firstPass.order).toBe("normal");
    expect(workflow.firstPass.parity).toBe("odd");
    expect(workflow.secondPass.pages).toEqual([2, 4, 6, 8]);
    expect(workflow.secondPass.order).toBe("normal");
    expect(workflow.secondPass.parity).toBe("even");
    expect(workflow.finalStackGoal).toBe("page_1_on_top");
  });

  it("uses an even-pages-first workflow for calibration", () => {
    const engine = new WorkflowEngine();

    const workflow = engine.createCalibrationWorkflow(
      { path: "/tmp/calibration.pdf", fileName: "calibration.pdf", pageCount: 4 },
      printer
    );

    expect(workflow.firstPass.pages).toEqual([2, 4]);
    expect(workflow.firstPass.order).toBe("normal");
    expect(workflow.firstPass.parity).toBe("even");
    expect(workflow.secondPass.pages).toEqual([]);
    expect(workflow.requiresLearning).toBe(true);
  });

  it("prints even pages first for same-stack printers so the final stack comes out page-1-first", () => {
    const engine = new WorkflowEngine();
    const profile: PrinterProfile = {
      printerName: printer.name,
      manufacturer: printer.manufacturer,
      model: printer.model,
      outputFace: "up",
      firstPassParity: "even",
      secondPassParity: "odd",
      firstPassOrder: "normal",
      evenPagesOrder: "reverse",
      reloadMethod: "same_stack",
      confidence: 98,
      learnedAt: "2026-06-09T00:00:00.000Z",
      source: "local"
    };

    const workflow = engine.createWorkflow(document, printer, profile);

    expect(workflow.requiresLearning).toBe(false);
    expect(workflow.firstPass.pages).toEqual([2, 4, 6, 8]);
    expect(workflow.firstPass.order).toBe("normal");
    expect(workflow.firstPass.parity).toBe("even");
    expect(workflow.firstPass.insertsBlankTrailingSheet).toBe(false);
    expect(workflow.secondPass.pages).toEqual([1, 3, 5, 7]);
    expect(workflow.secondPass.order).toBe("normal");
    expect(workflow.secondPass.parity).toBe("odd");
  });

  it("adds a blank trailing sheet on the first pass for odd-page same-stack jobs", () => {
    const engine = new WorkflowEngine();
    const oddDocument: DocumentInfo = {
      path: "/tmp/sample-odd.pdf",
      fileName: "sample-odd.pdf",
      pageCount: 5
    };
    const profile: PrinterProfile = {
      printerName: printer.name,
      manufacturer: printer.manufacturer,
      model: printer.model,
      outputFace: "up",
      firstPassParity: "even",
      secondPassParity: "odd",
      firstPassOrder: "normal",
      evenPagesOrder: "reverse",
      reloadMethod: "same_stack",
      confidence: 98,
      learnedAt: "2026-06-09T00:00:00.000Z",
      source: "local"
    };

    const workflow = engine.createWorkflow(oddDocument, printer, profile);

    expect(workflow.firstPass.pages).toEqual([2, 4]);
    expect(workflow.firstPass.insertsBlankTrailingSheet).toBe(true);
    expect(workflow.secondPass.pages).toEqual([1, 3, 5]);
    expect(workflow.secondPass.insertsBlankTrailingSheet).toBe(false);
  });

  it("reverses the first pass for flip-based reload profiles", () => {
    const engine = new WorkflowEngine();
    const profile: PrinterProfile = {
      printerName: printer.name,
      manufacturer: printer.manufacturer,
      model: printer.model,
      outputFace: "down",
      firstPassParity: "odd",
      secondPassParity: "even",
      firstPassOrder: "reverse",
      evenPagesOrder: "normal",
      reloadMethod: "flip_long_edge",
      confidence: 98,
      learnedAt: "2026-06-09T00:00:00.000Z",
      source: "local"
    };

    const workflow = engine.createWorkflow(document, printer, profile);

    expect(workflow.firstPass.pages).toEqual([7, 5, 3, 1]);
    expect(workflow.firstPass.order).toBe("reverse");
    expect(workflow.firstPass.parity).toBe("odd");
    expect(workflow.secondPass.pages).toEqual([2, 4, 6, 8]);
    expect(workflow.secondPass.order).toBe("normal");
    expect(workflow.secondPass.parity).toBe("even");
  });

  it("learns even-first same-stack behavior when the observed top sheet is page 2", () => {
    const engine = new WorkflowEngine();

    const learned = engine.learnFromCalibration("2");

    expect(learned.outputFace).toBe("down");
    expect(learned.firstPassParity).toBe("even");
    expect(learned.secondPassParity).toBe("odd");
    expect(learned.firstPassOrder).toBe("normal");
    expect(learned.evenPagesOrder).toBe("reverse");
    expect(learned.reloadMethod).toBe("same_stack");
  });

  it("learns normal flip-long-edge behavior when calibration finishes with a later page on top", () => {
    const engine = new WorkflowEngine();

    expect(engine.learnFromCalibration("4")).toMatchObject({
      outputFace: "down",
      firstPassParity: "odd",
      secondPassParity: "even",
      firstPassOrder: "reverse",
      evenPagesOrder: "normal",
      reloadMethod: "flip_long_edge"
    });
  });

  it("still derives second-pass plans from explicit reload answers", () => {
    const engine = new WorkflowEngine();

    expect(engine.learnFromAnswers(printer, {
      observedTopSheetPage: "1",
      reloadMethod: "flip_long_edge"
    })).toMatchObject({
      outputFace: "up",
      firstPassParity: "odd",
      secondPassParity: "even",
      firstPassOrder: "reverse",
      evenPagesOrder: "normal",
      reloadMethod: "flip_long_edge"
    });

    expect(engine.learnFromAnswers(printer, {
      observedTopSheetPage: "2",
      reloadMethod: "flip_long_edge"
    })).toMatchObject({
      outputFace: "down",
      firstPassParity: "odd",
      secondPassParity: "even",
      firstPassOrder: "reverse",
      evenPagesOrder: "normal",
      reloadMethod: "flip_long_edge"
    });

    expect(engine.learnFromAnswers(printer, {
      observedTopSheetPage: "abc",
      reloadMethod: "flip_long_edge"
    })).toMatchObject({
      outputFace: "up",
      firstPassParity: "odd",
      secondPassParity: "even",
      firstPassOrder: "reverse",
      evenPagesOrder: "normal",
      reloadMethod: "flip_long_edge"
    });
  });

  it("creates a second-pass plan directly from the observed top page", () => {
    const engine = new WorkflowEngine();

    expect(engine.createSecondPassPlan(document, {
      observedTopSheetPage: "7",
      reloadMethod: "same_stack"
    })).toMatchObject({
      pass: 2,
      pages: [8, 6, 4, 2],
      order: "reverse",
      parity: "even"
    });

    expect(engine.createSecondPassPlan(document, {
      observedTopSheetPage: "1",
      reloadMethod: "flip_long_edge"
    })).toMatchObject({
      pass: 2,
      pages: [2, 4, 6, 8],
      order: "normal",
      parity: "even"
    });
  });
});
