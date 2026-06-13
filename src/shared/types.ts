export type OutputFace = "up" | "down";
export type PageOrder = "normal" | "reverse";
export type ReloadMethod = "same_stack" | "flip_long_edge" | "flip_short_edge";
export type PageParity = "odd" | "even";

export interface PrinterProfile {
  printerName: string;
  manufacturer: string;
  model: string;
  outputFace: OutputFace;
  firstPassParity: PageParity;
  secondPassParity: PageParity;
  firstPassOrder: PageOrder;
  evenPagesOrder: PageOrder;
  reloadMethod: ReloadMethod;
  confidence: number;
  learnedAt: string;
  source: "local" | "cloud" | "manual";
}

export interface PrinterInfo {
  id: string;
  name: string;
  deviceName: string;
  manufacturer: string;
  model: string;
  hasLocalProfile: boolean;
}

export interface DocumentInfo {
  path: string;
  fileName: string;
  pageCount: number;
}

export interface PassPlan {
  pass: 1 | 2;
  description: string;
  pages: number[];
  order: PageOrder;
  parity: PageParity;
  insertsBlankTrailingSheet: boolean;
}

export interface WorkflowSummary {
  document: DocumentInfo;
  printer: PrinterInfo;
  profile?: PrinterProfile;
  firstPass: PassPlan;
  secondPass: PassPlan;
  requiresLearning: boolean;
  finalStackGoal: "page_1_on_top";
}

export interface CalibrationSummary {
  calibrationPdfPath: string;
  workflow: WorkflowSummary;
}

export interface LearningAnswers {
  observedTopSheetPage: string;
  reloadMethod: ReloadMethod;
}

export interface DuplexComputation {
  outputFace: OutputFace;
  firstPassParity: PageParity;
  secondPassParity: PageParity;
  firstPassOrder: PageOrder;
  evenPagesOrder: PageOrder;
  reloadMethod: ReloadMethod;
}
