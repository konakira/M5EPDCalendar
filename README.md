# M5Paper Low-Power Calendar Dashboard

A highly efficient, minimalist calendar dashboard for the M5Stack M5Paper (M5EPD), featuring deep-sleep technology and customizable week-start options.

## Features

- **Extreme Battery Life**: Designed to last for months on a single charge by utilizing full system shutdown between updates.
- **Customizable Week Layout**: Monday-start (default) and Sunday-start supported via build flags.
- **Auto-Sync**: Synchronizes time via NTP every 30 days to ensure accuracy.
- **Battery Monitoring**: Built-in battery percentage display with optimized voltage-to-capacity mapping.

## Why the Battery Lasts So Long

This project is optimized for "Deep-Sleep-and-Forget" usage. The longevity is achieved through three technical pillars:

1. **True Power-Off (RTC Wakeup)**: Instead of a standard software "light sleep," the device uses `M5.shutdown()`. This cuts power to the ESP32 almost entirely. The BM8563 Real-Time Clock (RTC) remains the only component drawing a tiny amount of current to trigger a reboot at midnight.
2. **Electronic Paper Display (e-Ink)**: The 4.7" e-Ink display requires zero power to maintain an image. Once the calendar is drawn, the device turns off, and the image stays on the screen indefinitely.
3. **Daily Duty Cycle**: The device only wakes up once every 24 hours for a few seconds to update the date. It stays in a total power-off state for 99.9% of the time.

## Installation & Build

This project uses **PlatformIO**. By default, the calendar is built with a **Monday-start** layout.

### Building for Monday-Start (Default)

No extra flags are needed. Your `platformio.ini` should look like this:

```ini
[env:m5stack-fire]
platform = espressif32
board = m5stack-fire
framework = arduino
