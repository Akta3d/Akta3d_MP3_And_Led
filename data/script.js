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
var lightsModeMode = 0;

var shutDownActive = false;
var shutDownInMinutes = 15;
var shutDownTimerMillis = 0; // not display for moment. TODO display time before shut down if active

var lightsModes = [
    { name: "OFF", id: 0, color1: false, color2: false, param: false},
    { name: "SINGLE", id: 1, color1: true, color2: false, param: false},
    { name: "FADE", id: 2, color1: true, color2: false, param: true},
    { name: "RANDOM_GRADATION", id: 3, color1: false, color2: false, param: true},
    { name: "RANDOM", id: 4, color1: false, color2: false, param: true},
];

// ------------ WEBSOCKET -------------------------
function initWebSocket() {
    if(connected) return;

    console.log('Trying to open a WebSocket connection...');

    var gateway = `ws://${gatewayIp}${websocketRoot}`;
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessages;
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

function onMessages(event) {
    var messages = event.data;
    var messages = messages.split('|');
    console.log(`Received ${messages.length - 1} actions`);
    messages.forEach(message => {
        if(message) {
            onMessage(message);
        }
    });
}

function onMessage(message) {
    var action = message;
    var value = 0;
    var pos = message.indexOf(':');
    if(pos !== -1) {
        action = message.substr(0, pos);
        value = message.substr(pos + 1);
    }
    console.log(`Received action: ${action}, Value:${value}`);

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
        case 'changeLightsMode':
            lightsModeMode = parseInt(value, 10);
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

function sendMessage(message) {
	console.log('send message : ' + message);
	websocket.send(message);
}
// --------------- SETUP ------------------------
window.addEventListener('load', onLoad);
function onLoad(event) {
    /*
    // register the service worker to have a progressive WebApp
    if ('serviceWorker' in navigator) {
        navigator.serviceWorker.register('/sw.js');
    } else {
        console.warn('Browser not allow serviceWorker. Impossible to install app in cache.')
    }
    */
    var hostname = location.hostname;
    if(!hostname) {
        var localIp = localStorage.getItem('gatewayIp');
        if(localIp) {
            gatewayIp = localIp;
        }
    } else {
        gatewayIp = hostname;
    }
    // display connection or gui
    switchGui();
}

function switchGui() {
    if(forceDisplayGui || connected) {
        // display gui
        document.getElementById('connectionGui').style.display = 'none';
        document.getElementById('mainGui').style.display = 'block';

        initMainGui();
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
function initMainGui() {
    var select = document.getElementById('lightsModeSelect');
    select.options.length = 0;

    for (i = 0; i < lightsModes.length; i += 1) {
        var newOption = new Option( lightsModes[i].name, lightsModes[i].id);
        select.add( newOption);
    }
}

function initMainGuiEventListeners() {
    // mp3    
    document.getElementById('playAlertButton').addEventListener('click', onPlayAlertButton);
    document.getElementById('prevTrackButton').addEventListener('click', onPrevTrackButton);
    document.getElementById('playPauseButton').addEventListener('click', onPlayPauseButton);
    document.getElementById('nextTrackButton').addEventListener('click', onNextTrackButton);
    document.getElementById('prevDirButton').addEventListener('click', onPrevDirButton);
    document.getElementById('loopButton').addEventListener('click', onLoopButton);
    document.getElementById('nextDirButton').addEventListener('click', onNextDirButton);
    document.getElementById('volumeSlider').addEventListener('change', onVolumeSlider);

    // lights mode
    document.getElementById('lightsModeSelect').addEventListener('change', onLightsModeSelect);
    document.getElementById('color1Picker').addEventListener('input', onColor1Picker);
    document.getElementById('color1Picker').addEventListener('change', onColor1Picker);
    document.getElementById('color2Picker').addEventListener('input', onColor2Picker);
    document.getElementById('color2Picker').addEventListener('change', onColor2Picker);
    document.getElementById('lightsModeParamInput').addEventListener('change', onLightsModeParamInput);    
    document.getElementById('lightsModeOffButton').addEventListener('click', onLightsModeOffButton);    
    
    // shut down
    document.getElementById('shutDownMinuteInput').addEventListener('change', onShutDownMinuteInput);
    document.getElementById('shutDownTimerButton').addEventListener('click', shutDownTimerButton);
    document.getElementById('shutDownButton').addEventListener('click', onShutDownButton);    
}

function refreshMainGui() {
    // mp3
    document.getElementById('playPauseButton').innerHTML = mp3Playing ? '||' : '>';
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

    // lightsModeMode
    var selectedOptionIndex = -1;
    for(var i = 0 ; i < lightsModes.length; i+=1) {
        if(lightsModes[i].id === lightsModeMode) {
            selectedOptionIndex = i;
        }
    }

    if(selectedOptionIndex !== -1) {
        var selectedOption = lightsModes[selectedOptionIndex];
        document.getElementById('lightsModeSelect').selectedIndex = selectedOptionIndex;

        document.getElementById('lightsModeColor1Ctn').style.display = selectedOption.color1 ? 'block' : 'none';
        document.getElementById('lightsModeColor2Ctn').style.display = selectedOption.color2 ? 'block' : 'none';
        document.getElementById('lightsModeParamCtn').style.display = selectedOption.param ? 'block' : 'none'; 

        document.getElementById('lightsModeOffButton').style.display = selectedOption.id !== 0 ? 'block' : 'none';         
    }
}

// -------------- ACTIONS --------------------------
// -- MP3
function onPlayAlertButton() {
    console.log('onPlayAlertButton');
    sendMessage('playRandomAlert');
}
function onPrevTrackButton() {
    console.log('onPrevTrackButton');
    sendMessage('playPreviousTrack');
}
function onPlayPauseButton() {
    console.log('onPlayPauseButton');
    sendMessage('switchPlayPause');
}
function onNextTrackButton() {
    console.log('onNextTrackButton');
    sendMessage('playNextTrack');
}
function onPrevDirButton() {
    console.log('onPrevDirButton');
    sendMessage('playPreviousDirectory');
}
function onLoopButton() {
    console.log('onLoopButton');
    sendMessage('switchLoopMode');
}
function onNextDirButton() {
    console.log('onNextDirButton');
    sendMessage('playNextDirectory');
}
function onVolumeSlider() {
    console.log(`onVolumeSlider : ${document.getElementById('volumeSlider').value}`);
    sendMessage(`setVolume:${document.getElementById('volumeSlider').value}`);
}

// -- LIGHTS MODE
function onLightsModeSelect() {
    var select = document.getElementById('lightsModeSelect');
    if(select.selectedIndex >= 0 && select.selectedIndex < lightsModes.length) {
        var option = lightsModes[select.selectedIndex];
        console.log(`onLightsModeSelect : ${option.name}`);
        sendMessage(`changeLightsMode:${option.id}`);
    }
}

function onColor1Picker() {
    console.log(`onColor1Picker : ${document.getElementById('color1Picker').value}`);
    var rgb = hexToRgb(document.getElementById('color1Picker').value);
    sendMessage(`setLightsModeColor1:${rgb.r},${rgb.g},${rgb.b}`);
}
function onColor2Picker() {
    console.log(`onColor2Picker : ${document.getElementById('color2Picker').value}`);
    var rgb = hexToRgb(document.getElementById('color2Picker').value);
    sendMessage(`setLightsModeColor2:${rgb.r},${rgb.g},${rgb.b}`);
}
function onLightsModeParamInput() {
    console.log(`onLightsModeParamInput : ${document.getElementById('lightsModeParamInput').value}`);
    sendMessage(`setLightsModeParam:${document.getElementById('lightsModeParamInput').value}`);
}
function onLightsModeOffButton() {
    console.log('onLightsModeOffButton');
    sendMessage('changeLightsMode:0');
}

// -- SHUT DOWN
function onShutDownMinuteInput() {
    console.log(`onShutDownMinuteInput : ${document.getElementById('shutDownMinuteInput').value}`);
    sendMessage(`shutDownMinutes:${document.getElementById('shutDownMinuteInput').value}`);
}

function shutDownTimerButton() {
    console.log('shutDownTimerButton');
    sendMessage('switchShutDown');
}

function onShutDownButton() {
    console.log('onShutDownButton');
    sendMessage('shutDown');
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