# Help Me Print

Help Me Print is a native cross-platform desktop app for manual duplex printing on printers without automatic double-sided support.

## Scope

- Select a PDF
- Select a system printer
- Detect local or Easure cloud printer profiles
- Prepare odd/even print passes
- Print first and second manual duplex passes
- Learn printer behavior through a calibration flow
- Save local printer profiles
- Normalize local profiles by manufacturer/model
- Reuse shared cloud profiles when available
- Submit learned profiles to Easure for pending moderation

## Tech Stack

- C++20
- Qt 6 Widgets
- CMake / CPack
- Azure Functions API for shared Easure printer profiles
- Azure Table Storage for cloud profile data

## Project Structure

- `cpp/app/` native Qt desktop app
- `cpp/core/` duplex workflow engine and shared models
- `cpp/platform/` local profile store and printer profile resolution
- `cpp/cloud/` Easure cloud profile repository contracts
- `cpp/tests/` C++ regression tests
- `api/` Azure Functions API for shared printer profiles
- `docs/` deployment, Azure, and publishing notes

## Build Native App

Install Qt 6 and configure CMake with the Qt prefix when needed:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt
cmake --build build --target duplexprint_qt
```

On macOS the native app is built at:

```text
build/cpp/app/help-me-print.app
```

## Test

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt
cmake --build build
ctest --test-dir build --output-on-failure
```

API tests:

```bash
dotnet test api/tests/Easure.PrintProfiles.Api.Tests.csproj
```

## Cloud Backend

Cloud profile storage is documented in [docs/azure-storage.md](/Users/siarheih/Documents/Projects/help-me-print/docs/azure-storage.md:1).

The Easure API is documented in [docs/easure-api.md](/Users/siarheih/Documents/Projects/help-me-print/docs/easure-api.md:1).

## Publishing

Release and package publishing notes are in [docs/publishing.md](/Users/siarheih/Documents/Projects/help-me-print/docs/publishing.md:1).
