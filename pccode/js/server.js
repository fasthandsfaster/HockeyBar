// WebSocket Server
// FROM https://www.npmjs.com/package/websocket
const OS = require('os');
const FS = require('fs');
const Path = require('path');

const WebSocketServer = require('websocket').server;
const http = require('http');

const myLocalIp = Object.values(OS.networkInterfaces())
      .flat()
      .filter(n => n.family == "IPv4" && !n.internal)
      .find(e => e).address;

const LISTENER_PORT = 8080;
const serverUrl = `ws://${myLocalIp}:${LISTENER_PORT}`;

const mimeTypes = new Map([
    ['.js', 'text/javascript'],
    ['.html', 'text/html']
]);

function httpServerHandler(request, response) {
    console.log('Received HTTP request for ' + request.url);
    const filename = request.url == '/' ? 'web/index.html' : request.url.substring(1);
    const fileInfo = Path.parse(filename);
    const contentType = mimeTypes.get(fileInfo.ext) ?? 'application/octet-stream';

    try {
        const data = FS.readFileSync(filename).toString();
        response.writeHeader(200, {"Content-Type": contentType});
        response.write(data);
    }
    catch(error) {
        console.error(error);
        response.writeHead(404);
    }
    response.end();
}

const httpServer = http.createServer(httpServerHandler);

httpServer.listen(LISTENER_PORT, () => {
    console.log(`Server is listening on ${serverUrl}`);
});

const wsServer = new WebSocketServer({
    httpServer: httpServer,
    // You should not use autoAcceptConnections for production
    // applications, as it defeats all standard cross-origin protection
    // facilities built into the protocol and the browser.  You should
    // *always* verify the connection's origin and decide whether or not
    // to accept it.
    autoAcceptConnections: false
});

function originIsAllowed(origin) {
  // put logic here to detect whether the specified origin is allowed.
  return true;
}

let targetConnections = new Map();
let frontConnections = new Map();


function getRandomConnection(activeConnections) {
    const index =  Math.floor(Math.random() * activeConnections.length);
    return activeConnections[index];
}

function sendToTarget(command) {
    // const connection = [...targetConnections.values()].find(c => c.connected);
    const connections = [...targetConnections.values()].filter(c => c.connected);

    const connection = getRandomConnection(connections);

    if (connection) {
        console.log("connection:", connection.remoteAddress); // !DEBUG!
        const timestamp = new Date().toJSON();
        const maxWait = 20000;
        const data = {command, timestamp, maxWait};
        const dataStr = JSON.stringify(data);
        console.log(`\n HIT IT  !!! data: ${dataStr}`);
        connection.sendUTF(dataStr);
    }
    else {
        console.log("NO target connection found");
    }
}

function sendToFront(data) {
    console.log("sendToFront, data:", data);
    
    const connections = [...frontConnections.values()].filter(c => c.connected);

    if (connections.length) {
        connections.forEach(connection => connection.sendUTF(data));
    }
    else {
        console.log("NO front connection found");
    }
}

let gameRunning = false;
let startTime = null;
let gameDuration = 60000; // 60 sec

wsServer.on('request', request => {
    if (!originIsAllowed(request.origin)) {
      // Make sure we only accept requests from an allowed origin
      request.reject();
      console.log('   Connection from origin ' + request.origin + ' rejected.');
      return;
    }

    // const keys = dumpKeys(request.socket);
    // console.log('keys:', keys); // !DEBUG!

    // console.log('request.httpRequest.rawHeaders:', request.httpRequest.rawHeaders); // !DEBUG!

    console.log("request.resourceURL.path:", request.resourceURL.path); // !DEBUG!
    
    const connection = request.accept(null, request.origin);
    const foreignAddress = `${connection.remoteAddress}:${request.socket._peername.port}`;
    console.log(`   ${foreignAddress} connected.`);

    if (request.resourceURL.path == '/target') {
        targetConnections.set(connection.remoteAddress, connection);
    }
    
    if (request.resourceURL.path == '/front-ui') {
        frontConnections.set(foreignAddress, connection);
    }

    // console.log('Connection accepted.', connection);
    // console.log(connection.socket+'', JSON.stringify(connection.socket._peername, getCircularReplacer(), 2)); // !DEBUG!

    // console.log('connection.frameHeader:', connection.frameHeader.toString()); // !DEBUG!

    connection.on('message', message => {

        if (message.type === 'utf8') {
            console.log(`>> ${foreignAddress}: ${message.utf8Data}`);
            const msgData = JSON.parse(message.utf8Data);

            if (msgData.origin == 'front-ui') {
                if (msgData.command == 'start' && !gameRunning) {
                    gameRunning = true;
                    startTime = new Date();
                    console.log("gameRunning:", gameRunning); // !DEBUG!
                }
                if (msgData.command == 'stop' && gameRunning) {
                    gameRunning = false;
                    console.log("gameRunning:", gameRunning); // !DEBUG!
                }
            }
            else {
                sendToFront(message.utf8Data);
            }

            if (gameRunning) {
                const now = new Date();
                if ((now.getTime() - startTime.getTime()) < gameDuration) {                
                    sendToTarget('start');
                }
                else {
                    gameRunning = false
                    sendToTarget('over');
                    sendToFront('Game Over');
                }
            }
        }
        else if (message.type === 'binary') {
            console.log('>> Binary message of ' + message.binaryData.length + ' bytes');
        }
    });

    connection.on('close', (reasonCode, description) => {
        console.log(`   {foreignAddress} disconnected.`);
    });
});

function loadFile(filename) {
    const text = FS.readFileSync(filename).toString();
    console.log(`${filename} was loaded`);
    return text;
}
