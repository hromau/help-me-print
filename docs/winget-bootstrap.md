# Winget Bootstrap

The repository already automates `winget` updates after the package exists in `microsoft/winget-pkgs`.

The first submission is different:

1. Publish a Windows `.exe` installer to a public GitHub release.
2. Generate the initial manifest set with `wingetcreate new`.
3. Copy those files into a fork of `microsoft/winget-pkgs`.
4. Open the first upstream pull request.

## Generate Initial Manifest Files With `wingetcreate`

Run:

```powershell
wingetcreate new `
  https://github.com/hromau/help-me-print/releases/download/v0.1.5/EXACT_FILE_NAME.exe `
  --id Easure.HelpMePrint `
  --out .\packaging\winget
```

`wingetcreate` will inspect the installer, derive the version, compute hashes, and write the manifest set locally.

The expected output path is:

```text
packaging/winget/manifests/e/Easure/HelpMePrint/<version>/
```

If `wingetcreate` needs manual cleanup or you want deterministic local generation from known values, the repository also includes:

```bash
scripts/generate-winget-manifests.sh <version> <installer-url> <sha256>
```

## Submit Upstream

In a fork of `microsoft/winget-pkgs`, copy:

```text
packaging/winget/manifests/e/Easure/HelpMePrint/<version>/
```

to:

```text
manifests/e/Easure/HelpMePrint/<version>/
```

Then validate and open the PR from the fork.

After the first package is merged, set the repository variable `WINGET_AUTOMATION_ENABLED=true` so `.github/workflows/publish-winget.yml` can keep future releases updated automatically.
