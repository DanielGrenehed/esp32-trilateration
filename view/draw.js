const canvas_padding = 200;
const canvas_resolution = 1000;
const canvas_scale = canvas_resolution - 2*canvas_padding;
const POINT_SIZE = 10;

let canvas = document.getElementById("canvas");
let context = canvas.getContext("2d");
let mac_view = document.getElementById("MAC-View");

let max_p=0, min_p=0, scale=0;
let device_setup = {}; // {A:{x:0,y:0},B:{x:1,y:2.4}}

function count_devices() {
    return Object.keys(device_setup).length;
}

function scaleScreen() {
    let max_x = 0;
    let min_x = Number.MAX_SAFE_INTEGER;
    let max_y = 0;
    let min_y = Number.MAX_SAFE_INTEGER;
    Object.values(device_setup).forEach(function(device) {
        if (device.x < min_x) min_x = device.x;
        if (device.x > max_x) max_x = device.x;
        if (device.y < min_y) min_y = device.y;
        if (device.y > max_y) max_y = device.y;
    });
    max_p = Math.max(max_x, max_y);
    min_p = max_p == max_x ? min_x : min_y;
    scale = max_p - min_p;
}

function getReal(p) {
    var rel = (p-min_p)/(max_p-min_p);
    return (rel*(canvas_resolution-(2*canvas_padding)))+canvas_padding;
}

function scaleUp(v) {
    return v*(canvas_scale/scale)+canvas_padding;
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

function drawAbsolutePoint(p, r) {
    context.beginPath();
    context.arc(p.x, p.y, Math.abs(r), 0, 2*Math.PI);
    context.fill();
}

let devices_data = [];

function addDeviceData(device) {
    device.color = "#"+Math.floor(Math.random()*16777215).toString(16);
    devices_data.push(device);
    context.fillStyle = device.color;
    drawAbsolutePoint(device.a_pos, device.p_size);
}

function handleDevice(mac, data) {
    if (Object.keys(data).length < 3) return;
    //console.log("Expand And Reduce %o", data);
    let c = expandAndReduce(data);
    if (c === undefined) return;
    //console.log("MAC: %s, EaR: %o",mac,c);
    let p = estimatePosition(c)
    let dev_data = {mac: mac, a_pos:{x:getReal(p.x), y:getReal(p.y)}, p_size:10, data:data};
    addDeviceData(dev_data);
    //console.log("Estimated position: %o", p);
    //drawPoint(p);
}

let latest_data = {}; // {"MAC":{A:rssi, B:rssi, C:rssi, time:13321318729819}}
function updateData(data) {
    device_data = [];
    latest_data = data;
    for (const [bt_mac, value] of Object.entries(latest_data)) {
        context.strokeStyle = "#"+Math.floor(Math.random()*16777215).toString(16);
        handleDevice(bt_mac, value);
    }
}

function interescts_device(p) {
    for (let i = 0; i < devices_data.length; i++) {
        if (distanceFromPoints(devices_data[i].a_pos, p) < (devices_data[i].p_size/2)) {
            return devices_data[i];
        }
    }
    let out={};
    for (const [alias, pos] of Object.entries(device_setup)) {
        if (distanceFromPoints({x:getReal(pos.x), y:getReal(pos.y)}, p) < 12/2) {
            out.color = "#ff0000";
            out.mac =alias;
        }
    }
    return Object.keys(out).length ==0 ? false: out;
}

canvas.addEventListener('mousemove', (event) => {
    var rect = canvas.getBoundingClientRect();
    let icx = event.clientX - rect.left;
    let icy = event.clientY - rect.top;
    let px = icx/canvas.clientWidth;
    let py = icy/canvas.clientHeight;
    let scx = px*canvas_resolution;
    let scy = py*canvas_resolution;
    pos = {x: scx, y: scy};
    let int = interescts_device(pos);
    if (int) {

        mac_view.innerHTML = int.mac;
        mac_view.style.color = int.color;
    }
    //console.log("pos: %o, ic{%f, %f}, sc{%f, %f}",pos, icx, icy, scx, scy);
    //drawAbsolutePoint(pos, 10);
});

function setDeviceSetup(setup) {
    device_setup = setup;
    scaleScreen();
    Object.keys(device_setup).forEach(function(device) {plotDevice(device_setup[device]);});
} 