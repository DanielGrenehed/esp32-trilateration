import * as bf from './buffer.js';
import * as cf from './config.js';

const scan_request_interval_ms = typeof cf.getSubtree('scanreq_i') === 'number' ? cf.getSubtree('scanreq_i'):1000;

function construct_scan_device(buffer_slice) {
    var output = {mac:buffer_slice.slice(2, 8), rssi:(buffer_slice[0]<<24>>24), flags:buffer_slice[1]};
    //console.log("%o, %s", output.mac, bf.bufferToHexString(output.mac));
    return output;
}

function create_scan_device_array(buffer) {
    let output = [];
    for (let i = 0; i < buffer.length; i+=8) {
        let scan_device = construct_scan_device(buffer.slice(i));
        output.push(scan_device);
    }
    return output;
}

let device_clients = [];

function printActiveDevices() {
    device_clients.forEach((device) => {
        if (device.active) console.log(device.alias);
    });
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
    let wf_mac_str = bf.bufferToHexString(device.wf_mac);
    let config = cf.getSubtree('devices');
    device.active = config[wf_mac_str].active;
    device.alias = config[wf_mac_str].alias;
    device.position = config[wf_mac_str].position;
    device.type = config[wf_mac_str].type;
}

function createDefaultConfig(device) {
    var def = cf.getSubtree('devices')
    def[bf.bufferToHexString(device.wf_mac)] = {
                "bt" : bf.bufferToHexString(device.bt_mac),
                "active" : false,
                "alias" : "",
                "position" : {"x":0, "y":0},
                "type":"receiver"
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
        const config = cf.getSubtree('devices');
        let wf_mac_str = bf.bufferToHexString(device.wf_mac)
        if (config.hasOwnProperty(wf_mac_str)) {
            if (config[wf_mac_str].bt == bf.bufferToHexString(device.bt_mac)) {
                setDeviceProperties(device);
                device_clients.push(device);
                if (device.active) {
                    printActiveDevices();
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

const device_handshake_identifier = [0xde, 0xad, 0xbe, 0xaf, 0xde, 0xca, 0xfe];

class DeviceSocket {
    constructor(ws) {
        this.socket = ws;
        this.alias = "";
        this.type = "";
        this.scan_device_array = [];
        var t = this;
        this.socket.hand_is_shaken = true;
        this.socket.on("message", function(data, isBinary) {t.onMessage(data, isBinary);});
        this.configureDevice();
        
        this.socket.on('close', function() {removeDevice(t);});
    }

    onMessage = function(data, isBinary) {
        if (isBinary) {
            if (data.length%8==0) this.scan_device_array = create_scan_device_array(data);   
            else if (data.length == 7) {
                if (data[0] === 0x77 ) {
                    //console.log("WiFi MAC: %o",data.slice(1,7));
                    this.setWiFiMac(data.slice(1,7));
                }
                else if (data[0] === 0x62 ) {
                    //console.log("BT MAC: %o", data.slice(1,7));
                    this.setBluetoothMac(data.slice(1,7));
                }
            } else console.log(data);
        } else console.log(bf.bufferToString(data));
    }

    configureDevice = function() {
        this.socket.send("btm");    // request bluetooth MAC
        this.socket.send("wfm");    // request WiFi MAC
    }

    setBluetoothMac = function(mac) {
        if (this.bt_mac === undefined || this.bt_mac === null) {
            this.bt_mac = mac;
            
            this.addDeviceIfValid();
        } else if (!bf.buffersDoesMatch(this.bt_mac, mac)) console.log("Trying to overwrite bt_mac");
        else console.log("%o Bluetooth-MAC: (%s) : %s",this.socket._socket.remoteAddress, this.alias, bf.bufferToHexString(mac));
    }
    
    setWiFiMac = function(mac) {
        if (this.wf_mac === undefined || this.wf_mac === null) {
            this.wf_mac = mac;
            this.addDeviceIfValid();
        } else if (!bf.buffersDoesMatch(this.wf_mac, mac)) console.log("Trying to overwrite wf_mac");
        else console.log("%o WiFi-MAC: (%s) : %s", this.socket._socket.remoteAddress, this.alias, bf.bufferToHexString(mac));
    }

    addDeviceIfValid = function() {
        if (this.wf_mac != undefined && this.wf_mac != null && this.bt_mac != undefined && this.bt_mac != null) {
            pushDeviceToList(this);
        }
    }

    start = function() {
        if (this.type === "receiver") {
            let t = this;
            this.interval = setInterval(function(){t.socket.send("sd");}, scan_request_interval_ms);
            this.socket.on('close', function close() { clearInterval(this.interval); });
            this.socket.send("btscan");
        } else if (this.type === "transmitter") {
            this.socket.send("btadv");
        } else {
            console.log("Type not set, unable to start. (%o)", this);
        }
    }
};

function evaluateConnection(data, socket) {
    if (bf.buffersDoesMatch(data, device_handshake_identifier)) new DeviceSocket(socket);
}

export {evaluateConnection, listActiveDevices, listInactiveDevices, sendToDevice, sendToAllDevices, sendToActiveDevices, sendToInactiveDevices, countActiveDevices, countInactiveDevices};