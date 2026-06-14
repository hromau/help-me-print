const { app } = require("@azure/functions");
const { TableClient, odata } = require("@azure/data-tables");
const {
  profileEntityFromRequest,
  publicProfile,
  tableKeys
} = require("../profileModel");

const TABLE_NAME = process.env.PRINTER_PROFILES_TABLE || "PrinterProfiles";

function tableClient() {
  const connectionString = process.env.AZURE_TABLE_CONNECTION_STRING;
  if (!connectionString) {
    throw Object.assign(new Error("AZURE_TABLE_CONNECTION_STRING is not configured"), { status: 500 });
  }
  return TableClient.fromConnectionString(connectionString, TABLE_NAME);
}

function json(status, body) {
  return {
    status,
    jsonBody: body,
    headers: {
      "cache-control": "no-store"
    }
  };
}

function errorResponse(error) {
  const status = Number.isInteger(error.status) ? error.status : 500;
  return json(status, {
    error: status >= 500 ? "internal_error" : "bad_request",
    message: error.message
  });
}

app.http("getPrinterProfile", {
  methods: ["GET"],
  authLevel: "anonymous",
  route: "profiles/{manufacturer}/{model}",
  handler: async (request) => {
    try {
      const { manufacturer, model } = request.params;
      const { partitionKey, rowKey } = tableKeys(manufacturer, model);
      const entity = await tableClient().getEntity(partitionKey, rowKey);
      return json(200, publicProfile(entity));
    } catch (error) {
      if (error.statusCode === 404) {
        return json(404, { error: "not_found" });
      }
      return errorResponse(error);
    }
  }
});

app.http("savePrinterProfile", {
  methods: ["PUT"],
  authLevel: "function",
  route: "profiles/{manufacturer}/{model}",
  handler: async (request) => {
    try {
      const { manufacturer, model } = request.params;
      const body = await request.json();
      const entity = profileEntityFromRequest(manufacturer, model, body);
      await tableClient().upsertEntity(entity, "Merge");
      return json(200, publicProfile(entity));
    } catch (error) {
      return errorResponse(error);
    }
  }
});

app.http("votePrinterProfile", {
  methods: ["POST"],
  authLevel: "function",
  route: "profiles/{manufacturer}/{model}/vote",
  handler: async (request) => {
    try {
      const { manufacturer, model } = request.params;
      const { partitionKey, rowKey } = tableKeys(manufacturer, model);
      const client = tableClient();
      const entity = await client.getEntity(partitionKey, rowKey);
      const votes = Number(entity.votes || 0) + 1;
      const confidence = Math.max(0, Math.min(100, Number(entity.confidence || 80)));

      await client.updateEntity({
        partitionKey,
        rowKey,
        votes,
        confidence,
        updatedAt: new Date().toISOString()
      }, "Merge");

      return json(200, {
        normalizedKey: entity.normalizedKey,
        votes,
        confidence
      });
    } catch (error) {
      if (error.statusCode === 404) {
        return json(404, { error: "not_found" });
      }
      return errorResponse(error);
    }
  }
});

app.http("listPrinterProfiles", {
  methods: ["GET"],
  authLevel: "function",
  route: "profiles",
  handler: async () => {
    try {
      const profiles = [];
      const entities = tableClient().listEntities({
        queryOptions: {
          filter: odata`PartitionKey ne ''`
        }
      });

      for await (const entity of entities) {
        profiles.push(publicProfile(entity));
        if (profiles.length >= 100) {
          break;
        }
      }

      return json(200, { profiles });
    } catch (error) {
      return errorResponse(error);
    }
  }
});
