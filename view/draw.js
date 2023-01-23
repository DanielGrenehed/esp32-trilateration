
let device_setup = {}; // {A:{x:0,y:0},B:{x:1,y:2.4}}

function count_devices() {
    return Object.keys(device_setup).length;
}

var canvas = document.getElementById("canvas");
var context = canvas.getContext("2d");

let max_p=0, min_p=0, scale=0;
const canvas_padding = 50;
const canvas_resolution = 1000;
const canvas_scale = canvas_resolution - 2*canvas_padding;
const POINT_SIZE = 10;

function getReal(p) {
    var rel = (p-min_p)/(max_p-min_p);
    return (rel*(canvas_resolution-(2*canvas_padding)))+canvas_padding;
}

function scaleUp(v) {
    return v*(canvas_scale/scale)+canvas_padding;
}

function scaleScreen() {
    let max_x = 0;
    let min_x = Number.MAX_SAFE_INTEGER;
    let max_y = 0;
    let min_y = Number.MAX_SAFE_INTEGER;
    Object.values(device_setup).forEach(function(device) {
        console.log(device);
        if (device.x < min_x) min_x = device.x;
        if (device.x > max_x) max_x = device.x;
        if (device.y < min_y) min_y = device.y;
        if (device.y > max_y) max_y = device.y;
    });
    max_p = Math.max(max_x, max_y);
    min_p = max_p == max_x ? min_x : min_y;
    scale = max_p - min_p;
}

function drawCircle(x, y, r, fill) {
    context.beginPath();
    context.arc(getReal(x), getReal(y), Math.abs(r), 0, 2*Math.PI);
    if (fill) context.fill();
    else context.stroke();
}

function plotDevice(device) {    
    context.fillStyle = "#ff0000";
    drawCircle(device.x, device.y, 12, true);
}

function drawPoint(p) {
    context.fillStyle = "#"+Math.floor(Math.random()*16777215).toString(16);
    drawCircle(p.x, p.y, POINT_SIZE, true);
}

function handleDevice(mac, data) {
    if (Object.keys(data).length < 3) return;
    //console.log("Expand And Reduce %o", data);
    let c = expandAndReduce(data);
    if (c === undefined) return;
    console.log("Mac: %s, EAR: %o",mac,c);
    drawPoint(estimatePosition(c));
}

let latest_data = {}; // {"MAC":{A:rssi, B:rssi, C:rssi, time:13321318729819}}
function updateData(data) {
    latest_data = data;
    for (const [bt_mac, value] of Object.entries(latest_data)) {
        context.strokeStyle = "#"+Math.floor(Math.random()*16777215).toString(16);
        handleDevice(bt_mac, value);
    }
}

function setDeviceSetup(setup) {
    device_setup = setup;
    scaleScreen();
    Object.keys(device_setup).forEach(function(device) {plotDevice(device_setup[device]);});
} 