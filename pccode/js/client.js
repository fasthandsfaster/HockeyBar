// WebSocket Client
// FROM https://www.npmjs.com/package/websocket

const WebSocketClient = require('websocket').client;

const client = new WebSocketClient();

client.on('connectFailed', error => {
    console.log('Connect Error: ' + error.toString());
});

client.on('connect', connection => {
    console.log('WebSocket Client Connected');
    
    connection.on('error', error => {
        console.log("Connection Error: " + error.toString());
    });
    
    connection.on('close', () => {
        console.log('Connection Closed');
    });
    
    connection.on('message', message => {
        if (message.type === 'utf8') {
            console.log("Received: '" + message.utf8Data + "'");
        }
    });
    
    function sendNumber() {
        if (connection.connected) {
            const clientNumber = Math.round(Math.random() * 0xFFFFFF);
            const timestamp = new Date().toJSON();
            const data = {clientNumber, timestamp};
            
            connection.sendUTF(JSON.stringify(data));
            setTimeout(sendNumber, 1000);
        }
    }
    sendNumber();
});

client.connect('ws://localhost:8080/', 'json');
