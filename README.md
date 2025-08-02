# 📡 WSPR Frequency Tester for ESP32 + Si5351 + TFT

> Publishing this app on the fly because my friend **Henryk** asked me to prepare it via an early WhatsApp message today 😄  
> His plan is to develop a Power Amplifier driven by vapours of red beet cooking.  
> Also, I have nothing clever to do this morning — and luckily the weather is bad,  
> so my wife can't send me out to work in the garden

This project is a touch-enabled frequency selector for WSPR bands, built using an **ESP32**, **Si5351 clock generator**, and a **320×240 TFT display**. It allows you to quickly select and transmit on any standard WSPR frequency (160m to 6m, including 50 MHz) via CLK0 of the Si5351.

---

## ✨ Features

- ✅ WSPR band selection (160m to 6m + 50 MHz)
- 🎨 Clean touchscreen UI with 2-column button layout
- 🖼️ PNG splash screen on startup
- 🔄 Visual feedback for selected band and frequency
- 📶 Si5351 output on CLK0 with maximum drive strength

---

## 🛠️ Hardware Required

- [x] ESP32 Dev Board (e.g., DevKitC)
- [x] Si5351 I2C clock generator module
- [x] 320×240 TFT display (e.g., ILI9341 / ILI9488)
- [x] Optional: Backlight control via GPIO

---

## 📚 Libraries Used

- [`TFT_eSPI`](https://github.com/Bodmer/TFT_eSPI)
- [`Si5351Arduino`](https://github.com/etherkit/Si5351Arduino)
- [`PNGdec`](https://github.com/bitbank2/PNGdec)
- Custom fonts: JetBrains Mono

---

## 🖱️ Touch Layout

- Buttons arranged in **2 columns** for optimal screen usage
- Tap to select a band and instantly transmit on the corresponding WSPR frequency

---

## 📝 License

MIT License. Feel free to modify and use for your own amateur radio projects.
