import * as bf from './buffer.js';
import * as cf from './config.js';
import * as dh from './device_handler.js';
import * as al from './alien.js';

const scan_request_interval_ms = cf.getValue('scanreq_i',1000);

function construct_scan_device(buffer_slice) {
    var output = {mac:bf.bufferToHexString(buffer_slice.slice(2, 8)), rssi:(buffer_slice[0]<<24>>24), flags:buffer_slice[1]};
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

const device_handshake_identifier = [0xde, 0xad, 0xbe, 0xaf, 0xde, 0xca, 0xfe];

class DeviceSocket {
    constructor(ws) {
        var t = this;
        this.socket = ws;
        this.socket.hand_is_shaken = true;
        this.alias = "";
        this.type = "";
        this.startup = [];
        //this.scan_device_array = [];
        this.alien = new al.Alien();
        this.schedule = {};
        this.socket.on("message", function(data, isBinary) {t.onMessage(data, isBinary);});
        this.requestID();
        
        this.socket.on('close', function() {
            t.clearSchedule();
            dh.removeDevice(t);
        });
    }

    onMessage = function(data, isBinary) {
        if (isBinary) {
            if (data.length%8==0) {
                let t = this;
                //console.log("new data");
                create_scan_device_array(data).forEach(function(device) {
                    t.alien.process(device);
                }); 
            } else if (data.length==9 && data[0] == 0x73) {
                // direct scanning
                //console.log('direct scan!');
                let alien = construct_scan_device(data.slice(1,9));
                //console.log(alien);
                this.alien.process(alien);
            } else if (data.length == 7) {
                if (data[0] === 0x77 ) this.setWiFiMac(bf.bufferToHexString(data.slice(1,7)));
                else if (data[0] === 0x62 ) this.setBluetoothMac(bf.bufferToHexString(data.slice(1,7)));
            } else console.log(data);
        } else console.log(bf.bufferToString(data));
    }

    requestID = function() {
        this.socket.send("btm");    // request bluetooth MAC
        this.socket.send("wfm");    // request WiFi MAC
    }

    setBluetoothMac = function(mac) {
        if (this.bt_mac === undefined || this.bt_mac === null) {
            this.bt_mac = mac;
            this.addDeviceIfValid();
        } else if (this.bt_mac != mac) console.log("Trying to overwrite bt_mac");
        else console.log("%o Bluetooth-MAC: (%s) : %s",this.socket._socket.remoteAddress, this.alias, mac);
    }
    
    setWiFiMac = function(mac) {
        if (this.wf_mac === undefined || this.wf_mac === null) {
            this.wf_mac = mac;
            this.addDeviceIfValid();
        } else if (this.wf_mac != mac) console.log("Trying to overwrite wf_mac");
        else console.log("%o WiFi-MAC: (%s) : %s", this.socket._socket.remoteAddress, this.alias, mac);
    }

    addDeviceIfValid = function() {
        if (this.wf_mac != undefined && this.wf_mac != null && this.bt_mac != undefined && this.bt_mac != null) dh.pushDeviceToList(this);
    }

    scheduleCommand(interval_t, command) {
        let t = this;
        this.schedule[command] = setInterval(function(){t.socket.send(command);}, interval_t);
    }
    
    clearSchedule() {
        let schedule = this.schedule;
        Object.keys(schedule).forEach( function(command) { clearInterval(schedule[command]); });
        this.schedule = {};
    }

    start = function() {
        let t = this;
        this.startup.forEach(function (command) {
            if (typeof command === "string") {
                 t.socket.send(command);
            } else if (typeof command === typeof {}) {
                if (command.hasOwnProperty("t") && command.hasOwnProperty("c")) {
                    //console.log("scheduled command");
                    t.scheduleCommand(command.t, command.c);
                } 
            }
            //console.log("%s Command: %s", t.alias, command);
        }); 
    }
};

function evaluateConnection(data, socket) {
    if (!bf.buffersDoesMatch(data, device_handshake_identifier)) return false;
    new DeviceSocket(socket);
    return true;
}

export {evaluateConnection};