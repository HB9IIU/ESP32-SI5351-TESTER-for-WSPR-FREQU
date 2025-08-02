#include <Wire.h>
#include <si5351.h>
#include <TFT_eSPI.h>
#include <JetBrainsMono_Light13pt7b.h>
#include <JetBrainsMono_Bold15pt7b.h>
#include <JetBrainsMono_Bold11pt7b.h>
#include <UbuntuMono_Regular8pt7b.h>
#include <Preferences.h>
#include <PNGdec.h>
#include "fancySplash.h" // Image is stored here in an 8-bit array  <https://notisrac.github.io/FileToCArray/ >(select treat as binary)
#include "qrcode.h" 
#define SI5351_SDA 25
#define SI5351_SCL 26
#define TFT_BLP 4

// Instances
TFT_eSPI tft = TFT_eSPI();
PNG png;
Si5351 si5351;
Preferences prefs;

struct WSPRBand
{
  uint64_t frequencyHz;
};

WSPRBand bands[] = {
    {1836600},
    {3568600},
    {7038600},
    {10138700},
    {14095600},
    {18104600},
    {21094600},
    {24924600},
    {28124600},

    {50293000}};

int selectedBand = -1;
int currentPage = 0;
int lastPage = -1;
int32_t correctionPpb = 0; // Global: current correction applied

// Function Prototypes
bool si5351CheckModule();
void setCLK0freqMHz(float freqMHz);
void drawFrequency(uint64_t freqHz, int x, int y, uint16_t textColor, uint16_t bgColor);
void drawBandButtons();
void checkTouchBandSelectionPage();
void displaySplashScreen();
void displayQRcodeScreen();
void pngDraw(PNGDRAW *pDraw);
void drawMainPage();
void drawCalibrationPage();
void checkTouchMainMenuPage();
void checkTouchCalibrationPage();
void drawFrequencyEntryPage();
String formatWithSwissSeparator(int32_t value);
String frequencyInputStr = "";
void checkTouchFrequencyEntryPage();
void drawAboutPage();
void checkTouchAboutPage();


void setup()
{
  Serial.begin(115200);

  tft.init();
  tft.setRotation(3); // Landscape
  displaySplashScreen();
  pinMode(TFT_BLP, OUTPUT);
  digitalWrite(TFT_BLP, HIGH);
  delay(1500);

  Wire.begin(SI5351_SDA, SI5351_SCL);

  if (si5351CheckModule())
  {
    Serial.println("✅ Si5351 detected and initialized.");
    prefs.begin("si5351", true); // Read-only
    correctionPpb = prefs.getInt("corr", 0);
    prefs.end();
    Serial.printf("Loaded correction: %ld ppb\n", correctionPpb);
    si5351.set_correction(correctionPpb, SI5351_PLL_INPUT_XO);
  }
  else
  {
    Serial.println("❌ Si5351 not found on I2C bus.");
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setFreeFont(&JetBrainsMono_Light13pt7b);
    tft.drawCentreString("Si5351 NOT FOUND!", tft.width() / 2, tft.height() / 4 * 3 - 20, 1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setFreeFont(&JetBrainsMono_Bold11pt7b);
    tft.drawCentreString("SDA->25  SCL->26", tft.width() / 2, tft.height() / 4 * 3 + 10, 1);
    while (1)
      ;
  }
}

void loop()
{
  if (currentPage != lastPage)
  {
    lastPage = currentPage;
    switch (currentPage)
    {
    case 0:
      drawMainPage();
      break;
    case 1:
      drawBandButtons();
      break;
    case 2:
      drawCalibrationPage();
      break;
    case 3:
      drawFrequencyEntryPage();
      break;
    case 4:
      drawAboutPage();
      break;
    }
  }

  // Touch handling for active page
  switch (currentPage)
  {
  case 0:
    checkTouchMainMenuPage();
    break;
  case 1:
    checkTouchBandSelectionPage();
    break;
  case 2:
    checkTouchCalibrationPage();
    break;
  case 3:
    checkTouchFrequencyEntryPage();
    break;
  case 4:
    checkTouchAboutPage();
    break;
  }
}



void displaySplashScreen()
{
  digitalWrite(TFT_BLP, LOW);

  // https://notisrac.github.io/FileToCArray/
  int16_t rc = png.openFLASH((uint8_t *)fancySplash, sizeof(fancySplash), pngDraw);

  if (rc == PNG_SUCCESS)
  {

    Serial.println("Successfully opened png file");
    Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
    tft.startWrite();
    uint32_t dt = millis();
    rc = png.decode(NULL, 0);
    Serial.print("Displayed in ");
    Serial.print(millis() - dt);
    Serial.println(" ms");
    tft.endWrite();
  }

  digitalWrite(TFT_BLP, HIGH);
}



void displayQRcodeScreen()
{
  digitalWrite(TFT_BLP, LOW);

  // https://notisrac.github.io/FileToCArray/
  int16_t rc = png.openFLASH((uint8_t *)qrcode, sizeof(qrcode), pngDraw);

  if (rc == PNG_SUCCESS)
  {

    Serial.println("Successfully opened png file");
    Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
    tft.startWrite();
    uint32_t dt = millis();
    rc = png.decode(NULL, 0);
    Serial.print("Displayed in ");
    Serial.print(millis() - dt);
    Serial.println(" ms");
    tft.endWrite();
  }

  digitalWrite(TFT_BLP, HIGH);
}



void pngDraw(PNGDRAW *pDraw)
{
  uint16_t lineBuffer[480];
  png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
  tft.pushImage(0, 0 + pDraw->y, pDraw->iWidth, 1, lineBuffer);
}

bool si5351CheckModule()
{
  return si5351.init(SI5351_CRYSTAL_LOAD_8PF, 25000000, 0);
}

void setCLK0freqMHz(float freqMHz)
{
  uint64_t freq_hundredths_Hz = (uint64_t)(freqMHz * 1e8);
  si5351.set_freq(freq_hundredths_Hz, SI5351_CLK0);
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);
  si5351.output_enable(SI5351_CLK0, 1);
  si5351.update_status();

  Serial.print("CLK0 set to ");
  Serial.print(freqMHz, 6);
  Serial.println(" MHz");
}

void drawFrequency(uint64_t freqHz, int x, int y, uint16_t textColor, uint16_t bgColor)
{
  uint32_t MHz = freqHz / 1000000;
  uint32_t frac = freqHz % 1000000;

  char buf[20];
  sprintf(buf, "%lu.%06lu", MHz, frac);

  tft.setTextColor(textColor, bgColor);
  tft.drawString(buf, x, y);
}

void drawBandButtons()
{
  const int btnWidth = 132, btnHeight = 34, spacingY = 11;
  const int col1X = 15, col2X = 165;

  tft.fillRect(0, 0, tft.width(), tft.height(), TFT_BLACK);

  for (int i = 0; i < 10; i++)
  {
    int row = i % 5;
    int col = i / 5;
    int x = (col == 0) ? col1X : col2X;
    int y = row * (btnHeight + spacingY) + 2;
    bool isSelected = (i == selectedBand);

    // Button background
    uint16_t bgColor = isSelected ? TFT_GREEN : TFT_DARKGREY;
    uint16_t textColor = isSelected ? TFT_BLACK : TFT_WHITE;

    // Fill background
    tft.fillRoundRect(x, y, btnWidth, btnHeight, 5, bgColor);

    // 1-pixel white frame
    tft.drawRoundRect(x, y, btnWidth, btnHeight, 5, TFT_WHITE);

    // Frequency text
    tft.setTextColor(textColor, bgColor);
    tft.setFreeFont(&JetBrainsMono_Bold11pt7b);
    tft.setTextDatum(MC_DATUM);
    drawFrequency(bands[i].frequencyHz, x + btnWidth / 2, y + btnHeight / 2, textColor, bgColor);
  }

  // Bottom info text
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("Long Press to Exit", 160, 225, 1);
}
void checkTouchBandSelectionPage()
{
  uint16_t x, y_raw;
  if (tft.getTouch(&x, &y_raw))
  {
    int y = tft.height() - y_raw;

    const int btnWidth = 132;
    const int btnHeight = 34;
    const int spacingY = 11;
    const int col1X = 15;
    const int col2X = 165;
    const unsigned long longPressThreshold = 1000; // milliseconds

    for (int i = 0; i < 10; i++)
    {
      int row = i % 5;
      int col = i / 5;
      int bx = (col == 0) ? col1X : col2X;
      int by = row * (btnHeight + spacingY) + 2;

      if (x >= bx && x <= bx + btnWidth && y >= by && y <= by + btnHeight)
      {
        // Start timing the press
        unsigned long startTime = millis();

        // Wait while still pressing
        while (tft.getTouch(&x, &y_raw))
        {
          delay(10);
        }

        unsigned long pressDuration = millis() - startTime;

        if (pressDuration >= longPressThreshold)
        {
          Serial.println("📴 Long press detected — returning to main page");
          currentPage = 0;
        }
        else
        {
          if (i != selectedBand)
          {
            selectedBand = i;
            drawBandButtons();
            setCLK0freqMHz(bands[i].frequencyHz / 1000000.0f);
          }
        }
        break;
      }
    }
  }
}

void drawMainPage()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setFreeFont(&JetBrainsMono_Light13pt7b);
  tft.setTextDatum(MC_DATUM);
  tft.drawCentreString("✅ Si5351 Found & Ready", tft.width() / 2, 2, 1);
  tft.setFreeFont(&UbuntuMono_Regular8pt7b);

  tft.setTextColor(TFT_GOLD, TFT_BLACK);

  String corrStr = formatWithSwissSeparator(correctionPpb);
  String line = "calfactor applied: " + corrStr + " ppb";
  tft.drawCentreString(line, tft.width() / 2, 33, 1);
  // Button layout
  const int btnWidth = 200;
  const int btnHeight = 40;
  const int spacingY = 11;
  const int startY = 58;
  const int cornerRadius = 6;

  struct
  {
    const char *label;
    int id;
  } buttons[] = {
      {"WSPR Freqs", 1},
      {"Calibration", 2},
      {"Manual Entry", 3}};

  tft.setTextDatum(MC_DATUM); // Centered text

  for (int i = 0; i < 3; i++)
  {
    int x = (tft.width() - btnWidth) / 2;
    int y = startY + i * (btnHeight + spacingY);

    // Button body
    tft.fillRoundRect(x, y, btnWidth, btnHeight, cornerRadius, TFT_NAVY);

    // 1-pixel rounded border
    tft.drawRoundRect(x, y, btnWidth, btnHeight, cornerRadius, TFT_WHITE);

    // Button label
    tft.setFreeFont(&JetBrainsMono_Bold11pt7b);
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.drawCentreString(buttons[i].label, tft.width() / 2, y + btnHeight / 2 - 10, 1);
  }
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setFreeFont(&UbuntuMono_Regular8pt7b);
  tft.drawCentreString("About...", tft.width() / 2, 215, 1);
}
void checkTouchMainMenuPage()
{
  uint16_t x, y_raw;
  if (tft.getTouch(&x, &y_raw))
  {
    int y = tft.height() - y_raw; // Flip Y axis if needed

    // ---- Main buttons area ----
    const int btnWidth = 200;
    const int btnHeight = 40;
    const int spacingY = 11;
    const int startY = 58;

    for (int i = 0; i < 3; i++)
    {
      int bx = (tft.width() - btnWidth) / 2;
      int by = startY + i * (btnHeight + spacingY);

      if (x >= bx && x <= bx + btnWidth &&
          y >= by && y <= by + btnHeight)
      {
        currentPage = i + 1; // Pages: 1 = WSPR, 2 = Calib, 3 = Manual
        Serial.printf("Page changed to %d\n", currentPage);
        return;
      }
    }

    // ---- About... label at bottom ----
    const int aboutY = 215;       // Y position of "About..." label
    const int aboutH = 16;        // Estimated height of text
    const int textWidth = 160;    // Approximate clickable width
    const int centerX = tft.width() / 2;

    if (x >= centerX - textWidth / 2 && x <= centerX + textWidth / 2 &&
        y >= aboutY - 5 && y <= aboutY + aboutH + 5)
    {
      currentPage = 4; // About page
      Serial.println("Navigating to About page");
      delay(150);
    }
  }
}


void drawCalibrationPage()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);

  // Header / instructions
  tft.setFreeFont(&JetBrainsMono_Bold11pt7b);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString("14.0 MHz on CLK0", tft.width() / 2, 20, 1);
  tft.setFreeFont(&UbuntuMono_Regular8pt7b);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("Use freq counter or rig display", tft.width() / 2, 50, 1);
  tft.drawCentreString("to measure the actual frequency.", tft.width() / 2, 70, 1);

  // Start output at 14 MHz
  setCLK0freqMHz(14.0f); // Already defined
  si5351.set_correction(correctionPpb, SI5351_PLL_INPUT_XO);

  // Correction value display
  tft.setFreeFont(&JetBrainsMono_Bold11pt7b);
  tft.setTextColor(TFT_GOLD, TFT_BLACK);
  String corrStr = formatWithSwissSeparator(correctionPpb);
  String line = "Correction: " + corrStr + " ppb";
  tft.drawCentreString(line, tft.width() / 2, 100, 1);

  // Correction buttons
  const char *labels[6] = {"<<<", "<<", "<", ">", ">>", ">>>"};
  const int values[6] = {-1000, -100, -10, +10, +100, +1000};
  const int btnW = 48, btnH = 35, spacing = 5;
  const int startX = (tft.width() - (6 * btnW + 5 * spacing)) / 2;
  const int y = 140;

  tft.setFreeFont(&JetBrainsMono_Bold11pt7b);

  for (int i = 0; i < 6; i++)
  {
    int x = startX + i * (btnW + spacing);
    tft.fillRoundRect(x, y, btnW, btnH, 5, TFT_DARKGREY);
    tft.drawRoundRect(x, y, btnW, btnH, 5, TFT_WHITE);
    tft.drawRoundRect(x + 1, y + 1, btnW - 2, btnH - 2, 5, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft.drawCentreString(labels[i], x + btnW / 2, y + btnH / 2 - 9, 1);
  }
  // Draw return button
  const int retBtnW = 120;
  const int retBtnH = 34;
  const int retX = (tft.width() - retBtnW) / 2;
  const int retY = tft.height() - retBtnH - 10;

  tft.fillRoundRect(retX, retY, retBtnW, retBtnH, 6, TFT_NAVY);
  tft.drawRoundRect(retX, retY, retBtnW, retBtnH, 6, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setFreeFont(&JetBrainsMono_Bold11pt7b);
  tft.drawCentreString("Return", tft.width() / 2, retY + retBtnH / 2 - 11, 1);
}

void checkTouchCalibrationPage()
{
  uint16_t x, y_raw;
  if (tft.getTouch(&x, &y_raw))
  {
    int y = tft.height() - y_raw; // Flip if needed

    // --- Correction buttons ---
    const int btnW = 48, btnH = 35, spacing = 5;
    const int startX = (tft.width() - (6 * btnW + 5 * spacing)) / 2;
    const int btnY = 140;
    const int values[6] = {+1000, 100, +10, -10, -100, -1000};

    for (int i = 0; i < 6; i++)
    {
      int bx = startX + i * (btnW + spacing);
      if (x >= bx && x <= bx + btnW &&
          y >= btnY && y <= btnY + btnH)
      {
        // Apply correction
        correctionPpb += values[i];
        si5351.set_correction(correctionPpb, SI5351_PLL_INPUT_XO);
        // Save to flash
        prefs.begin("si5351", false);
        prefs.putInt("corr", correctionPpb);
        prefs.end();

        Serial.printf("Applied correction: %ld ppb (saved)\n", correctionPpb);

        setCLK0freqMHz(14.0f);

        // Redraw only the correction display
        tft.fillRect(0, 95, tft.width(), 30, TFT_BLACK); // Clear area
        tft.setFreeFont(&JetBrainsMono_Bold11pt7b);
        tft.setTextColor(TFT_GOLD, TFT_BLACK);
        String corrStr = formatWithSwissSeparator(correctionPpb);
        String line = "Correction: " + corrStr + " ppb";
        tft.drawCentreString(line, tft.width() / 2, 100, 1);
        delay(150); // Basic debounce
        return;
      }
    }

    // --- Return button ---
    const int retBtnW = 120;
    const int retBtnH = 34;
    const int retX = (tft.width() - retBtnW) / 2;
    const int retY = tft.height() - retBtnH - 10;

    if (x >= retX && x <= retX + retBtnW &&
        y >= retY && y <= retY + retBtnH)
    {
      currentPage = 0;
      Serial.println("Returning to main menu");
      delay(200); // Debounce
    }
  }
}

String formatWithSwissSeparator(int32_t value)
{
  char temp[20];
  sprintf(temp, "%ld", value);

  String str = temp;
  String out = "";

  int len = str.length();
  int start = (str[0] == '-') ? 1 : 0;
  int count = 0;

  for (int i = len - 1; i >= start; i--)
  {
    out = str[i] + out;
    count++;
    if (count % 3 == 0 && i > start)
    {
      out = "'" + out; // Swiss style: apostrophe as separator
    }
  }

  if (start == 1)
    out = "-" + out;

  return out;
}

void drawFrequencyEntryPage()
{
  tft.fillScreen(TFT_NAVY);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(&JetBrainsMono_Bold15pt7b);

  // Calculator-style display boxfillRect
  tft.drawRoundRect(35, 4, 250, 40, 6, TFT_WHITE);
  tft.fillRoundRect(36, 5, 248, 38, 6, TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  // Format frequency with Swiss-style thousand separator
  uint64_t freq = frequencyInputStr.toInt();
  String formattedFreq = formatWithSwissSeparator(freq);
  tft.drawCentreString(formattedFreq + " Hz", tft.width() / 2, 8, 1);

  // Keypad layout
  const char *keys[12] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "C", "0", "OK"};
  int btnW = 77, btnH = 40, spacingX = 10, spacingY = 10;
  int startX = (tft.width() - (3 * btnW + 2 * spacingX)) / 2;
  int startY = 50;

  tft.setFreeFont(&JetBrainsMono_Bold11pt7b);
  for (int i = 0; i < 12; i++)
  {
    int col = i % 3;
    int row = i / 3;
    int x = startX + col * (btnW + spacingX);
    int y = startY + row * (btnH + spacingY);

    tft.fillRoundRect(x, y, btnW, btnH, 5, TFT_DARKGREY);
    tft.drawRoundRect(x, y, btnW, btnH, 5, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft.drawCentreString(keys[i], x + btnW / 2, y + btnH / 2 - 9, 1);
  }
}

void checkTouchFrequencyEntryPage()
{
  uint16_t x, y_raw;
  if (!tft.getTouch(&x, &y_raw))
    return;

  int y = tft.height() - y_raw;

  const int btnW = 77, btnH = 40, spacingX = 10, spacingY = 10;
  const int startX = (tft.width() - (3 * btnW + 2 * spacingX)) / 2;
  const int startY = 50;

  const char *keys[12] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "C", "0", "OK"};

  for (int i = 0; i < 12; i++)
  {
    int col = i % 3;
    int row = i / 3;
    int bx = startX + col * (btnW + spacingX);
    int by = startY + row * (btnH + spacingY);

    if (x >= bx && x <= bx + btnW && y >= by && y <= by + btnH)
    {
      const char *key = keys[i];

      if (strcmp(key, "C") == 0)
      {
        frequencyInputStr = "0";
      }
      else if (strcmp(key, "OK") == 0)
      {
        if (frequencyInputStr.length() > 0)
        {
          uint64_t freqHz = strtoull(frequencyInputStr.c_str(), NULL, 10);

          if (freqHz >= 8000 && freqHz <= 160000000)
          {
            float freqMHz = freqHz / 1000000.0f;
            Serial.printf("✅ Setting CLK0 to %llu Hz (%.6f MHz)\n", freqHz, freqMHz);
            setCLK0freqMHz(freqMHz);
            currentPage = 0; // Return to main
            return;
          }
          else
          {
            Serial.printf("❌ Invalid frequency entered: %llu Hz\n", freqHz);

            // Show error in red
            tft.fillRoundRect(36, 5, 248, 38, 6, TFT_BLACK);
            tft.setTextColor(TFT_RED, TFT_BLACK);
            tft.setFreeFont(&JetBrainsMono_Bold15pt7b);
            tft.setTextDatum(MC_DATUM);
            tft.drawCentreString("Out of range!", tft.width() / 2, 8, 1);

            delay(2000); // Wait 2 seconds

            // Reset to "0"
            frequencyInputStr = "0";
          }
        }
      }
      else
      {
        if (frequencyInputStr == "0")
          frequencyInputStr = ""; // prevent leading zero
        if (frequencyInputStr.length() < 10)
          frequencyInputStr += key;
      }

      // Redraw frequency display
      tft.fillRoundRect(36, 5, 248, 38, 6, TFT_BLACK);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.setFreeFont(&JetBrainsMono_Bold15pt7b);
      tft.setTextDatum(MC_DATUM);
      uint64_t freq = strtoull(frequencyInputStr.c_str(), NULL, 10);
      String formattedFreq = formatWithSwissSeparator(freq);
      tft.drawCentreString(formattedFreq + " Hz", tft.width() / 2, 8, 1);

      delay(150); // Basic debounce
      return;
    }
  }
}

void drawAboutPage()
{
 displayQRcodeScreen();
  
}

void checkTouchAboutPage()
{
  uint16_t x, y_raw;
  if (tft.getTouch(&x, &y_raw))
  {
    Serial.println("Touch detected on About page, returning to main");
    currentPage = 0;
    delay(200);
  }
}
