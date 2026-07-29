"use strict";

const assert = require("node:assert/strict");
const path = require("node:path");

class FakeElement {
  constructor(tagName) {
    this.tagName = tagName;
    this.className = "";
    this.textContent = "";
  }
}

global.HTMLElement = FakeElement;
global.document = {
  createElement(tagName) {
    return new FakeElement(tagName);
  },
};
global.window = {
  addEventListener() {},
};

const {
  formatDisplayNumber,
  formatTrendNumber,
  renderFieldValue,
} = require(path.join(__dirname, "mission_console", "static", "mission-console.js"));

assert.equal(formatDisplayNumber(1.234), "1.23");
assert.equal(formatDisplayNumber(1.2), "1.2");
assert.equal(formatDisplayNumber(-0.004), "0");
assert.equal(formatDisplayNumber("59.000"), "59");
assert.equal(formatDisplayNumber("39.126"), "39.13");
assert.equal(formatTrendNumber(0.1267), "0.13");

for (const preserved of [48, "48", "AUTO", "payload-49", "0x10", "1e-3"]) {
  assert.equal(formatDisplayNumber(preserved), null);
}

assert.equal(renderFieldValue("EPS_SOC", "59.126").textContent, "59.13");
assert.equal(renderFieldValue("captureIndex", "49").textContent, "49");
assert.equal(renderFieldValue("mode", "AUTO").textContent, "AUTO");
assert.equal(renderFieldValue("opcode", "0x10030001").textContent, "0x10030001");
assert.equal(
  renderFieldValue("values", [1.234, "2.500", 3, "AUTO", "0x10", "1e-3"]).textContent,
  "1.23, 2.5, 3, AUTO, 0x10, 1e-3",
);

console.log("mission-console display formatting: PASS");
