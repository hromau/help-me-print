# Publishing

This project publishes in stages:

1. CI validates the app, API, and C++ core on every push and pull request.
2. A `v*` tag builds the native C++ project and uploads artifacts to GitHub Releases.
3. The GitHub release uploads per-platform validation metadata alongside installers.
4. Optional publish workflows update Homebrew, Winget, and an APT repository when the required release input is safe to publish.

The release pipeline is intentionally native-first. Desktop distribution uses the C++/Qt application only.

## macOS signing and notarization

Unsigned macOS app bundles are treated as damaged or untrusted when users install them from direct downloads, Homebrew Cask, or other quarantined distribution paths. A plain `.dmg` from CI is not sufficient.

The macOS release job now expects these GitHub Actions secrets before a public drag-and-drop macOS installer should be trusted:

- `MACOS_CERTIFICATE_NAME`: the full Developer ID Application identity name
- `MACOS_CERTIFICATE_P12`: base64-encoded `.p12` certificate export
- `MACOS_CERTIFICATE_PASSWORD`: password for the `.p12` export
- `MACOS_NOTARY_APPLE_ID`: Apple ID used for notarization
- `MACOS_NOTARY_TEAM_ID`: Apple Developer Team ID
- `MACOS_NOTARY_APP_PASSWORD`: app-specific password for `notarytool`

When those secrets are present, the workflow:

1. imports the Developer ID certificate into a temporary keychain
2. installs the staged `.app`, signs it, and packages a `.dmg`
3. submits the `.dmg` to Apple notarization and staples the result
4. produces a notarized `.dmg` suitable for direct download

If those secrets are missing, the workflow still produces a macOS artifact for manual validation, but that `.dmg` should not be treated as a safe public installer.

For local development, `script/build_and_run.sh` now re-signs the built `.app` with an ad-hoc signature so the bundle stays internally consistent after rebuilds. That does not replace Developer ID signing or notarization for public distribution.

## Homebrew on macOS without Apple enrollment

The current macOS workaround is to publish a Homebrew formula instead of a Homebrew cask.

That formula builds Help Me Print from the tagged source tarball on the user's Mac and installs:

- `help-me-print.app` into the Homebrew Cellar
- a `help-me-print` launcher script into `bin/`

Because Homebrew is compiling locally from source instead of downloading a quarantined `.app`, this path does not depend on Apple Developer ID or notarization. It is the cleanest free distribution path for macOS while Apple enrollment is unresolved.

## Windows signing

Windows signing is recommended but not required for automated winget publishing. The release workflow accepts these GitHub Actions secrets when available:

- `WINDOWS_CERTIFICATE_PFX`: base64-encoded Authenticode `.pfx`
- `WINDOWS_CERTIFICATE_PASSWORD`: password for the `.pfx`

When those secrets are present, the workflow signs the generated `.exe` with `signtool`, verifies the signature, and emits `validation-windows.json` with `reason: "signed Windows installer"`. Without them, the `.exe` is still attached to the GitHub release and the winget publish workflow can continue, but users may see Microsoft SmartScreen warnings because the installer is unsigned.

## Platform independence

The macOS matrix leg is allowed to fail without blocking the Windows and Linux release assets. This keeps APT and winget publish paths moving while macOS signing, notarization, or Apple Developer enrollment are still unresolved.

## Release validation

Each tagged GitHub release now includes:

- `validation-mac.json`
- `validation-windows.json`
- `validation-linux.json`

The `winget` and `apt` workflows consume these files and refuse to publish if the corresponding platform asset is not marked `distributable: true`. This prevents a bad GitHub release asset from automatically propagating into those package managers.

## GitHub Release

Create a release by pushing a tag:

```bash
git tag v0.1.0
git push origin v0.1.0
```

The release workflow installs Qt in CI, packages the native desktop app with CPack, and uploads:

- macOS: `.dmg`
- Windows: `.exe`
- Linux: `.deb`

Package versioning is derived from the Git tag. For example, tag `v0.1.1` produces installers branded as version `0.1.1`.

Those assets are the inputs for Winget, the GitHub Pages APT repository, and optional direct macOS downloads.

For Winget, the GitHub repository and release assets must be publicly reachable. A private or missing repository, or a release without a public `.exe`, will prevent downstream package managers from installing the app.

## Homebrew Formula

Create a tap repository, for example:

```text
hromau/homebrew-tap
```

Configure repository variables and secrets in `hromau/help-me-print`:

- Variable `HOMEBREW_TAP_REPOSITORY`: `hromau/homebrew-tap`
- Secret `HOMEBREW_TAP_TOKEN`: a GitHub token with write access to the tap repo

The workflow writes:

```text
Formula/help-me-print.rb
```

The formula points at the GitHub tag source archive:

```text
https://github.com/<owner>/<repo>/archive/refs/tags/vX.Y.Z.tar.gz
```

and declares Homebrew dependencies for `cmake` and `qt`.

This is intentionally not a cask. The goal is to avoid shipping an unsigned downloaded `.app` that Gatekeeper will flag as damaged.

Users install with:

```bash
brew install hromau/tap/help-me-print
```

## Winget

Configure:

- Secret `WINGET_TOKEN`: GitHub token suitable for opening pull requests against `microsoft/winget-pkgs`
- Variable `WINGET_AUTOMATION_ENABLED`: set to `true` only after the first `Easure.HelpMePrint` package has been merged into `microsoft/winget-pkgs`

The `publish-winget` workflow uses `vedantmgoyal2009/winget-releaser` and package identifier:

```text
Easure.HelpMePrint
```

The workflow only updates an existing package in `microsoft/winget-pkgs`. It does not author the initial metadata-rich manifest, so the first upstream submission should include:

```yaml
PackageName: Help Me Print
ShortDescription: Manual duplex printing for printers without automatic double-sided support
Moniker: duplexprint
Tags:
  - duplex
  - double-sided
  - printing
  - printer
  - pdf
  - manual-duplex
```

The Windows release now publishes an `.exe` installer generated by CPack/NSIS.

If the main GitHub repository or release assets are not public, Winget cannot consume the installer URL.

For the first submission, bootstrap the manifests with `wingetcreate new <public-exe-url> --id Easure.HelpMePrint`; the detailed flow is in [docs/winget-bootstrap.md](/Users/siarheih/Documents/Projects/help-me-print/docs/winget-bootstrap.md:1).

## APT

The `publish-apt` workflow publishes a small signed APT repository to GitHub Pages on `gh-pages` in a separate public repository.

Configure:

- Secret `APT_GPG_PRIVATE_KEY`: private GPG key used to sign `InRelease` and `Release.gpg`
- Secret `APT_REPOSITORY_TOKEN`: GitHub token with write access to the public APT repository
- Variable `APT_REPOSITORY`: repository that hosts the published APT site, for example `hromau/help-me-print-apt`

Users install with:

```bash
curl -fsSL https://hromau.github.io/help-me-print-apt/apt/help-me-print-archive-keyring.asc | sudo gpg --dearmor -o /usr/share/keyrings/help-me-print.gpg
echo "deb [signed-by=/usr/share/keyrings/help-me-print.gpg] https://hromau.github.io/help-me-print-apt/apt stable main" | sudo tee /etc/apt/sources.list.d/help-me-print.list
sudo apt update
sudo apt install help-me-print
```

## Easure API

Configure:

- Variable `AZURE_FUNCTIONAPP_NAME`: `easure-duplexprint-api`
- Secret `AZURE_FUNCTIONAPP_PUBLISH_PROFILE`: publish profile XML from the Azure Function App

Then `publish-api` deploys changes under `api/` automatically from `main`.

## Static landing page

The SEO landing page lives under `site/` and is published directly to Azure Storage Static Website with Azure CLI.

Deployment setup and post-deploy SEO checks are documented in [docs/static-site.md](/Users/siarheih/Documents/Projects/help-me-print/docs/static-site.md:1).
