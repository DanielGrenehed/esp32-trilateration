import * as cf from './config.js';

const ring_buffer_size = cf.getValue('ring_buffer_size', 20);
const min_buffer_size = cf.getValue('min_rbuffer_size', 3);
const max_time_unannounced = cf.getValue('max_announce_interval', 1000 * 60 * 10);

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
        this.ring_buffers = {};
        this.timestamps = {};
    }

    process = function(device) {
        if (!this.ring_buffers.hasOwnProperty(device.mac)) {
            this.ring_buffers[device.mac] = new RingBuffer(ring_buffer_size);
            //console.log("New ringbuffer for %s", device.mac);
        }
        this.ring_buffers[device.mac].push(device.rssi);
        this.timestamps[device.mac] = new Date();
    }
    
    constructIV = function() {
        let t = this;
        let out = [];
        Object.keys(this.ring_buffers).forEach(function(mac) {
            if (t.ring_buffers[mac].ptr >= min_buffer_size) {
                if (t.timestamps[mac] < new Date(Date.now() - max_time_unannounced)) {
                    delete t.ring_buffers[mac];
                    delete t.timestamps[mac];
                } else out.push({mac:mac, rssi:getValue(t.ring_buffers[mac]), latest_update:t.timestamps[mac]});
            }
        });
        return out;
    }

    getMAC = function(mac) {
        if (this.ring_buffers.hasOwnProperty(mac)) {
            if (this.timestamps[mac] < new Date(Date.now() - max_time_unannounced)) {
                delete this.ring_buffers[mac];
                delete this.timestamps[mac];
            } else return {mac:mac, rssi:getValue(this.ring_buffers[mac]), latest_update:this.timestamps[mac]};
        }
        return null;
    }
};

export {Alien};