import * as fs from 'fs';

const config_file = 'config.json';
let raw_config = fs.readFileSync(config_file);
let config = JSON.parse(raw_config);

function updateConfig() {
    fs.writeFileSync(config_file, JSON.stringify(config));
}

function getSubtree(subtree) {
    if (!config.hasOwnProperty(subtree)) {
        return {};
    }
    return config[subtree];
}

function setSubtree(key, subtree) {
    config[key] = subtree;
}

function getValue(subtree, default_v) {
    return typeof getSubtree(subtree) === typeof default_v ? getSubtree(subtree) : default_v;
}

function refresh() {
    raw_config = fs.readFileSync(config_file);
    config = JSON.parse(raw_config);
}


export {updateConfig, getSubtree, setSubtree, getValue, refresh};