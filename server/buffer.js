
function bufferToString(buffer) {
    let string = "";
    for (let i = 0; i < buffer.length; i++) string += String.fromCharCode(buffer[i]);
    return string;
}

function buffersDoesMatch(a, b) {
    for (let i = 0; i < a.length && i < b.length; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}


function bufferToHexString(buffer) {
    var output = "";
    for (var i = 0; i < buffer.length; i++) {
        let byte = buffer[i].toString(16);
        if (byte.length != 2) byte = "0"+byte;
        output += byte;
    }
    return output;
}

export {bufferToString, bufferToHexString, buffersDoesMatch};