// =====================================================================
//  DIY KICK STOMPBOX  -  Arduino Nano (ATmega328P, 16 MHz)
//  Stomp the pedal -> a piezo detects the hit -> a kick drum sound plays.
//
//  (c) 2026 3Dvarius. Licence: CC BY-NC-ND 4.0 - you may change the
//  settings and the sounds for your own pedal, but not publish modified
//  versions or sell it.
// =====================================================================


// #####################################################################
// #####################################################################
// ###                                                               ###
// ###     >>>>>>>>>>>>>>   S E N S I T I V I T Y   <<<<<<<<<<<<<<   ###
// ###                                                               ###
// ###     The only setting most builders need to change.            ###
// ###     Write a number from 1 to 10 on the line below:            ###
// ###                                                               ###
// ###        1 = least sensitive (only hard stomps play)            ###
// ###        5 = default, tested on the prototype                   ###
// ###       10 = most sensitive (light taps play too)               ###
// ###                                                               ###
// ###     It plays by itself        ->  LOWER the number            ###
// ###     Light stomps do not play  ->  RAISE the number            ###
// ###                                                               ###
// ###     After changing it, upload the sketch again.               ###
// ###                                                               ###
// #####################################################################

#define SENSITIVITY 5

// #####################################################################
// #####################################################################


//  SOUNDS - nothing to write here: the sketch looks at the sound files
//  in its folder (next to this file) and sets the knob by itself.
//
//    sample_a.h + sample_b.h  ->  the knob BLENDS sound A <-> sound B
//                                 (as supplied: A = acoustic kick, B = 808)
//    only one of the two      ->  the knob is the VOLUME of that sound
//
//  To use your own sounds, make sample_a.h / sample_b.h with
//  sample_converter.html (in the zip), put them in this folder and upload.
//
//  WIRING  (see the wiring diagram in the build guide)
//    A0  <- piezo:  piezo(+) -> 100k -> A0 ;  1M from piezo(+) to GND ;
//           1N914 from A0 to 5V (band towards 5V) ;
//           1N914 from GND to A0 (band towards A0) ;  piezo(-) to GND
//    A1  <- wiper of a B10K linear pot (ends to 5V and GND)
//    D3  -> 1k -> node [47 nF to GND] -> 10 uF (+ towards the node)
//           -> 2.2k -> jack TIP ;  1k from jack TIP to GND ;  jack SLEEVE to GND
//           (line level output, about 0.8 V peak)
//    D4  -> 1k -> LED long leg (+) ;  LED short leg (-) to GND
//    A2  <- battery check:  switched 9 V -> 100k -> A2 -> 47k -> GND ;
//           100 nF from A2 to GND
//    VIN <- power switch <- 9 V battery (+) ;  battery (-) to GND
//
//  THE LED
//    on          = pedal on, battery OK (or powered from the USB cable)
//    slow blink  = battery low: change it soon
//    fast blink  = battery almost empty: change it now
//
//  TESTS - Serial Monitor at 115200 baud, USB connected.
//    Type a number and press Enter. The tests never change any setting.
//      1 = piezo test   (stomp and watch the numbers)
//      2 = knob test    (turn it end to end, it must go from 0 to 1023)
//      3 = jack test    (2-second beep on the output)
//      4 = play sound A           5 = play sound B
//      6 = battery test (battery switch ON: shows the battery voltage)
//      0 = stop the tests         h = help
//
//  This file and the sample files must be in a folder called "stompbox_kick".
// =====================================================================

#include <Arduino.h>
#include <avr/pgmspace.h>

#define SAMPLE_RATE 16000UL          // playback rate: the sample files are made at 16 kHz

// ---------------- which sounds are in the folder ----------------
#if !defined(__has_include)
  #error "Please update the Arduino IDE (version 1.8.10 or newer)"
#endif
#if __has_include("sample_a.h")
  #include "sample_a.h"
  #define HAS_A 1
#else
  #define HAS_A 0
#endif
#if __has_include("sample_b.h")
  #include "sample_b.h"
  #define HAS_B 1
#else
  #define HAS_B 0
#endif

#if !HAS_A && !HAS_B
  #error "No sound in the folder: put sample_a.h and/or sample_b.h next to this file (make them with sample_converter.html)"
#endif
#if HAS_A && SAMPLE_A_RATE != SAMPLE_RATE
  #error "sample_a.h is not a 16 kHz sample: make it again with sample_converter.html"
#endif
#if HAS_B && SAMPLE_B_RATE != SAMPLE_RATE
  #error "sample_b.h is not a 16 kHz sample: make it again with sample_converter.html"
#endif

// sound 1 is always present; sound 2 only when there are two files (then the knob blends)
#if HAS_A
  #define S1_DATA sampleA
  #define S1_LEN  SAMPLE_A_LENGTH
  #define S1_NAME "sample_a.h"
#else
  #define S1_DATA sampleB
  #define S1_LEN  SAMPLE_B_LENGTH
  #define S1_NAME "sample_b.h"
#endif
#if HAS_A && HAS_B
  #define BLEND 1
  #define S2_DATA sampleB
  #define S2_LEN  SAMPLE_B_LENGTH
#else
  #define BLEND 0
  #define S2_LEN  0UL
#endif
#if S1_LEN + S2_LEN > 22000
  #error "the samples are too long for the Nano memory: together they can use 22000 bytes (1.3 s)"
#endif

// ---------------- audio output pin: 3 or 11 ----------------
#define AUDIO_PIN 3

// =====================================================================
//  ADVANCED SETTINGS - you normally do not need to touch anything below.
// =====================================================================

// ---------------- trigger ----------------
// SENSITIVITY (top of the file) picks the trigger threshold from this table
// (0-1023 scale, a piezo at rest reads about 15, a stomp 200-1000):
//   SENSITIVITY   1    2    3    4    5    6    7    8    9   10
//   threshold   420  340  270  215  168  135  110   90   72   58
constexpr int thresholdFor(int s) {
  return s <= 1 ? 420 : s == 2 ? 340 : s == 3 ? 270 : s == 4 ? 215 : s == 5 ? 168 :
         s == 6 ? 135 : s == 7 ? 110 : s == 8 ?  90 : s == 9 ?  72 : 58;
}
#define CUSTOM_THRESHOLD 0         // experts: a value from 30 to 1000 here overrides SENSITIVITY (0 = off)

#if SENSITIVITY < 1 || SENSITIVITY > 10
  #error "SENSITIVITY must be a whole number from 1 to 10"
#endif
#if CUSTOM_THRESHOLD != 0 && (CUSTOM_THRESHOLD < 30 || CUSTOM_THRESHOLD > 1000)
  #error "CUSTOM_THRESHOLD must be 0 (off) or a number from 30 to 1000"
#endif
const int      THRESHOLD  = CUSTOM_THRESHOLD ? CUSTOM_THRESHOLD : thresholdFor(SENSITIVITY);

const uint16_t LOCKOUT_MS = 200;   // after a hit, no other hit for this long (200 ms = 8th notes up to 150 BPM)
const uint16_t QUIET_MS   = 30;    // then re-arm only after the piezo has been quiet this long (0 = off)
// Foot-release rejection: a real stomp rises from silence to the threshold in 2-4 ms,
// lifting a resting foot rises slowly (25-70 ms). Only fast rises trigger a hit.
const int      START_LEVEL = THRESHOLD / 3 > 25 ? THRESHOLD / 3 : 25;   // a "rise" starts above this level (1/3 of the threshold)...
const uint16_t REST_MS     = 30;   // ...after having been below it for at least this long
const uint16_t ATTACK_MS   = 6;    // the threshold must be reached within this time from the start of the rise
const uint16_t BG_MS       = 400;  // if the pedal keeps vibrating with no pause for longer than this
                                   // (shaking stage), release rejection is suspended

// ---------------- knob ----------------
const uint16_t KNOB_LOW  = 50;     // below: first end of the knob (sound A only / silent)
const uint16_t KNOB_HIGH = 973;    // above: other end (sound B only / full volume)
const uint16_t KNOB_EVERY_MS = 20; // how often the knob is read

// ---------------- power / battery LED ----------------
const uint8_t  LED_PIN       = 4;
const uint16_t BATT_LOW_MV   = 7000;   // below this: slow blink (change the battery soon)
const uint16_t BATT_EMPTY_MV = 6500;   // below this: fast blink (change it now)
const uint16_t BATT_USB_MV   = 3000;   // below this there is no battery: powered from USB, LED steady
const uint32_t DIV_TOP       = 100;    // battery divider: kOhm from the switched 9 V to A2...
const uint32_t DIV_BOTTOM    = 47;     // ...and from A2 to GND
const uint16_t VCC_MV        = 5000;   // the Nano's 5 V (ADC reference)

#define USE_LED13 1                    // the Nano's own LED (pin 13) lights while a sound plays

// ---------------------------------------------------------------------
#if AUDIO_PIN == 3
  #define AUDIO_OCR  OCR2B
  #define AUDIO_COM  _BV(COM2B1)
#elif AUDIO_PIN == 11
  #define AUDIO_OCR  OCR2A
  #define AUDIO_COM  _BV(COM2A1)
#else
  #error "AUDIO_PIN must be 3 or 11"
#endif

const uint8_t PIN_PIEZO = A0;
const uint8_t PIN_KNOB  = A1;
const uint8_t PIN_BATT  = A2;

const uint16_t TONE_INC = (uint16_t)((440UL * 65536UL) / SAMPLE_RATE);

// ---- voice state (written with interrupts off, read by the ISR) ----
volatile bool playing = false;
uint16_t idx1, idx2;    // sample positions
uint8_t  mix2;          // 0..255: amount of sound B (blend)
uint8_t  vol;           // 0..255: volume (one sound)
uint8_t  knobVal = 0;   // last knob value, 0..255
int16_t  knobRaw = 0;   // raw knob reading 0..1023 (for the knob test)

volatile uint16_t toneLeft = 0;   // test tone: samples still to play
uint16_t tonePhase = 0;

// Timer1: one audio sample every 1/SAMPLE_RATE seconds (16 kHz = 1000 CPU cycles each)
ISR(TIMER1_COMPA_vect) {
  if (toneLeft) {                    // jack test: 440 Hz square wave
    tonePhase += TONE_INC;
    AUDIO_OCR = (tonePhase & 0x8000) ? 178 : 78;
    if (--toneLeft == 0) AUDIO_OCR = 128;
    return;
  }
  if (!playing) return;              // at rest the output sits at 128 (silence)

  const uint8_t m = mix2;
  int16_t x1 = 0;
  uint16_t i = idx1;
  if (m != 255 && i < S1_LEN) { x1 = (int16_t)pgm_read_byte(&S1_DATA[i]) - 128; idx1 = ++i; }
  bool done = (m == 255) || (i >= S1_LEN);

#if BLEND
  int16_t x2 = 0;
  uint16_t j = idx2;
  if (m && j < S2_LEN) { x2 = (int16_t)pgm_read_byte(&S2_DATA[j]) - 128; idx2 = ++j; }
  done = done && ((m == 0) || (j >= S2_LEN));
  int16_t out = ((x1 * (int16_t)(255 - m)) >> 8) + ((x2 * (int16_t)m) >> 8);
#else
  int16_t out = (x1 * (int16_t)vol) >> 8;
#endif
  if (out > 127) out = 127;
  else if (out < -128) out = -128;
  AUDIO_OCR = (uint8_t)(out + 128);

  if (done) {
    playing = false;
    AUDIO_OCR = 128;
#if USE_LED13
    PORTB &= ~_BV(PB5);              // LED 13 off
#endif
  }
}

// Reads the knob and turns it into 0..255, with dead zones at both ends.
// Filters for scratchy pots: median of 5 readings, moving average, hysteresis.
static void readKnob() {
  static int16_t filt = -1;
  static int16_t used = -100;

  analogRead(PIN_KNOB);                            // channel change: discard first reading
  int16_t r[5];
  for (uint8_t n = 0; n < 5; n++) {                // 5 sorted readings
    int16_t v = analogRead(PIN_KNOB);
    int8_t j = n - 1;
    while (j >= 0 && r[j] > v) { r[j + 1] = r[j]; j--; }
    r[j + 1] = v;
  }
  int16_t med = r[2];
  knobRaw = med;

  if (filt < 0) filt = med * 4;
  else filt += med - (filt >> 2);                  // moving average (about 80 ms)
  int16_t k = filt >> 2;
  if (abs(k - used) >= 8 || k <= 2 || k >= 1021) used = k;

  if (used <= (int16_t)KNOB_LOW)       knobVal = 0;
  else if (used >= (int16_t)KNOB_HIGH) knobVal = 255;
  else knobVal = (uint8_t)(((uint32_t)(used - KNOB_LOW) * 255UL) / (KNOB_HIGH - KNOB_LOW));

  // the piezo input has a high impedance: discard readings after the channel change,
  // otherwise what is left of the knob voltage would look like a hit
  analogRead(PIN_PIEZO);
  analogRead(PIN_PIEZO);
}

// volume curve: a linear pot feels better with a squared ("audio") curve
static inline uint8_t volumeFromKnob(uint8_t k) { return (uint8_t)(((uint16_t)k * k + 254) / 255); }

static void startVoice(uint8_t m, uint8_t v) {
  noInterrupts();
  idx1 = 0; idx2 = 0;
  mix2 = m; vol = v;
  playing = true;
  interrupts();
#if USE_LED13
  PORTB |= _BV(PB5);
#endif
}

static void triggerKick() {
#if BLEND
  startVoice(knobVal, 255);
#else
  startVoice(0, volumeFromKnob(knobVal));
#endif
}

// ---- foot-release rejection: how long ago did the current rise start? ----
uint32_t riseStart = 0;
static void trackRise(int v, uint32_t now) {
  static bool     below = true;
  static uint32_t belowSince = 0;
  if (v < START_LEVEL) {
    if (!below) { below = true; belowSince = now; }
  } else if (below) {
    below = false;
    if (now - belowSince >= REST_MS) riseStart = now;   // new rise from a quiet pedal
    // otherwise it is just an oscillation of the same vibration: the rise goes on
  }
}
static inline bool attackOk(uint32_t now) {
  uint32_t age = now - riseStart;
  return age <= ATTACK_MS || age > BG_MS;
}

// =====================================================================
//  Power / battery LED
// =====================================================================
enum { BATT_UNKNOWN, BATT_USB, BATT_OK, BATT_LOW, BATT_EMPTY };
uint8_t  battState = BATT_UNKNOWN;
uint16_t battMv = 0;

static uint16_t readBatteryMv() {
  analogRead(PIN_BATT);                            // channel change: discard first reading
  uint16_t sum = 0;
  for (uint8_t n = 0; n < 4; n++) sum += analogRead(PIN_BATT);
  analogRead(PIN_PIEZO);                           // back to the piezo (see readKnob)
  analogRead(PIN_PIEZO);
  return (uint16_t)(((uint32_t)sum * VCC_MV * (DIV_TOP + DIV_BOTTOM)) / (4UL * 1023UL * DIV_BOTTOM));
}

static void batteryTask(uint32_t now) {
  static uint32_t lastRead = 0;
  static uint16_t filt = 0;
  if (now - lastRead >= 500 || battState == BATT_UNKNOWN) {
    lastRead = now;
    uint16_t mv = readBatteryMv();
    bool jump = (mv > filt + 1000) || (mv + 1000 < filt);   // battery switched on/off: no averaging
    filt = (battState == BATT_UNKNOWN || jump) ? mv : (uint16_t)((filt * 3UL + mv) / 4);   // about 2 s average
    battMv = filt;
    const uint16_t H = 150;                        // hysteresis, so the LED does not flicker between states
    if (filt < BATT_USB_MV)                                          battState = BATT_USB;
    else if (filt < BATT_EMPTY_MV)                                   battState = BATT_EMPTY;
    else if (battState == BATT_EMPTY && filt < BATT_EMPTY_MV + H)    battState = BATT_EMPTY;
    else if (filt < BATT_LOW_MV)                                     battState = BATT_LOW;
    else if (battState == BATT_LOW && filt < BATT_LOW_MV + H)        battState = BATT_LOW;
    else                                                             battState = BATT_OK;
  }
  static bool ledOn = true;
  bool on = true;
  if (battState == BATT_LOW)   on = !(now & 512);      // slow blink (about 1 per second)
  if (battState == BATT_EMPTY) on = !(now & 128);      // fast blink (about 4 per second)
  if (on != ledOn) { ledOn = on; digitalWrite(LED_PIN, on ? HIGH : LOW); }
}

// =====================================================================
//  Hardware tests (Serial Monitor)
// =====================================================================
const int      SCAN_LEVEL = 25;    // from this level up, a vibration is examined...
const uint8_t  SCAN_MS    = 4;     // ...for this long, looking for a hit
uint8_t  testMode = 0;             // 0 none, 1 piezo, 2 knob, 6 battery

static void showSettings() {
#if CUSTOM_THRESHOLD
  Serial.print(F("Settings: CUSTOM_THRESHOLD ")); Serial.println(THRESHOLD);
#else
  Serial.print(F("Settings: SENSITIVITY ")); Serial.print(SENSITIVITY);
  Serial.print(F(" (threshold ")); Serial.print(THRESHOLD); Serial.println(')');
#endif
#if BLEND
  Serial.println(F("Sounds: sample_a.h + sample_b.h   knob: blend A <-> B"));
#else
  Serial.print(F("Sound: ")); Serial.print(F(S1_NAME)); Serial.println(F("   knob: volume"));
#endif
}

static void help() {
#if BLEND
  Serial.println(F("Tests: 1 piezo | 2 knob | 3 jack (beep) | 4 sound A | 5 sound B | 6 battery | 0 stop | h help"));
#else
  Serial.println(F("Tests: 1 piezo | 2 knob | 3 jack (beep) | 4 sound | 6 battery | 0 stop | h help"));
#endif
}

// ---- hardware tests ----
static int monMax = 0, monMin = 1023;
static void monAdd(int v) { if (v > monMax) monMax = v; if (v < monMin) monMin = v; }

static void bar(int v, int full) {
  int n = (long)v * 40 / full;
  for (int i = 0; i < 40; i++) Serial.print(i < n ? '#' : '.');
}

static void testTask(uint32_t now) {
  static uint32_t last = 0;
  if (testMode == 1 && now - last >= 100) {
    last = now;
    Serial.print(F("piezo  peak ")); Serial.print(monMax);
    Serial.print(F("\tmin ")); Serial.print(monMin); Serial.print('\t');
    bar(monMax, 1023);
    if (monMax >= THRESHOLD) Serial.print(F("  <- above threshold"));
    Serial.println();
    monMax = 0; monMin = 1023;
  }
  if (testMode == 2 && now - last >= 200) {
    last = now;
    Serial.print(F("knob  ")); Serial.print(knobRaw); Serial.print('\t');
    bar(knobRaw, 1023);
#if BLEND
    if (knobVal == 0) Serial.print(F("  sound A only"));
    else if (knobVal == 255) Serial.print(F("  sound B only"));
    else { Serial.print(F("  sound B ")); Serial.print((int)knobVal * 100 / 255); Serial.print('%'); }
#else
    Serial.print(F("  volume ")); Serial.print((int)volumeFromKnob(knobVal) * 100 / 255); Serial.print('%');
#endif
    Serial.println();
  }
  if (testMode == 6 && now - last >= 500) {
    last = now;
    if (battState == BATT_USB) Serial.println(F("battery: none (powered from USB) - switch the pedal ON"));
    else {
      Serial.print(F("battery ")); Serial.print(battMv / 1000); Serial.print('.');
      Serial.print((battMv % 1000) / 100); Serial.print(F(" V  "));
      Serial.println(battState == BATT_OK ? F("OK") : battState == BATT_LOW ? F("LOW - change it soon") : F("EMPTY - change it now"));
    }
  }
}

static void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case '1': testMode = 1; monMax = 0; monMin = 1023;
                Serial.println(F("PIEZO TEST: at rest the peak must stay low (under 20-30)."));
                Serial.print(F("A stomp must go above the threshold (")); Serial.print(THRESHOLD); Serial.println(F("). 0 = stop"));
                break;
      case '2': testMode = 2;
                Serial.println(F("KNOB TEST: turn it end to end, it must go from 0 to 1023 without jumps. 0 = stop"));
                break;
      case '3': noInterrupts(); toneLeft = 2 * SAMPLE_RATE; interrupts();
                Serial.println(F("JACK TEST: 2-second beep. Nothing heard: check the output parts, the jack, the cable and the mixer."));
                break;
#if BLEND
      case '4': startVoice(0, 255);   Serial.println(F("sound A")); break;
      case '5': startVoice(255, 255); Serial.println(F("sound B")); break;
#else
      case '4': startVoice(0, 255);   Serial.println(F("sound")); break;
#endif
      case '6': testMode = 6;
                Serial.println(F("BATTERY TEST: with the pedal switched ON a new 9 V battery reads 9.0-9.6 V. 0 = stop"));
                break;
      case '0': testMode = 0; Serial.println(F("tests stopped")); help(); break;
      case 'h': case 'H': case '?': help(); break;
    }
  }
}

// =====================================================================
void setup() {
  ADCSRA = (ADCSRA & 0xF8) | 0x05;                 // faster ADC: prescaler 32 (~26 us per reading)

  pinMode(AUDIO_PIN, OUTPUT);                      // Timer2: 8-bit fast PWM at 62.5 kHz
  TCCR2A = AUDIO_COM | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS20);
  AUDIO_OCR = 128;

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);                     // power LED on
#if USE_LED13
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
#endif

  noInterrupts();                                  // Timer1: interrupt at SAMPLE_RATE
  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV(CS10);
  OCR1A  = (uint16_t)(F_CPU / SAMPLE_RATE - 1);
  TCNT1  = 0;
  TIMSK1 = _BV(OCIE1A);
  interrupts();

  Serial.begin(115200);
  Serial.println();
  Serial.println(F("DIY kick stompbox ready."));
  showSettings();
  help();

  readKnob();
  batteryTask(millis());
}

void loop() {
  static uint32_t lastTrig = (uint32_t)0 - 100000;
  static uint32_t lastLoud = (uint32_t)0 - 100000;
  static uint32_t lastKnob = 0;

  handleSerial();

  uint32_t now = millis();
  if (now - lastKnob >= KNOB_EVERY_MS) { lastKnob = now; readKnob(); }
  batteryTask(now);

  int v = analogRead(PIN_PIEZO);
  trackRise(v, now);
  if (testMode) { monAdd(v); testTask(now); }
  if (v < SCAN_LEVEL) return;

  // a vibration: follow it for a few ms and decide on every reading
  int peak = v;
  bool fired = false;
  uint32_t t0 = millis();
  do {
    uint32_t n = millis();
    bool loud  = (v >= THRESHOLD);
    bool armed = (n - lastTrig >= LOCKOUT_MS) && (n - lastLoud >= QUIET_MS);
    if (loud) lastLoud = n;
    if (armed && loud && !fired && attackOk(n)) { triggerKick(); lastTrig = n; fired = true; }
    if (v > peak) peak = v;
    v = analogRead(PIN_PIEZO);
    trackRise(v, millis());
    if (testMode) monAdd(v);
  } while (millis() - t0 < SCAN_MS);

  if (fired && !testMode) { Serial.print(F("kick  (strength ")); Serial.print(peak); Serial.println(')'); }
}
