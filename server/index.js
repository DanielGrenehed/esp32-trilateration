import {WebSocketServer} from 'ws';
import * as fs from 'fs';
import * as bf from './buffer.js';

const config_file = 'config.json';
let raw_config = fs.readFileSync(config_file);
let config = JSON.parse(raw_config);

if (!config.hasOwnProperty('devices')) {
    config.devices = {};
}

function updateConfig() {
    fs.writeFileSync(config_file, JSON.stringify(config));
}

const wss = new WebSocketServer({port:80});
const scan_request_interval_ms = 1000;

function construct_scan_device(buffer_slice) {
    return {mac:buffer_slice.slice(2, 8), rssi:(buffer_slice[0]<<24>>24), flags:buffer_slice[1]};
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

function setDeviceProperties(device) {
    let wf_mac_str = bf.bufferToHexString(device.wf_mac);
    device.active = config.devices[wf_mac_str].active;
    device.alias = config.devices[wf_mac_str].alias;
    device.position = config.devices[wf_mac_str].position;
}

function createDefaultConfig(device) {
    config.devices[bf.bufferToHexString(device.wf_mac)] = {
                "bt" : bf.bufferToHexString(device.bt_mac),
                "active" : false,
                "alias" : "",
                "position" : {"x":0, "y":0}
            }
    updateConfig();
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
        let wf_mac_str = bf.bufferToHexString(device.wf_mac)
        if (config.devices.hasOwnProperty(wf_mac_str)) {
            if (config.devices[wf_mac_str].bt == bf.bufferToHexString(device.bt_mac)) {
                setDeviceProperties(device);
                device_clients.push(device);
                if (device.active) console.log("%s Has connected!",device.alias);
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



const device_handshake_identifier = [0xde, 0xad, 0xbe, 0xaf, 0xde, 0xca, 0xfe];

class DeviceSocket {
    constructor(ws) {
        this.socket = ws;
        var t = this;
        this.socket.hand_is_shaken = true;
        this.socket.on("message", function(data, isBinary) {t.onMessage(data, isBinary);});
        this.configureDevice();
        
        this.interval = setInterval(function(){t.socket.send("sd");}, scan_request_interval_ms);
        this.socket.on('close', function close() {
            clearInterval(this.interval);
            device_clients.pop(t);
        });
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
        this.socket.send("btscan"); // start bluetooth scanner
        this.socket.send("btm");    // request bluetooth MAC
        this.socket.send("wfm");    // request WiFi MAC
    }

    setBluetoothMac = function(mac) {
        if (this.bt_mac === undefined || this.bt_mac === null) {
            this.bt_mac = mac;
            this.addDeviceIfValid();
        } else if (!bf.buffersDoesMatch(this.bt_mac, mac)) console.log("Trying to overwrite bt_mac");
    }
    
    setWiFiMac = function(mac) {
        if (this.wf_mac === undefined || this.wf_mac === null) {
            this.wf_mac = mac;
            this.addDeviceIfValid();
        } else if (!bf.buffersDoesMatch(this.wf_mac, mac)) console.log("Trying to overwrite wf_mac");
    }

    addDeviceIfValid = function() {
        if (this.wf_mac != undefined && this.wf_mac != null && this.bt_mac != undefined && this.bt_mac != null) {
            pushDeviceToList(this);
        }
    }
};



wss.on('connection', function connection(ws) {
    console.log("Connected");
    ws.isAlive = true; 
    ws.on('pong', function heartbeat() { this.isAlive = true; });
    ws.on("message", function message(data, isBinary) {
        if (ws.hand_is_shaken) return;        

        if (isBinary) {
            if (data.length == 7) if (bf.buffersDoesMatch(data, device_handshake_identifier)) new DeviceSocket(ws);
            else console.log(data);
        } else console.log(bf.bufferToString(data));
    });
});

const interval = setInterval(function ping() {
    wss.clients.forEach(function each(ws) {
        if (ws.isAlive === false) return ws.terminate();
        ws.isAlive = false;
        ws.ping();
    });
}, 30000);

wss.on('close', function close() {
    clearInterval(interval);
});

