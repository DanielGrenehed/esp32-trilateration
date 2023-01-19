import {WebSocketServer} from 'ws';
import * as bf from './buffer.js';
import * as device from './device.js';
import * as cf from './config.js';
import * as vw from './view.js';

const device_socket_server = new WebSocketServer({port:typeof cf.getSubtree('device_port') === 'number' ? cf.getSubtree('device_port') : 80});
const controller_socket_server = new WebSocketServer({port:typeof cf.getSubtree('controller_port') === 'number' ? cf.getSubtree('controller_port') : 81});
const viewer_socket_server = new WebSocketServer({port:typeof cf.getSubtree('view_port') === 'number' ? cf.getSubtree('view_port') : 82});

function handleDeviceConnect(ws) {
    ws.on("message", function message(data, isBinary) {
        if (ws.hand_is_shaken) return;
        if (isBinary) {
            if (data.length == 7) device.evaluateConnection(data, ws);
            else console.log(data);
        } else console.log(bf.bufferToString(data));
    });
}

function handleViewConnect(ws) {
    //console.log("View connected");
    vw.connectView(ws);
}

function handleControllerConnect(ws) {
    console.lot("Controler connected");
}

function setupServerWebSocket(server, handler_function) {
    server.on('connection', (ws) => {
        ws.isAlive = true;
        ws.on('pong', function() {this.isAlive=true;});
        handler_function(ws);
    });
    server.interval = setInterval(() => {
        server.clients.forEach((ws) => {
            if (ws.isAlive === false) return ws.terminate();
            ws.isAlive = false;
            ws.ping();
        });
    }, 30000);
    server.on('close', ()=>{clearInterval(server.interval);});
}


setupServerWebSocket(device_socket_server, handleDeviceConnect);
setupServerWebSocket(viewer_socket_server, handleViewConnect);
setupServerWebSocket(controller_socket_server, handleControllerConnect);