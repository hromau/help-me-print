# Azure storage for DuplexPrint

## Short answer

For MVP, create **an Azure Storage Account and use Azure Table Storage**.

Do **not** start with Azure SQL.
Do **not** start with Cosmos DB unless you already know you need global distribution, autoscale throughput, or multi-region replication.

## Why this is the right choice

Your current data model is a strong fit for Table Storage:

- key-value style access
- simple entities
- no joins
- lookup by manufacturer and model
- low operational complexity
- low cost for an open-source donation-funded app

For DuplexPrint, the main cloud entity is a printer profile:

```json
{
  "PartitionKey": "HP",
  "RowKey": "Smart Tank 580",
  "outputFace": "up",
  "evenPagesOrder": "reverse",
  "reloadMethod": "same_stack",
  "confidence": 98,
  "votes": 241,
  "updatedAt": "2026-06-09T00:00:00.000Z"
}
```

## Recommended Azure resources

Create:

1. `Resource Group`
2. `Storage Account`
3. Table named `PrinterProfiles`

Suggested naming:

- Resource Group: `duplexprint-prod-rg`
- Storage Account: `duplexprint7ea1b526`
- Table: `PrinterProfiles`

## Created production resources

Created on June 14, 2026 in the personal Azure subscription:

- Subscription: `Default-subscription`
- Subscription ID: `7ea1b526-76d9-4665-8a36-4ccfb56eed21`
- Tenant: `whynotsergeigmail.onmicrosoft.com`
- Resource Group: `duplexprint-prod-rg`
- Location: `westeurope`
- Storage Account: `duplexprint7ea1b526`
- Table: `PrinterProfiles`
- Table endpoint: `https://duplexprint7ea1b526.table.core.windows.net/`

Use the isolated local Azure CLI profile for this project:

```bash
AZURE_CONFIG_DIR=/Users/siarheih/Documents/Projects/help-me-print/.azure-personal az account show
```

## How to create it in Azure Portal

As of June 14, 2026, the simplest path is still:

1. Open [Azure Portal](https://portal.azure.com/).
2. Create a `Resource group`.
   Suggested name: `duplexprint-prod-rg`
3. Create a `Storage account`.
   Suggested name: `duplexprint7ea1b526`
4. In the storage account, open `Data storage` -> `Tables`.
5. Create a table named `PrinterProfiles`.

Recommended storage account settings for MVP:

- Performance: `Standard`
- Redundancy: `LRS`
- Region: `westeurope`
- Hierarchical namespace: `Disabled`
- Allow storage account key access: `Enabled` for MVP

You do not need:

- Azure SQL
- Cosmos DB at day one
- Blob containers for profile data
- Event Grid
- Service Bus

## How the app should authenticate

For the first shipping version, the least painful option is:

- desktop app talks to your backend API
- backend API talks to Azure Table Storage

Do not ship a desktop client that writes directly to Table Storage with an embedded account key or long-lived secret.

Recommended shape:

1. Desktop app sends printer fingerprint to Easure API.
2. Easure API reads/writes canonical `PrinterProfiles` records.
3. Anonymous desktop submissions land in `PrinterProfileSubmissions` with `pending` status.
3. API returns either:
   - known profile
   - no match
   - profile accepted but pending moderation

For local dev or private experiments, direct storage access is acceptable, but it should not be your production trust model.

## Minimal entity design

Use two tables:

- `PrinterProfiles` for canonical approved profiles
- `PrinterProfileSubmissions` for anonymous pending uploads from desktop clients

Partitioning:

- `PartitionKey = normalized manufacturer`
- `RowKey = normalized manufacturer + ":" + normalized model`

Why not raw display name:

- display names vary too much by driver and locale
- manufacturer/model is much more stable

Suggested entity:

```json
{
  "PartitionKey": "hp",
  "RowKey": "hp:smart tank 580",
  "displayName": "HP Smart Tank 580",
  "manufacturer": "HP",
  "model": "Smart Tank 580",
  "normalizedKey": "hp:smart tank 580",
  "outputFace": "up",
  "firstPassParity": "even",
  "secondPassParity": "odd",
  "firstPassOrder": "normal",
  "evenPagesOrder": "reverse",
  "reloadMethod": "same_stack",
  "confidence": 98,
  "votes": 241,
  "createdAt": "2026-06-09T00:00:00.000Z",
  "updatedAt": "2026-06-09T00:00:00.000Z",
  "source": "cloud",
  "schemaVersion": 1
}
```

## Data model

Use:

- `PartitionKey = normalized manufacturer`
- `RowKey = normalized manufacturer + ":" + normalized model`

Recommended columns:

- `displayName`
- `manufacturer`
- `model`
- `outputFace`
- `evenPagesOrder`
- `reloadMethod`
- `confidence`
- `votes`
- `createdAt`
- `updatedAt`
- `source`
- `schemaVersion`

## Normalization rules

Normalize `RowKey` aggressively so community data merges correctly:

- trim whitespace
- lowercase
- collapse repeated spaces
- remove cosmetic suffix noise when safe

Example:

- `HP Smart Tank 580`
- `hp smart tank 580`
- `HP  Smart  Tank 580`

All should resolve to the same normalized model key.

## When Table Storage stops being enough

Move to **Azure Cosmos DB for Table** only if one of these becomes real:

- you need multi-region replication
- you need very high read/write throughput
- you need stricter global availability targets
- you need autoscale behavior at larger scale

Important engineering choice:

Use the `Azure.Data.Tables` SDK from day one. Microsoft documents that the same client library targets Azure Table Storage and Azure Cosmos DB for Table, which keeps migration friction low later. Sources:

- [Azure Table storage documentation](https://learn.microsoft.com/en-us/azure/storage/tables/)
- [Azure Tables client library for .NET](https://learn.microsoft.com/en-us/dotnet/api/azure.data.tables?view=azure-dotnet)
- [Azure Cosmos DB for Table overview](https://learn.microsoft.com/en-us/azure/cosmos-db/table/overview)

## Suggested rollout

### MVP

- local printer profiles
- optional upload/download of known profiles
- one canonical `PrinterProfiles` table
- one `PrinterProfileSubmissions` table for pending writes

### V2

- add vote counters
- add profile confidence updates
- add moderation flags

### V3

- add telemetry aggregates in a separate table
- keep analytics separate from `PrinterProfiles`

## What not to do

- do not use Azure SQL for this profile store
- do not put telemetry and printer profiles into the same entity shape
- do not key by raw printer display name only
- do not rely on full-table scans for lookups

## Minimal backend shape

If you expose an API later, keep it very small:

- `GET /profiles/:manufacturer/:model`
- `POST /profile-submissions/:manufacturer/:model`
- `PUT /profiles/:manufacturer/:model`
- `POST /profiles/:manufacturer/:model/vote`

That is enough for the first cloud iteration.
