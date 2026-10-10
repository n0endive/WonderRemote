# Wonder Remote

A small M5StickS3 IR remote for a Sony STR-KS100. It sends Sony SIRC15 at
40 kHz using device address 48, with the command numbers verified for that
receiver in the parent IRcito project.

## Button actions

| Button | Gesture | IR command |
|---|---|---|
| A | Single press | Video 2 |
| A | Double press | TV |
| A | Long press (800 ms) | Power |
| B | Single press | Volume Up |
| B | Double press | Volume Down |

A single press is sent after a 350 ms double-click window. Button B long
presses are ignored. Each command is transmitted as three Sony frames at
approximately 45 ms start-to-start.

## Build

Open `WonderRemote.code-workspace` in VS Code to make Wonder Remote the
workspace's PlatformIO project. Connect the StickS3 over USB, select its
serial port if prompted, then use the PlatformIO **Upload** button (or
**PlatformIO: Upload**) to build and upload this firmware. The PlatformIO
extension is recommended if it is not already installed.

From this directory:

```bash
pio run -e m5stack-sticks3
```

To upload from the command line, use `pio run -e m5stack-sticks3 -t upload`.

The application image is
`.pio/build/m5stack-sticks3/firmware.bin`. This is an ESP32-S3 application
image, not a merged full-flash image; use a launcher that writes to the
application partition and do not flash it at address `0x0`.

## Host-side logic test

```bash
g++ -std=c++17 -Wall -Wextra -Werror \
  test/remote_logic_host.cpp -o /tmp/wonder-remote-test \
  && /tmp/wonder-remote-test
```

The listed Sony commands are verified for the STR-KS100 only. Other Sony
receivers can use different device addresses or command numbers.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE).
