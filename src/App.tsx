import { useEffect, useState } from "react";
import type { CalibrationSummary, DocumentInfo, PassPlan, PrinterInfo, PrinterProfile, WorkflowSummary } from "./shared/types";

type Stage = "setup" | "awaiting-training" | "awaiting-second-pass" | "done";
type StatusTone = "neutral" | "info" | "success" | "warning";

export function App() {
  const [document, setDocument] = useState<DocumentInfo | null>(null);
  const [printers, setPrinters] = useState<PrinterInfo[]>([]);
  const [profiles, setProfiles] = useState<PrinterProfile[]>([]);
  const [selectedPrinter, setSelectedPrinter] = useState("");
  const [workflow, setWorkflow] = useState<WorkflowSummary | null>(null);
  const [calibration, setCalibration] = useState<CalibrationSummary | null>(null);
  const [secondPassPlan, setSecondPassPlan] = useState<PassPlan | null>(null);
  const [stage, setStage] = useState<Stage>("setup");
  const [status, setStatus] = useState("Load a PDF and choose a printer.");
  const [statusTone, setStatusTone] = useState<StatusTone>("neutral");
  const [topSheetPage, setTopSheetPage] = useState("");

  useEffect(() => {
    void refreshSidebarData();
  }, []);

  async function refreshSidebarData(): Promise<void> {
    const [availablePrinters, savedProfiles] = await Promise.all([
      window.duplexPrint.listPrinters(),
      window.duplexPrint.listProfiles()
    ]);

    setPrinters(availablePrinters);
    setProfiles(savedProfiles);

    if (!selectedPrinter && availablePrinters.length > 0) {
      setSelectedPrinter(availablePrinters[0].name);
    }
  }

  async function pickPdf(): Promise<void> {
    const result = await window.duplexPrint.openPdf();
    if (!result) {
      return;
    }

    setDocument(result);
    setStatus(`Loaded ${result.fileName}.`);
    setStatusTone("success");
  }

  async function runCalibration(): Promise<void> {
    if (!selectedPrinter) {
      setStatus("Choose a printer first.");
      setStatusTone("warning");
      return;
    }

    const nextCalibration = await window.duplexPrint.startCalibration(selectedPrinter);
    setCalibration(nextCalibration);
    setWorkflow(null);
    setSecondPassPlan(null);
    setTopSheetPage("");

    setStatus("Printing calibration pages...");
    setStatusTone("info");

    const result = await window.duplexPrint.printPass(
      nextCalibration.calibrationPdfPath,
      nextCalibration.workflow.printer.deviceName,
      nextCalibration.workflow.firstPass
    );

    setStatus(result.message);
    setStatusTone("success");
    setStage("awaiting-training");
  }

  async function saveTraining(): Promise<void> {
    if (!calibration) {
      return;
    }

    const profile = await window.duplexPrint.learnPrinter(calibration.workflow.printer, {
      observedTopSheetPage: topSheetPage,
      reloadMethod: "same_stack"
    });

    setProfiles((current) => {
      const rest = current.filter((item) => item.printerName !== profile.printerName);
      return [...rest, profile];
    });

    setCalibration(null);
    setTopSheetPage("");
    setStage("setup");
    setStatus("Printer learned. Real documents will now use the saved duplex order.");
    setStatusTone("success");
  }

  async function runFirstPass(): Promise<void> {
    if (!document || !selectedPrinter) {
      setStatus("Select both a PDF and a printer.");
      setStatusTone("warning");
      return;
    }

    const nextWorkflow = await window.duplexPrint.createWorkflow(document.path, selectedPrinter);
    if (!nextWorkflow.profile) {
      setStatus("Run printer learning first, then print the real document.");
      setStatusTone("warning");
      return;
    }

    setWorkflow(nextWorkflow);
    setCalibration(null);
    setSecondPassPlan(null);
    setStatus(`Printing ${nextWorkflow.firstPass.parity} pages...`);
    setStatusTone("info");

    const result = await window.duplexPrint.printPass(
      nextWorkflow.document.path,
      nextWorkflow.printer.deviceName,
      nextWorkflow.firstPass
    );

    setStatus(result.message);
    setStatusTone("success");
    setStage("awaiting-second-pass");
  }

  async function runSecondPass(): Promise<void> {
    if (!workflow) {
      return;
    }

    setSecondPassPlan(workflow.secondPass);
    setStatus(`Printing ${workflow.secondPass.parity} pages...`);
    setStatusTone("info");

    const result = await window.duplexPrint.printPass(
      workflow.document.path,
      workflow.printer.deviceName,
      workflow.secondPass
    );

    setStatus(result.message);
    setStatusTone("success");
    setStage("done");
  }

  function resetJob(): void {
    setWorkflow(null);
    setCalibration(null);
    setSecondPassPlan(null);
    setStage("setup");
    setTopSheetPage("");
    setStatus("Load a PDF and choose a printer.");
    setStatusTone("neutral");
  }

  const canPrint = Boolean(document && selectedPrinter);
  const selectedProfile = profiles.find((item) => item.printerName === selectedPrinter);
  const firstPassFacts = workflow
    ? [
        `Pages ${workflow.firstPass.pages.join(", ")}`,
        `Order ${workflow.firstPass.order}`,
        ...(workflow.firstPass.insertsBlankTrailingSheet ? ["Adds 1 blank sheet"] : [])
      ]
    : [];
  const secondPassFacts = secondPassPlan
    ? [
        `Pages ${secondPassPlan.pages.join(", ")}`,
        `Order ${secondPassPlan.order}`,
        ...(secondPassPlan.insertsBlankTrailingSheet ? ["Adds 1 blank sheet"] : [])
      ]
    : [];

  return (
    <div className="app-shell">
      <main className="workflow-shell">
        <article className="action-card">
          <div className="card-head compact">
            <h2>{getPrimaryActionLabel(stage, Boolean(document))}</h2>
          </div>

          <div className="action-stack">
            <div className="setup-grid">
              <div className="field-block">
                <span className="field-label">Document</span>
                <div className="file-panel">
                  <div className="file-panel-copy">
                    <strong>{document ? document.fileName : "No PDF selected"}</strong>
                    <p className="field-hint">
                      {document ? `${document.pageCount} pages ready for duplex printing.` : "Pick a PDF to start."}
                    </p>
                  </div>
                  <button className="secondary-button file-button" onClick={() => void pickPdf()}>
                    {document ? "Change PDF" : "Choose PDF"}
                  </button>
                </div>
              </div>

              <div className="field-block">
                <label className="field-label" htmlFor="printer-select">
                  Printer
                </label>
                <select id="printer-select" value={selectedPrinter} onChange={(event) => setSelectedPrinter(event.target.value)}>
                  {printers.map((printer) => (
                    <option key={printer.id} value={printer.name}>
                      {printer.name}
                    </option>
                  ))}
                </select>
                <p className="field-hint">{selectedProfile ? "Training saved for this printer." : "This printer is not trained yet."}</p>
              </div>

              {workflow && firstPassFacts.length > 0 && (
                <div className="fact-row">
                  {firstPassFacts.map((fact) => (
                    <span key={fact}>{fact}</span>
                  ))}
                </div>
              )}

              {stage === "setup" && (
                <>
                  <button className="primary-button cta-button" disabled={!canPrint} onClick={() => void runFirstPass()}>
                    Print
                  </button>
                  <button className="secondary-button cta-button" disabled={!selectedPrinter} onClick={() => void runCalibration()}>
                    Learn printer
                  </button>
                </>
              )}
            </div>

            {stage === "awaiting-training" && (
              <>
                <div className="instruction-card">
                  <p className="instruction-title">
                    Calibration printed. Enter the page number that ended up on top of the printed stack.
                  </p>
                </div>

                <label className="input-block" htmlFor="top-sheet-page">
                  <span className="field-label">Top visible page</span>
                  <input
                    id="top-sheet-page"
                    value={topSheetPage}
                    onChange={(event) => setTopSheetPage(event.target.value)}
                    placeholder="Example: 1"
                  />
                </label>

                <button className="primary-button cta-button" disabled={topSheetPage.trim() === ""} onClick={() => void saveTraining()}>
                  Save training
                </button>
              </>
            )}

            {stage === "awaiting-second-pass" && (
              <>
                <div className="instruction-card">
                  <p className="instruction-title">Put the stack back into the printer with the blank side ready, then print the remaining pages.</p>
                </div>
                <button className="primary-button cta-button" onClick={() => void runSecondPass()}>
                  Print remaining pages
                </button>
              </>
            )}

            {stage === "done" && (
              <>
                {secondPassFacts.length > 0 && (
                  <div className="fact-row">
                    {secondPassFacts.map((fact) => (
                      <span key={fact}>{fact}</span>
                    ))}
                  </div>
                )}
                <div className="completion-card">
                  <strong>Duplex job complete. Take the stack straight from the printer.</strong>
                </div>
                <button className="primary-button cta-button" onClick={resetJob}>
                  Start another job
                </button>
              </>
            )}
          </div>
        </article>
      </main>
    </div>
  );
}

function getPrimaryActionLabel(stage: Stage, hasDocument: boolean): string {
  if (stage === "setup" && !hasDocument) {
    return "Choose your document";
  }

  if (stage === "setup") {
    return "Print or learn";
  }

  if (stage === "awaiting-training") {
    return "Save printer training";
  }

  if (stage === "awaiting-second-pass") {
    return "Print the second side";
  }

  return "Job finished";
}
