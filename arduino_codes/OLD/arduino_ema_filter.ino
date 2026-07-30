/*
 * ECG Filter - EMA (Exponential Moving Average) VERSION
 * 
 * Más simple que IIR, pero funciona bien para baseline wandering
 * Usa solo 2 variables de estado
 */

#define INPUT_PIN A0
#define SAMPLE_TIME 4  // ms (250 Hz)

// ============================================================
// PARÁMETRO DEL FILTRO EMA
// Alpha más bajo = filtrado más agresivo
// ============================================================

#define EMA_ALPHA_NUM 1      // numerador
#define EMA_ALPHA_DEN 32     // denominador → alpha = 1/32 ≈ 0.03125

int32_t ema_prev = 0;
int32_t baseline_ema = 0;

int32_t rawValue = 0;
int32_t filtered = 0;
uint8_t dac_out = 128;
unsigned long lastTime = 0;
int sample_count = 0;

void setup() {
  pinMode(INPUT_PIN, INPUT);
  
  for(int i = 2; i <= 9; i++) {
    pinMode(i, OUTPUT);
  }
  
  Serial.begin(115200);
  Serial.println("=== ECG EMA Filter (SIMPLE) ===");
}

void loop() {
  unsigned long now = millis();
  
  if(now - lastTime >= SAMPLE_TIME) {
    lastTime = now;
    
    // Leer ADC (0-1023)
    rawValue = analogRead(INPUT_PIN);
    
    // Calcular EMA del baseline (baja frecuencia)
    // baseline_ema = alpha * raw + (1-alpha) * baseline_ema
    int32_t delta = rawValue - baseline_ema;
    baseline_ema += (delta * EMA_ALPHA_NUM) / EMA_ALPHA_DEN;
    
    // Señal filtrada = raw - baseline
    filtered = rawValue - baseline_ema;
    
    // Escalar a DAC (0-255)
    // filtered va de -512 a +511 aprox
    // amplificar y centrar: (filtered/2) + 128
    int32_t scaled = (filtered >> 1) + 128;
    dac_out = constrain(scaled, 0, 255);
    
    // Escribir DAC
    writeDAC(dac_out);
    
    // Serial output
    if(sample_count++ % 10 == 0) {
      Serial.print(rawValue);
      Serial.print(",");
      Serial.print(filtered);
      Serial.print(",");
      Serial.println(dac_out);
    }
  }
}

void writeDAC(uint8_t value) {
  digitalWrite(2, (value >> 7) & 1);  // MSB
  digitalWrite(3, (value >> 6) & 1);
  digitalWrite(4, (value >> 5) & 1);
  digitalWrite(5, (value >> 4) & 1);
  digitalWrite(6, (value >> 3) & 1);
  digitalWrite(7, (value >> 2) & 1);
  digitalWrite(8, (value >> 1) & 1);
  digitalWrite(9,  value       & 1);  // LSB
}
