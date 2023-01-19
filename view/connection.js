let ws = new WebSocket(URI);

function requestDeviceSetup() {
    ws.send('D');
}
function requestData() {
    ws.send('T');
}

function onData(data) {
    let valid_data = {};
    let ignored_data_len = 0;
    Object.keys(data).forEach( (mac) => {
        let dms = Object.keys(data[mac]);
        if (dms.length == count_devices()) {
            valid_data[mac] = data[mac];
        } else if (dms.length > count_devices()) {
            requestDeviceSetup();
            return;
        } else {
            ignored_data_len++;
        }
    });
    console.log("Ignored %d, %o", ignored_data_len,latest_data);
    updateData(valid_data);
}

ws.onopen = (event) => {
    requestDeviceSetup();
};

ws.onmessage = (event) => {
    console.log(event.data);
    var json_data = JSON.parse(event.data)
    console.log(json_data);
    if (json_data === {}) return; 
    if (Object.keys(json_data)[0].length == 12) {
        onData(json_data);
    } else {
        setDeviceSetup(json_data);
        requestData();
    }
}
ws.onclose = (event) => {
    console.log("Websocket closed");
};
