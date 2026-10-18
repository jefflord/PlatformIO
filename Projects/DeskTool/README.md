# DeskTool

DeskTool is PlatformIO firmware for an ESP32-based desk presence and light
controller. It monitors a sensor input and controls a separate light using
ESP-NOW and a local HTTP endpoint.

## Features

- Reads the digital detection input on GPIO 23 and mirrors its state to the
  onboard LED on GPIO 2.
- Sends `on` and `off` commands to a configured ESP-NOW peer. The current
  firmware also sends HTTP commands to the light controller as a backup.
- Uses the ESP32 touch input on GPIO 12 and the BOOT button on GPIO 0 to toggle
  the light.
- Accepts ESP-NOW `tog`, `on`, and `off` messages and forwards them to the
  light controller.
- Connects to Wi-Fi, starts an mDNS responder at `desktool.local`, synchronizes
  time with `pool.ntp.org`, and initializes ArduinoOTA.
- Contains an optional UART radar-frame reader on UART1 (RX GPIO 16, TX GPIO
  17, 256000 baud). It is disabled by default in the source.

## Hardware connections

| ESP32 pin | Purpose |
| --- | --- |
| GPIO 2 | Onboard/status LED output |
| GPIO 0 | BOOT button input (active low; debounced, armed after release at startup) |
| GPIO 12 | Capacitive touch input |
| GPIO 23 | Digital detection input (`LD2450C_PIN` in the source) |
| GPIO 16 / 17 | Optional UART1 RX / TX for the radar sensor |

The source comments indicate that the exact radar model should be confirmed.
The active detection logic currently reads GPIO 23 as a digital input; the
UART frame reader is separately controlled by `readSensorSerial`.

## Build and upload

The project targets the `esp32dev` board using the Arduino framework and the
Espressif32 PlatformIO platform. Install PlatformIO (for example, through the
VS Code PlatformIO extension), open this folder as a project, then run:

```sh
pio run -e esp32dev
# Upload over USB serial (COM4)
pio run -e esp32dev -t upload
# Upload over Wi-Fi OTA using desktool.local
pio run -e esp32dev-ota -t upload
pio device monitor -p COM4 -b 115200
```

The `esp32dev` environment uploads over USB serial to `COM4` at 1500000 baud.
The `esp32dev-ota` environment uploads over Wi-Fi to the mDNS hostname
`desktool.local`; it does not rely on a fixed IP address. For OTA, the computer
and ESP32 must be on a network where that mDNS name resolves. The serial monitor
uses 115200 baud, as set by `Serial.begin()` in the firmware.

PlatformIO installs the configured libraries, including NTPClient and
HLK-LD2450, when building the project.

## Network and light-controller setup

The ESP32 first tries its stored Wi-Fi credentials. If it cannot connect, the
firmware starts SmartConfig provisioning and restarts if provisioning does not
complete within two minutes.

The light controller is expected to be reachable at
`http://reartvlight.local/doAction` and accept JSON POST requests such as:

```json
{"action":"light_on"}
```

The HTTP endpoint, mDNS name, sensor pins, and ESP-NOW peer MAC address are
configured in `src/main.cpp`. Update them there to match your hardware and
network. The ESP-NOW peer address is currently hard-coded, and
`forceAlwaysUseWiFiBackup` is enabled, so HTTP is also attempted after an
ESP-NOW send.

## Project files

- `platformio.ini` — board, framework, upload settings, and library dependencies.
- `src/main.cpp` — application, sensor input, Wi-Fi, ESP-NOW, HTTP, and OTA logic.
- `include/bmp.h` — bitmap/frame data and declarations for display-related code.
- `test/` — PlatformIO test directory; no project tests are currently present.
