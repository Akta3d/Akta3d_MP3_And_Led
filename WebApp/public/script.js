var gatewayIp = '192.168.1.61';
var websocketRoot = '/ws';
var websocket;
var connected = false;
var forceDisplayGui = false; // allow to display main gui without connected socket

// global variables
var mp3Playing = false;
var loopTrack = false;
var volume = 10;
var currentMp3Folder = 1; // not display for moment.

var lightsModeColor1 = '#FF0000';
var lightsModeColor2 = '#0000FF';
var lightsModeParam = 0;

var shutDownActive = false;
var shutDownInMinutes = 15;
var shutDownTimerMillis = 0; // not display for moment. TODO display time before shut down if active

// ------------ WEBSOCKET -------------------------
function initWebSocket() {
    if(connected) return;

    console.log('Trying to open a WebSocket connection...');

    var gateway = `ws://${gatewayIp}${websocketRoot}`;
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    connected = true;
    switchGui();
}

function onClose(event) {
    console.log('Connection closed');
    connected = false;
    switchGui();
}

function onMessage(event) {
    var action = event.data;
    var value = 0;
    var pos = event.data.indexOf(':');
    if(pos !== -1) {
        action = event.data.substr(0, pos);
        value = event.data.substr(pos + 1);
    }
    console.log(`Received event. Action: ${action}, Value:${value}`);

    switch(action) {
        case 'mp3Playing':
            mp3Playing = value === '1';
            break;
        case 'currentMp3Folder':
            currentMp3Folder = parseInt(value, 10);
            break;
        case 'loopTrack':
            loopTrack = value === '1';
            break;
        case 'lightsModeColor1':
            var r = 0;
            var g = 0;
            var b = 0;
            pos = value.indexOf(',');
            if(pos !== -1) {
                r = parseInt(value.substr(0, pos));
                value = value.substr(pos + 1);
            }
            pos = value.indexOf(',');
            if(pos !== -1) {
                g = parseInt(value.substr(0, pos));
                b = parseInt(value.substr(pos + 1));
            }
            lightsModeColor1 = rgbToHex(r, g, b);
            break;
        case 'lightsModeColor2':
            var r = 0;
            var g = 0;
            var b = 0;
            pos = value.indexOf(',');
            if(pos !== -1) {
                r = parseInt(value.substr(0, pos));
                value = value.substr(pos + 1);
            }
            pos = value.indexOf(',');
            if(pos !== -1) {
                g = parseInt(value.substr(0, pos));
                b = parseInt(value.substr(pos + 1));
            }
            lightsModeColor2 = rgbToHex(r, g, b);
            break;                            
        case 'lightsModeParam': 
            lightsModeParam = parseInt(value, 10);
            break;            
        case 'volume':
            volume = parseInt(value, 10);
            break;
        case 'shutDownActive':
            shutDownActive = value === '1';
            break;  
        case 'shutDownInMinutes':
            shutDownInMinutes = parseInt(value, 10);
            break;
        case 'shutDownTimerMillis':
            shutDownTimerMillis = parseInt(value, 10);
            break;                     
        default:
            console.error(`Unknown action: ${action}, Value:${value}`)
    }

    refreshMainGui();
}

// --------------- SETUP ------------------------
window.addEventListener('load', onLoad);
function onLoad(event) {
    // register the service worker to have a progressive WebApp
    if ('serviceWorker' in navigator) {
        navigator.serviceWorker.register('./sw.js');
    }

    var localIp = localStorage.getItem('gatewayIp');
    if(localIp) {
        gatewayIp = localIp;
    }

    // display connection or gui
    switchGui();
}

function switchGui() {
    if(forceDisplayGui || connected) {
        // display gui
        document.getElementById('connectionGui').style.display = 'none';
        document.getElementById('mainGui').style.display = 'block';

        initMainGuiEventListeners();
        refreshMainGui();
    } else {
        //display connection
        document.getElementById('connectionGui').style.display = 'block';
        document.getElementById('mainGui').style.display = 'none';

        initConnectionGuiListeners();
        refreshConnectionGui();

        initWebSocket();
    } 
}

// --------------- CONNECTION GUI ---------------------
function initConnectionGuiListeners() {
    document.getElementById('changeGatewayIpButton').addEventListener('click', onChangeGatewayIpButton);
}
function refreshConnectionGui() {
    if(!document.getElementById('gatewayIpInput').value) {
        document.getElementById('gatewayIpInput').value = gatewayIp;
    }
    document.getElementById('connectionMessage').innerHTML = `Try to connect to "ws://${gatewayIp}${websocketRoot}"`;
}
function onChangeGatewayIpButton() {
    gatewayIp = document.getElementById('gatewayIpInput').value;
    console.log(`onChangeGatewayIpButton : ${gatewayIp}`);

    localStorage.setItem('gatewayIp', gatewayIp);

    refreshConnectionGui();
    initWebSocket();
}

// --------------- MAIN GUI ---------------------
function initMainGuiEventListeners() {
    // mp3
    document.getElementById('prevTrackButton').addEventListener('click', onPrevTrackButton);
    document.getElementById('playPauseButton').addEventListener('click', onPlayPauseButton);
    document.getElementById('nextTrackButton').addEventListener('click', onNextTrackButton);
    document.getElementById('prevDirButton').addEventListener('click', onPrevDirButton);
    document.getElementById('loopButton').addEventListener('click', onLoopButton);
    document.getElementById('nextDirButton').addEventListener('click', onNextDirButton);
    document.getElementById('volumeSlider').addEventListener('change', onVolumeSlider);

    // lights mode
    document.getElementById('color1Picker').addEventListener('input', onColor1Picker);
    document.getElementById('color1Picker').addEventListener('change', onColor1Picker);
    document.getElementById('color2Picker').addEventListener('input', onColor2Picker);
    document.getElementById('color2Picker').addEventListener('change', onColor2Picker);
    document.getElementById('lightsModeParamInput').addEventListener('change', onLightsModeParamInput);    

    // shut down
    document.getElementById('shutDownMinuteInput').addEventListener('change', onShutDownMinuteInput);
    document.getElementById('shutDownTimerButton').addEventListener('click', shutDownTimerButton);
    document.getElementById('shutDownButton').addEventListener('click', onShutDownButton);    
}

function refreshMainGui() {
    // mp3
    document.getElementById('playPauseButton').innerHTML = mp3Playing ? 'Pause' : 'Play';
    document.getElementById('loopButton').innerHTML = loopTrack ? 'Loop track' : 'Loop Dir';
    document.getElementById('volumeLabel').innerHTML = `Volume : ${volume}`;
    document.getElementById('volumeSlider').value = volume;

    // lights mode
    document.getElementById('color1Picker').value = lightsModeColor1;
    document.getElementById('color2Picker').value = lightsModeColor2;
    document.getElementById('lightsModeParamInput').value = lightsModeParam;    

    // shut down    
    document.getElementById('shutDownMinuteInput').value = shutDownInMinutes;
    document.getElementById('shutDownTimerButton').innerHTML = shutDownActive ? 'Cancel' : 'Start';
    document.getElementById('shutDownTimerButton').style.background = shutDownActive ? 'red' : 'green';
}

// -------------- ACTIONS --------------------------
// -- MP3
function onPrevTrackButton() {
    console.log('onPrevTrackButton');
    websocket.send('playPreviousTrack');
}
function onPlayPauseButton() {
    console.log('onPlayPauseButton');
    websocket.send('switchPlayPause');
}
function onNextTrackButton() {
    console.log('onNextTrackButton');
    websocket.send('playNextTrack');
}
function onPrevDirButton() {
    console.log('onPrevDirButton');
    websocket.send('playPreviousDirectory');
}
function onLoopButton() {
    console.log('onLoopButton');
    websocket.send('switchLoopMode');
}
function onNextDirButton() {
    console.log('onNextDirButton');
    websocket.send('playNextDirectory');
}
function onVolumeSlider() {
    console.log(`onVolumeSlider : ${document.getElementById('volumeSlider').value}`);
    websocket.send(`setVolume:${document.getElementById('volumeSlider').value}`);
}

// -- LIGHTS MODE
function onColor1Picker() {
    console.log(`onColor1Picker : ${document.getElementById('color1Picker').value}`);
    var rgb = hexToRgb(document.getElementById('color1Picker').value);
    websocket.send(`setLightsModeColor1:${rgb.r},${rgb.g},${rgb.b}`);
}
function onColor2Picker() {
    console.log(`onColor2Picker : ${document.getElementById('color2Picker').value}`);
    var rgb = hexToRgb(document.getElementById('color2Picker').value);
    websocket.send(`setLightsModeColor2:${rgb.r},${rgb.g},${rgb.b}`);
}
function onLightsModeParamInput() {
    console.log(`onLightsModeParamInput : ${document.getElementById('lightsModeParamInput').value}`);
    websocket.send(`setLightsModeParam:${document.getElementById('lightsModeParamInput').value}`);
}

// -- SHUT DOWN
function onShutDownMinuteInput() {
    console.log(`onShutDownMinuteInput : ${document.getElementById('shutDownMinuteInput').value}`);
    websocket.send(`shutDownMinutes:${document.getElementById('shutDownMinuteInput').value}`);
}

function shutDownTimerButton() {
    console.log('shutDownTimerButton');
    websocket.send('switchShutDown');
}

function onShutDownButton() {
    console.log('onShutDownButton');
    websocket.send('shutDown');
}


// -------- HELPERS -------------
function componentToHex(c) {
    var hex = c.toString(16);
    return hex.length == 1 ? '0' + hex : hex;
}
function rgbToHex(r, g, b) {
    return '#' + componentToHex(r) + componentToHex(g) + componentToHex(b);
}
function hexToRgb(hex) {
    var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result ? {
        r: parseInt(result[1], 16),
        g: parseInt(result[2], 16),
        b: parseInt(result[3], 16)
    } : null;
}