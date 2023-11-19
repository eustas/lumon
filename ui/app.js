const DEVICE_NAME_PREFIX = 'Lumo';
let valueToSend = 'b\x3F';

function makeUuid(short) {
  return '0000' + short + '-0000-1000-8000-00805f9b34fb';
}

const AUTOMATION_SERVICE = makeUuid('1815');
const STRING_CHARACTERISTIC = makeUuid('2a3d');
const DEVICE_REQUEST = {
  filters: [  // Devices matching ANY item will be shown
    {namePrefix: DEVICE_NAME_PREFIX}
  ],
  optionalServices: [AUTOMATION_SERVICE],
  // acceptAllDevices: true,
};

let luckyDevice = null;

function onDeviceSelected(device) {
  // TODO(eustas): save device object?
  console.log("Device ID: " + device.id);
  console.log("Device name: " + device.name);
  device.gatt.connect().then(onDeviceConnected);
}

const pairDevices = () => {
  navigator.bluetooth.requestDevice(DEVICE_REQUEST).then(onDeviceSelected);
}

const onQuickStart = (event) => {
  document.getElementById("controls").scrollIntoView({behavior: "smooth"});
  if (luckyDevice !== null) {
    // TODO(eustas): connect to lucky device
  } else {
    pairDevices();
  }
};

function onAvailableDevicesList(devices) {
  const numAvailable = devices.length;
  if (numAvailable) {
    // Fallback: some device is ready.
    luckyDevice = devices[0];
    // TODO(eustas): read from local storage
    let lastConnectedDeviceId = null;
    for (let i = 0; i < numAvailable; ++i) {
      const d = devices[i];
      console.log("Available device " + i + " ID: " + d.id);
      console.log("Available device " + i + " name: " + d.name);
      // Last connected is better choice, if available.
      if (d.id === lastConnectedDeviceId) {
        luckyDevice = d;
      }
    }
  } else {
    luckyDevice = null;
    console.log("No available devices found");
  }
}

const refreshAvailable = () => {
  navigator.bluetooth.getDevices().then(onAvailableDevicesList);
};

const onAvailabilityReport = (available) => {
  console.log("Bluetooth available: " + (available ? "yes" : "no"));
  if (!available) {
    // TODO(eustas): notify user
    return;
  }
  // Request already paired devices list.
  refreshAvailable();
};

const main = (event) => {
  const luckyButton = document.getElementById("lucky");
  luckyButton.addEventListener("click", onQuickStart);
  // Request Bluetooth status.
  navigator.bluetooth.getAvailability().then(onAvailabilityReport);
};

document.addEventListener("DOMContentLoaded", main);

/*function onValueRead(dataView) {
  console.log('onValueRead');
  console.log(dataView);
}

function onValueWritten(str) {
  console.log('onValueWritten');
}

function onCharacteristicsListed(characteristics) {
  for (let i = 0; i < characteristics.length; ++i) {
    const c = characteristics[i];
    console.log("Characteristic " + i + " UUID: " + c.uuid);
    console.log("Characteristic " + i + " cached value: " + c.value);

    // c.readValue().then(onValueRead);

    const n = valueToSend.length;
    const newValue = new ArrayBuffer(n);
    const view = new Uint8Array(newValue);
    for (let i = 0; i < n; ++i) {
      view[i] = valueToSend.charCodeAt(i);
    }
    c.writeValue(newValue).then(onValueWritten);
  }
}

function onServiceConnected(service) {
  console.log("Service UUID: " + service.uuid);
  service.getCharacteristics().then(onCharacteristicsListed);
}

function onServicesListed(services) {
  for (let i = 0; i < services.length; ++i) {
    const s = services[i];
    console.log("Service " + i + " UUID: " + s.uuid);
  }
}

function onDeviceConnected(server) {
  // TODO(eustas): save server object?
  console.log("Device connected");
  //server.getPrimaryServices().then(onServicesListed);
  server.getPrimaryService(AUTOMATION_SERVICE).then(onServiceConnected);
}
*/