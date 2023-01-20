import * as cf from './config.js';

const ring_buffer_size = cf.getValue('ring_buffer_size', 10);

class RingBuffer {
    constructor(size) {
        this.size = size;
        this.is_full = false;
        this.ptr = 0;
        this.array = [];
    }

    push = function(value) {
        if (this.is_full) {
            this.array[this.ptr++] = value;
            if (this.ptr === this.size) {
                this.ptr = 0;
            }
        } else {
            this.array.push(value);
            this.ptr++;
            if (this.ptr === this.size) {
                this.is_full = true;
                this.ptr = 0;
            }
        }
        return this.ptr > -1 ? this.size : this.ptr -1;
    }

    latest = function() {
        if (this.ptr == 0 && !this.is_full) return null;
        if (this.ptr == 0 && this.is_full) return this.array[this.size-1];
        return this.array[this.ptr-1];
    }

    inOrder = function() {//fifo
        let out = [];
        if (this.is_full) {
            for (let i = this.ptr; i < this.size; i++) out.push(this.array[i]);
        }
        for (let i = 0; i < this.ptr; i++) out.push(this.array[i]);
        return out;
    }
};

function mean(ringbuffer) {
    let sum = 0, iter = 0;
    ringbuffer.inOrder().forEach( function(value) {
        sum += value;
        iter++;
    });
    return sum / iter;
}

function standardDeviation(m, ringbuffer) {
    let sum = 0, iter = 0;
    ringbuffer.inOrder().forEach(function(x) {
        sum += Math.pow(Math.abs(x - m), 2);
        iter++;
    });
    return Math.sqrt(sum/iter);
}

function getValue(ringbuffer) {
    let m = mean(ringbuffer), std = standardDeviation(m, ringbuffer);
    let comparataor = m-2*std;
    let sum = 0, iter = 0;
    ringbuffer.inOrder().forEach(function(x) {
        if (x >= comparataor) {
            sum += x;
            iter++;
        }
    });
    return sum / iter;
}

class Alien {
    constructor() {
        this.map = {};
    }

    process = function(device) {
        if (!this.map.hasOwnProperty(device.mac)) this.map[device.mac] = new RingBuffer(ring_buffer_size);
        this.map[device.mac].push(device.rssi);
    }
    
    constructIV = function() {
        let t = this;
        let out = [];
        Object.keys(this.map).forEach(function(mac) {
            out.push({mac:mac, rssi:getValue(t.map[mac])});
        });
        return out;
    }
};

export {Alien};