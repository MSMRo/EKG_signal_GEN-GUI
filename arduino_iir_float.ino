/*
 * ECG IIR Filter - FLOATING POINT VERSION (para debugging)
 * 
 * Filtro: Butterworth paso alto, orden 4, 0.5 Hz
 * Usa punto flotante para verificar si los coeficientes funcionan
 */

#define INPUT_PIN A0
#define SAMPLE_TIME 4  // ms (250 Hz)

// ============================================================
// COEFICIENTES DEL FILTRO IIR - PUNTO FLOTANTE
// Butterworth orden 4, 0.5 Hz, 250 Hz fs
// ============================================================

// SECCIÓN 1
float b0_s1 = 0.900000f;
float b1_s1 = -1.800000f;
float b2_s1 = 0.900000f;
float a1_s1 = -1.961670f;
float a2_s1 = 0.975962f;

// SECCIÓN 2 (cascada)
float b0_s2 = 0.900000f;
float b1_s2 = -1.800000f;
float b2_s2 = 0.900000f;
float a1_s2 = -1.961670f;
float a2_s2 = 0.975962f;

// Estados del filtro
float x1_s1 = 0, x2_s1 = 0;
float y1_s1 = 0, y2_s1 = 0;
float x1_s2 = 0, x2_s2 = 0;
float y1_s2 = 0, y2_s2 = 0;

int32_t rawValue = 0;
float filtered = 0;
uint8_t dac_out = 128;
unsigned long lastTime = 0;

void setup() {
  pinMode(INPUT_PIN, INPUT);
  
  for(int i = 2; i <= 9; i++) {
    pinMode(i, OUTPUT);
  }
  
  Serial.begin(115200);
  Serial.println("=== ECG IIR Filter (FLOAT) ===");
  Serial.println("250 Hz | 0.5 Hz Cutoff | Order 4");
  Serial.println("RAW,FILTERED,DAC");
}

void loop() {
  unsigned long now = millis();
  
  if(now - lastTime >= SAMPLE_TIME) {
    lastTime = now;
    
    // Leer ADC
    rawValue = analogRead(INPUT_PIN);
    float centered = (float)(rawValue - 512);
    
    // Aplicar filtro
    filtered = applyIIRFilter(centered);
    
    // Escalar a DAC - con amplificación
    float scaled = filtered * 0.1f + 128.0f;  // Amplificar 10x
    dac_out = constrain((int32_t)scaled, 0, 255);
    
    // Escribir DAC
    for(int bit = 7; bit >= 0; bit--) {
      digitalWrite(2 + (7-bit), (dac_out >> bit) & 1);
    }
    
    // Serial cada muestra
    Serial.print(rawValue);
    Serial.print(",");
    Serial.print((int32_t)filtered);
    Serial.print(",");
    Serial.println(dac_out);
  }
}

float applyIIRFilter(float input) {
  float out_s1, out_s2;
  
  // SECCIÓN 1
  out_s1 = b0_s1 * input + b1_s1 * x1_s1 + b2_s1 * x2_s1 
           - a1_s1 * y1_s1 - a2_s1 * y2_s1;
  
  x2_s1 = x1_s1;
  x1_s1 = input;
  y2_s1 = y1_s1;
  y1_s1 = out_s1;
  
  // SECCIÓN 2
  out_s2 = b0_s2 * out_s1 + b1_s2 * x1_s2 + b2_s2 * x2_s2
           - a1_s2 * y1_s2 - a2_s2 * y2_s2;
  
  x2_s2 = x1_s2;
  x1_s2 = out_s1;
  y2_s2 = y1_s2;
  y1_s2 = out_s2;
  
  return out_s2;
}
