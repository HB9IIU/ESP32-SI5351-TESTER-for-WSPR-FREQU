# 📡 WSPR Frequency Tester for ESP32 + Si5351 + TFT

This is a self-contained test application for evaluating **Si5351 output frequencies** using an **ESP32**, a **320×240 touchscreen TFT**, and a **simple UI**. It’s designed to output WSPR band frequencies through CLK0 of the Si5351.

This tool is useful to:

- Verify if your Si5351 module is working
- Inject a reference signal into a spectrum analyzer or power amplifier
- Tune or debug WSPR-related hardware

👉 **Note:** This app does **not** transmit the WSPR protocol — it outputs a **continuous sine-like signal** at standard WSPR frequencies.

---

## ✨ Features

- ✅ Frequency selection for all WSPR bands (160m to 6m + 50 MHz)
- 📶 Output on CLK0 with 8 mA drive (approx. **+10 dBm**, unfiltered)
- 🎛️ Touchscreen band selector (2-column layout)
- 🔧 Calibration page for applying oscillator correction (± ppb)
- 🔢 Manual frequency entry via keypad (8 kHz to 160 MHz)
- 🖼️ PNG splash screen and optional QR code About page

---

## 📦 Hardware

| Device       | Function       | ESP32 Pin |
|--------------|----------------|-----------|
| Si5351       | SDA (I²C data) | GPIO 25   |
| Si5351       | SCL (I²C clk)  | GPIO 26   |
| TFT Display  | Touch + SPI    | See `platformio.ini` |

> 📌 **Check your wiring against the actual pin mapping** in [`platformio.ini`](./platformio.ini). No additional setup is needed.

---

## 📚 Libraries

All required libraries are included locally in the `lib/` folder for portability:

- [`TFT_eSPI`](https://github.com/Bodmer/TFT_eSPI)
- [`Si5351Arduino`](https://github.com/etherkit/Si5351Arduino)
- [`PNGdec`](https://github.com/bitbank2/PNGdec)
- [`QRCode`](https://github.com/ricmoo/QRCode)
- Custom fonts (JetBrains Mono, Ubuntu Mono)

---

## 🖱️ Touch UI

- Tap **WSPR Freqs** to access band buttons (10 preconfigured bands)
- Long-press any band button to return to main menu
- Tap **Calibration** to adjust oscillator correction in ±10/100/1000 ppb
- Tap **Manual Entry** to use the keypad and enter any valid frequency

---

## ⚠️ Notes

- Output is **not filtered** – it's a square wave approximation of sine
- Ensure proper filtering if connecting to a sensitive RF chain
- Use a spectrum analyzer or frequency counter for output verification

---

## 📝 License

MIT License.  
Feel free to use and adapt this for your own ham radio or RF test applications.
