# DuplexPrint

DuplexPrint is a cross-platform Electron desktop app that guides manual duplex printing for printers without automatic double-sided support.

## Current MVP Scope

- Select a PDF
- Select a printer
- Prepare odd and even print passes
- Run a first-pass print flow
- Learn printer behavior from a simple question
- Save a local printer profile
- Reuse learned behavior on later jobs

## Tech Stack

- Electron
- TypeScript
- React
- Node.js
- Vite

## Project Structure

- `electron/` main-process code, preload bridge, and workflow services
- `src/` renderer app and shared UI
- `src/shared/` domain models shared between renderer and main process

## Important Notes

This scaffold currently uses:

- simulated printer discovery
- simulated PDF page count
- simulated print completion

These placeholders isolate the workflow and profile model first. The next engineering step is to replace them with:

1. real printer enumeration
2. real PDF inspection and page extraction
3. real OS print-job submission and completion tracking
4. optional Azure Table Storage sync for shared printer profiles

## Azure backend note

Cloud profile storage guidance is documented in [docs/azure-storage.md](/Users/siarheih/Documents/Projects/help-me-print/docs/azure-storage.md:1).

## Development

```bash
npm install
npm run dev
npm test
```

## Packaging Targets

Planned distribution:

- Winget
- Homebrew Cask
- AppImage
