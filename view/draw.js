
let device_setup = {}; // {A:{x:0,y:0},B:{x:1,y:2.4}}

function count_devices() {
    return Object.keys(device_setup).length;
}

var canvas = document.getElementById("canvas");
var context = canvas.getContext("2d");

let max_x=0, min_x=Number.MAX_SAFE_INTEGER, max_y=0, min_y=Number.MAX_SAFE_INTEGER, max_p=0;
const canvas_padding = 50;
const canvas_resolution = 1000;

function getRealX(x) {
    var rel = (x-min_x)/(max_p-min_x);
    var out = (rel*(canvas_resolution-(2*canvas_padding)))+canvas_padding
    //console.log("rel: %f, out: %d, canvas width: %d", rel, out, canvas.getBoundingClientRect().width);
    return out;
}
function getRealY(y) {
    var rel = (y-min_y)/(max_p-min_y);
    return (rel*(canvas_resolution-(2*canvas_padding)))+canvas_padding
}

function plotDevice(device) {    
    context.fillStyle = "#ff0000";
    context.beginPath();
    context.arc(getRealX(device.x), getRealY(device.y), 10, 0, 2*Math.PI);
    context.fill();
    console.log("Plotted device: %o", device);
}

function scaleScreen() {
    max_x = 0;
    min_x = Number.MAX_SAFE_INTEGER;
    max_y = 0;
    min_y = Number.MAX_SAFE_INTEGER;
    Object.values(device_setup).forEach(function(device) {
        console.log(device);
        if (device.x < min_x) min_x = device.x;
        if (device.x > max_x) max_x = device.x;
        if (device.y < min_y) min_y = device.y;
        if (device.y > max_y) max_y = device.y;
    });
    max_p = Math.max(max_x, max_y);
}

function plotData(data) {

}


// 1m path loss 37
const REFERENS_RSSI = 40;

const MHz = 2457;
const FREE_SPACE_PATH_LOSS = 27.55;
const INDOOR_PATH_LOSS = 1.7;

function RSSI_TO_M(rssi) {
    return Math.pow(10, (rssi-REFERENS_RSSI)/(10*INDOOR_PATH_LOSS));
}

function distanceFromRSSI(rssi) { // returns in meters
    return Math.pow(10, (FREE_SPACE_PATH_LOSS - (20 * Math.log10(MHz)) + (-rssi)) / 20);
}

function distanceFromPoints(p1, p2) {
    //console.log("p1: %o , p2: %o", p1, p2);

    if (Object.keys(p1).length === 3 && Object.keys(p2).length === 3) {
        return Math.cbrt(Math.pow(p2.x - p1.x, 2) + Math.pow(p2.y - p1.y, 2) + Math.pow(p2.z - p1.z, 2));
    } else {
        return Math.cbrt(Math.pow(p2.x - p1.x, 2) + Math.pow(p2.y - p1.y, 2));
    }
}

class Receiver {
    constructor(rssi, pos) {
        this.rssi = rssi; 
        this.pdis = (100+rssi)/100;
        this.radius = RSSI_TO_M(-rssi);
        this.e_rad = 0;
        this.position = pos;
    }

    intersects(receiver, m) {
        let d = distanceFromPoints(this.position, receiver.position);
        let r1 = this.radius + m, r2 = receiver.radius + m; 
        //console.log("D: %f, R1: %f, R2: %f", d, r1, r2);
        if ( d > (r1+r2)) return false;
        return true; 
    }

    draw() {
        context.beginPath();
        context.arc(getRealX(this.position.x), getRealY(this.position.y), ((Math.abs(this.radius+this.e_rad))*max_p*canvas_resolution), 0, 2*Math.PI);
        context.stroke();
        console.log(this);
    }

    fill() {
        context.beginPath();
        context.arc(getRealX(this.position.x), getRealY(this.position.y), ((Math.abs(this.radius+this.e_rad))*max_p*canvas_resolution), 0, 2*Math.PI);
        context.fill();
    }
}

Array.prototype.insert = function ( index, ...items ) {
    this.splice( index, 0, ...items );
};

function createReceivers(transmitter) { // transmitter = {A:rssi, B:rssi...
    let receivers = [];
    //console.log(transmitter);
    for (const [receiver_alias, rssi] of Object.entries(transmitter)){
        let added = false;
        for (let i = 0; i < receivers.length; i++) {
            if (rssi > receivers[i].rssi) {
                receivers.insert(i, new Receiver(rssi, device_setup[receiver_alias]));
                added = true;
                break;
            }
        }
        if (!added) receivers.push(new Receiver(rssi, device_setup[receiver_alias]));
    }
    return receivers;
}

function receiversIntersects(receivers, m) {
    for (let i = 1; i < receivers.length; i++) {
        if (!receivers[0].intersects(receivers[i], m)) return false;
    }
    return true;
}

let M = 1, K = 0.1;
/*
    Compare to receiver with best(largest) rssi,
    increase circle size until all receivers intersects
    then decrease until they do not intersect    
*/
function expandAndReduce(data) {
    //console.log(data);
    let receivers = createReceivers(data);
    //console.log("Created receivers: %o", receivers);
    if (receivers.length < 3) return;
    let radie_i =0.0;
    
    console.log(receiversIntersects(receivers, radie_i));
    while (!receiversIntersects(receivers, radie_i)) radie_i += M;
    //console.log("expansion done, %o", receivers);
    
    while (receiversIntersects(receivers, radie_i)) {
        //console.log("reduction : %d ", radie_i);
        radie_i -= K;
    }
    receivers.forEach(function(receiver) {receiver.e_rad = radie_i;});
    return receivers;
}

function drawCircle(device, rssi) {
    context.beginPath();
    context.arc(getRealX(device_setup[device]['x']), getRealY(device_setup[device]['y']), (2*Math.sqrt(100+rssi))*100, 0, 2*Math.PI);
    context.stroke();
}

function Equation_1() {
    let d12 = Math.pow(1-x,2)+Math.pow(3-y, 2);
    let d22 = Math
}

function estimatePosition(device) {
    
    var d12 = Math.pow(x1-x, 2) + Math.pow(y1-y, 2);
    var d22 = Math.pow(x2-x, 2) + Math.pow(y2-y, 2);
    var d32 = Math.pow(x3-x, 2) + Math.pow(y3-y, 2); 
    var x = (24 + 2*d12 - d22 - d23)/8;
    var y = (24 - d22 - d32)/8;
    var X32 = x3-x2;
    var X13 = x1-x3;
    var X21 = x2-x1;
    var Y32 = y3-y2;
    var Y13 = y1-y3;
    var Y21 = Y2-y1;
    var A = (x1*x1) + (y1*y1) - d12;
    var B = (x2*x2) + (y2*y2) - d22;
    var C = (x3*x3) + (y3*y3) - d32;
    var X = (A * Y32 + B * Y13 + C * Y21) / 2*(y1*Y32+y2*Y13+y3*Y21);
    var Y = (A * X32 + B * X13 + C * X21) / 2*(x1*X32+x2*X13+x3*X21);
}

let latest_data = {}; // {"MAC":{A:rssi, B:rssi}}
function updateData(data) {
    latest_data = data;
    for (const [wifi_mac, value] of Object.entries(latest_data)) {
        context.strokeStyle = "#"+Math.floor(Math.random()*16777215).toString(16);
        expandAndReduce(value).forEach((receiver) => {receiver.draw()});
        console.log(wifi_mac);
        /* for (const [device, rssi] of Object.entries(value)) {

            drawCircle(device, rssi);
        } */
    }
}

function setDeviceSetup(setup) {
    device_setup = setup;
    scaleScreen();
    Object.keys(device_setup).forEach(function(device) {plotDevice(device_setup[device]);});
} 