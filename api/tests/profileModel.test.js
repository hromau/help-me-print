const assert = require("node:assert/strict");
const test = require("node:test");
const {
  normalizedKey,
  profileEntityFromRequest,
  tableKeys
} = require("../src/profileModel");

test("normalizes manufacturer and model into stable table keys", () => {
  assert.equal(normalizedKey("HP", " Smart  Tank-580 "), "hp:smart tank 580");
  assert.deepEqual(tableKeys("HP", "Smart Tank 580"), {
    partitionKey: "hp",
    rowKey: "hp:smart tank 580"
  });
});

test("builds a valid table entity from a profile payload", () => {
  const entity = profileEntityFromRequest("HP", "Smart Tank 580", {
    outputFace: "up",
    firstPassParity: "even",
    secondPassParity: "odd",
    firstPassOrder: "normal",
    evenPagesOrder: "reverse",
    reloadMethod: "same_stack",
    confidence: 98
  }, new Date("2026-06-14T00:00:00.000Z"));

  assert.equal(entity.partitionKey, "hp");
  assert.equal(entity.rowKey, "hp:smart tank 580");
  assert.equal(entity.normalizedKey, "hp:smart tank 580");
  assert.equal(entity.source, "cloud");
  assert.equal(entity.schemaVersion, 1);
  assert.equal(entity.updatedAt, "2026-06-14T00:00:00.000Z");
});

test("rejects invalid profile enum values", () => {
  assert.throws(() => profileEntityFromRequest("HP", "Smart Tank 580", {
    outputFace: "sideways",
    firstPassParity: "even",
    secondPassParity: "odd",
    firstPassOrder: "normal",
    evenPagesOrder: "reverse",
    reloadMethod: "same_stack"
  }), /outputFace/);
});
