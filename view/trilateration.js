// 1m path loss 37
const REFERENS_RSSI = -40;

const MHz = 2457;
const FREE_SPACE_PATH_LOSS = 27.55;
const INDOOR_PATH_LOSS = 4;

function rssiToMeters(rssi) {
    return Math.pow(10, (REFERENS_RSSI - rssi)/(10*INDOOR_PATH_LOSS));
}

function distanceFromPoints(p1, p2) {
    if (Object.keys(p1).length === 3 && Object.keys(p2).length === 3) {
        return Math.cbrt(Math.pow(p2.x - p1.x, 2) + Math.pow(p2.y - p1.y, 2) + Math.pow(p2.z - p1.z, 2));
    } else {
        return Math.cbrt(Math.pow(p2.x - p1.x, 2) + Math.pow(p2.y - p1.y, 2));
    }
}

function estimatePosition(receivers) {
    let A = Math.pow(receivers[0].position.x, 2) + Math.pow(receivers[0].position.y, 2) - Math.pow(receivers[0].distanceToDevice(), 2);
    let B = Math.pow(receivers[1].position.x, 2) + Math.pow(receivers[1].position.y, 2) - Math.pow(receivers[1].distanceToDevice(), 2);
    let C = Math.pow(receivers[2].position.x, 2) + Math.pow(receivers[2].position.y, 2) - Math.pow(receivers[2].distanceToDevice(), 2);
    let X32 = receivers[2].position.x - receivers[1].position.x;
    let X13 = receivers[0].position.x - receivers[2].position.x;
    let X21 = receivers[1].position.x - receivers[0].position.x;
    let Y32 = receivers[2].position.y - receivers[1].position.y;
    let Y13 = receivers[0].position.y - receivers[2].position.y;
    let Y21 = receivers[1].position.y - receivers[0].position.y;
    let X = (A*Y32 + B*Y13 + C*Y21) / (2*(receivers[0].position.x*X32+receivers[1].position.x*Y13+receivers[2].position.x*Y21));
    let Y = (A*X32 + B*X13 + C*X21) / (2*(receivers[0].position.y*Y32+receivers[1].position.y*X13+receivers[2].position.y*X21));
    // console.log(A, B, C, X32, X13, X21, Y32, Y13, Y21, X, Y, receivers);
    return {x:X, y:Y};
}

function receiversIntersects(receivers, m) {
    for (let i = 1; i < receivers.length; i++) {
        if (!receivers[0].intersects(receivers[i], m)) return false;
    }
    return true;
}

let M = 0.1, K = 0.01;
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
    radie_i += K;
    receivers.forEach(function(receiver) {receiver.e_rad = radie_i;});
    return receivers;
}

class Receiver {
    constructor(rssi, pos, alias) {
        this.rssi = rssi; 
        this.alias = alias;
        this.radius = rssiToMeters(rssi);
        this.e_rad = 0;
        this.position = pos;
        console.log(this);
    }

    intersects(receiver, m) {
        let d = distanceFromPoints(this.position, receiver.position);
        let r1 = this.radius + m, r2 = receiver.radius + m; 
        //console.log("D: %f, R1: %f, R2: %f", d, r1, r2);
        if ( d > (r1+r2)) return false;
        return true; 
    }

    distanceToDevice() {
        return this.radius + this.e_rad
    }
}


function createReceivers(transmitter) { // transmitter = {A:rssi, B:rssi...
    let receivers = [];
    //console.log(transmitter);
    for (const [receiver_alias, rssi] of Object.entries(transmitter)) {
        if (receiver_alias === 'time') return;
        let added = false;
        for (let i = 0; i < receivers.length; i++) {
            if (rssi > receivers[i].rssi) {
                receivers.splice(i, 0, new Receiver(rssi, device_setup[receiver_alias], receiver_alias));
                added = true;
                break;
            }
        }
        if (!added) receivers.push(new Receiver(rssi, device_setup[receiver_alias], receiver_alias));
    }
    return receivers;
}

