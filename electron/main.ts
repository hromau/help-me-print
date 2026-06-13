import { app, BrowserWindow, dialog, ipcMain } from "electron";
import path from "node:path";
import type { LearningAnswers, PassPlan, PrinterInfo, PrinterProfile } from "../src/shared/types";
import { createCalibrationPdf } from "./services/pdfPreparation";
import { PrinterProfileService } from "./services/printerProfiles";
import { PrintService } from "./services/printService";
import { WorkflowEngine } from "./services/workflowEngine";

const profileService = new PrinterProfileService();
const workflowEngine = new WorkflowEngine();
const printService = new PrintService();

async function getAvailablePrinters(): Promise<PrinterInfo[]> {
  const mainWindow = BrowserWindow.getAllWindows()[0];
  if (!mainWindow) {
    return [];
  }

  const printers = await mainWindow.webContents.getPrintersAsync();

  return printers.map((printer) => {
    const printerName = printer.displayName || printer.name;
    const parts = printerName.split(" ");
    const manufacturer = parts[0] || printerName;
    const model = parts.slice(1).join(" ") || printerName;

    return {
      id: printer.name,
      name: printerName,
      deviceName: printer.name,
      manufacturer,
      model,
      hasLocalProfile: Boolean(profileService.get(printerName))
    };
  });
}

function createWindow(): void {
  const window = new BrowserWindow({
    show: false,
    width: 430,
    height: 520,
    minWidth: 390,
    minHeight: 420,
    center: true,
    backgroundColor: "#f4efe7",
    webPreferences: {
      preload: path.join(__dirname, "preload.js"),
      contextIsolation: true,
      nodeIntegration: false
    }
  });

  window.once("ready-to-show", () => {
    console.log("window ready-to-show");
    window.show();
    window.focus();
    window.webContents.openDevTools({ mode: "detach" });
  });

  window.on("show", () => {
    console.log("window shown");
  });

  window.on("closed", () => {
    console.log("window closed");
  });

  window.webContents.on("did-fail-load", (_event, errorCode, errorDescription) => {
    console.error("renderer failed to load", errorCode, errorDescription);
  });

  window.webContents.on("render-process-gone", (_event, details) => {
    console.error("renderer process gone", details);
  });

  window.webContents.on("did-finish-load", () => {
    console.log("renderer finished load");
    window.webContents
      .executeJavaScript(
        `Math.ceil(document.documentElement.scrollHeight || document.body.scrollHeight || 0)`,
        true
      )
      .then((contentHeight) => {
        const targetHeight = Math.max(420, Math.min(Number(contentHeight) + 36, 720));
        window.setContentSize(430, targetHeight);
      })
      .catch((error) => {
        console.error("failed to measure renderer height", error);
      });
  });

  const rendererUrl = process.env.ELECTRON_RENDERER_URL;
  if (rendererUrl) {
    void window.loadURL(rendererUrl);
    return;
  }

  void window.loadFile(path.join(__dirname, "../../dist/index.html"));
}

process.on("uncaughtException", (error) => {
  console.error("uncaughtException", error);
});

process.on("unhandledRejection", (reason) => {
  console.error("unhandledRejection", reason);
});

app.whenReady().then(() => {
  createWindow();

  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

app.on("window-all-closed", () => {
  if (process.platform !== "darwin") {
    app.quit();
  }
});

ipcMain.handle("dialog:openPdf", async () => {
  const result = await dialog.showOpenDialog({
    properties: ["openFile"],
    filters: [{ name: "PDF Files", extensions: ["pdf"] }]
  });

  if (result.canceled || result.filePaths.length === 0) {
    return null;
  }

  return workflowEngine.createDocument(result.filePaths[0]);
});

ipcMain.handle("printers:list", async () => {
  return getAvailablePrinters();
});

ipcMain.handle("profiles:list", async () => profileService.list());

ipcMain.handle("workflow:create", async (_event, documentPath: string, printerName: string) => {
  const document = await workflowEngine.createDocument(documentPath);
  const printer = (await getAvailablePrinters()).find((item) => item.name === printerName);

  if (!printer) {
    throw new Error(`Unknown printer: ${printerName}`);
  }

  return workflowEngine.createWorkflow(document, printer, profileService.get(printer.name));
});

ipcMain.handle("calibration:start", async (_event, printerName: string) => {
  const printer = (await getAvailablePrinters()).find((item) => item.name === printerName);

  if (!printer) {
    throw new Error(`Unknown printer: ${printerName}`);
  }

  const calibrationPdfPath = await createCalibrationPdf();
  const document = await workflowEngine.createDocument(calibrationPdfPath);
  const workflow = workflowEngine.createCalibrationWorkflow(document, printer);

  return { calibrationPdfPath, workflow };
});

ipcMain.handle("workflow:secondPass", async (_event, documentPath: string, answers: LearningAnswers) => {
  const document = await workflowEngine.createDocument(documentPath);
  return workflowEngine.createSecondPassPlan(document, answers);
});

ipcMain.handle("print:pass", async (_event, pdfPath: string, printerName: string, passPlan: PassPlan) => {
  return printService.printPass(pdfPath, printerName, passPlan);
});

ipcMain.handle("profiles:learn", async (_event, printer: PrinterInfo, answers: LearningAnswers) => {
  const learned = workflowEngine.learnFromCalibration(answers.observedTopSheetPage);
  const profile: PrinterProfile = {
    printerName: printer.name,
    manufacturer: printer.manufacturer,
    model: printer.model,
    outputFace: learned.outputFace,
    firstPassParity: learned.firstPassParity,
    secondPassParity: learned.secondPassParity,
    firstPassOrder: learned.firstPassOrder,
    evenPagesOrder: learned.evenPagesOrder,
    reloadMethod: learned.reloadMethod,
    confidence: 80,
    learnedAt: new Date().toISOString(),
    source: "local"
  };

  profileService.save(profile);
  return profile;
});
