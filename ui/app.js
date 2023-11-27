const DEVICE_NAME_PREFIX = 'Lumo';

let historyItems = [];
/** @type {BluetoothRemoteGATTCharacteristic?} */
let btSink = null;

const makeUuid = (short) => {
  return '0000' + short + '-0000-1000-8000-00805f9b34fb';
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

/*const pictograms = () => {
  const start = [0x1F947];
  const len = [64];
  const all = [];
  for (let k = 0; k < start.length; ++k) {
    for (let i = 0; i < len[k]; ++i) {
      all.push(String.fromCodePoint(start[k] + i));
    }
  }
  return all.join('');
};*/

const info = (text) => {
  const debug = document.getElementById("debug");
  const p = document.createElement("p");
  p.textContent = "INFO: " + text;
  p.classList.add("blue");
  debug.prepend(p);
};

const error = (text) => {
  const debug = document.getElementById("debug");
  const p = document.createElement("p");
  p.textContent = "ERROR: " + text;
  p.classList.add("red");
  debug.prepend(p);
}

const dumpError = (errorObj) => {
  error(errorObj);
}

const unescapeCommand = (/** @type{string} */ text) => {
  let state = 0;
  let len = 0;
  let hex = 0;
  const buffer = new Uint8Array(text.length);
  for (let i = 0; i < text.length; ++i) {
    let c = text.charAt(i);
    let ord = c.charCodeAt(0);
    switch (state) {
      case 0: // free state
        if (c === "\\") {
          state = 1;
        } else {
          buffer[len++] = ord;
        }
        break;

      case 1: // after backslash
        if (c === "\\") {
          state = 0;
          buffer[len++] = ord;
        } else if (c === "x") {
          state = 2;
        } else {
          error("Invalid escape sequence");
          return null;
        }
        break;

      case 2: // first hex digit
      case 3: // second hex digit
        let digit = 0;
        if (ord >= 48 && ord <= 57) {
          digit = ord - 48;
        } else if (ord >= 65 && ord <= 70) {
          digit = ord - 65 + 10;
        } else if (ord >= 97 && ord <= 102) {
          digit = ord - 97 + 10;
        } else {
          error("Invalid escape sequence");
          return null;
        }
        if (state === 2) {
          hex = digit;
          state = 3;
        } else {
          hex = hex * 16 + digit;
          buffer[len++] = hex;
          state = 0;
        }
        break;

      default:
        error("Unreachable");
        return null;
    }
  }
  return buffer.slice(0, len);
};

const deviceToString = (device) => {
  return device.name + " (" + device.id + ")";
};

const characteristicToString = (characteristic) => {
  return characteristic.uuid;
};

/*const onCharacteristicsListed = (characteristics) => {
  for (let i = 0; i < characteristics.length; ++i) {
    const c = characteristics[i];
    info("Characteristic " + i + ": " + characteristicToString(c));
  }
};*/

const onCharacteristicObtained = (characteristic) => {
  info("Obtained characteristic: " + characteristicToString(characteristic));
  btSink = characteristic;
};

const onServiceConnected = (service) => {
  info("Service UUID: " + service.uuid);
  service.getCharacteristic(STRING_CHARACTERISTIC).then(onCharacteristicObtained).catch(dumpError);
  //service.getCharacteristics().then(onCharacteristicsListed).catch(dumpError);
};

const onDeviceConnected = (server) => {
  // TODO(eustas): save server object?
  info("Connected to GATT server");
  server.getPrimaryService(AUTOMATION_SERVICE).then(onServiceConnected).catch(dumpError);
};

const onDisconnected = (event) => {
  info("Bluetooth device disconnected");
  document.getElementById("current").textContent = "N/A";
};

const onDeviceSelected = (device) => {
  // TODO(eustas): save device object?
  const s = deviceToString(device);
  info("Connecting to device: " + s);
  document.getElementById("current").textContent = s;
  device.addEventListener("gattserverdisconnected", onDisconnected);
  device.gatt.connect().then(onDeviceConnected).catch(dumpError);
};

const onAvailabilityReport = (available) => {
  info("Bluetooth available: " + (available ? "yes" : "no"));
};

const onPairClick = (event) => {
  navigator.bluetooth.requestDevice(DEVICE_REQUEST).then(onDeviceSelected).catch(dumpError);
};

const onValueWritten = (unknown) => {
  info("Command successfully sent");
};

const makeHistoryNode = (text) => {
  const item = document.createElement("a");
  item.href = "#";
  item.textContent = text;
  item.addEventListener("click", onHistoryItemClick);
  return item;
};

const onSendClick = (event) => {
  if (btSink === null) {
    error("Can't send: not connected");
    return;
  }
  const cmdInput = document.getElementById("cmd");
  const rawCmd = cmdInput.value;
  const binaryCmd = unescapeCommand(rawCmd);
  if (binaryCmd === null) {
    error("Can't send: invalid command");
    return;
  }

  // Remove item from history / list
  const existingIndex = historyItems.indexOf(rawCmd);
  if (existingIndex >= 0) {
    historyItems.splice(existingIndex, 1);
  }
  const history = document.getElementById("history");
  for (let node = history.firstElementChild; node;) {
    let current = node;
    node = node.nextElementSibling;
    if (current.textContent === rawCmd) {
      history.removeChild(current);
    }
  }
  // Add item at top.
  historyItems.splice(0,0, rawCmd);
  history.prepend(makeHistoryNode(rawCmd));
  // Deal with long tail.
  if (historyItems.length > 100) {
    historyItems.pop();
    history.removeChild(history.lastElementChild);
  }
  // Save
  localStorage.setItem("history", JSON.stringify(historyItems));

  btSink.writeValueWithoutResponse(binaryCmd).then(onValueWritten).catch(dumpError);
};

const onHistoryItemClick = (/** @type{MouseEvent} */ event) => {
  const text = event.target.textContent;
  const cmdInput = document.getElementById("cmd");
  cmdInput.value = text;
};

const main = (event) => {
  const pairButton = document.getElementById("pair");
  pairButton.addEventListener("click", onPairClick);
  const sendButton = document.getElementById("send");
  sendButton.addEventListener("click", onSendClick);
  const serializedHistory = localStorage.getItem("history");
  if (serializedHistory) {
    historyItems = JSON.parse(serializedHistory);
  } else {
    historyItems = ["b\\x00", "b\\x01", "b\\x02", "b\\x04", "b\\x08",
      "b\\x10", "b\\x20", "b\\x40", "b\\x80", "b\\xFF"
    ];
  }
  const history = document.getElementById("history");
  for (let i = 0; i < historyItems.length; ++i) {
    history.appendChild(makeHistoryNode(historyItems[i]));
  }
  // Request Bluetooth status.
  navigator.bluetooth.getAvailability().then(onAvailabilityReport).catch(dumpError);
};

document.addEventListener("DOMContentLoaded", main);
