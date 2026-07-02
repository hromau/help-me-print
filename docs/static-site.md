# Static landing page

The static landing page lives in `site/` and is designed for Azure Storage Static Website hosting.

## Local preview

Open `site/index.html` in a browser, or serve it locally:

```bash
python3 -m http.server 8080 --directory site
```

Then open:

```text
http://localhost:8080
```

## Azure resources

Use the existing production resource group:

```text
duplexprint-prod-rg
```

Suggested storage account name:

```text
helpmeprintsite7ea1b526
```

Current static website endpoint:

```text
https://helpmeprintsite7ea1b526.z6.web.core.windows.net/
```

Create the storage account in `westeurope` with standard LRS redundancy, then enable static website hosting:

```bash
az storage account create \
  --resource-group duplexprint-prod-rg \
  --name helpmeprintsite7ea1b526 \
  --location westeurope \
  --sku Standard_LRS \
  --kind StorageV2

az storage blob service-properties update \
  --account-name helpmeprintsite7ea1b526 \
  --static-website \
  --index-document index.html
```

Get the primary static website endpoint:

```bash
az storage account show \
  --resource-group duplexprint-prod-rg \
  --name helpmeprintsite7ea1b526 \
  --query "primaryEndpoints.web" \
  --output tsv
```

## Deploy with Azure CLI

Upload the current static files directly into the `$web` container:

```bash
az storage blob upload-batch \
  --account-name helpmeprintsite7ea1b526 \
  --destination '$web' \
  --source site \
  --overwrite true \
  --content-cache-control "public, max-age=300"
```

After uploading, check the public endpoint:

```bash
curl -I "$(az storage account show \
  --resource-group duplexprint-prod-rg \
  --name helpmeprintsite7ea1b526 \
  --query "primaryEndpoints.web" \
  --output tsv)"
```

## SEO follow-up after deployment

If a custom domain is added later, replace the Azure endpoint in these files:

- `site/index.html`
- `site/robots.txt`
- `site/sitemap.xml`

Then verify:

- `/` serves the landing page.
- `/robots.txt` is reachable.
- `/sitemap.xml` is reachable.
- The page title and meta description show the final domain in search/social previews.
