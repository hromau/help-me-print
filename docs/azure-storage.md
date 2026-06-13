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
- Storage Account: `duplexprintprod`
- Table: `PrinterProfiles`

## Data model

Use:

- `PartitionKey = manufacturer`
- `RowKey = normalized model`

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

Use the `Azure.Data.Tables` SDK from day one. It targets both Azure Table Storage and Azure Cosmos DB for Table, which reduces migration friction later.

## Suggested rollout

### MVP

- local printer profiles
- optional upload/download of known profiles
- one shared `PrinterProfiles` table

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
- `PUT /profiles/:manufacturer/:model`
- `POST /profiles/:manufacturer/:model/vote`

That is enough for the first cloud iteration.
