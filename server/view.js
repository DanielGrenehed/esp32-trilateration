import * as bf from './buffer.js';
import * as sc from './scanner.js';

function connectView(connection) {
    connection.on('message', function(data, isBinary) {
        if (isBinary) {
            console.log('View sent binary message!!!');
        } else {
            let message = bf.bufferToString(data);

            if (message === "D") {
                connection.send(JSON.stringify(sc.getDeviceSetup()));
            } else if (message === "I") {
                connection.send(JSON.stringify(sc.getScanIV()));
            } else if (message === "T") {
                connection.send(JSON.stringify(sc.getTransmittersScanIV()));
            } else if (message.startsWith("M")) {
                connection.send(JSON.stringify(sc.getScanForMAC(message.substring(1))));
            } else {
                console.log("Received unknown message from view: '%s'", message);
            }
        } 
    });
    //connection.on('close',function() {console.log("View closed"); });
}

export {connectView};
