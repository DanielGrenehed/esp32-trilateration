import {WebSocketServer} from 'ws';


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

function construct_string_from_buffer(buffer) {
    let string = "";
    for (let i = 0; i < buffer.length; i++) {
        string += String.fromCharCode(buffer[i]);
    }
    return string;
}

class DeviceSocket {
    constructor(ws) {
        this.socket = ws;
        this.socket.hand_is_shaken = true;
        this.socket.on("message", function message(data, isBinary) {
            if (isBinary) {
                //console.log("Data was binary");
                //console.log(data.length/8+" Devices, data size: " + data.length);
                if (data.length%8==0) {
                    this.scan_device_array = create_scan_device_array(data);
                   
                } else if (data.length == 7) {
                    if (data[0] === 0x77 ) console.log("WiFi MAC: %o",data.slice(1,7));
                    else if (data[0] === 0x62 ) console.log("BT MAC: %o", data.slice(1,7));
                } else {
                    console.log(data);
                }
                 //console.log(this.scan_device_array);
            } else console.log(construct_string_from_buffer(data));
        });
        console.log("Created new device socket");
        this.configureDevice();
        var t = this;
        this.interval = setInterval(function(){t.socket.send("sd");}, scan_request_interval_ms);
        this.socket.on('close', function close() {
            clearInterval(this.interval);
        });
    }

    configureDevice() {
        this.socket.send("btscan");
        this.socket.send("btm");
        this.socket.send("wfm");
    }

};

function match(a, b) {
    for (let i = 0; i < a.length && i < b.length; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

let device_clients = [];
const device_handshake_identifier = [0xde, 0xad, 0xbe, 0xaf, 0xde, 0xca, 0xfe]

wss.on('connection', function connection(ws) {
    console.log("Connected");
    ws.isAlive = true; 
    ws.on('pong', function heartbeat() { this.isAlive = true; });
    ws.on("message", function message(data, isBinary) {
        if (ws.hand_is_shaken) return;        

        if (isBinary) {
            if (data.length == 7)  {
                if (match(data, device_handshake_identifier)) {
                    device_clients.push(new DeviceSocket(ws));
                }
            }
            else console.log(data);
        } else console.log(construct_string_from_buffer(data));
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

