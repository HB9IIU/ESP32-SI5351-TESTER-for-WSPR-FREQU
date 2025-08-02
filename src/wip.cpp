#include <Wire.h>
#include <si5351.h>
#include <TFT_eSPI.h>
#include <JetBrainsMono_Light13pt7b.h>
#include <JetBrainsMono_Bold15pt7b.h>
#include <JetBrainsMono_Bold11pt7b.h>
#include <PNGdec.h>
#include "fancySplash.h" // Image is stored here in an 8-bit array  <https://notisrac.github.io/FileToCArray/ >(select treat as binary)
#define SI5351_SDA 25
#define SI5351_SCL 26
#define TFT_BLP 4

// Instances
TFT_eSPI tft = TFT_eSPI();
PNG png;
Si5351 si5351;

struct WSPRBand
{
  uint64_t frequencyHz;
};

WSPRBand bands[] = {
    {1836600},
    {3568600},
    {7038600},
    {10138700},
    {28124600},
    {14095600},
    {18104600},
    {21094600},
    {24924600},
    {50293000}};

int selectedBand = -1;
// Function Prototypes
bool si5351CheckModule();
void setCLK0freqMHz(float freqMHz);
void drawFrequency(uint64_t freqHz, int x, int y, uint16_t textColor, uint16_t bgColor);
void drawBandButtons();
void checkTouch();
void displaySplashScreen();
void pngDraw(PNGDRAW *pDraw);

void setup()
{
  Serial.begin(115200);

  tft.init();
  tft.setRotation(3); // Landscape
  displaySplashScreen();
  pinMode(TFT_BLP, OUTPUT);
  digitalWrite(TFT_BLP, HIGH);
  delay(2500);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GOLD, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(&JetBrainsMono_Bold15pt7b);
  tft.drawString("SI5351 Tester", tft.width() / 2, tft.height() / 2 / 2);

  Wire.begin(SI5351_SDA, SI5351_SCL);

  if (si5351CheckModule())
  {
    Serial.println("✅ Si5351 detected and initialized.");
    tft.setFreeFont(&JetBrainsMono_Light13pt7b);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawCentreString("Si5351 FOUND!", tft.width() / 2, tft.height() - 30, 1);
    delay(1500);
    selectedBand = 5; // Default to 20m
    drawBandButtons();
    setCLK0freqMHz(bands[selectedBand].frequencyHz / 1000000.0f);
    drawBandButtons();
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
  checkTouch();
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
  const int btnWidth = 126, btnHeight = 34, spacingY = 12;
  const int col1X = 15, col2X = 165;

  // Optionally clear full area first
  tft.fillRect(0, 0, tft.width(), tft.height(), TFT_BLACK);

  for (int i = 0; i < 10; i++)
  {
    int row = i % 5;
    int col = i / 5;
    int x = (col == 0) ? col1X : col2X;
    int y = row * (btnHeight + spacingY) + 10;
    bool isSelected = (i == selectedBand);

    // Clear previous button area completely
    tft.fillRect(x, y, btnWidth, btnHeight, TFT_BLACK);

    // Draw button background
    tft.fillRoundRect(x, y, btnWidth, btnHeight, 5, isSelected ? TFT_GREEN : TFT_DARKGREY);

    // Draw only the frequency
    tft.setTextColor(TFT_WHITE, isSelected ? TFT_GREEN : TFT_DARKGREY);
    tft.setFreeFont(&JetBrainsMono_Bold11pt7b);
    tft.setTextDatum(MC_DATUM);
    drawFrequency(bands[i].frequencyHz, x + btnWidth / 2, y + btnHeight / 2,
                  TFT_WHITE, isSelected ? TFT_GREEN : TFT_DARKGREY);
  }
}

void checkTouch()
{
  uint16_t x, y_raw;
  if (tft.getTouch(&x, &y_raw))
  {
    // Invert Y axis if necessary (uncomment next line if Y is flipped)
    int y = tft.height() - y_raw;
    // int y = y_raw;

    const int btnWidth = 126;
    const int btnHeight = 34;
    const int spacingY = 12;
    const int col1X = 15;
    const int col2X = 165;

    for (int i = 0; i < 10; i++)
    {
      int row = i % 5;
      int col = i / 5;
      int bx = (col == 0) ? col1X : col2X;
      int by = row * (btnHeight + spacingY) + 10;

      if (x >= bx && x <= bx + btnWidth &&
          y >= by && y <= by + btnHeight)
      {
        if (i != selectedBand)
        {
          selectedBand = i;
          drawBandButtons();
          setCLK0freqMHz(bands[i].frequencyHz / 1000000.0f);
        }
        break;
      }
    }
  }
}
