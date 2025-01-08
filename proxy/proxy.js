const WebSocket = require('ws');
const { spawn } = require('child_process');
const path = require('path');
const fs = require('fs');

/**
 * Example websocket client connection from esp32:
 * void send_frame(camera_fb_t *fb)
 * {
 *   if (esp_websocket_client_is_connected(client))
 *   {
 *     esp_websocket_client_send_bin(client, (const char *)fb->buf, fb->len, portMAX_DELAY);
 *   }
 * }
 */

const rtmpFile = path.join(__dirname, 'rtmp.url');

const wss = new WebSocket.Server({ port: 8080,  });

let currentConnection = null;
let lastFrame = null;
const fps = 30;
const interval = 1000 / fps;

wss.on('connection', (ws) => {
  if (currentConnection) {
    ws.write('Another client is already connected');
    ws.close();
    return;
  }

  currentConnection = ws;

  console.log('Client connected');

  if (!fs.existsSync(rtmpFile)) {
    console.error('RTMP URL file not found');
    ws.close();
    return;
  }

  const rtmpUrl = (fs.readFileSync(rtmpFile) + "").trim();
  console.log(`RTMP URL: ${rtmpUrl}`);

  const ffmpeg = spawn('ffmpeg', [
    '-re',
    '-i', '-',
    '-c:v', 'libx264',
    '-preset', 'veryfast',
    '-maxrate', '3000k',
    '-bufsize', '6000k',
    '-pix_fmt', 'yuv420p',
    '-g', '50',
    '-f', 'flv',
    rtmpUrl
  ]);

  ws.on('message', (message) => {
    lastFrame = message;
    process.stdout.clearLine(0);
    process.stdout.cursorTo(0);
    process.stdout.write(`Frame size: ${message.length} bytes, at: ${new Date().toISOString()}`);
  });

  const intervalId = setInterval(() => {
    if (lastFrame) {
      ffmpeg.stdin.write(lastFrame);
    }
  }, interval);

  ws.on('close', () => {
    console.log('Client disconnected');
    ffmpeg?.stdin.end();
    currentConnection = null;
    clearInterval(intervalId);
  });

  ws.on('error', (err) => {
    console.error(err);
    ffmpeg?.stdin.end();
    currentConnection = null;
    clearInterval(intervalId);
  });
});

console.log('WebSocket server started on port 8080');