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
            } else {
                console.log("Received unknown message from view: '%s'", message);
            }
        } 
    });
    connection.on('close',function() {/*console.log("View closed");*/});
}

export {connectView};