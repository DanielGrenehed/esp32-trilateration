import * as cf from './config.js';

let device_clients = [];

function printActiveDevices() {
    device_clients.forEach((device) => {
        if (device.active) console.log(device.alias);
    });
}

function listReceiverDevices() {
    var out = [];
    device_clients.forEach(function (device) {
        if (device.active && device.type === "Rx") {out.push(device);}
    });
    return out;
}

function listTransmitterDevices() {
    var out = [];
    device_clients.forEach(function (device) {
        if (device.active && device.type === "Tx") {out.push(device);}
    });
    return out;
}

function listActiveDevices() {
    var out = [];
    device_clients.forEach((device) => {
        if (device.active) out.push(device);
    });
    return out;
}

function listInactiveDevices() {
    var out = [];
    device_clients.forEach((device) => {
        if (!device.active) out.push(device);
    });
    return out;
}


function sendToDevice(alias, command) {
    if (alias.length>0) {
        device_clients.forEach((device) => {
            if (alias == device.alias) {
                device.socket.send(command);
            }
        });
    }
}

function sendToActiveDevices(command) {
    device_clients.forEach((device) => {
        if (device.active) device.socket.send(command);
    });
}

function sendToInactiveDevices(command) {
    device_clients.forEach((device) => {
        if (!device.active) device.socket.send(command);
    });
}

function sendToAllDevices(command) {
    device_clients.forEach((device) => {device.socket.send(command);});
}

function countActiveDevices() {
    var out = 0;
    device_clients.forEach((device) => {if (device.active) out++;});
    return out;
}

function countInactiveDevices() {
    var out =0;
    device_clients.forEach((device) => {if (!device.active) out++;});
    return out;
}


function setDeviceProperties(device) {
    let config = cf.getSubtree('devices')[device.wf_mac];
    if (device.bt_mac != config.bt) console.log("Bluetooth MAC invalid: WiFi:%s Bluetooth:%s Config Bluetooth mac:%s", device.wf_mac, device.bt_mac, config.bt);
    device.active = config.active;
    device.alias = config.alias;
    device.position = config.position;
    device.type = config.type;
    device.startup = config.startup;
}

function createDefaultConfig(device) {
    var def = cf.getSubtree('devices')
    def[device.wf_mac] = {
                "bt" : device.bt_mac,
                "active" : false,
                "alias" : "",
                "position" : {"x":0, "y":0},
                "type":"Rx",
                "startup": ["btscan", {"t":scan_request_interval_ms,"c":"sd"}]
            };
    cf.setSubtree('devices', def);
    cf.updateConfig();
}

function pushDeviceToList(device) {
    if (!device_clients.includes(device)) {
        /*
            Look for device in config, if found, load properties
            if not found, add to config as found

            in config.devices
                match wf_mac, 
                    if bt_mac matches
                        load properties
        */
        let config = cf.getSubtree('devices');
        if (config.hasOwnProperty(device.wf_mac)) {
            cf.refresh();
            config = cf.getSubtree('devices');
            if (config[device.wf_mac].bt == device.bt_mac) {
                setDeviceProperties(device);
                device_clients.push(device);
                if (device.active) {
                    //printActiveDevices();
                    console.log("%s Has connected!",device.alias);
                    device.start();
                }
            } else {
                console.log("WiFi and Bluetooth MAC is not correct!");
                device.socket.close();
                device.isAlive = false;
                return;
            }
        } else { // new WiFi-MAC, new device
            createDefaultConfig(device);
            setDeviceProperties(device);
            device_clients.push(device);
        }
    }
}



function removeDevice(device) {
    const index = device_clients.indexOf(device);
    if (index > -1) device_clients.splice(index, 1);
    if (device.alias != "" && device.alias != null) {
        console.log("%s disconnected", device.alias);
        printActiveDevices();
    }
}


export {listActiveDevices, listInactiveDevices, sendToDevice, sendToAllDevices, sendToActiveDevices, sendToInactiveDevices, countActiveDevices, countInactiveDevices, listReceiverDevices, listTransmitterDevices, pushDeviceToList, removeDevice, printActiveDevices};