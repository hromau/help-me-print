import { contextBridge, ipcRenderer } from "electron";
import type {
  CalibrationSummary,
  DocumentInfo,
  LearningAnswers,
  PassPlan,
  PrinterInfo,
  PrinterProfile,
  WorkflowSummary
} from "../src/shared/types";

const api = {
  openPdf: (): Promise<DocumentInfo | null> => ipcRenderer.invoke("dialog:openPdf"),
  listPrinters: (): Promise<PrinterInfo[]> => ipcRenderer.invoke("printers:list"),
  listProfiles: (): Promise<PrinterProfile[]> => ipcRenderer.invoke("profiles:list"),
  startCalibration: (printerName: string): Promise<CalibrationSummary> => ipcRenderer.invoke("calibration:start", printerName),
  createWorkflow: (documentPath: string, printerName: string): Promise<WorkflowSummary> =>
    ipcRenderer.invoke("workflow:create", documentPath, printerName),
  printPass: (pdfPath: string, printerName: string, passPlan: PassPlan): Promise<{ completed: boolean; message: string }> =>
    ipcRenderer.invoke("print:pass", pdfPath, printerName, passPlan),
  learnPrinter: (printer: PrinterInfo, answers: LearningAnswers): Promise<PrinterProfile> =>
    ipcRenderer.invoke("profiles:learn", printer, answers)
};

contextBridge.exposeInMainWorld("duplexPrint", api);
