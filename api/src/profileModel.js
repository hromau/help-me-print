const VALID_OUTPUT_FACES = new Set(["up", "down"]);
const VALID_PAGE_PARITIES = new Set(["odd", "even"]);
const VALID_PAGE_ORDERS = new Set(["normal", "reverse"]);
const VALID_RELOAD_METHODS = new Set(["same_stack", "flip_long_edge", "flip_short_edge"]);

function normalizeTokenStream(value) {
  return String(value ?? "")
    .trim()
    .toLowerCase()
    .replace(/[^a-z0-9]+/g, " ")
    .replace(/\s+/g, " ")
    .trim();
}

function normalizeManufacturer(manufacturer) {
  return normalizeTokenStream(manufacturer);
}

function normalizeModel(model) {
  return normalizeTokenStream(model);
}

function normalizedKey(manufacturer, model) {
  const normalizedManufacturer = normalizeManufacturer(manufacturer);
  const normalizedModel = normalizeModel(model);
  return normalizedManufacturer && normalizedModel
    ? `${normalizedManufacturer}:${normalizedModel}`
    : normalizedManufacturer || normalizedModel;
}

function tableKeys(manufacturer, model) {
  const partitionKey = normalizeManufacturer(manufacturer);
  const rowKey = normalizedKey(manufacturer, model);

  if (!partitionKey || !rowKey) {
    throw Object.assign(new Error("manufacturer and model are required"), { status: 400 });
  }

  return { partitionKey, rowKey };
}

function requireEnum(value, field, validValues) {
  if (!validValues.has(value)) {
    throw Object.assign(new Error(`${field} must be one of: ${Array.from(validValues).join(", ")}`), { status: 400 });
  }
  return value;
}

function integerOrDefault(value, defaultValue) {
  const parsed = Number.parseInt(value, 10);
  return Number.isFinite(parsed) ? parsed : defaultValue;
}

function profileEntityFromRequest(manufacturer, model, body, now = new Date()) {
  const { partitionKey, rowKey } = tableKeys(manufacturer, model);
  const confidence = Math.max(0, Math.min(100, integerOrDefault(body.confidence, 80)));

  return {
    partitionKey,
    rowKey,
    displayName: String(body.displayName || `${manufacturer} ${model}`).trim(),
    manufacturer: String(body.manufacturer || manufacturer).trim(),
    model: String(body.model || model).trim(),
    normalizedKey: rowKey,
    outputFace: requireEnum(body.outputFace, "outputFace", VALID_OUTPUT_FACES),
    firstPassParity: requireEnum(body.firstPassParity, "firstPassParity", VALID_PAGE_PARITIES),
    secondPassParity: requireEnum(body.secondPassParity, "secondPassParity", VALID_PAGE_PARITIES),
    firstPassOrder: requireEnum(body.firstPassOrder, "firstPassOrder", VALID_PAGE_ORDERS),
    evenPagesOrder: requireEnum(body.evenPagesOrder, "evenPagesOrder", VALID_PAGE_ORDERS),
    reloadMethod: requireEnum(body.reloadMethod, "reloadMethod", VALID_RELOAD_METHODS),
    confidence,
    votes: Math.max(0, integerOrDefault(body.votes, 1)),
    source: "cloud",
    schemaVersion: 1,
    updatedAt: now.toISOString(),
    createdAt: body.createdAt || now.toISOString()
  };
}

function publicProfile(entity) {
  if (!entity) {
    return null;
  }

  return {
    displayName: entity.displayName,
    manufacturer: entity.manufacturer,
    model: entity.model,
    normalizedKey: entity.normalizedKey,
    outputFace: entity.outputFace,
    firstPassParity: entity.firstPassParity,
    secondPassParity: entity.secondPassParity,
    firstPassOrder: entity.firstPassOrder,
    evenPagesOrder: entity.evenPagesOrder,
    reloadMethod: entity.reloadMethod,
    confidence: entity.confidence,
    votes: entity.votes,
    source: entity.source,
    schemaVersion: entity.schemaVersion,
    updatedAt: entity.updatedAt
  };
}

module.exports = {
  normalizeManufacturer,
  normalizeModel,
  normalizedKey,
  profileEntityFromRequest,
  publicProfile,
  tableKeys
};
