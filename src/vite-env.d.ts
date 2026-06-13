/// <reference types="vite/client" />

import type {
  CalibrationSummary,
  DocumentInfo,
  LearningAnswers,
  PassPlan,
  PrinterInfo,
  PrinterProfile,
  WorkflowSummary
} from "./shared/types";

declare global {
  interface Window {
    duplexPrint: {
      openPdf: () => Promise<DocumentInfo | null>;
      listPrinters: () => Promise<PrinterInfo[]>;
      listProfiles: () => Promise<PrinterProfile[]>;
      startCalibration: (printerName: string) => Promise<CalibrationSummary>;
      createWorkflow: (documentPath: string, printerName: string) => Promise<WorkflowSummary>;
      printPass: (pdfPath: string, printerName: string, passPlan: PassPlan) => Promise<{ completed: boolean; message: string }>;
      learnPrinter: (printer: PrinterInfo, answers: LearningAnswers) => Promise<PrinterProfile>;
    };
  }
}

export {};
