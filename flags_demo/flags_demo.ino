// Signed comparison flags demo for Arduino Uno R3 (ATmega328P, AVR)
//
// Idea: do a real 8-bit compare (CP = subtract, throw away result, keep flags),
// grab SREG right after, and check that the ARM-style conditions built from
// N, Z, V give the same answer as the normal C++ operators.
//
// Open Serial Monitor at 9600 baud.

// LED Output Pins
const uint8_t LED_N = 5;  // Negative flag LED
const uint8_t LED_Z = 4;  // Zero flag LED
const uint8_t LED_V = 3;  // Overflow flag LED

// SREG bit positions on AVR
const uint8_t BIT_C = 0;  // carry (borrow after a subtraction on AVR)
const uint8_t BIT_Z = 1;  // zero
const uint8_t BIT_N = 2;  // negative (sign bit of result)
const uint8_t BIT_V = 3;  // signed overflow
const uint8_t BIT_S = 4;  // sign flag = N XOR V (AVR precomputes it)

// Helper function to flush any left-over newline/carriage return characters
// and wait for fresh user input before continuing.
void waitForUserInput() {
  // Clear out any trailing bytes in the serial buffer (like \r or \n)
  while (Serial.available() > 0) {
    Serial.read();
  }
  
  // Wait for the user to send a new character/key press
  while (Serial.available() == 0) {
    // Wait for input
  }
}

// Runs a - b on the CPU and returns SREG right after the compare.
uint8_t cpuCompare(int8_t a, int8_t b) {
  uint8_t sreg;
  asm volatile(
    "cp %1, %2        \n\t"   // computes a - b, sets flags, discards result
    "in %0, __SREG__  \n\t"   // read the status register immediately
    : "=r"(sreg)
    : "r"(a), "r"(b)
  );
  return sreg;
}

bool bitOf(uint8_t sreg, uint8_t bit) {
  return (sreg >> bit) & 1;
}

void runCase(int8_t a, int8_t b) {
  uint8_t sreg = cpuCompare(a, b);

  bool C = bitOf(sreg, BIT_C);
  bool Z = bitOf(sreg, BIT_Z);
  bool N = bitOf(sreg, BIT_N);
  bool V = bitOf(sreg, BIT_V);
  bool S = bitOf(sreg, BIT_S);

  // Update physical LEDs on digital ports 5, 4, and 3
  digitalWrite(LED_N, N ? HIGH : LOW);
  digitalWrite(LED_Z, Z ? HIGH : LOW);
  digitalWrite(LED_V, V ? HIGH : LOW);

  // The four conditions straight from the table (only Z, N, V used)
  bool LT = (N != V);
  bool LE = (Z == 1) || (N != V);
  bool GT = (Z == 0) && (N == V);
  bool GE = (N == V);

  // What C++ says the answer really is
  bool okLT = (LT == (a <  b));
  bool okLE = (LE == (a <= b));
  bool okGT = (GT == (a >  b));
  bool okGE = (GE == (a >= b));

  Serial.print(F("a = ")); Serial.print(a);
  Serial.print(F(", b = ")); Serial.print(b);
  Serial.print(F("   (true a-b = ")); Serial.print((int)a - (int)b);
  Serial.print(F(", 8-bit result = ")); Serial.print((int8_t)(a - b));
  Serial.println(F(")"));

  Serial.print(F("  Z=")); Serial.print(Z);
  Serial.print(F(" N=")); Serial.print(N);
  Serial.print(F(" V=")); Serial.print(V);
  Serial.print(F("   (C=")); Serial.print(C);
  Serial.print(F(" S=")); Serial.print(S);
  Serial.println(F(")"));

  Serial.print(F("  LT=")); Serial.print(LT);
  Serial.print(F(" LE=")); Serial.print(LE);
  Serial.print(F(" GT=")); Serial.print(GT);
  Serial.print(F(" GE=")); Serial.print(GE);
  Serial.print(F("   -> "));
  Serial.println((okLT && okLE && okGT && okGE) ? F("all match C++") : F("MISMATCH"));

  // The "why not just use N?" moment
  bool naiveLT = N;
  Serial.print(F("  N alone says a<b is ")); Serial.print(naiveLT);
  Serial.print(F(", truth is ")); Serial.print(a < b);
  if (naiveLT != (a < b)) Serial.print(F("   <-- overflow fooled N!"));
  Serial.println();
  Serial.println();
}

void setup() {
  // Initialize LED indicator pins as outputs
  pinMode(LED_N, OUTPUT);
  pinMode(LED_Z, OUTPUT);
  pinMode(LED_V, OUTPUT);

  // Turn off all LEDs at startup
  digitalWrite(LED_N, LOW);
  digitalWrite(LED_Z, LOW);
  digitalWrite(LED_V, LOW);

  Serial.begin(9600);
  while (!Serial) { }

  Serial.println(F("=== Signed compare flags on RISC ==="));
  Serial.println(F("Press Send or Enter in Serial Monitor to start...\n"));
  
  waitForUserInput();

  for (int i = 0; i < 8; i++) {

    switch (i) {
      case 0:
        runCase(5, 10);       // plain a < b, no overflow;
        break;
      case 1:
        runCase(10, 10);      // equal, Z = 1;
        break;
      case 2:
        runCase(10, 5);       // plain a > b;
        break;
      case 3:
        runCase(-5, -10);     // negatives, no overflow;
        break;
      case 4:
        runCase(100, -100);   // 200 doesn't fit in int8: wraps negative, N=1 V=1;
        break;
      case 5:
        runCase(-100, 100);   // -200 wraps positive, N=0 V=1, yet a < b;
        break;
      case 6:
        runCase(-128, 1);     // extreme case, overflow again;
        break;
      case 7:
        runCase(127, -1);     // 128 doesn't fit, overflow again;
        break; 
    }

    if (i < 7) {
      Serial.println(F("[Press Enter for next case...]"));
      waitForUserInput();
    }
  }

  Serial.println(F("Done."));
}

void loop() {
  // nothing, everything runs once in setup()
}