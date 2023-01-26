import * as dev from './device_handler.js';
import * as cf from './config.js';

const max_time_unannounced = cf.getValue('max_announce_interval', 1000 * 60 * 10);

function filter(iv) {
    Object.entries(iv).forEach((array, _value) => {
        let mac = array[0];
        let values = array[1];
        //console.log("mac: %o",mac);
        //console.log("Mac: %o, Values: %o", mac, values);
        let num_non_aliases = values.hasOwnProperty('time') ? 1:0;
        if (Object.keys(values).length < (3 + num_non_aliases)) {
            delete iv[mac];
            //console.log("Excluded %s, too few receivers (%d: %o)", mac, Object.keys(values).length, Object.keys(values));
        } else if (values.time < new Date(Date.now() - max_time_unannounced)) {
            delete iv[mac];
            console.log("Excluded %s because of time", mac);
        } else {
            //console.log("Included %o",mac);
        }
        //console.log("|");
    });
    return iv;
}

function addToIVIfValid(iv, sd, d) {
    //console.log("maybe adding %o", sd);
    if (sd.mac.length !=12) {
        console.log("Invalid mac length: %d from %s", sd.mac.length, sd.mac);
        return iv;
    }
    if (!iv.hasOwnProperty(sd.mac)) iv[sd.mac] = {};
    iv[sd.mac][d.alias] = sd.rssi;
    iv[sd.mac]['time'] = sd.latest_update;
    //console.log("added: %o", iv[sd.mac])
    return iv;
}

function getScanIV() {
    const active_devices = dev.listReceiverDevices();
    let iv = {};
    active_devices.forEach((device) => {
        device.alien.constructIV().forEach((scan_device) => {
            iv = addToIVIfValid(iv, scan_device, device);
            
        });
    });
    
    return filter(iv);
};

function getTransmittersScanIV() {
    const receivers = dev.listReceiverDevices();
    const transmitters = dev.listTransmitterDevices();
    let iv = {};
    receivers.forEach((device) => {
        device.alien.constructIV().forEach((scan_device) => {
            transmitters.forEach((transmitter) => {
                if (scan_device.mac == transmitter.bt_mac) {
                    iv = addToIVIfValid(iv, scan_device, device);
                }
            });
        });
    });

    return filter(iv);
}

function getScanForMAC(mac) {
    //console.log("Looking for mac: %o, length: %d", mac, mac.length); 
    let iv = {};
    dev.listReceiverDevices().forEach((receiver) => {
        iv = addToIVIfValid(iv, receiver.alien.getMAC(mac), receiver);
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

export {getScanIV, getDeviceSetup, getTransmittersScanIV, getScanForMAC};