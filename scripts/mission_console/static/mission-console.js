function xhrRequest(method, url, payload) {
  return new Promise((resolve, reject) => {
    const request = new XMLHttpRequest();
    request.open(method, url, true);
    request.responseType = "text";
    request.setRequestHeader("Accept", "application/json");
    if (payload !== undefined) {
      request.setRequestHeader("Content-Type", "application/json");
    }
    request.onload = () => {
      try {
        resolve(JSON.parse(request.responseText || "null"));
      } catch (error) {
        reject(error);
      }
    };
    request.onerror = () => reject(new Error(`request failed: ${method} ${url}`));
    request.send(payload === undefined ? null : JSON.stringify(payload));
  });
}

async function apiGet(url) {
  if (typeof fetch === "function") {
    const response = await fetch(url);
    return response.json();
  }
  return xhrRequest("GET", url);
}

async function apiPost(url, payload) {
  if (typeof fetch === "function") {
    const response = await fetch(url, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload),
    });
    return response.json();
  }
  return xhrRequest("POST", url, payload);
}

function pretty(value) {
  return JSON.stringify(value, null, 2);
}

const DECIMAL_NUMERIC_TEXT = /^[+-]?(?:(?:\d+\.\d*)|(?:\d*\.\d+))(?:[eE][+-]?\d+)?$/;

function formatDisplayNumber(value) {
  let parsed = null;
  if (typeof value === "number") {
    if (Number.isInteger(value)) {
      return null;
    }
    parsed = value;
  } else if (typeof value === "string" && DECIMAL_NUMERIC_TEXT.test(value)) {
    parsed = Number(value);
  } else {
    return null;
  }
  if (!Number.isFinite(parsed)) {
    return null;
  }
  const rounded = Number(parsed.toFixed(2));
  return String(Object.is(rounded, -0) ? 0 : rounded);
}

function node(tagName, className, textContent) {
  const element = document.createElement(tagName);
  if (className) {
    element.className = className;
  }
  if (textContent !== undefined && textContent !== null) {
    element.textContent = String(textContent);
  }
  return element;
}

function clearChildren(element) {
  if (element) {
    element.replaceChildren();
  }
}

function commandShortName(commandName) {
  return String(commandName || "").split(".").slice(-1)[0] || String(commandName || "");
}

function humanizeIdentifier(value) {
  return String(value || "").replaceAll("_", " ");
}

const DASHBOARD_FIELD_LABELS = {
  SYS_MODE: "OBC Mission Mode",
  ADCS_MODE: "ADCS Mode",
  ADCS_Q0: "ADCS Q0",
  GPS_SOURCE_MODE: "GPS Source Mode",
  PAYLOAD_STATE: "Payload State",
  EPS_VBAT: "Battery Voltage",
  EPS_IBAT: "Battery Current",
  EPS_SOC: "Battery State Of Charge",
  EPS_TEMP_BAT: "Battery Temperature",
  EPS_PDU_STATUS: "EPS PDU Status",
  SYS_UPTIME_SEC: "Uptime (s)",
  SYS_REBOOT_COUNT: "Reboot Count",
  TTC_POLICY_WINDOW_ACTIVE: "Pass Window Active",
  TTC_POLICY_TTC_ACTIVE: "TTC Policy Active",
  COMM_ACTIVE_BAND: "Active Band",
  COMM_PRIMARY_COMMAND_LINK: "Primary Command Link",
  COMM_PRIMARY_TELEMETRY_LINK: "Primary Telemetry Link",
  COMM_PRIMARY_FILE_LINK: "Primary File Link",
  COMM_S_BAND_AVAILABLE: "S-Band Available",
  COMM_UHF_AVAILABLE: "UHF Available",
  COMM_S_BAND_AVAILABILITY_REASON: "S-Band Availability Reason",
  COMM_UHF_AVAILABILITY_REASON: "UHF Availability Reason",
  COMM_FDIR_FAULT_LATCHED: "Comm FDIR Latched",
  COMM_FDIR_FAULT_KIND: "Comm FDIR Fault Kind",
  SEQ_CONTEXTS_ACTIVE: "Active Contexts",
  SEQ_LAST_CONTEXT_ID: "Last Context ID",
  SEQ_LAST_STATE: "Last Sequence State",
  SEQ_LAST_REASON: "Last Sequence Reason",
  SEQ_REJECT_TOTAL: "Sequence Reject Total",
  SESSION_LAST_ACCEPTED_SEQUENCE: "Last Accepted Secure Sequence",
  PAYLOAD_LAST_RESULT: "Last Payload Result",
  PAYLOAD_LAST_CAPTURE_ID: "Last Capture ID",
  SYS_CPU_USAGE: "CPU Usage",
  SYS_MEM_RSS_MB: "Memory RSS (MB)",
  SYS_RESOURCE_DEGRADED: "Resource Degraded",
  SYS_LOW_MEMORY: "Low Memory",
  lastObservedAt: "Last Beacon Time",
  sequence: "Sequence",
};

const DASHBOARD_CARD_DESCRIPTIONS = {
  "Satellite Status": "Current satellite-wide mode and subsystem status truth.",
  "EPS Snapshot": "Latest EPS electrical snapshot from the maintained operator surface.",
  "ADCS Snapshot": "Latest ADCS mode and selected continuous attitude indicators.",
  "Mission State": "Mission runtime, reboot, uptime, and TTC policy.",
  "Comm State": "Command, telemetry, file-link, and observability routing.",
  "Secure Session": "Local secure session state and latest operator action.",
  Beacon: "Latest UHF beacon summary from the maintained manual surface side-channel.",
  "Sequence & Ops": "Sequence controller status and secure-sequence progress.",
  "Health Snapshot": "Core resource health and degradation indicators.",
};

const TREND_SERIES_COLORS = {
  EPS_VBAT: "#147a73",
  EPS_IBAT: "#1f9e89",
  EPS_SOC: "#2d6f9f",
  EPS_TEMP_BAT: "#a45f1f",
  ADCS_Q0: "#5a6fb2",
  ADCS_Q1: "#765fb8",
  ADCS_Q2: "#8b5fb1",
  ADCS_Q3: "#ad5f96",
  ADCS_OMEGA_X: "#0e7c86",
  ADCS_OMEGA_Y: "#1b958f",
  ADCS_OMEGA_Z: "#49a45a",
  SYS_CPU_USAGE: "#8f4cb5",
  SYS_MEM_RSS_MB: "#cf7c36",
};

const SELECTOR_STORAGE_KEY = "mission-console.selector-state.v1";
const DETAILS_STORAGE_KEY = "mission-console.details-state.v1";
const MAX_TREND_PANELS = 4;

function loadJsonStorage(key, fallback) {
  try {
    const raw = window.localStorage.getItem(key);
    if (!raw) {
      return fallback;
    }
    return JSON.parse(raw);
  } catch (_error) {
    return fallback;
  }
}

function saveJsonStorage(key, value) {
  try {
    window.localStorage.setItem(key, JSON.stringify(value));
  } catch (_error) {
    // Ignore storage failures; UI still works with in-memory state.
  }
}

function loadSelectorState() {
  const fallback = { lastContextId: null, perContextLastBand: {} };
  const loaded = loadJsonStorage(SELECTOR_STORAGE_KEY, fallback);
  return {
    lastContextId: loaded?.lastContextId || null,
    perContextLastBand: loaded?.perContextLastBand || {},
  };
}

function persistSelectorState(contextId, band) {
  const state = loadSelectorState();
  if (contextId) {
    state.lastContextId = contextId;
    if (band) {
      state.perContextLastBand[contextId] = band;
    }
  }
  saveJsonStorage(SELECTOR_STORAGE_KEY, state);
}

function loadDetailsState() {
  return loadJsonStorage(DETAILS_STORAGE_KEY, {});
}

function persistDetailsState(key, open) {
  const state = loadDetailsState();
  state[key] = Boolean(open);
  saveJsonStorage(DETAILS_STORAGE_KEY, state);
}

function formatTimestamp(value) {
  if (!value) {
    return "Unavailable";
  }
  let normalized = value;
  if (typeof value === "string") {
    normalized = value.replace(/(\.\d{3})\d+/, "$1");
  }
  const parsed = new Date(normalized);
  if (Number.isNaN(parsed.getTime())) {
    return String(value);
  }
  return new Intl.DateTimeFormat("en-GB", {
    year: "numeric",
    month: "2-digit",
    day: "2-digit",
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
  }).format(parsed);
}

function isBooleanLike(name, value) {
  const normalized = String(value ?? "").trim().toLowerCase();
  if (normalized === "true" || normalized === "false") {
    return true;
  }
  if (normalized !== "0" && normalized !== "1") {
    return false;
  }
  return /(ACTIVE|AVAILABLE|ENABLED|INVALIDATED|DEGRADED|LOW_MEMORY|LATCHED|QUIET)$/i.test(String(name || ""));
}

function booleanLabel(value) {
  const normalized = String(value ?? "").trim().toLowerCase();
  return normalized === "true" || normalized === "1";
}

function toneForText(value) {
  const normalized = String(value ?? "").trim().toLowerCase();
  if (["true", "1", "active", "ready", "running", "succeeded", "authenticated", "explicit-reject"].includes(normalized)) {
    return "good";
  }
  if (["false", "0", "inactive", "invalidated", "failed", "stopped", "missing", "inconclusive"].includes(normalized)) {
    return "bad";
  }
  if (["bounded-no-op", "running", "queued", "pending", "partial", "warning"].includes(normalized)) {
    return "warn";
  }
  return "neutral";
}

function badge(text, tone = "neutral") {
  const element = node("span", `badge badge-${tone}`, text);
  return element;
}

function persistentDetails(title, content, storageKey = null, open = false, className = "advanced-block") {
  const details = node("details", "advanced-block");
  details.className = className;
  details.open = storageKey ? Boolean(loadDetailsState()[storageKey]) : open;
  if (storageKey) {
    details.dataset.storageKey = storageKey;
  }
  const summary = node("summary", "", title);
  if (storageKey) {
    details.addEventListener("toggle", () => persistDetailsState(storageKey, details.open));
  }
  details.append(summary, content);
  return details;
}

function persistVisibleDetailsState(root) {
  if (!root) {
    return;
  }
  root.querySelectorAll("details[data-storage-key]").forEach((details) => {
    persistDetailsState(details.dataset.storageKey, details.open);
  });
}

function rawJsonDetails(title, payload, open = false, storageKey = null) {
  const pre = node("pre", "raw-block", pretty(payload));
  return persistentDetails(title, pre, storageKey, open);
}

function renderFieldValue(name, value) {
  if (value === null || value === undefined || value === "") {
    return node("span", "muted", "Unavailable");
  }
  if (value instanceof HTMLElement) {
    return value;
  }
  if (typeof value === "boolean") {
    return badge(value ? "True" : "False", value ? "good" : "bad");
  }
  if (Array.isArray(value)) {
    return node("span", "", value.map((item) => formatDisplayNumber(item) ?? item).join(", "));
  }
  if (typeof value === "object") {
    return node("code", "inline-code", pretty(value));
  }
  if (isBooleanLike(name, value)) {
    const state = booleanLabel(value);
    return badge(state ? "True" : "False", state ? "good" : "bad");
  }
  return node("span", "", formatDisplayNumber(value) ?? value);
}

function displayFieldLabel(name) {
  return DASHBOARD_FIELD_LABELS[name] || humanizeIdentifier(name);
}

function isVerboseArtifactValue(value) {
  if (typeof value !== "string") {
    return false;
  }
  const trimmed = value.trim();
  return trimmed.length > 120 || trimmed.startsWith("{") || trimmed.startsWith("[");
}

function keyValueList(rows) {
  const host = node("div", "kv-list");
  rows.forEach((row) => {
    const item = node("div", `kv-item${row.fullWidth ? " kv-item-wide" : ""}`);
    const key = node("div", "kv-key");
    key.append(node("span", "", row.label));
    if (row.secondary && row.secondaryDisplay === "tooltip") {
      key.append(node("span", "kv-meta-indicator", "i"));
      item.title = row.secondary;
      item.setAttribute("aria-label", `${row.label}: ${row.secondary.replace(/\n/g, ", ")}`);
    }
    const value = node("div", "kv-value");
    value.append(renderFieldValue(row.label, row.value));
    if (row.secondary && row.secondaryDisplay !== "tooltip") {
      value.append(node("div", "kv-secondary", row.secondary));
    }
    item.append(key, value);
    host.appendChild(item);
  });
  return host;
}

function renderSectionTitle(title, description) {
  const wrapper = node("div", "section-title");
  wrapper.append(node("h3", "", title));
  if (description) {
    wrapper.append(node("p", "muted", description));
  }
  return wrapper;
}

function renderStructuredEvent(event) {
  const item = node("article", "timeline-item");
  const head = node("div", "timeline-head");
  head.append(
    node("div", "timeline-time", formatTimestamp(event.timestamp)),
    badge(event.eventName, "neutral"),
  );
  const body = node("div", "timeline-body");
  body.append(node("div", "timeline-message", event.message || "No message"));
  const structured = event.structured || {};
  const rows = Object.entries(structured)
    .filter(([key]) => key !== "rawMessage")
    .map(([key, value]) => ({ label: key, value }));
  if (rows.length) {
    body.append(keyValueList(rows));
  }
  item.append(head, body);
  return item;
}

function renderTimeline(events) {
  const host = node("div", "timeline");
  if (!events || !events.length) {
    host.append(node("p", "muted", "No recent events."));
    return host;
  }
  events.forEach((event) => host.appendChild(renderStructuredEvent(event)));
  return host;
}

function renderChannelRows(channels) {
  const orderedEntries = Object.entries(channels || {}).sort((left, right) => {
    const leftMissing = !left[1] || left[1].value === null || left[1].value === undefined || left[1].value === "";
    const rightMissing = !right[1] || right[1].value === null || right[1].value === undefined || right[1].value === "";
    if (leftMissing === rightMissing) {
      return 0;
    }
    return leftMissing ? 1 : -1;
  });
  const rows = orderedEntries.map(([name, snapshot]) => {
    if (!snapshot) {
      return {
        label: displayFieldLabel(name),
        value: "Unavailable",
      };
    }
    const meta = [];
    if (snapshot.observationSource === "refresh") {
      meta.push(`Refreshed by ${commandShortName(snapshot.refreshCommand)}`);
    } else if (snapshot.observationSource === "live-update") {
      meta.push("Background update");
    } else if (snapshot.observationSource) {
      meta.push(humanizeIdentifier(snapshot.observationSource));
    }
    if (snapshot.timestamp) {
      meta.push(`Flight sampled ${formatTimestamp(snapshot.timestamp)}`);
    }
    if (snapshot.observationAt) {
      meta.push(`Gateway observed ${formatTimestamp(snapshot.observationAt)}`);
    }
    if (snapshot.sourceBand) {
      meta.push(`Via ${snapshot.sourceBand}`);
    }
    return {
      label: displayFieldLabel(name),
      value: snapshot.value,
      secondary: meta.length ? meta.join("\n") : undefined,
      secondaryDisplay: meta.length ? "tooltip" : undefined,
    };
  });
  return keyValueList(rows);
}

function renderDashboardCards(cards) {
  const host = document.getElementById("dashboard-cards");
  if (!host) {
    return;
  }
  clearChildren(host);
  const preferredOrder = [
    "Satellite Status",
    "EPS Snapshot",
    "ADCS Snapshot",
    "Comm State",
    "Mission State",
    "Secure Session",
    "Beacon",
    "Health Snapshot",
    "Sequence & Ops",
  ];
  const orderedEntries = Object.entries(cards || {}).sort(([left], [right]) => {
    const leftIndex = preferredOrder.indexOf(left);
    const rightIndex = preferredOrder.indexOf(right);
    const normalizedLeft = leftIndex === -1 ? Number.MAX_SAFE_INTEGER : leftIndex;
    const normalizedRight = rightIndex === -1 ? Number.MAX_SAFE_INTEGER : rightIndex;
    return normalizedLeft - normalizedRight || left.localeCompare(right);
  });
  orderedEntries.forEach(([title, payload]) => {
    const card = node("article", "card data-card");
    if (title === "Satellite Status") {
      card.classList.add("satellite-status");
    }
    if (title === "Comm State") {
      card.classList.add("comm-state");
    }
    card.append(renderSectionTitle(title, DASHBOARD_CARD_DESCRIPTIONS[title]));
    if (title === "Secure Session") {
      const session = payload.secureSession || {};
      const sessionRows = [
        { label: "Selected Band", value: payload.selectedBand || "Unavailable" },
        { label: "Session State", value: session.active ? "Active" : (session.invalidated ? "Invalidated" : "Missing") },
        { label: "Authority Mode", value: session.authorityMode || "Unavailable" },
        { label: "Next Secure Sequence", value: session.nextSecureSequence ?? "Unavailable" },
        { label: "Last Auth Time", value: session.lastAuthTime ? formatTimestamp(session.lastAuthTime * 1000) : "Unavailable" },
      ];
      if (session.invalidationReason) {
        sessionRows.push({ label: "Invalidation Reason", value: session.invalidationReason });
      }
      card.append(keyValueList(sessionRows));
      if (payload.lastAction) {
        const action = payload.lastAction.result || {};
        card.append(renderSectionTitle("Last Action", payload.lastAction.request?.kind || undefined));
        card.append(keyValueList([
          { label: "Status", value: action.status || "Unavailable" },
          { label: "Command", value: action.commandName || "Unavailable" },
          { label: "Finished", value: action.finishedAt ? formatTimestamp(action.finishedAt) : "Unavailable" },
        ]));
      }
      return host.appendChild(card);
    }
    if (title === "Beacon") {
      const rows = [
        {
          label: "Last Beacon Time",
          value: payload.available ? formatTimestamp(payload.lastObservedAt) : "Unavailable",
        },
        {
          label: "Sequence",
          value: payload.available ? (payload.sequence ?? "Unavailable") : "Unavailable",
          secondary: (!payload.available && payload.reason) ? payload.reason : undefined,
          secondaryDisplay: (!payload.available && payload.reason) ? "tooltip" : undefined,
        },
      ];
      card.append(keyValueList(rows));
      return host.appendChild(card);
    }
    const channelPayload = { ...payload };
    const latestPayloadEvent = channelPayload.latestPayloadEvent;
    delete channelPayload.latestPayloadEvent;
    card.append(renderChannelRows(channelPayload));
    if (latestPayloadEvent) {
      card.append(renderSectionTitle("Latest Payload Event"));
      card.append(renderTimeline([latestPayloadEvent]));
    }
    host.appendChild(card);
  });
}

function renderBeaconLatest(payload) {
  const host = document.getElementById("beacon-latest");
  if (!host) {
    return;
  }
  clearChildren(host);
  const stack = node("div", "result-stack");
  const summaryRows = [
    { label: "Supported", value: payload.supported ? "True" : "False" },
    { label: "Available", value: payload.available ? "True" : "False" },
    { label: "Selected Band", value: payload.selectedBand || "Unavailable" },
    { label: "Source Band", value: payload.sourceBand || "Unavailable" },
    { label: "Source Kind", value: payload.sourceKind || "Unavailable" },
    { label: "Last Observed", value: payload.lastObservedAt ? formatTimestamp(payload.lastObservedAt) : "Unavailable" },
    { label: "Sequence", value: payload.sequence ?? "Unavailable" },
  ];
  if (payload.reason) {
    summaryRows.push({ label: "Reason", value: payload.reason });
  }
  stack.append(renderSectionTitle("Summary"));
  stack.append(keyValueList(summaryRows));
  if (payload.capture) {
    stack.append(renderSectionTitle("Capture"));
    stack.append(keyValueList([
      { label: "Path", value: payload.capture.path || "Unavailable", fullWidth: true },
      { label: "Size (bytes)", value: payload.capture.sizeBytes ?? "Unavailable" },
      { label: "Frame Size", value: payload.capture.frameSize ?? "Unavailable" },
      { label: "Frame Count", value: payload.capture.frameCount ?? "Unavailable" },
    ]));
  }
  if (payload.decode) {
    stack.append(renderSectionTitle("Decode"));
    stack.append(keyValueList([
      { label: "Status", value: payload.decode.status || "Unavailable" },
      { label: "Error", value: payload.decode.error || "Unavailable", fullWidth: true },
    ]));
  }
  if (payload.decoded) {
    const decoded = payload.decoded;
    stack.append(renderSectionTitle("Decoded Fields"));
    stack.append(keyValueList([
      { label: "Flight Seconds", value: decoded.time?.seconds ?? "Unavailable" },
      { label: "Mode", value: decoded.mode ?? "Unavailable" },
      { label: "Boot Slot", value: decoded.boot_slot ?? "Unavailable" },
      { label: "Reboot Count", value: decoded.reboot_count ?? "Unavailable" },
      { label: "Uptime (s)", value: decoded.uptime_sec ?? "Unavailable" },
      { label: "Battery SoC", value: decoded.battery_soc ?? "Unavailable" },
      { label: "Battery Voltage", value: decoded.battery_voltage ?? "Unavailable" },
      { label: "Battery Current", value: decoded.battery_current ?? "Unavailable" },
      { label: "Battery Temp C", value: decoded.battery_temp_c ?? "Unavailable" },
      { label: "ADCS Mode", value: decoded.adcs_mode ?? "Unavailable" },
      { label: "GPS Fix Valid", value: decoded.gps_fix_valid ?? "Unavailable" },
      { label: "CSP TX Packets", value: decoded.csp_tx_packets ?? "Unavailable" },
      { label: "CSP RX Packets", value: decoded.csp_rx_packets ?? "Unavailable" },
      { label: "Radio TX Bytes", value: decoded.radio_tx_bytes ?? "Unavailable" },
      { label: "Radio RX Bytes", value: decoded.radio_rx_bytes ?? "Unavailable" },
      { label: "CRC", value: decoded.crc ?? "Unavailable" },
    ]));
    stack.append(rawJsonDetails("Raw Decoded Payload", decoded, false, `beacon:${selectedContext()}:decoded`));
  }
  host.append(stack);
}

function renderBeaconHistory(historyPayload) {
  const host = document.getElementById("beacon-history");
  if (!host) {
    return;
  }
  clearChildren(host);
  const entries = historyPayload.history || [];
  if (!entries.length) {
    host.append(node("p", "empty-state", "No beacon history has been captured yet."));
    return;
  }
  const stack = node("div", "cards");
  entries.slice().reverse().forEach((entry) => {
    const card = node("article", "card data-card");
    card.append(renderSectionTitle(
      `Sequence ${entry.sequence ?? "Unavailable"}`,
      entry.lastObservedAt ? formatTimestamp(entry.lastObservedAt) : "Unavailable",
    ));
    card.append(keyValueList([
      { label: "Available", value: entry.available ? "True" : "False" },
      { label: "Source Band", value: entry.sourceBand || "Unavailable" },
      { label: "Source Kind", value: entry.sourceKind || "Unavailable" },
      { label: "Decode Status", value: entry.decode?.status || "Unavailable" },
      { label: "Capture Bytes", value: entry.capture?.sizeBytes ?? "Unavailable" },
      { label: "Frame Count", value: entry.capture?.frameCount ?? "Unavailable" },
    ]));
    if (entry.summary) {
      card.append(keyValueList([
        { label: "Flight Seconds", value: entry.summary.lastBeaconTime ?? "Unavailable" },
        { label: "Mode", value: entry.summary.mode ?? "Unavailable" },
        { label: "Battery SoC", value: entry.summary.batterySoc ?? "Unavailable" },
        { label: "ADCS Mode", value: entry.summary.adcsMode ?? "Unavailable" },
      ]));
    }
    stack.append(card);
  });
  host.append(stack);
}

function renderRecentEvents(events) {
  const host = document.getElementById("recent-events");
  if (!host) {
    return;
  }
  clearChildren(host);
  host.appendChild(renderTimeline(events));
}

function selectedContext() {
  return document.getElementById("context-select")?.value;
}

function selectedBand() {
  return document.getElementById("band-select")?.value;
}

function hasSelectedBand() {
  const bandSelect = document.getElementById("band-select");
  return Boolean(bandSelect && !bandSelect.disabled && bandSelect.value);
}

function currentContextRecord() {
  const contexts = window.MISSION_CONSOLE_CONTEXTS || {};
  return contexts[selectedContext()] || null;
}

function bandUnavailableMessage(context = currentContextRecord()) {
  if (!context) {
    return "No context metadata is available yet.";
  }
  if (context.errors?.includes("manifest-missing") || context.errors?.includes("status-missing")) {
    return `No active bands are available because ${context.contextId} is not currently running.`;
  }
  return `No active bands are exported for ${context.contextId}.`;
}

function setEmptyState(host, message) {
  if (!host) {
    return;
  }
  clearChildren(host);
  host.append(node("p", "empty-state", message));
}

function syncBandRequiredControls() {
  const enabled = hasSelectedBand();
  document.querySelectorAll("[data-band-required]").forEach((element) => {
    if ("disabled" in element) {
      element.disabled = !enabled;
    }
  });
}

function guardBandRequiredAction(sink, actionLabel) {
  if (hasSelectedBand()) {
    return true;
  }
  setEmptyState(sink, `${actionLabel} is unavailable: ${bandUnavailableMessage()}`);
  return false;
}

async function refreshSelectors() {
  const payload = await apiGet("/api/contexts");
  window.MISSION_CONSOLE_CONTEXTS = payload.contexts || {};
  const contextSelect = document.getElementById("context-select");
  const bandSelect = document.getElementById("band-select");
  if (!contextSelect || !bandSelect) {
    return payload;
  }
  const initialContext = document.body.dataset.initialContext;
  const initialBand = document.body.dataset.initialBand;
  const previousContext = contextSelect.value;
  const previousBand = bandSelect.value;
  const stored = loadSelectorState();
  const contexts = payload.contexts || {};
  clearChildren(contextSelect);
  const contextIds = Object.keys(contexts);
  const preferredContext = contextIds.includes(stored.lastContextId)
    ? stored.lastContextId
    : (contextIds.includes(previousContext) ? previousContext : (contextIds.includes(initialContext) ? initialContext : contextIds[0]));
  contextIds.forEach((contextId) => {
    const option = node("option", "", contextId);
    option.value = contextId;
    if (contextId === preferredContext) {
      option.selected = true;
    }
    contextSelect.appendChild(option);
  });
  if (!contextSelect.value && contextIds.length) {
    contextSelect.value = contextIds[0];
  }
  const renderBands = (preferredBandOverride) => {
    const contextId = contextSelect.value;
    const context = contexts[contextId];
    const bandNames = Object.keys((context || {}).bands || {});
    const persistedBand = stored.perContextLastBand?.[contextId];
    const previousForSameContext = previousContext === contextId ? previousBand : "";
    const desiredBand = [
      preferredBandOverride,
      previousForSameContext,
      persistedBand,
      contextId === initialContext ? initialBand : "",
      bandNames[0],
    ].find((candidate) => candidate && bandNames.includes(candidate));
    clearChildren(bandSelect);
    bandSelect.disabled = false;
    bandNames.forEach((bandName) => {
      const option = node("option", "", bandName);
      option.value = bandName;
      if (bandName === desiredBand) {
        option.selected = true;
      }
      bandSelect.appendChild(option);
    });
    if (bandNames.length) {
      if (!bandSelect.value) {
        bandSelect.value = bandNames[0];
      }
      bandSelect.title = "";
      persistSelectorState(contextId, bandSelect.value);
    } else {
      persistSelectorState(contextId, "");
      const option = node("option", "", "No active bands available");
      option.value = "";
      option.selected = true;
      bandSelect.appendChild(option);
      bandSelect.disabled = true;
      bandSelect.title = bandUnavailableMessage(context);
    }
    syncBandRequiredControls();
  };
  renderBands();
  contextSelect._renderBands = renderBands;
  return payload;
}

function renderOperationResult(result) {
  const wrapper = node("div", "result-stack");
  const topRows = [
    { label: "Status", value: result.status || "Unavailable" },
    { label: "Started", value: result.startedAt ? formatTimestamp(result.startedAt) : "Unavailable" },
    { label: "Finished", value: result.finishedAt ? formatTimestamp(result.finishedAt) : "Unavailable" },
  ];
  if (result.commandName) {
    topRows.push({ label: "Command", value: result.commandName });
  }
  if (result.args && result.args.length) {
    topRows.push({ label: "Arguments", value: result.args.join(" ") });
  }
  if (result.secureSequence !== null && result.secureSequence !== undefined) {
    topRows.push({ label: "Secure Sequence", value: result.secureSequence });
  }
  wrapper.append(keyValueList(topRows));
  if (result.readback) {
    const freshness = result.readback.freshness || {};
    wrapper.append(renderSectionTitle("Readback Saved", "Open Readback to inspect the saved subsystem card and full proof payload."));
    wrapper.append(keyValueList([
      { label: "Family", value: result.readback.family || "Unavailable" },
      { label: "Channel Source", value: result.readback.channelSource || "Unavailable" },
      { label: "Event Source", value: result.readback.eventSource || "Unavailable" },
      { label: "Fresh Channels", value: Object.keys(freshness.freshChannels || {}).length },
      { label: "Fresh Events", value: (freshness.freshEvents || []).length },
    ]));
  }
  if (result.artifacts && Object.keys(result.artifacts).length) {
    wrapper.append(renderSectionTitle("Artifacts"));
    const artifactRows = Object.entries(result.artifacts)
      .filter(([, value]) => {
        if (value === null || value === undefined) {
          return true;
        }
        if (!["string", "number", "boolean"].includes(typeof value)) {
          return false;
        }
        return !isVerboseArtifactValue(value);
      })
      .map(([key, value]) => ({ label: humanizeIdentifier(key), value }));
    if (artifactRows.length) {
      wrapper.append(keyValueList(artifactRows));
    }
    const needsRawArtifact = Object.values(result.artifacts).some(
      (value) => (typeof value === "object" && value !== null) || isVerboseArtifactValue(value),
    );
    if (needsRawArtifact || !artifactRows.length) {
      wrapper.append(rawJsonDetails("Artifact Details", result.artifacts));
    }
  }
  if (result.error) {
    wrapper.append(renderSectionTitle("Error"));
    wrapper.append(node("pre", "raw-block", result.error));
  }
  return wrapper;
}

function renderReadbackPayload(readback, detailsKeyPrefix = null) {
  const host = node("div", "result-stack");
  const freshness = readback.freshness || {};
  const readbackChannelNames = Object.keys(readback.channels || {});
  const freshChannelNames = Object.keys(freshness.freshChannels || {});
  const sameChannelSet = readbackChannelNames.length === freshChannelNames.length
    && readbackChannelNames.every((name) => freshChannelNames.includes(name));
  host.append(keyValueList([
    { label: "Family", value: readback.family || "Unavailable" },
    { label: "Channel Source", value: readback.channelSource || "Unavailable" },
    { label: "Event Source", value: readback.eventSource || "Unavailable" },
    { label: "Fresh Channel Names", value: Object.keys(freshness.freshChannels || {}).join(", ") || "Unavailable" },
  ]));
  host.append(renderSectionTitle("Last Refresh Evidence"));
  host.append(keyValueList([
    { label: "Fresh Channels", value: Object.keys(freshness.freshChannels || {}).length },
    { label: "Fresh Events", value: (freshness.freshEvents || []).length },
    { label: "Reject Events", value: (freshness.rejectEvents || []).length },
  ]));
  if (!sameChannelSet && freshness.freshChannels && Object.keys(freshness.freshChannels).length) {
    host.append(renderSectionTitle("Last Refresh Evidence Fields"));
    host.append(renderChannelRows(freshness.freshChannels));
  }
  if (readback.events && readback.events.length) {
    host.append(renderSectionTitle("Event Evidence"));
    host.append(renderTimeline(readback.events));
  }
  if (readback.mainEvidence) {
    host.append(renderSectionTitle("Main Evidence"));
    host.append(renderGenericObject({
      kind: readback.mainEvidence.kind,
      source: readback.mainEvidence.source,
    }));
    if (readback.mainEvidence.events && readback.mainEvidence.events.length) {
      host.append(renderTimeline(readback.mainEvidence.events));
    }
  }
  if (readback.fallbackEvidence) {
    host.append(renderSectionTitle("Fallback Evidence"));
    host.append(renderGenericObject({
      kind: readback.fallbackEvidence.kind,
      source: readback.fallbackEvidence.source,
    }));
    if (readback.fallbackEvidence.events && readback.fallbackEvidence.events.length) {
      host.append(renderTimeline(readback.fallbackEvidence.events));
    }
  }
  if (readback.incompleteResult) {
    host.append(renderSectionTitle("Incomplete Result"));
    host.append(renderGenericObject({
      kind: readback.incompleteResult.kind,
      reason: readback.incompleteResult.reason,
      source: readback.incompleteResult.source,
    }));
    if (readback.incompleteResult.events && readback.incompleteResult.events.length) {
      host.append(renderTimeline(readback.incompleteResult.events));
    }
  }
  if (readback.completionEvidence) {
    host.append(renderSectionTitle("Completion Evidence"));
    host.append(renderTimeline([readback.completionEvidence]));
  }
  host.append(rawJsonDetails("Raw Readback Payload", readback, false, detailsKeyPrefix ? `${detailsKeyPrefix}:raw` : null));
  return host;
}

function renderSavedReadbackValues(savedValues) {
  if (!savedValues) {
    return node("p", "muted", "No saved values yet.");
  }
  if (savedValues.kind === "channels") {
    return renderChannelRows(savedValues.fields || {});
  }
  if (savedValues.kind === "event") {
    const host = node("div", "result-stack");
    host.append(keyValueList([
      { label: "Event", value: savedValues.eventName || "Unavailable" },
      { label: "Timestamp", value: savedValues.timestamp ? formatTimestamp(savedValues.timestamp) : "Unavailable" },
    ]));
    host.append(keyValueList(
      Object.entries(savedValues.fields || {}).map(([key, value]) => ({ label: humanizeIdentifier(key), value })),
    ));
    return host;
  }
  if (savedValues.kind === "event-group") {
    const host = node("div", "result-stack");
    const events = savedValues.events || [];
    if (!events.length) {
      host.append(node("p", "muted", "No structured event records saved."));
      return host;
    }
    events.forEach((event) => {
      const card = node("article", "mini-card");
      card.append(
        node("strong", "", event.eventName || "Event"),
        node("p", "muted", event.timestamp ? formatTimestamp(event.timestamp) : "Unavailable"),
      );
      card.append(keyValueList(
        Object.entries(event.fields || {}).map(([key, value]) => ({ label: humanizeIdentifier(key), value })),
      ));
      host.append(card);
    });
    return host;
  }
  return renderGenericObject(savedValues);
}

function readbackRefreshKey(commandName) {
  return `${selectedContext()}:${selectedBand()}:${commandName}`;
}

function readbackRefreshTone(status) {
  if (status === "refreshing") {
    return "warn";
  }
  if (status === "succeeded") {
    return "good";
  }
  if (status === "failed") {
    return "bad";
  }
  return "neutral";
}

function readbackRefreshLabel(status) {
  if (status === "refreshing") {
    return "Refreshing";
  }
  if (status === "succeeded") {
    return "Refreshed";
  }
  if (status === "failed") {
    return "Refresh failed";
  }
  return "Idle";
}

async function waitForJobQuiet(jobId, onUpdate = null) {
  for (;;) {
    const payload = await apiGet(`/api/jobs/${jobId}`);
    if (typeof onUpdate === "function") {
      onUpdate(payload);
    }
    if (payload.status === "finished" || payload.status === "failed" || payload.status === "missing") {
      return payload;
    }
    await new Promise((resolve) => setTimeout(resolve, 1000));
  }
}

async function submitReadbackQuickRefresh(card, state) {
  if (!card.refreshable || !card.refreshCommandName || !hasSelectedBand()) {
    return;
  }
  const refreshKey = readbackRefreshKey(card.commandName);
  state.quickRefresh[refreshKey] = {
    status: "refreshing",
    jobId: null,
    error: null,
  };
  renderReadbackViewer(state, state.payload || { tabs: [] });
  try {
    const queued = await apiPost("/api/readback/run", {
      contextId: selectedContext(),
      band: selectedBand(),
      commandName: card.refreshCommandName,
      commandArgs: [],
      ensureAuth: true,
    });
    state.quickRefresh[refreshKey] = {
      status: "refreshing",
      jobId: queued.jobId,
      error: null,
    };
    renderReadbackViewer(state, state.payload || { tabs: [] });
    const payload = await waitForJobQuiet(queued.jobId);
    const resultStatus = payload?.result?.status;
    const failed = payload?.status === "failed" || resultStatus === "failed" || payload?.status === "missing";
    state.quickRefresh[refreshKey] = {
      status: failed ? "failed" : "succeeded",
      jobId: queued.jobId,
      error: failed ? String(payload?.error || payload?.result?.error || "Quick refresh failed.") : null,
    };
  } catch (error) {
    state.quickRefresh[refreshKey] = {
      status: "failed",
      jobId: null,
      error: error instanceof Error ? error.message : String(error),
    };
  }
  await state.refresh();
}

function renderReadbackViewerCard(card, state) {
  const shell = node("article", "card data-card");
  const head = node("div", "panel-head");
  const refreshState = state.quickRefresh[readbackRefreshKey(card.commandName)] || { status: "idle", error: null };
  const titleBlock = node("div", "readback-card-title");
  const actions = node("div", "readback-card-actions");
  head.append(titleBlock, actions);
  head.firstChild.append(
    node("h3", "", commandShortName(card.commandName)),
    node("p", "muted", `Source command: ${card.sourceCommand || card.commandName}`),
  );
  if (card.refreshable && card.refreshCommandName) {
    const refreshButton = node("button", "icon-button readback-refresh-button");
    refreshButton.type = "button";
    refreshButton.title = `Refresh via ${card.refreshCommandName}`;
    refreshButton.setAttribute("aria-label", `Refresh via ${card.refreshCommandName}`);
    refreshButton.disabled = refreshState.status === "refreshing" || !hasSelectedBand();
    refreshButton.textContent = refreshState.status === "refreshing" ? "..." : "↻";
    refreshButton.onclick = () => void submitReadbackQuickRefresh(card, state);
    actions.append(refreshButton);
  }
  actions.append(badge(card.state || "empty", toneForText(card.state || "neutral")));
  if (refreshState.status !== "idle") {
    actions.append(badge(readbackRefreshLabel(refreshState.status), readbackRefreshTone(refreshState.status)));
  }
  shell.append(head);
  shell.append(keyValueList([
    { label: "Last Refresh", value: card.lastRefreshTime ? formatTimestamp(card.lastRefreshTime) : "Unavailable" },
    { label: "Readback Family", value: card.family || "Unavailable" },
  ]));
  if (refreshState.status === "refreshing") {
    shell.append(node("p", "muted readback-refresh-note", "Refreshing saved readback..."));
  } else if (refreshState.status === "failed" && refreshState.error) {
    shell.append(node("p", "readback-refresh-error", refreshState.error.split("\n")[0]));
  }
  shell.append(renderSectionTitle("Latest Saved Values"));
  const savedValues = renderSavedReadbackValues(card.savedValues);
  savedValues.classList.add("readback-values-panel");
  shell.append(savedValues);
  const debug = card.debugPayload;
  if (debug) {
    const detailsKey = `readback:${selectedContext()}:${selectedBand()}:${card.commandName}`;
    shell.append(
      persistentDetails(
        "Proof / Debug Details",
        renderReadbackPayload(debug, detailsKey),
        detailsKey,
        false,
        "advanced-block proof-panel",
      ),
    );
  }
  if (card.error) {
    shell.append(renderSectionTitle("Latest Failure"));
    shell.append(node("pre", "raw-block", String(card.error).split("\n")[0]));
  }
  return shell;
}

function renderPacketSummary(summary) {
  const host = node("div", "result-stack");
  host.append(keyValueList([
    { label: "Packet Source", value: summary.source || "Unavailable" },
    { label: "Highlight", value: summary.highlight || "Unavailable" },
    { label: "Payload Length", value: summary.transport?.payloadLength ?? "Unavailable" },
  ]));
  if (summary.fwPacketHeader) {
    host.append(renderSectionTitle("FW Packet Header"));
    host.append(keyValueList([
      { label: "Packet Kind", value: summary.fwPacketHeader.packetKind ?? "Unavailable" },
      { label: "Opcode", value: summary.fwPacketHeader.opcode || "Unavailable" },
    ]));
  }
  if (summary.secureHeader && Object.keys(summary.secureHeader).length) {
    host.append(renderSectionTitle("Secure Header"));
    host.append(keyValueList([
      { label: "Magic", value: summary.secureHeader.magic || "Unavailable" },
      { label: "Version", value: summary.secureHeader.version ?? "Unavailable" },
      { label: "Secure Sequence", value: summary.secureHeader.secureSequence ?? "Unavailable" },
      { label: "Inner Length", value: summary.secureHeader.innerLength ?? "Unavailable" },
      { label: "MAC Length", value: summary.secureHeader.macLength ?? "Unavailable" },
    ]));
  }
  if (summary.innerCommand && Object.keys(summary.innerCommand).length) {
    host.append(renderSectionTitle("Inner Command"));
    host.append(keyValueList([
      { label: "Packet Kind", value: summary.innerCommand.packetKind ?? "Unavailable" },
      { label: "Opcode", value: summary.innerCommand.opcode || "Unavailable" },
      { label: "Command Name", value: summary.innerCommand.commandName || "Unavailable" },
    ]));
  }
  if (summary.authPreview && Object.keys(summary.authPreview).length) {
    host.append(renderSectionTitle("Auth Preview"));
    host.append(keyValueList([
      { label: "Head", value: summary.authPreview.head || "Unavailable" },
      { label: "Tail", value: summary.authPreview.tail || "Unavailable" },
    ]));
  }
  return host;
}

function renderPacketLabRecord(record) {
  const host = node("div", "result-stack");
  host.append(keyValueList([
    { label: "Case", value: record.case || "Unavailable" },
    { label: "Description", value: record.description || "Unavailable" },
    { label: "Expected Failure", value: record.expectedFailureReason || "Unavailable" },
    { label: "Observed Flight Rejection", value: record.observedEvidence?.kind || record.status || "Unavailable" },
    { label: "Timestamp", value: record.timestamp ? formatTimestamp(record.timestamp) : "Unavailable" },
  ]));
  if (record.faultExplanation) {
    host.append(renderSectionTitle("Injected Fault Model"));
    host.append(keyValueList([
      { label: "Fault Kind", value: record.faultExplanation.faultKind || "Unavailable" },
      { label: "Affected Field", value: record.faultExplanation.affectedField || "Unavailable" },
      { label: "Expected Condition", value: record.faultExplanation.expectedCondition || "Unavailable", fullWidth: true },
      { label: "Actual Injected Condition", value: record.faultExplanation.actualInjectedCondition || "Unavailable", fullWidth: true },
      { label: "Expected Failure", value: record.expectedFailureReason || "Unavailable" },
      { label: "Ground-side Interpretation", value: record.faultExplanation.whyRejected || "Unavailable", fullWidth: true },
    ]));
  }
  if (record.packetSummary) {
    host.append(renderSectionTitle("Packet Summary"));
    host.append(renderPacketSummary(record.packetSummary));
  }
  if (record.observedEvidence) {
    host.append(renderSectionTitle("Observed Flight Rejection"));
    const observedRows = [
      { label: "Observed Kind", value: record.observedEvidence.kind || "Unavailable" },
      { label: "Reject Path", value: record.observedEvidence.rejectPath || "Unavailable" },
      { label: "Reject Reason", value: record.observedEvidence.reasonName || "Unavailable" },
      { label: "Reject Reason Code", value: record.observedEvidence.reasonValue ?? "Unavailable" },
    ];
    host.append(keyValueList(observedRows));
    const extraObservedRows = Object.entries(record.observedEvidence || {})
      .filter(([key, value]) => ![
        "kind",
        "rejectPath",
        "reasonName",
        "reasonValue",
        "events",
        "rawMessage",
        "rawBytesHex",
      ].includes(key) && value !== null && value !== undefined && value !== "")
      .map(([key, value]) => ({ label: humanizeIdentifier(key), value }));
    if (extraObservedRows.length) {
      host.append(keyValueList(extraObservedRows));
    }
    if (Array.isArray(record.observedEvidence.events) && record.observedEvidence.events.length) {
      host.append(renderSectionTitle("Observed Reject Events"));
      host.append(renderTimeline(record.observedEvidence.events));
    }
  }
  if (record.error) {
    host.append(renderSectionTitle("Error"));
    host.append(node("pre", "raw-block", record.error));
  }
  if (record.rawBytesHex) {
    host.append(rawJsonDetails("Raw Packet Hex", { rawBytesHex: record.rawBytesHex }, false, `packet-lab:${record.timestamp || record.case}:raw`));
  }
  return host;
}

function renderGenericObject(value) {
  const rows = Object.entries(value || {}).map(([key, item]) => {
    if (key === "rawMessage" || key === "rawBytesHex") {
      return null;
    }
    if (typeof item === "object" && item !== null && !Array.isArray(item)) {
      return { label: key, value: pretty(item), fullWidth: true };
    }
    return { label: key, value: item };
  }).filter(Boolean);
  if (!rows.length) {
    return node("p", "muted", "No structured fields.");
  }
  return keyValueList(rows);
}

function renderJobEnvelope(payload, sink) {
  clearChildren(sink);
  const host = node("div", "result-stack");
  host.append(keyValueList([
    { label: "Job", value: payload.jobId || "Unavailable" },
    { label: "State", value: payload.status || "Unavailable" },
  ]));
  if (payload.result) {
    if (payload.result.observedEvidence || payload.result.packetSummary) {
      host.append(renderSectionTitle("Result"));
      host.append(renderPacketLabRecord(payload.result));
    } else {
      host.append(renderSectionTitle("Result"));
      host.append(renderOperationResult(payload.result));
    }
  }
  if (payload.error) {
    host.append(renderSectionTitle("Error"));
    host.append(node("pre", "raw-block", payload.error));
  }
  sink.append(host);
}

async function waitForJob(jobId, sink) {
  for (;;) {
    const payload = await apiGet(`/api/jobs/${jobId}`);
    renderJobEnvelope(payload, sink);
    if (payload.status === "finished" || payload.status === "failed" || payload.status === "missing") {
      return payload;
    }
    await new Promise((resolve) => setTimeout(resolve, 1000));
  }
}

async function initDashboard() {
  await refreshSelectors();
  const contextSelect = document.getElementById("context-select");
  const bandSelect = document.getElementById("band-select");
  const clearEventsButton = document.getElementById("events-clear-current");
  const clearCacheButton = document.getElementById("cache-clear-current");
  const refresh = async () => {
    const payload = await apiGet(`/api/dashboard?contextId=${selectedContext()}&band=${selectedBand()}`);
    renderDashboardCards(payload.cards || {});
    renderRecentEvents(payload.recentEvents || []);
    if (clearEventsButton) {
      clearEventsButton.disabled = !hasSelectedBand();
    }
    if (clearCacheButton) {
      clearCacheButton.disabled = !selectedContext();
    }
  };
  contextSelect.onchange = async () => {
    contextSelect._renderBands?.();
    persistSelectorState(selectedContext(), selectedBand());
    await refresh();
  };
  bandSelect.onchange = async () => {
    persistSelectorState(selectedContext(), selectedBand());
    await refresh();
  };
  clearEventsButton?.addEventListener("click", async () => {
    if (!hasSelectedBand()) {
      return;
    }
    clearEventsButton.disabled = true;
    try {
      await apiPost("/api/events/clear", {
        contextId: selectedContext(),
        band: selectedBand(),
      });
      await refresh();
    } finally {
      clearEventsButton.disabled = !hasSelectedBand();
    }
  });
  clearCacheButton?.addEventListener("click", async () => {
    if (!selectedContext()) {
      return;
    }
    clearCacheButton.disabled = true;
    try {
      await apiPost("/api/cache/clear", {
        contextId: selectedContext(),
      });
      await refresh();
    } finally {
      clearCacheButton.disabled = !selectedContext();
    }
  });
  await refresh();
  setInterval(refresh, 2000);
}

async function initBeacon() {
  await refreshSelectors();
  const contextSelect = document.getElementById("context-select");
  const bandSelect = document.getElementById("band-select");
  const refresh = async () => {
    const latest = await apiGet(`/api/beacon/latest?contextId=${selectedContext()}&band=${selectedBand()}`);
    const history = await apiGet(`/api/beacon/history?contextId=${selectedContext()}&band=${selectedBand()}&limit=20`);
    renderBeaconLatest(latest);
    renderBeaconHistory(history);
  };
  contextSelect.onchange = async () => {
    contextSelect._renderBands?.();
    persistSelectorState(selectedContext(), selectedBand());
    await refresh();
  };
  bandSelect.onchange = async () => {
    persistSelectorState(selectedContext(), selectedBand());
    await refresh();
  };
  await refresh();
  setInterval(refresh, 2000);
}

function commandSearchIndex(command) {
  const parts = [
    command.name,
    command.label,
    command.group,
    ...(command.args || []).flatMap((arg) => [arg.name, arg.typeName]),
  ];
  return parts.filter(Boolean).join(" ").toLowerCase();
}

function commandSecondarySearchIndex(command) {
  const parts = [
    command.description,
    ...(command.args || []).map((arg) => arg.annotation),
  ];
  return parts.filter(Boolean).join(" ").toLowerCase();
}

function catalogSourceLabel(source) {
  if (source === "curated") {
    return "Curated overlay + dictionary";
  }
  if (source === "dictionary") {
    return "Dictionary only";
  }
  return source || "Unavailable";
}

function catalogSourceHelp(source) {
  if (source === "curated") {
    return "Uses repo-owned grouping, labels, and field hints on top of the active F' dictionary.";
  }
  if (source === "dictionary") {
    return "Rendered directly from the active F' dictionary because no repo-specific overlay is defined yet.";
  }
  return null;
}

function renderCommandSummary(command) {
  const host = node("div", "result-stack");
  const readbackMode = isReadbackCommand(command.name)
    ? "Saved to Readback viewer"
    : "Structured result only";
  host.append(keyValueList([
    { label: "Command", value: command.name },
    { label: "Label", value: command.label },
    { label: "Group", value: command.group },
    { label: "Command Kind", value: command.commandKind },
    { label: "Opcode", value: `0x${Number(command.opcode).toString(16).toUpperCase().padStart(8, "0")}` },
    { label: "Catalog Source", value: catalogSourceLabel(command.source) },
    { label: "Result Routing", value: readbackMode },
  ]));
  host.append(node("p", "muted", command.description || "No description."));
  const sourceHelp = catalogSourceHelp(command.source);
  if (sourceHelp) {
    host.append(node("p", "muted", sourceHelp));
  }
  return host;
}

function isReadbackCommand(commandName) {
  const readbackCommands = window.MISSION_CONSOLE_BOOT.readbackCommands || {};
  return (readbackCommands.channelRefresh || []).includes(commandName)
    || (readbackCommands.eventBased || []).includes(commandName);
}

async function submitOpsCommand(commandName, commandArgs, ensureAuth, sink) {
  const payload = {
    contextId: selectedContext(),
    band: selectedBand(),
    commandName,
    commandArgs,
    ensureAuth,
  };
  const route = isReadbackCommand(commandName) ? "/api/readback/run" : "/api/commands/send";
  const queued = await apiPost(route, payload);
  await waitForJob(queued.jobId, sink);
}

function createInputForArg(arg) {
  const wrapper = node("label", "field-label");
  const labelText = `${arg.name}${arg.required ? "" : " (optional)"}`;
  wrapper.append(node("span", "field-title", labelText));
  let input;
  if (arg.inputKind === "enum" || arg.inputKind === "bool") {
    input = node("select");
    const placeholder = node("option", "", arg.placeholder || `Select ${arg.name}`);
    placeholder.value = "";
    placeholder.disabled = true;
    placeholder.selected = !arg.defaultValue;
    input.appendChild(placeholder);
    (arg.options || []).forEach((option) => {
      const element = node("option", "", option.label || option.value);
      element.value = option.value;
      if (arg.defaultValue !== null && arg.defaultValue !== undefined && String(arg.defaultValue) === String(option.value)) {
        element.selected = true;
      }
      input.appendChild(element);
    });
  } else {
    input = node("input");
    input.type = "text";
    input.placeholder = arg.placeholder || "";
    if (arg.defaultValue !== null && arg.defaultValue !== undefined) {
      input.value = String(arg.defaultValue);
    }
  }
  input.name = `arg:${arg.name}`;
  input.dataset.argName = arg.name;
  if (arg.required) {
    input.required = true;
  }
  wrapper.appendChild(input);
  if (arg.annotation) {
    wrapper.append(node("span", "field-hint", arg.annotation));
  }
  return wrapper;
}

function renderSelectedCommand(state) {
  const header = document.getElementById("selected-command-header");
  const summary = document.getElementById("selected-command-summary");
  const fields = document.getElementById("selected-command-fields");
  const hiddenName = document.getElementById("selected-command-name");
  const command = state.selectedCommand;
  clearChildren(header);
  clearChildren(summary);
  clearChildren(fields);
  hiddenName.value = command ? command.name : "";
  if (!command) {
    header.append(node("h3", "", "Select a command"), node("p", "muted", "Choose a command from the filtered catalog."));
    return;
  }
  header.append(node("h3", "", command.label), node("p", "muted", command.description || "No description."));
  header.append(node("div", "badge-row"));
  header.lastChild.append(
    badge(command.group, command.visibleByDefault ? "good" : "neutral"),
    badge(command.commandKind, "neutral"),
    badge(command.source === "curated" ? "overlay" : "dictionary", command.source === "curated" ? "good" : "neutral"),
  );
  summary.append(renderCommandSummary(command));
  if (!command.args.length) {
    fields.append(node("p", "muted", "This command has no formal parameters."));
    return;
  }
  command.args.forEach((arg) => fields.appendChild(createInputForArg(arg)));
}

function filteredCommands(state) {
  const catalog = state.catalog || { commands: [] };
  const showAll = document.getElementById("show-all-commands")?.checked;
  const query = document.getElementById("command-search")?.value?.trim().toLowerCase() || "";
  const groupFilter = state.groupFilter || "ALL";
  const scopedCommands = catalog.commands.filter((command) => {
    if (!showAll && !command.visibleByDefault) {
      return false;
    }
    if (groupFilter !== "ALL" && command.group !== groupFilter) {
      return false;
    }
    return true;
  });
  if (!query) {
    return scopedCommands;
  }
  const primaryMatches = scopedCommands.filter((command) => commandSearchIndex(command).includes(query));
  if (primaryMatches.length) {
    return primaryMatches;
  }
  return scopedCommands.filter((command) => commandSecondarySearchIndex(command).includes(query));
}

function renderCommandGroups(state) {
  const host = document.getElementById("command-group-chips");
  clearChildren(host);
  const showAll = document.getElementById("show-all-commands")?.checked;
  const groups = showAll
    ? [...window.MISSION_CONSOLE_BOOT.visibleCommandGroups, ...window.MISSION_CONSOLE_BOOT.hiddenCommandGroups]
    : [...window.MISSION_CONSOLE_BOOT.visibleCommandGroups];
  const makeChip = (group, label) => {
    const button = node("button", `chip${state.groupFilter === group ? " chip-active" : ""}`, label);
    button.type = "button";
    button.onclick = () => {
      state.groupFilter = group;
      renderCommandGroups(state);
      renderCommandCatalog(state);
    };
    return button;
  };
  host.appendChild(makeChip("ALL", "All groups"));
  groups.forEach((group) => host.appendChild(makeChip(group, group)));
}

function renderCommandCatalog(state) {
  const host = document.getElementById("command-results");
  const count = document.getElementById("command-count");
  clearChildren(host);
  const commands = filteredCommands(state);
  count.textContent = `${commands.length} command${commands.length === 1 ? "" : "s"}`;
  if (!commands.length) {
    host.append(node("p", "muted", "No commands match the current filter."));
    if (state.selectedCommand) {
      state.selectedCommand = null;
      renderSelectedCommand(state);
    }
    return;
  }
  const matchingSelectedCommand = state.selectedCommand
    ? commands.find((command) => command.name === state.selectedCommand.name)
    : null;
  if (matchingSelectedCommand && matchingSelectedCommand !== state.selectedCommand) {
    state.selectedCommand = matchingSelectedCommand;
    renderSelectedCommand(state);
  } else if (!matchingSelectedCommand) {
    state.selectedCommand = commands[0];
    renderSelectedCommand(state);
  }
  commands.forEach((command) => {
    const button = node("button", `command-row${state.selectedCommand?.name === command.name ? " command-row-active" : ""}`);
    button.type = "button";
    button.title = [command.name, command.description || "No description."].join("\n");
    const title = node("div", "command-row-title");
    title.append(node("strong", "", command.label), badge(command.group, command.visibleByDefault ? "good" : "neutral"));
    button.append(title);
    button.onclick = () => {
      state.selectedCommand = command;
      renderSelectedCommand(state);
      renderCommandCatalog(state);
    };
    host.appendChild(button);
  });
}

function serializeCommandArgs(command, form) {
  return (command.args || []).map((arg) => {
    const field = form.querySelector(`[data-arg-name="${arg.name}"]`);
    return field ? String(field.value) : "";
  });
}

async function refreshOpsAuthStatus() {
  const host = document.getElementById("auth-status");
  if (!host || !selectedContext()) {
    return;
  }
  if (!hasSelectedBand()) {
    setEmptyState(host, bandUnavailableMessage());
    return;
  }
  const payload = await apiGet(`/api/dashboard?contextId=${selectedContext()}&band=${selectedBand()}`);
  clearChildren(host);
  host.append(node("p", "muted", "Current secure session state for the selected context and band."));
  const secureCard = payload.cards?.["Secure Session"]?.secureSession || {};
  host.append(keyValueList([
    { label: "Active", value: secureCard.active ? "True" : "False" },
    { label: "Authority Mode", value: secureCard.authorityMode || "Unavailable" },
    { label: "Next Secure Sequence", value: secureCard.nextSecureSequence ?? "Unavailable" },
    { label: "Last Auth Time", value: secureCard.lastAuthTime ? formatTimestamp(secureCard.lastAuthTime * 1000) : "Unavailable" },
    { label: "Invalidation Reason", value: secureCard.invalidationReason || "Unavailable" },
  ]));
}

function updateSequenceFieldVisibility() {
  const form = document.getElementById("sequence-form");
  if (!form) {
    return;
  }
  const action = form.querySelector('select[name="action"]').value;
  const allowed = new Set(window.MISSION_CONSOLE_BOOT.sequenceActionFields[action] || []);
  form.querySelectorAll("[data-sequence-field]").forEach((field) => {
    field.hidden = !allowed.has(field.dataset.sequenceField);
    const input = field.querySelector("input");
    if (input) {
      input.required = allowed.has(field.dataset.sequenceField);
    }
  });
}

async function initOps() {
  await refreshSelectors();
  const contextSelect = document.getElementById("context-select");
  const bandSelect = document.getElementById("band-select");
  const searchInput = document.getElementById("command-search");
  const showAllInput = document.getElementById("show-all-commands");
  searchInput.value = "";
  showAllInput.checked = false;
  const state = {
    catalog: null,
    selectedCommand: null,
    groupFilter: "ALL",
  };
  const sink = document.getElementById("job-result");
  const loadCatalog = async () => {
    const payload = await apiGet(`/api/command-catalog?contextId=${selectedContext()}`);
    state.catalog = payload;
    renderCommandGroups(state);
    renderCommandCatalog(state);
  };
  const refreshAll = async () => {
    await loadCatalog();
    await refreshOpsAuthStatus();
    syncBandRequiredControls();
  };
  contextSelect.onchange = async () => {
    contextSelect._renderBands?.();
    persistSelectorState(selectedContext(), selectedBand());
    await refreshAll();
  };
  bandSelect.onchange = async () => {
    persistSelectorState(selectedContext(), selectedBand());
    syncBandRequiredControls();
    await refreshOpsAuthStatus();
  };
  searchInput.addEventListener("input", () => renderCommandCatalog(state));
  showAllInput.addEventListener("change", () => {
    renderCommandGroups(state);
    renderCommandCatalog(state);
  });
  document.getElementById("auth-form").onsubmit = async (event) => {
    event.preventDefault();
    if (!guardBandRequiredAction(sink, "Auth")) {
      return;
    }
    const queued = await apiPost("/api/auth/ensure", { contextId: selectedContext(), band: selectedBand() });
    await waitForJob(queued.jobId, sink);
    await refreshOpsAuthStatus();
  };
  document.getElementById("command-form").onsubmit = async (event) => {
    event.preventDefault();
    const command = state.selectedCommand;
    if (!command) {
      return;
    }
    if (!guardBandRequiredAction(sink, "Secure command dispatch")) {
      return;
    }
    const form = event.currentTarget;
    if (!form.reportValidity()) {
      return;
    }
    await submitOpsCommand(
      command.name,
      serializeCommandArgs(command, form),
      Boolean(form.querySelector('input[name="ensureAuth"]').checked),
      sink,
    );
    await refreshOpsAuthStatus();
  };
  document.getElementById("raw-command-form").onsubmit = async (event) => {
    event.preventDefault();
    if (!guardBandRequiredAction(sink, "Raw command dispatch")) {
      return;
    }
    const form = new FormData(event.currentTarget);
    await submitOpsCommand(
      String(form.get("commandName")),
      String(form.get("commandArgs") || "").trim().split(/\s+/).filter(Boolean),
      Boolean(form.get("ensureAuth")),
      sink,
    );
    await refreshOpsAuthStatus();
  };
  document.getElementById("file-form").onsubmit = async (event) => {
    event.preventDefault();
    if (!guardBandRequiredAction(sink, "Governed upload")) {
      return;
    }
    const form = new FormData(event.currentTarget);
    const queued = await apiPost("/api/files/upload", {
      contextId: selectedContext(),
      band: selectedBand(),
      localPath: form.get("localPath"),
      destinationLeaf: form.get("destinationLeaf"),
    });
    await waitForJob(queued.jobId, sink);
  };
  document.getElementById("sequence-form").onsubmit = async (event) => {
    event.preventDefault();
    if (!guardBandRequiredAction(sink, "Sequence action")) {
      return;
    }
    const form = new FormData(event.currentTarget);
    const action = form.get("action");
    const queued = await apiPost(`/api/sequences/${action}`, {
      contextId: selectedContext(),
      band: selectedBand(),
      sequencePath: form.get("sequencePath"),
      sequenceContextId: form.get("sequenceContextId"),
      runMode: form.get("runMode"),
    });
    await waitForJob(queued.jobId, sink);
  };
  document.querySelector('#sequence-form select[name="action"]').addEventListener("change", updateSequenceFieldVisibility);
  updateSequenceFieldVisibility();
  await refreshAll();
  setInterval(refreshOpsAuthStatus, 2000);
}

function renderReadbackTabs(state, payload) {
  const host = document.getElementById("readback-tabs");
  clearChildren(host);
  (payload.tabs || []).forEach((tab) => {
    const button = node("button", `chip${state.activeTab === tab.name ? " chip-active" : ""}`, tab.name);
    button.type = "button";
    button.onclick = () => {
      state.activeTab = tab.name;
      renderReadbackTabs(state, payload);
      renderReadbackViewer(state, payload);
    };
    host.append(button);
  });
}

function renderReadbackViewer(state, payload) {
  const host = document.getElementById("readback-result");
  persistVisibleDetailsState(host);
  clearChildren(host);
  state.payload = payload;
  const active = (payload.tabs || []).find((tab) => tab.name === state.activeTab) || payload.tabs?.[0];
  if (!active) {
    host.append(node("p", "empty-state", "No saved readback tabs are available yet."));
    return;
  }
  const grid = node("div", "readback-card-stack");
  (active.cards || []).forEach((card) => grid.append(renderReadbackViewerCard(card, state)));
  host.append(grid);
}

async function initReadback() {
  await refreshSelectors();
  const contextSelect = document.getElementById("context-select");
  const bandSelect = document.getElementById("band-select");
  const state = { activeTab: "OBC", quickRefresh: {}, payload: null, refresh: async () => {} };
  const refresh = async () => {
    if (!hasSelectedBand()) {
      setEmptyState(document.getElementById("readback-result"), bandUnavailableMessage());
      state.payload = null;
      return;
    }
    const payload = await apiGet(`/api/readback/viewer?contextId=${selectedContext()}&band=${selectedBand()}`);
    if (!(payload.tabs || []).some((tab) => tab.name === state.activeTab)) {
      state.activeTab = payload.tabs?.[0]?.name || "OBC";
    }
    renderReadbackTabs(state, payload);
    renderReadbackViewer(state, payload);
  };
  state.refresh = refresh;
  contextSelect.onchange = async () => {
    contextSelect._renderBands?.();
    persistSelectorState(selectedContext(), selectedBand());
    await refresh();
  };
  bandSelect.onchange = async () => {
    persistSelectorState(selectedContext(), selectedBand());
    await refresh();
  };
  await refresh();
  setInterval(refresh, 2000);
}

function trendSeriesColor(channelName) {
  return TREND_SERIES_COLORS[channelName] || "#0d6a66";
}

function numericTrendValue(sample) {
  const parsed = Number(sample?.value);
  return Number.isFinite(parsed) ? parsed : null;
}

function formatTrendNumber(value) {
  return formatDisplayNumber(value) ?? String(value ?? "Unavailable");
}

function niceTickStep(span, targetTicks = 4) {
  if (!Number.isFinite(span) || span <= 0) {
    return 1;
  }
  const rough = span / Math.max(1, targetTicks);
  const power = 10 ** Math.floor(Math.log10(rough));
  const normalized = rough / power;
  if (normalized <= 1) {
    return power;
  }
  if (normalized <= 2) {
    return 2 * power;
  }
  if (normalized <= 5) {
    return 5 * power;
  }
  return 10 * power;
}

function niceAxisBounds(minValue, maxValue) {
  if (!Number.isFinite(minValue) || !Number.isFinite(maxValue)) {
    return { min: 0, max: 1, step: 0.25 };
  }
  if (minValue === maxValue) {
    const pad = Math.abs(minValue || 1) * 0.08 || 1;
    minValue -= pad;
    maxValue += pad;
  }
  const span = maxValue - minValue;
  const padding = span * 0.12;
  const paddedMin = minValue - padding;
  const paddedMax = maxValue + padding;
  const step = niceTickStep(paddedMax - paddedMin);
  const axisMin = Math.floor(paddedMin / step) * step;
  const axisMax = Math.ceil(paddedMax / step) * step;
  return { min: axisMin, max: axisMax, step };
}

function drawTrendChart(canvas, seriesList) {
  const context = canvas.getContext("2d");
  const width = canvas.clientWidth || 900;
  const logicalHeight = Number(canvas.dataset.logicalHeight || canvas.getAttribute("height") || 320);
  const devicePixelRatio = window.devicePixelRatio || 1;
  canvas.dataset.logicalHeight = String(logicalHeight);
  canvas.width = Math.max(1, Math.round(width * devicePixelRatio));
  canvas.height = Math.max(1, Math.round(logicalHeight * devicePixelRatio));
  context.setTransform(devicePixelRatio, 0, 0, devicePixelRatio, 0, 0);
  const height = logicalHeight;
  context.clearRect(0, 0, width, height);
  context.fillStyle = "#fffdf8";
  context.fillRect(0, 0, width, height);
  const margin = { top: 16, right: 18, bottom: 28, left: 54 };
  const plotWidth = width - margin.left - margin.right;
  const plotHeight = height - margin.top - margin.bottom;
  const points = seriesList.flatMap((series) => series.samples.map((sample) => ({ x: sample.time, y: numericTrendValue(sample) }))).filter((point) => point.y !== null);
  if (!points.length) {
    context.fillStyle = "#5f6c73";
    context.font = '14px "Avenir Next", sans-serif';
    context.fillText("Select one or more channels with numeric history.", margin.left, margin.top + 20);
    return;
  }
  const minX = Math.min(...points.map((point) => point.x));
  const maxX = Math.max(...points.map((point) => point.x));
  const rawMinY = Math.min(...points.map((point) => Number(point.y)));
  const rawMaxY = Math.max(...points.map((point) => Number(point.y)));
  const bounds = niceAxisBounds(rawMinY, rawMaxY);
  const xFor = (value) => margin.left + ((value - minX) / Math.max(1, maxX - minX)) * plotWidth;
  const yFor = (value) => margin.top + plotHeight - ((value - bounds.min) / Math.max(1e-9, bounds.max - bounds.min)) * plotHeight;
  context.strokeStyle = "rgba(22, 34, 41, 0.12)";
  context.lineWidth = 1;
  const tickValues = [];
  for (let value = bounds.min; value <= bounds.max + bounds.step * 0.5; value += bounds.step) {
    tickValues.push(Number(value.toFixed(6)));
  }
  tickValues.forEach((value) => {
    const y = yFor(value);
    context.beginPath();
    context.moveTo(margin.left, y);
    context.lineTo(width - margin.right, y);
    context.stroke();
  });
  context.fillStyle = "#5f6c73";
  context.font = '12px "Avenir Next", sans-serif';
  tickValues.forEach((value) => {
    const y = yFor(value);
    context.fillText(formatTrendNumber(value), 8, y + 4);
  });
  context.fillText(formatTimestamp(minX), margin.left, height - 8);
  const endLabel = formatTimestamp(maxX);
  const endWidth = context.measureText(endLabel).width;
  context.fillText(endLabel, width - margin.right - endWidth, height - 8);
  seriesList.forEach((series) => {
    const usableSamples = series.samples
      .map((sample) => ({ x: sample.time, y: numericTrendValue(sample) }))
      .filter((sample) => sample.y !== null);
    if (!usableSamples.length) {
      return;
    }
    context.strokeStyle = series.color;
    context.lineWidth = 2.4;
    context.beginPath();
    usableSamples.forEach((sample, index) => {
      const x = xFor(sample.x);
      const y = yFor(Number(sample.y));
      if (index === 0) {
        context.moveTo(x, y);
      } else {
        context.lineTo(x, y);
      }
    });
    context.stroke();
    const latest = usableSamples[usableSamples.length - 1];
    if (latest) {
      const x = xFor(latest.x);
      const y = yFor(Number(latest.y));
      context.fillStyle = series.color;
      context.beginPath();
      context.arc(x, y, 3.6, 0, Math.PI * 2);
      context.fill();
      context.strokeStyle = "rgba(255, 255, 255, 0.95)";
      context.lineWidth = 1.2;
      context.stroke();
    }
  });
}

function createTrendPanel(groupName, selectedChannels) {
  return {
    id: `trend-panel-${Date.now()}-${Math.random().toString(16).slice(2, 8)}`,
    activeGroup: groupName,
    selectedChannels: new Set(selectedChannels || []),
    allowCrossGroupOverlay: false,
  };
}

function nextTrendPanelTemplate(existingPanels) {
  const usedGroups = new Set(existingPanels.map((panel) => panel.activeGroup));
  if (!usedGroups.has("Health")) {
    return createTrendPanel("Health", ["SYS_MEM_RSS_MB"]);
  }
  if (!usedGroups.has("EPS")) {
    return createTrendPanel("EPS", ["EPS_VBAT"]);
  }
  return createTrendPanel("ADCS", ["ADCS_OMEGA_X"]);
}

function ensureTrendPanelState(panel, catalog) {
  const groups = catalog.groups || [];
  if (!groups.length) {
    panel.activeGroup = "EPS";
    panel.selectedChannels = new Set();
    panel.allowCrossGroupOverlay = false;
    return;
  }
  if (!groups.some((group) => group.name === panel.activeGroup)) {
    panel.activeGroup = groups[0].name;
  }
  const validChannelNames = new Set(groups.flatMap((group) => (group.channels || []).map((channel) => channel.name)));
  panel.selectedChannels = new Set([...panel.selectedChannels].filter((channelName) => validChannelNames.has(channelName)));
}

function buildTrendSeries(channelName, historyMap) {
  const samples = (historyMap[channelName] || []).map((sample) => ({
    ...sample,
    time: Date.parse(sample.gatewayObservedAt || sample.flightTimestamp || "") || Date.now(),
  }));
  const latest = samples[samples.length - 1];
  return {
    channelName,
    color: trendSeriesColor(channelName),
    samples,
    latestLabel: latest ? formatTrendNumber(latest.value) : "Unavailable",
  };
}

function renderTrendPanel(state, catalog, historyMap, panel, index) {
  const section = node("section", "panel trend-panel");
  const header = node("div", "panel-head");
  const title = node("div");
  title.append(
    node("h2", "", `Trend Panel ${index + 1}`),
    node("p", "muted", "Each panel fits its own Y-axis to the selected series only."),
  );
  const headerActions = node("div", "button-row");
  const removeButton = node("button", "secondary-button", "Remove");
  removeButton.type = "button";
  removeButton.disabled = state.panels.length === 1;
  removeButton.onclick = () => {
    state.panels = state.panels.filter((candidate) => candidate.id !== panel.id);
    state.refresh();
  };
  headerActions.append(removeButton);
  header.append(title, headerActions);
  section.append(header);

  const groupsRow = node("div", "trend-group-row");
  const chipHost = node("div", "chip-list");
  (catalog.groups || []).forEach((group) => {
    const button = node("button", `chip${panel.activeGroup === group.name ? " chip-active" : ""}`, group.name);
    button.type = "button";
    button.onclick = () => {
      if (!panel.allowCrossGroupOverlay && panel.activeGroup !== group.name) {
        const groupNames = new Set((group.channels || []).map((channel) => channel.name));
        panel.selectedChannels = new Set(
          [...panel.selectedChannels].filter((channelName) => groupNames.has(channelName))
        );
      }
      panel.activeGroup = group.name;
      state.refresh();
    };
    chipHost.append(button);
  });
  groupsRow.append(chipHost);
  const overlayToggle = node("label", "checkbox-inline trend-overlay-toggle");
  const overlayInput = node("input");
  overlayInput.type = "checkbox";
  overlayInput.checked = panel.allowCrossGroupOverlay;
  overlayInput.onchange = () => {
    panel.allowCrossGroupOverlay = overlayInput.checked;
    if (!panel.allowCrossGroupOverlay) {
      const activeGroup = (catalog.groups || []).find((entry) => entry.name === panel.activeGroup);
      const allowedNames = new Set((activeGroup?.channels || []).map((channel) => channel.name));
      panel.selectedChannels = new Set(
        [...panel.selectedChannels].filter((channelName) => allowedNames.has(channelName))
      );
    }
    state.refresh();
  };
  overlayToggle.append(overlayInput, node("span", "", "Keep cross-group overlay"));
  groupsRow.append(overlayToggle);
  section.append(groupsRow);

  const toggleGrid = node("div", "toggle-grid");
  const activeGroup = (catalog.groups || []).find((entry) => entry.name === panel.activeGroup) || catalog.groups?.[0];
  (activeGroup?.channels || []).forEach((channel) => {
    const label = node("label", "toggle-pill");
    const input = node("input");
    input.type = "checkbox";
    input.checked = panel.selectedChannels.has(channel.name);
    input.onchange = () => {
      if (input.checked) {
        panel.selectedChannels.add(channel.name);
      } else {
        panel.selectedChannels.delete(channel.name);
      }
      state.refresh();
    };
    label.append(input, node("span", "", displayFieldLabel(channel.name)));
    toggleGrid.append(label);
  });
  section.append(toggleGrid);

  const selectedSeries = [...panel.selectedChannels].map((channelName) => buildTrendSeries(channelName, historyMap));
  const legend = node("div", "trend-panel-legend");
  if (!selectedSeries.length) {
    legend.append(node("p", "legend-help", "Select one or more channels to populate this panel."));
  } else {
    selectedSeries.forEach((series) => {
      const row = node("div", "legend-row");
      const left = node("div", "legend-main");
      const swatch = node("span", "legend-swatch");
      swatch.style.background = series.color;
      left.append(
        swatch,
        node("strong", "", displayFieldLabel(series.channelName)),
        node("span", "legend-value", series.latestLabel),
      );
      row.append(left);
      legend.append(row);
    });
  }
  section.append(legend);

  const chartShell = node("div", "chart-shell");
  const canvas = node("canvas");
  canvas.height = 360;
  chartShell.append(canvas);
  section.append(chartShell);

  return {
    element: section,
    draw: () => drawTrendChart(canvas, selectedSeries),
  };
}

async function initTrends() {
  await refreshSelectors();
  const contextSelect = document.getElementById("context-select");
  const bandSelect = document.getElementById("band-select");
  const panelsHost = document.getElementById("trend-panels");
  const addPanelButton = document.getElementById("trend-add-panel");
  const helpHost = document.getElementById("trend-help-summary");
  const state = {
    panels: [
      createTrendPanel("EPS", ["EPS_VBAT", "EPS_IBAT"]),
      createTrendPanel("ADCS", ["ADCS_OMEGA_X", "ADCS_OMEGA_Y", "ADCS_OMEGA_Z"]),
    ],
    catalog: null,
    refresh: async () => {},
  };
  const refresh = async () => {
    if (!hasSelectedBand()) {
      setEmptyState(panelsHost, bandUnavailableMessage());
      return;
    }
    const catalog = await apiGet(`/api/trends/catalog?contextId=${selectedContext()}&band=${selectedBand()}`);
    state.catalog = catalog;
    state.panels.forEach((panel) => ensureTrendPanelState(panel, catalog));
    const requestedChannels = [...new Set(state.panels.flatMap((panel) => [...panel.selectedChannels]))];
    const historyPayload = requestedChannels.length
      ? await apiGet(`/api/trends/history?contextId=${selectedContext()}&band=${selectedBand()}&channels=${requestedChannels.join(",")}`)
      : { history: {} };
    clearChildren(panelsHost);
    if (!state.panels.length) {
      panelsHost.append(node("p", "empty-state", "No trend panels are configured."));
    }
    const renderJobs = state.panels.map((panel, index) => renderTrendPanel(state, catalog, historyPayload.history || {}, panel, index));
    renderJobs.forEach((job) => panelsHost.append(job.element));
    renderJobs.forEach((job) => job.draw());
    if (addPanelButton) {
      addPanelButton.disabled = state.panels.length >= MAX_TREND_PANELS;
    }
    if (helpHost) {
      clearChildren(helpHost);
      helpHost.append(
        node(
          "p",
          "muted",
          `History is bounded to ${catalog.historySampleCap || "the configured"} samples per context, band, and channel. Add a separate panel when you want a different axis scale instead of mixing unrelated ranges.`,
        ),
      );
    }
  };
  state.refresh = refresh;
  if (addPanelButton) {
    addPanelButton.onclick = async () => {
      if (state.panels.length >= MAX_TREND_PANELS) {
        return;
      }
      state.panels.push(nextTrendPanelTemplate(state.panels));
      await state.refresh();
    };
  }
  contextSelect.onchange = async () => {
    contextSelect._renderBands?.();
    persistSelectorState(selectedContext(), selectedBand());
    await refresh();
  };
  bandSelect.onchange = async () => {
    persistSelectorState(selectedContext(), selectedBand());
    await refresh();
  };
  await refresh();
  setInterval(refresh, 2000);
}

function createSequenceStepEditorRow(step, commandCatalog, onSync, onRerender, onDelete, onMoveUp, onMoveDown) {
  const row = node("div", "sequence-step");
  const controls = node("div", "sequence-step-controls");
  const offsetField = node("label", "field-label");
  offsetField.append(node("span", "field-title", "Offset"));
  const offsetInput = node("input");
  offsetInput.value = step.offset || "R00:00:00";
  offsetInput.oninput = () => {
    step.offset = offsetInput.value;
    onSync();
  };
  offsetField.append(offsetInput);
  const commandField = node("label", "field-label");
  commandField.append(node("span", "field-title", "Command"));
  const commandSelect = node("select");
  (commandCatalog.commands || []).filter((entry) => entry.visibleByDefault).forEach((command) => {
    const option = node("option", "", command.label);
    option.value = command.name;
    if (command.name === step.commandName) {
      option.selected = true;
    }
    commandSelect.append(option);
  });
  commandSelect.onchange = () => {
    step.commandName = commandSelect.value;
    step.commandArgs = [];
    onRerender();
  };
  commandField.append(commandSelect);
  controls.append(offsetField, commandField);
  const buttonRow = node("div", "button-row button-row-compact");
  const up = node("button", "", "Up");
  up.type = "button";
  up.onclick = onMoveUp;
  const down = node("button", "", "Down");
  down.type = "button";
  down.onclick = onMoveDown;
  const remove = node("button", "", "Delete");
  remove.type = "button";
  remove.onclick = onDelete;
  buttonRow.append(up, down, remove);
  controls.append(buttonRow);
  row.append(controls);
  const argsHost = node("div", "field-grid");
  const command = (commandCatalog.commands || []).find((entry) => entry.name === step.commandName);
  (command?.args || []).forEach((arg, index) => {
    const wrapper = createInputForArg(arg);
    const input = wrapper.querySelector("[data-arg-name]");
    input.value = step.commandArgs?.[index] ?? input.value ?? "";
    input.oninput = () => {
      const values = (command.args || []).map((entry) => {
        const field = row.querySelector(`[data-arg-name="${entry.name}"]`);
        return field ? String(field.value) : "";
      });
      step.commandArgs = values;
      onSync();
    };
    argsHost.append(wrapper);
  });
  if (!command?.args?.length) {
    argsHost.append(node("p", "muted", "This step currently has no formal parameters."));
  }
  row.append(argsHost);
  return row;
}

async function initSequences() {
  await refreshSelectors();
  const contextSelect = document.getElementById("context-select");
  const bandSelect = document.getElementById("band-select");
  const stepsHost = document.getElementById("sequence-steps");
  const sourceArea = document.getElementById("sequence-source");
  const titleInput = document.getElementById("sequence-title");
  const draftsHost = document.getElementById("sequence-drafts-list");
  const compileResultHost = document.getElementById("sequence-compile-result");
  const actionResultHost = document.getElementById("sequence-action-result");
  const state = {
    draftId: null,
    steps: [{ offset: "R00:00:00", commandName: "OBCApp.modeManager.MODE_GET", commandArgs: [] }],
    catalog: null,
    compiledPath: null,
    compiledLeaf: null,
    uploadedSequencePath: null,
  };
  const loadCatalog = async () => {
    state.catalog = await apiGet(`/api/command-catalog?contextId=${selectedContext()}`);
  };
  const syncSource = () => {
    sourceArea.value = state.steps.map((step) => {
      const args = (step.commandArgs || []).map((value) => {
        const text = String(value || "");
        return /\s/.test(text) ? JSON.stringify(text) : text;
      });
      return [step.offset || "R00:00:00", step.commandName || "", ...args].filter(Boolean).join(" ");
    }).join("\n");
    if (sourceArea.value) {
      sourceArea.value = `; Mission Console generated sequence\n${sourceArea.value}\n`;
    }
  };
  const renderDrafts = async () => {
    const payload = await apiGet(`/api/sequences/drafts?contextId=${selectedContext()}`);
    clearChildren(draftsHost);
    if (!(payload.drafts || []).length) {
      draftsHost.append(node("p", "empty-state", "No saved drafts yet."));
      return;
    }
    payload.drafts.forEach((draft) => {
      const card = node("article", "mini-card");
      card.append(node("strong", "", draft.title || draft.draftId));
      card.append(node("p", "muted", draft.updatedAt ? formatTimestamp(draft.updatedAt) : "Unavailable"));
      card.onclick = () => {
        state.draftId = draft.draftId;
        state.steps = draft.steps || state.steps;
        titleInput.value = draft.title || "";
        sourceArea.value = draft.rawSource || "";
        renderSteps();
      };
      draftsHost.append(card);
    });
  };
  const renderSteps = () => {
    clearChildren(stepsHost);
    state.steps.forEach((step, index) => {
      stepsHost.append(createSequenceStepEditorRow(
        step,
        state.catalog || { commands: [] },
        () => {
          syncSource();
        },
        () => {
          syncSource();
          renderSteps();
        },
        () => {
          state.steps.splice(index, 1);
          if (!state.steps.length) {
            state.steps.push({ offset: "R00:00:00", commandName: "OBCApp.modeManager.MODE_GET", commandArgs: [] });
          }
          syncSource();
          renderSteps();
        },
        () => {
          if (index === 0) {
            return;
          }
          [state.steps[index - 1], state.steps[index]] = [state.steps[index], state.steps[index - 1]];
          syncSource();
          renderSteps();
        },
        () => {
          if (index >= state.steps.length - 1) {
            return;
          }
          [state.steps[index + 1], state.steps[index]] = [state.steps[index], state.steps[index + 1]];
          syncSource();
          renderSteps();
        },
      ));
    });
  };
  const refreshAll = async () => {
    await loadCatalog();
    syncSource();
    renderSteps();
    await renderDrafts();
  };
  document.getElementById("sequence-add-step").onclick = () => {
    state.steps.push({ offset: "R00:00:00", commandName: "OBCApp.modeManager.MODE_GET", commandArgs: [] });
    syncSource();
    renderSteps();
  };
  document.getElementById("sequence-save-draft").onclick = async () => {
    const payload = await apiPost("/api/sequences/drafts", {
      contextId: selectedContext(),
      draftId: state.draftId,
      title: titleInput.value,
      steps: state.steps,
      rawSource: sourceArea.value,
    });
    state.draftId = payload.draftId;
    await renderDrafts();
  };
  document.getElementById("sequence-compile").onclick = async () => {
    const payload = await apiPost("/api/sequences/compile", {
      contextId: selectedContext(),
      draftId: state.draftId,
      title: titleInput.value,
      steps: state.steps,
      rawSource: sourceArea.value,
    });
    state.draftId = payload.draftId;
    state.compiledPath = payload.compiledPath;
    state.compiledLeaf = payload.compiledPath ? payload.compiledPath.split("/").slice(-1)[0] : null;
    clearChildren(compileResultHost);
    compileResultHost.append(keyValueList([
      { label: "Source Valid", value: payload.sourceText ? "True" : "False" },
      { label: "Compile Success", value: payload.success ? "True" : "False" },
      { label: "Source Path", value: payload.sourcePath || "Unavailable", fullWidth: true },
      { label: "Compiled Path", value: payload.compiledPath || "Unavailable", fullWidth: true },
    ]));
    if (payload.diagnostics) {
      compileResultHost.append(rawJsonDetails("Compile Diagnostics", { diagnostics: payload.diagnostics }, true));
    }
    await renderDrafts();
  };
  document.getElementById("sequence-upload").onclick = async () => {
    if (!guardBandRequiredAction(actionResultHost, "Sequence upload") || !state.compiledPath || !state.compiledLeaf) {
      return;
    }
    const queued = await apiPost("/api/files/upload", {
      contextId: selectedContext(),
      band: selectedBand(),
      localPath: state.compiledPath,
      destinationLeaf: state.compiledLeaf,
    });
    const payload = await waitForJob(queued.jobId, actionResultHost);
    if (payload.status === "finished") {
      state.uploadedSequencePath = `.sequence-staging/${state.compiledLeaf}`;
    }
  };
  document.getElementById("sequence-validate").onclick = async () => {
    if (!guardBandRequiredAction(actionResultHost, "SEQ_VALIDATE") || !state.uploadedSequencePath) {
      return;
    }
    const queued = await apiPost("/api/sequences/validate", {
      contextId: selectedContext(),
      band: selectedBand(),
      sequencePath: state.uploadedSequencePath,
    });
    await waitForJob(queued.jobId, actionResultHost);
  };
  document.getElementById("sequence-run").onclick = async () => {
    if (!guardBandRequiredAction(actionResultHost, "SEQ_RUN") || !state.uploadedSequencePath) {
      return;
    }
    const queued = await apiPost("/api/sequences/run", {
      contextId: selectedContext(),
      band: selectedBand(),
      sequencePath: state.uploadedSequencePath,
      runMode: "NO_WAIT",
    });
    await waitForJob(queued.jobId, actionResultHost);
  };
  sourceArea.addEventListener("input", () => {});
  contextSelect.onchange = async () => {
    contextSelect._renderBands?.();
    persistSelectorState(selectedContext(), selectedBand());
    state.draftId = null;
    state.compiledPath = null;
    state.compiledLeaf = null;
    state.uploadedSequencePath = null;
    await refreshAll();
  };
  bandSelect.onchange = () => {
    persistSelectorState(selectedContext(), selectedBand());
    syncBandRequiredControls();
  };
  await refreshAll();
}
function renderPacketLabHistory(entries) {
  const host = document.getElementById("packet-lab-history");
  clearChildren(host);
  if (!entries.length) {
    host.append(node("p", "muted", "No packet-lab history for the current filter yet."));
    return;
  }
  const timeline = node("div", "timeline");
  entries.slice().reverse().forEach((entry) => {
    const item = node("article", "timeline-item");
    const head = node("div", "timeline-head");
    head.append(
      node("div", "timeline-time", formatTimestamp(entry.timestamp)),
      badge(entry.case || "unknown", toneForText(entry.status || entry.observedEvidence?.kind || "neutral")),
    );
    const body = node("div", "timeline-body");
    body.append(node("div", "timeline-message", entry.description || entry.error || "No description"));
    body.append(keyValueList([
      { label: "Context", value: entry.contextId || "Unavailable" },
      { label: "Band", value: entry.band || "Unavailable" },
      { label: "Expected Failure", value: entry.expectedFailureReason || "Unavailable" },
      { label: "Injected Fault", value: entry.faultExplanation?.faultKind || "Unavailable" },
      { label: "Observed Reject", value: entry.observedEvidence?.reasonName || entry.observedEvidence?.reasonValue || "Unavailable" },
      { label: "Rejection Kind", value: entry.observedEvidence?.kind || entry.status || "Unavailable" },
    ]));
    item.append(head, body);
    timeline.appendChild(item);
  });
  host.appendChild(timeline);
}

async function initPacketLab() {
  await refreshSelectors();
  const contextSelect = document.getElementById("context-select");
  const bandSelect = document.getElementById("band-select");
  const caseSelect = document.getElementById("packet-case-select");
  const caseCards = document.getElementById("packet-case-cards");
  const caseDetail = document.getElementById("packet-case-detail");
  const historyShowAllToggle = document.getElementById("packet-history-show-all");
  const clearCurrentButton = document.getElementById("packet-history-clear-current");
  const clearAllButton = document.getElementById("packet-history-clear-all");
  const cases = window.MISSION_CONSOLE_BOOT.packetLabCases || {};
  const availableCases = () => {
    if (selectedContext() === "target-manual-ground-dual-gds") {
      return ["replay-captured-raw", "duplicate-sequence", "tampered-mac"];
    }
    return Object.keys(cases);
  };
  const refreshCaseOptions = () => {
    const previouslySelected = caseSelect.value;
    clearChildren(caseSelect);
    availableCases().forEach((name) => {
      const option = node("option", "", name);
      option.value = name;
      if (name === previouslySelected) {
        option.selected = true;
      }
      caseSelect.appendChild(option);
    });
    if (!caseSelect.value) {
      caseSelect.value = availableCases()[0] || "";
    }
  };
  const renderCaseCards = () => {
    clearChildren(caseCards);
    availableCases().forEach((name) => {
      const description = cases[name];
      const card = node("article", `mini-card${caseSelect.value === name ? " mini-card-active" : ""}`);
      card.append(node("strong", "", name), node("p", "muted", description));
      card.onclick = () => {
        caseSelect.value = name;
        renderCaseCards();
        renderCaseDetail();
      };
      caseCards.appendChild(card);
    });
  };
  const renderCaseDetail = () => {
    clearChildren(caseDetail);
    const selected = caseSelect.value;
    caseDetail.append(keyValueList([
      { label: "Case", value: selected || "Unavailable" },
      { label: "Description", value: cases[selected] || "Unavailable" },
    ]));
  };
  caseSelect.onchange = () => {
    renderCaseCards();
    renderCaseDetail();
  };
  contextSelect.onchange = async () => {
    contextSelect._renderBands?.();
    persistSelectorState(selectedContext(), selectedBand());
    refreshCaseOptions();
    renderCaseCards();
    renderCaseDetail();
  };
  bandSelect.onchange = () => {
    persistSelectorState(selectedContext(), selectedBand());
    syncBandRequiredControls();
  };
  const refreshHistory = async () => {
    const payload = await apiGet(`/api/packet-lab/history?showAll=${historyShowAllToggle?.checked ? "1" : "0"}`);
    renderPacketLabHistory(payload.entries || []);
  };
  const resultHost = document.getElementById("packet-lab-result");
  document.getElementById("packet-lab-form").onsubmit = async (event) => {
    event.preventDefault();
    if (!guardBandRequiredAction(resultHost, "Packet-lab injection")) {
      return;
    }
    const queued = await apiPost("/api/packet-lab/inject", {
      contextId: selectedContext(),
      band: selectedBand(),
      case: caseSelect.value,
    });
    await waitForJob(queued.jobId, resultHost);
    await refreshHistory();
  };
  historyShowAllToggle?.addEventListener("change", refreshHistory);
  clearCurrentButton?.addEventListener("click", async () => {
    await apiPost("/api/packet-lab/history/clear", { clearAll: false });
    await refreshHistory();
  });
  clearAllButton?.addEventListener("click", async () => {
    await apiPost("/api/packet-lab/history/clear", { clearAll: true });
    await refreshHistory();
  });
  refreshCaseOptions();
  renderCaseCards();
  renderCaseDetail();
  await refreshHistory();
  syncBandRequiredControls();
  setInterval(refreshHistory, 2000);
}

function renderSurfaceContextCard(context) {
  const card = node("article", "card data-card");
  const head = node("div", "panel-head");
  const title = node("div");
  title.append(node("h2", "", context.contextId), node("p", "muted", context.surfaceType || context.envName || "Context"));
  const badges = node("div", "badge-row");
  badges.append(
    badge(context.lifecycleState || "missing", toneForText(context.lifecycleState || "missing")),
    badge(context.ownerAlive ? "owner-alive" : "owner-dead", context.ownerAlive ? "good" : "bad"),
  );
  head.append(title, badges);
  card.append(head);
  card.append(keyValueList([
    { label: "Root", value: context.root || "Unavailable" },
    { label: "Manifest", value: context.manifestPath || "Unavailable" },
    { label: "Status", value: context.statusPath || "Unavailable" },
    { label: "Dictionary", value: context.dictionaryPath || "Unavailable" },
  ]));
  const bandsHost = node("div", "subcard-grid");
  Object.values(context.bands || {}).forEach((band) => {
    const bandCard = node("section", "subcard");
    bandCard.append(node("h3", "", band.band));
    bandCard.append(keyValueList([
      { label: "Manifest Key", value: band.manifestKey || "Unavailable" },
      { label: "TTS Port", value: band.gdsTtsPort ?? "Unavailable" },
      { label: "GUI", value: band.guiUrl || "Unavailable" },
      { label: "Secure State", value: band.secureState?.active ? "Active" : (band.secureState ? "Invalidated" : "Missing") },
      { label: "Next Sequence", value: band.secureState?.nextSecureSequence ?? "Unavailable" },
      { label: "Last Auth", value: band.secureState?.lastAuthTime ? formatTimestamp(band.secureState.lastAuthTime * 1000) : "Unavailable" },
    ]));
    bandsHost.appendChild(bandCard);
  });
  card.append(bandsHost);
  return card;
}

function renderHistoryTable(entries) {
  const host = document.getElementById("history-table");
  clearChildren(host);
  if (!entries.length) {
    host.append(node("p", "muted", "No action history for the current filter yet."));
    return;
  }
  const table = node("table", "compact-table");
  const head = node("thead");
  const headRow = node("tr");
  ["Time", "Context", "Band", "Action", "Status", "Command", "Summary"].forEach((title) => headRow.append(node("th", "", title)));
  head.appendChild(headRow);
  const body = node("tbody");
  entries.slice().reverse().forEach((entry) => {
    const request = entry.request || {};
    const result = entry.result || {};
    const row = node("tr");
    const summary = result.error
      ? result.error.split("\n")[0]
      : (result.readback?.family || result.artifacts?.status || result.artifacts?.performed || "Completed");
    [
      formatTimestamp(entry.startedAt),
      request.contextId || "Unavailable",
      request.band || "Unavailable",
      request.kind || "Unavailable",
      result.status || "Unavailable",
      result.commandName || "Unavailable",
      summary,
    ].forEach((value) => row.append(node("td", "", value)));
    body.appendChild(row);
  });
  table.append(head, body);
  host.appendChild(table);
}

async function initSurfaces() {
  const contextsHost = document.getElementById("surfaces-contexts");
  const showAllToggle = document.getElementById("history-show-all");
  const clearCurrentButton = document.getElementById("history-clear-current");
  const clearAllButton = document.getElementById("history-clear-all");
  const refresh = async () => {
    const contextsPayload = await apiGet("/api/contexts");
    const historyPayload = await apiGet(`/api/history?showAll=${showAllToggle?.checked ? "1" : "0"}`);
    clearChildren(contextsHost);
    Object.values(contextsPayload.contexts || {}).forEach((context) => contextsHost.appendChild(renderSurfaceContextCard(context)));
    renderHistoryTable(historyPayload.entries || []);
  };
  showAllToggle?.addEventListener("change", refresh);
  clearCurrentButton?.addEventListener("click", async () => {
    await apiPost("/api/history/clear", { clearAll: false });
    await refresh();
  });
  clearAllButton?.addEventListener("click", async () => {
    await apiPost("/api/history/clear", { clearAll: true });
    await refresh();
  });
  await refresh();
  setInterval(refresh, 2000);
}

window.addEventListener("DOMContentLoaded", async () => {
  const page = document.body.dataset.page;
  if (page === "dashboard") {
    await initDashboard();
  }
  if (page === "beacon") {
    await initBeacon();
  }
  if (page === "ops") {
    await initOps();
  }
  if (page === "trends") {
    await initTrends();
  }
  if (page === "readback") {
    await initReadback();
  }
  if (page === "sequences") {
    await initSequences();
  }
  if (page === "surfaces") {
    await initSurfaces();
  }
  if (page === "packet-lab") {
    await initPacketLab();
  }
});

if (typeof module !== "undefined" && module.exports) {
  module.exports = {
    formatDisplayNumber,
    formatTrendNumber,
    renderFieldValue,
  };
}
