import {WebSocketServer} from 'ws';
const wss = new WebSocketServer({port:80});
import * as bf from './buffer.js';
import * as device from './device.js';

wss.on('connection', function connection(ws) {
    //console.log("Connected");
    ws.isAlive = true; 
    ws.on('pong', function heartbeat() { this.isAlive = true; });
    ws.on("message", function message(data, isBinary) {
        if (ws.hand_is_shaken) return;        

        if (isBinary) {
            if (data.length == 7) device.evaluateConnection(data, ws);//if (bf.buffersDoesMatch(data, device_handshake_identifier)) new DeviceSocket(ws);
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

