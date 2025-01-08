# ESP32 CAM - Streaming video to YouTube/Twitch

This project is a simple example of how to stream video from an ESP32-CAM to YouTube and Twitch using the official [esp32-camera](https://github.com/espressif/esp32-camera) and [esp_websocket_client](https://github.com/espressif/esp-protocols/blob/master/components/esp_websocket_client/README.md) components.

## Main problems

Streaming directly to YouTube using RTMP protocol with the ESP32 camera can be challenging due to the limitations of the ESP32 hardware and the complexity of the RTMP protocol.

The chip lacks the processing power and memory needed to handle the RTMP protocol directly, as well as the encoding requirements needed for platforms like YouTube. Typically, it can capture and send raw video frames or MJPEG streams, but it cannot handle the RTMP protocol or the H.264 encoding required by YouTube.

To overcome these limitations, we will use a server to handle the RTMP protocol and the H.264 encoding. The server will receive the video stream from the ESP32 and forward it to YouTube or Twitch using the RTMP protocol.

This service can be ran on a Raspberry Pi or any other computer capable of running a server. My choice was to use a Digital Ocean droplet running Ubuntu 20.04., but for testing purposes, we can run the server on our local machine, even inside a Docker container.

## Project requirements
### Hardware
- I've used an ESP32-WROVER-DEV board with an OV2640 camera module
  - Theorycally, any ESP32-CAM board should work
- A computer or hosting to run the server

### Software
- [esp-idf](https://github.com/espressif/esp-idf), and requirements
- Docker (optional)
- [FFmpeg](https://ffmpeg.org/)
- Node.js (tested with v20)

## Environment setup

Clone the repository:
```bash
git clone xx espcam-streaming
cd espcam-streaming
```

### Hardware setup
Set up ESP-IDF environment of your choice, I've used VSCode with [ESP-IDF extension](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension)

In `menuconfig`, set the following configurations:
- `Serial flasher config` -> `Flash size` -> `4MB`
  - This is the flash size of my ESP32-WROVER-DEV board, you may need to change it according to your board
- `Application WiFi/Websocket settings` fill:
  - `WiFi SSID`
  - `WiFi Password`
  - `Websocket URL` (Like: `ws://192.168.0.10:8080`) - This is where your encoder server is running
- `Component config` -> `ESP PSRAM` -> `Support for external, SPI-connected RAM` -> `Enabled`
- In `Camera configuration` I deselected every other camera model except `OV2640 2MP`

Build & burn & monitor the project.

### Server setup

Inside the proxy folder copy the `rtmp.url.example` to `rtmp.url` and fill it with your RTMP server URL.

- For YouTube, you can find the RTMP URL in the YouTube Studio -> Go Live -> Stream settings -> Stream Key
  - ```bash
    rtmp://a.rtmp.youtube.com/live2/your-stream-key
    ```
- For Twitch, you can find the RTMP URL in the Twitch Studio -> Preferences -> Stream -> Stream Key
  - ```bash
    rtmp://live.twitch.tv/app/your-stream-key
    ```

#### Local
If you want to run the server locally, you can use the provided package.json to install the dependencies and run the server.

```bash
sudo apt install -y ffmpeg      # install ffmpeg if necessary
sudo apt install -y nodejs npm  # install nodejs and npm if necessary
cd proxy                        # go to the proxy folder
npm install                     # install the dependencies
npm start                       # start the server
```

#### Docker
If you want to run the server inside a Docker container, you can use the provided Dockerfile to build the image and run the container.

```bash
cd proxy                        # go to the proxy folder
docker build -t ffmpeg-node .   # build the image
docker run -it -w /app -p 8080:8080 --rm -v $(pwd):/app ffmpeg-node:latest npm start
                                # run the container
```

#### Digital Ocean

If you want to run the server on a Digital Ocean droplet, it is possible, but not tested yet.

## Known issues & limitations

The video stream is not smooth, and the quality is not the best. Frame rate is higly dependent on the frame size. I've tested with different frame sizes and the resutls were not good.

- QVGA (320x240) - 19-22 fps
- VGA (640x480) - 10-12 fps
- SVGA (800x600) - 6-8 fps
- UXGA (1600x1200) - 1-3 fps

The server is not handling the video stream properly, it just listens carefully on the websocket stream for individual JPEG frame data, saves it in a local variable for backup reasons, then triing to stick with the framerate with calling ffmpeg from a `setInterval` function. This is not the best solution, and it doesn't really works at all.