import * as dev from './device_handler.js';
import * as bf from './buffer.js';


function getScanIV() {
    const active_devices = dev.listReceiverDevices();
    let iv = {};
    active_devices.forEach((device) => {
        device.alien.constructIV().forEach((scan_device) => {
            if (scan_device.mac.length !=12) {
                console.log("Invalid mac length: %d from %s", scan_device.mac.length, scan_device.mac);
                return;
            }
            if (iv.hasOwnProperty(scan_device.mac)) {
                iv[scan_device.mac][device.alias] = scan_device.rssi;
            } else {
                iv[scan_device.mac] = {};
                iv[scan_device.mac][device.alias] = scan_device.rssi;
            }
        });
    });
    return iv;
};

function getTransmittersScanIV() {
    const receivers = dev.listReceiverDevices();
    const transmitters = dev.listTransmitterDevices();
    let iv = {};
    receivers.forEach((device) => {
        device.alien.constructIV().forEach((scan_device) => {
            transmitters.forEach((transmitter) => {
                if (scan_device.mac == transmitter.bt_mac) {
                    if (scan_device.mac.length != 12) {
                        console.log("Invalid mac length: %i from %s", scan_device.mac.length, scan_device.mac);
                        return;
                    }
                    if (iv.hasOwnProperty(scan_device.mac)) {
                        iv[scan_device.mac][device.alias] = scan_device.rssi;
                    } else {
                        iv[scan_device.mac] = {};
                        iv[scan_device.mac][device.alias] = scan_device.rssi;
                    }
                }
            });
        });
    });
    return iv;
}

function getDeviceSetup() {
    let devices = {};
    dev.listReceiverDevices().forEach((device) => {
        if (device.active && device.alias.length > 0) {
            devices[device.alias] = device.position;
        }
    });
    return devices;
}

export {getScanIV, getDeviceSetup, getTransmittersScanIV};