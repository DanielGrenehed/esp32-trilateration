import * as dev from './device.js';
import * as bf from './buffer.js';


function getScanIV() {
    const active_devices = dev.listActiveDevices();
    let iv = {};
    active_devices.forEach((device) => {
        device.scan_device_array.forEach((scan_device) => {
            let mac = bf.bufferToHexString(scan_device.mac);
            if (iv.hasOwnProperty(mac)) {
                iv[mac][device.alias] = scan_device.rssi;
            } else {
                iv[mac] = {};
                iv[mac][device.alias] = scan_device.rssi;
            }
        });
    });
    return iv;
};

function getDeviceSetup() {
    let devices = {};
    dev.listActiveDevices().forEach((device) => {
        if (device.active && device.alias.length > 0) {
            devices[device.alias] = device.position;
        }
    });
    return devices;
}

export {getScanIV, getDeviceSetup};