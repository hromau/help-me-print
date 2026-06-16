# Easure API for DuplexPrint

## Hosting choice

Use Azure Functions for the first API version.

Recommended plan for the smallest bill:

- Azure Functions `Consumption`
- .NET 9 isolated worker runtime
- Existing `duplexprint7ea1b526` storage account
- Existing `PrinterProfiles` table

Microsoft currently recommends Flex Consumption for new serverless apps, while the classic Consumption plan still has a very strong free grant. For this tiny API, classic Consumption is acceptable and cheap. Keep `Always Ready` disabled if using Flex Consumption later, otherwise idle cost appears.

Relevant Microsoft pricing note: Azure Functions Consumption pricing includes a monthly free grant of 1 million requests and 400,000 GB-s per subscription. See [Azure Functions pricing](https://azure.microsoft.com/en-us/pricing/details/functions/) and [Azure Functions scale and hosting](https://learn.microsoft.com/en-us/azure/azure-functions/functions-scale).

## Created API resources

Created on June 14, 2026:

- Function App: `easure-duplexprint-api`
- Hostname: `https://easure-duplexprint-api.azurewebsites.net`
- Plan: Windows Consumption, `Dynamic`
- Runtime: .NET 9 isolated worker, Azure Functions v4
- Resource Group: `duplexprint-prod-rg`
- Storage Account: `duplexprint7ea1b526`
- Table: `PrinterProfiles`

Linux Consumption was tested first, but its SCM/runtime endpoint returned `503 Site Unavailable` for this app. Windows Consumption was used instead because zip deploy works cleanly and the billing model is still serverless.

## API endpoints

Base path after deployment:

```text
https://easure-duplexprint-api.azurewebsites.net/api
```

Endpoints:

```text
GET  /profiles/{manufacturer}/{model}
PUT  /profiles/{manufacturer}/{model}
POST /profiles/{manufacturer}/{model}/vote
GET  /profiles
```

Access:

- `GET /profiles/{manufacturer}/{model}` is anonymous for desktop clients.
- `PUT /profiles/{manufacturer}/{model}` requires a function key.
- `POST /profiles/{manufacturer}/{model}/vote` requires a function key.
- `GET /profiles` requires a function key and returns at most 100 profiles.

## Required app settings

```text
AZURE_TABLE_CONNECTION_STRING=<storage connection string>
PRINTER_PROFILES_TABLE=PrinterProfiles
```

The desktop app must not contain the Azure Storage connection string. It should call the Easure API instead.

## Local development

Create `api/local.settings.json` locally:

```json
{
  "IsEncrypted": false,
  "Values": {
    "AzureWebJobsStorage": "UseDevelopmentStorage=true",
    "FUNCTIONS_WORKER_RUNTIME": "dotnet-isolated",
    "AZURE_TABLE_CONNECTION_STRING": "<storage connection string>",
    "PRINTER_PROFILES_TABLE": "PrinterProfiles"
  }
}
```

Run:

```bash
dotnet test api/tests/Easure.PrintProfiles.Api.Tests.csproj
dotnet run --project api/Easure.PrintProfiles.Api.csproj
```

## Deploy outline

Use the isolated personal Azure profile:

```bash
export AZURE_CONFIG_DIR=/Users/siarheih/Documents/Projects/help-me-print/.azure-personal
```

Create a function app using the existing resource group and storage account:

```bash
az functionapp create \
  --resource-group duplexprint-prod-rg \
  --name easure-duplexprint-api \
  --storage-account duplexprint7ea1b526 \
  --consumption-plan-location westeurope \
  --runtime dotnet-isolated \
  --runtime-version 9 \
  --functions-version 4 \
  --os-type Windows
```

Set app settings:

```bash
az storage account show-connection-string \
  --resource-group duplexprint-prod-rg \
  --name duplexprint7ea1b526 \
  --query connectionString \
  --output tsv
```

```bash
az functionapp config appsettings set \
  --resource-group duplexprint-prod-rg \
  --name easure-duplexprint-api \
  --settings \
    AZURE_TABLE_CONNECTION_STRING="<connection-string>" \
    PRINTER_PROFILES_TABLE="PrinterProfiles"
```

Deploy from `api/`:

```bash
dotnet publish api/Easure.PrintProfiles.Api.csproj --configuration Release --output /tmp/easure-api-publish
cd /tmp/easure-api-publish
zip -r /tmp/easure-duplexprint-api-package.zip .
az functionapp deployment source config-zip \
  --resource-group duplexprint-prod-rg \
  --name easure-duplexprint-api \
  --src /tmp/easure-duplexprint-api-package.zip
```

Smoke test:

```bash
curl https://easure-duplexprint-api.azurewebsites.net/api/profiles/HP/Smart%20Tank%20580
```
