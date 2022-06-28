let ws;

const targetDataTag = $("#target-data");

function startGame() {
    const origin = 'front-ui';
    const task = 'start';
    targetDataTag.empty();
    ws.send(JSON.stringify({origin, task}));
}

function stopGame() {
    const origin = 'front-ui';
    const task = 'stop';
    ws.send(JSON.stringify({origin, task}));
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
