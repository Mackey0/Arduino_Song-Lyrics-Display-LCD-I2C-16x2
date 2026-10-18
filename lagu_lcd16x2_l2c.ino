#define USE_I2C 1          

#if USE_I2C
  #include <Wire.h>
  #include <LiquidCrystal_I2C.h>
  #define LCD_ADDR 0x27    
  LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
#else
  #include <LiquidCrystal.h>
  LiquidCrystal lcd(12, 11, 6, 5, 4, 3);   // RS, E, D4, D5, D6, D7
#endif

#define BUTTON_PIN 2       

// ====== kecepatan lagu ======
const uint16_t BPM = 108;
const unsigned long START_DELAY = 0;   // jeda (ms) setelah tombol ditekan
const unsigned long BEAT_MS = 60000UL / BPM;

//lyric
const char L0[] PROGMEM = "I want one ticket out of your heavy gaze";
const char L1[] PROGMEM = "I  want one ticket off of your carousel";
const char L2[] PROGMEM = "";
const char L3[] PROGMEM = "But you should know";
const char L4[] PROGMEM = "that I die slow";
const char L5[] PROGMEM = "Running through the halls";
const char L6[] PROGMEM = "of your haunted home";
const char L7[] PROGMEM = "And the toughest part";
const char L8[] PROGMEM = "is that we both know";
const char L9[] PROGMEM = "What happened to you";
const char L10[] PROGMEM = "Why you're out on your own";
const char L11[] PROGMEM = "";
const char L12[] PROGMEM = "Merry Christmas";
const char L13[] PROGMEM = "please don't call";
const char L14[] PROGMEM = "Merry Christmas";
const char L15[] PROGMEM = "I'm not yours at all";
const char L16[] PROGMEM = "Merry Christmas";
const char L17[] PROGMEM = "please don't call mee T-T";
const char L18[] PROGMEM = "..............";


// ... tambah L19, L20, dst

struct LyricLine {
  const char* text;
  uint8_t beats;
};

const LyricLine lyrics[] PROGMEM = {
  { L0, 7 },
  { L1, 7 },
  { L2, 1 },
  { L3, 4 },
  { L4, 4 },
  { L5, 4 },
  { L6, 4 },
  { L7, 4 },
  { L8, 4 },
  { L9, 4 },
  { L10, 4 },
  { L11, 1 },
  { L12, 4 },
  { L13, 4 },
  { L14, 4 },
  { L15, 4 },
  { L16, 4 },
  { L17, 4 },
  { L18, 20 },
  // ... tambah { L19(urutan lirik tampil), 1(lama durasi lirik tampil) }, dst
};

const uint8_t LYRIC_COUNT = sizeof(lyrics) / sizeof(lyrics[0]);
// -----------------------------------------------------------

char buf[48];
uint8_t rowCount = 1, pageCount = 1;
int8_t curPage = -1;
int8_t currentLine = -1;
unsigned long lineStart = 0, lineDur = 0;
bool running = false;


uint8_t wrapBuf() {
  uint8_t lines = 1, lineLen = 0;
  char* lastSpace = NULL;
  for (char* p = buf; *p; p++) {
    if (*p == ' ') lastSpace = p;
    lineLen++;
    if (lineLen > 16) {
      if (!lastSpace) { buf[16] = 0; return 1; }   // kata terlalu panjang
      *lastSpace = '\n';
      lines++;
      lineLen = p - lastSpace;
      lastSpace = NULL;
    }
  }
  return lines;
}


char* rowPtr(uint8_t idx, uint8_t &len) {
  char* p = buf;
  for (uint8_t i = 0; i < idx; i++) {
    char* e = strchr(p, '\n');
    if (!e) { len = 0; return NULL; }
    p = e + 1;
  }
  char* e = strchr(p, '\n');
  len = e ? (e - p) : strlen(p);
  return p;
}

// Tampilkan 2 baris pada halaman tertentu, rata tengah
void showPage(uint8_t page) {
  lcd.clear();
  for (uint8_t r = 0; r < 2; r++) {
    uint8_t len;
    char* p = rowPtr(page * 2 + r, len);
    if (!p || len == 0) continue;
    lcd.setCursor(0, r);
    //lcd.setCursor((16 - len) / 2, r);
    for (uint8_t i = 0; i < len; i++) lcd.write(p[i]);
  }
}

void loadLine(uint8_t i) {
  LyricLine cur;
  memcpy_P(&cur, &lyrics[i], sizeof(cur));
  strncpy_P(buf, cur.text, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = 0;

  rowCount  = wrapBuf();
  pageCount = (rowCount + 1) / 2;
  lineDur   = (unsigned long)cur.beats * BEAT_MS;
  curPage   = 0;
  showPage(0);
}

void showReady() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Tekan tombol"));
  lcd.setCursor(0, 1);
  lcd.print(F("untuk mulai"));
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
#if USE_I2C
  lcd.init();
  lcd.backlight();
#else
  lcd.begin(16, 2);
#endif
  showReady();
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(30);
    if (digitalRead(BUTTON_PIN) == LOW) {
      lcd.clear();
      delay(START_DELAY);
      running = true;
      currentLine = 0;
      lineStart = millis();
      loadLine(0);
      while (digitalRead(BUTTON_PIN) == LOW);
    }
  }

  if (!running) return;

  unsigned long elapsed = millis() - lineStart;

  if (elapsed >= lineDur) {
    currentLine++;
    if (currentLine >= LYRIC_COUNT) {
      running = false;
      showReady();
      return;
    }
    lineStart += lineDur;          // akumulasi agar timing tidak melenceng
    loadLine(currentLine);
  } else if (pageCount > 1) {
    // ganti halaman sesuai porsi waktu
    uint8_t page = (elapsed * pageCount) / lineDur;
    if (page != curPage) {
      curPage = page;
      showPage(page);
    }
  }
}