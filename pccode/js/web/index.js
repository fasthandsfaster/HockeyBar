let ws;

const targetDataTag = $("#target-data");

function startGame() {
    const origin = 'front-ui';
    const command = 'start';
    targetDataTag.empty();
    ws.send(JSON.stringify({origin, command}));
}

function stopGame() {
    const origin = 'front-ui';
    const command = 'stop';
    ws.send(JSON.stringify({origin, command}));
    targetDataTag.append('Stopped', $("<br/>"));
}

function webSocketConnected() {
    console.log("ws connected");
}

function webSocketClosed() {
    console.log("ws closed");
}

function webSocketMessage(evt) {
    console.log("webSocketMessage:", evt.data);
    targetDataTag.append(evt.data, $("<br/>"));
};

$(function () {
    const serverUrl = `ws://${location.host}/front-ui`;

    ws = new WebSocket(serverUrl);
    
    ws.onopen = webSocketConnected;    
    ws.onclose = webSocketClosed;
    ws.onmessage = webSocketMessage;
});
