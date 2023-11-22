const DEVICE_NAME_PREFIX = 'Lumo';
let valueToSend = 'b\x3F';

let historyItems = [];

const makeUuid = (short) => {
  return '0000' + short + '-0000-1000-8000-00805f9b34fb';
};

const info = (text) => {
  const debug = document.getElementById("debug");
  const p = document.createElement("p");
  p.textContent = "INFO: " + text;
  p.classList.add("blue");
  debug.appendChild(p);
};

const error = (text) => {
  const debug = document.getElementById("debug");
  const p = document.createElement("p");
  p.textContent = "ERROR: " + text;
  p.classList.add("red");
  debug.appendChild(p);
}

const dumpError = (errorObj) => {
  error(errorObj);
}

const unescapeCommand = (text) => {
  return text;
};

const AUTOMATION_SERVICE = makeUuid('1815');
const STRING_CHARACTERISTIC = makeUuid('2a3d');
const DEVICE_REQUEST = {
  filters: [  // Devices matching ANY item will be shown
    {namePrefix: DEVICE_NAME_PREFIX}
  ],
  optionalServices: [AUTOMATION_SERVICE],
  // acceptAllDevices: true,
};

const deviceToString = (device) => {
  return device.name + " (" + device.id + ")";
};

const characteristicToString = (characteristic) => {
  return characteristic.uuid;
};

function onCharacteristicsListed(characteristics) {
  for (let i = 0; i < characteristics.length; ++i) {
    const c = characteristics[i];
    info("Characteristic " + i + ":  " + characteristicToString(c));
  }
};

const onServiceConnected = (service) => {
  info("Service UUID: " + service.uuid);
  service.getCharacteristics().then(onCharacteristicsListed).catch(dumpError);
};

const onDeviceConnected = (server) => {
  // TODO(eustas): save server object?
  info("Connected to GATT server");
  server.getPrimaryService(AUTOMATION_SERVICE).then(onServiceConnected).catch(dumpError);
};

const onDeviceSelected = (device) => {
  // TODO(eustas): save device object?
  const s = deviceToString(device);
  info("Connecting to device: " + s);
  document.getElementById("current").textContent = s;
  device.gatt.connect().then(onDeviceConnected).catch(dumpError);
};

const onAvailabilityReport = (available) => {
  info("Bluetooth available: " + (available ? "yes" : "no"));
  if (!available) {
    // TODO(eustas): notify user
    return;
  }
};

const onPairClick = (event) => {
  navigator.bluetooth.requestDevice(DEVICE_REQUEST).then(onDeviceSelected).catch(dumpError);
};

const onHistoryItemClick = (/** @type{MouseEvent} */ event) => {
  const text = event.target.textContent;
  const cmdInput = document.getElementById("cmd");
  cmdInput.value = text;
};

const main = (event) => {
  const pairButton = document.getElementById("pair");
  pairButton.addEventListener("click", onPairClick);
  const serializedHistory = localStorage.getItem("history");
  if (serializedHistory) {
    historyItems = JSON.parse(serializedHistory);
  } else {
    historyItems = ["cat", "dog"];
  }
  const history = document.getElementById("history");
  for (let i = 0; i < historyItems.length; ++i) {
    const item = document.createElement("a");
    item.href = "#";
    item.textContent = historyItems[i];
    item.addEventListener("click", onHistoryItemClick);
    history.appendChild(item);
  }
  // Request Bluetooth status.
  navigator.bluetooth.getAvailability().then(onAvailabilityReport).catch(dumpError);
};

document.addEventListener("DOMContentLoaded", main);

/*
    const n = valueToSend.length;
    const newValue = new ArrayBuffer(n);
    const view = new Uint8Array(newValue);
    for (let i = 0; i < n; ++i) {
      view[i] = valueToSend.charCodeAt(i);
    }
    c.writeValue(newValue).then(onValueWritten);
*/