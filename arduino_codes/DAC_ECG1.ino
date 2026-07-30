/* =======================================================
   ECG Reader + Filtro Paso-Alto + Salida por DAC de
   6 bits (PORTD, pines 2..7)
   ===================================================== */

// Frecuencia de muestreo (Hz)
#define SAMPLE_RATE      250
#define SAMPLE_PERIOD_MS (1000 / SAMPLE_RATE)

// Filtro paso-alto digital (baseline removal)
float alpha = 0.975f;   // controla frecuencia de corte
float y_lp_prev = 0.0f; // estado anterior de filtro pasa-bajo

unsigned long lastTime = 0;

void setup() {
  // Configura PORTD todos como salidas
  DDRD |= B11111111; // D0..D7 como salida

  // Serial para depuración (opcional)
  Serial.begin(115200);
  Serial.println("ECG + High-Pass Filter + 6-bit DAC");
}

void loop() {
  unsigned long now = millis();
  if (now - lastTime >= SAMPLE_PERIOD_MS) {
    lastTime = now;

    // --- 1) Leer señal ECG ---
    int rawADC = analogRead(A0);
    // Mapea 0-1023 a voltaje (0..5V)
    float vin = (rawADC / 1023.0f) * 5.0f;

    // --- 2) Filtro paso-alto digital ---
    float y_lp = alpha * y_lp_prev + (1.0f - alpha) * vin;
    float y_hp = vin - y_lp;
    y_lp_prev = y_lp;

    // --- 3) Centrar y escalar para DAC 6 bits (0..63) ---
    // Ajusta tensión para evitar valores negativos
    // ECG filtrado puede ser positivo o negativo:
    float shifted = y_hp + 2.5f;   // centrar en 2.5V
    if (shifted < 0.0f) shifted = 0.0f;
    if (shifted > 5.0f) shifted = 5.0f;

    // Mapea 0..5V → 0..63
    uint8_t dac6 = (uint8_t)((shifted / 5.0f) * 63.0f + 0.5f);

    // --- 4) Escribir en DAC paralelo ---
    // El DAC está conectado a PORTD bits 2..7:
    // bit0 del DAC → PORTD bit2
    // bit1 del DAC → PORTD bit3
    // ...
    // bit5 del DAC → PORTD bit7
    uint8_t outBits = (dac6 << 2) & B11111100;
    // Conservar bits no conectados (D0..D1)
    PORTD = (PORTD & B00000011) | outBits;

    // --- 5) Depuración opcional ---
    Serial.print(vin, 4);
    Serial.print(", ");
    Serial.print(y_hp, 4);
    Serial.print(", DAC6=");
    Serial.println(dac6);
  }
}