/*
 * ECG IIR Filter - SIMPLE VERSION
 * 
 * Filtro: Butterworth paso alto, orden 4, 0.5 Hz
 * Objetivo: Remover baseline wandering de señal ECG
 * Entrada: Pin A0 (ADC)
 * Salida: Pines 2-9 (DAC R2R)
 * 
 * SIMPLIFICADO al máximo: Solo lo esencial
 */

// CONFIGURACIÓN DE PINES
#define INPUT_PIN A0
#define DAC_BIT0 9
#define DAC_BIT1 8
#define DAC_BIT2 7
#define DAC_BIT3 6
#define DAC_BIT4 5
#define DAC_BIT5 4
#define DAC_BIT6 3
#define DAC_BIT7 2

#define SAMPLE_TIME 4  // ms (250 Hz)

// ============================================================
// COEFICIENTES DEL FILTRO IIR - FORMATO Q15 (16 BITS)
// Generados automáticamente desde Python/SciPy
// ============================================================

// SECCIÓN 1 (2do orden)
int16_t b0_s1 = 29490;   // +0.9000000358
int16_t b1_s1 = -58980;  // -1.8000000715
int16_t b2_s1 = 29490;   // +0.9000000358
int16_t a1_s1 = -64308;  // -1.9616699219
int16_t a2_s1 = 32043;   // +0.9759616852

// SECCIÓN 2 (2do orden, idéntica)
int16_t b0_s2 = 29490;   // +0.9000000358
int16_t b1_s2 = -58980;  // -1.8000000715
int16_t b2_s2 = 29490;   // +0.9000000358
int16_t a1_s2 = -64308;  // -1.9616699219
int16_t a2_s2 = 32043;   // +0.9759616852

// ============================================================
// VARIABLES GLOBALES
// ============================================================

// Estados del filtro (deben mantenerse entre llamadas)
int32_t x1_s1 = 0, x2_s1 = 0;  // Entradas previas sección 1
int32_t y1_s1 = 0, y2_s1 = 0;  // Salidas previas sección 1
int32_t x1_s2 = 0, x2_s2 = 0;  // Entradas previas sección 2
int32_t y1_s2 = 0, y2_s2 = 0;  // Salidas previas sección 2

int32_t rawValue = 0;
int32_t filtered = 0;
uint8_t dac_out = 128;
unsigned long lastTime = 0;

// ============================================================
// SETUP
// ============================================================

void setup() {
  // Configurar ADC
  pinMode(INPUT_PIN, INPUT);
  
  // Configurar DAC
  for(int i = 2; i <= 9; i++) {
    pinMode(i, OUTPUT);
  }
  
  // Serial para debugging
  Serial.begin(9600);
  Serial.println("=== ECG IIR Filter ===");
  Serial.println("250 Hz | 0.5 Hz Cutoff | Order 4");
  Serial.println("READY");
}

// ============================================================
// LOOP PRINCIPAL
// ============================================================

void loop() {
  unsigned long now = millis();
  
  // Timing: muestrear cada 4ms (250 Hz)
  if(now - lastTime >= SAMPLE_TIME) {
    lastTime = now;
    
    // 1. Leer ADC (0-1023)
    rawValue = analogRead(INPUT_PIN);
    
    // 2. Centrar en 0 (resta 512)
    int32_t centered = rawValue - 512;  // -512 a +511
    
    // 3. Aplicar filtro IIR
    filtered = applyIIRFilter(centered);
    
    // 4. ESCALA MEJORADA para DAC
    //    Amplificar la salida filtrada porque es muy pequeña
    //    filtered en rango [-32768, +32767] -> escalar a [0, 255]
    // 
    //    Para remover DC y amplificar AC:
    //    - Dividir por 128 (>> 7) para evitar pérdida de escala
    //    - Sumar 128 para centrar en punto medio DAC
    
    int32_t scaled = (filtered >> 7) + 128;
    dac_out = constrain(scaled, 0, 255);
    
    // 5. Escribir salida al DAC
    writeDAC(dac_out);
    
    // 6. Serial output CADA MUESTRA para debugging
    Serial.print(rawValue);
    Serial.print(",");
    Serial.print(filtered);
    Serial.print(",");
    Serial.println(dac_out);
  }
}

// ============================================================
// FUNCIÓN: APLICAR FILTRO IIR
// ============================================================
/*
 * Implementa: y[n] = (b0*x[n] + b1*x[n-1] + b2*x[n-2]) - (a1*y[n-1] + a2*y[n-2])
 * 
 * En cascada: Sección 1 → Sección 2
 * 
 * Aritmética Q15:
 * - Coeficientes son int16_t (escalados por 2^15)
 * - Multiplicación produce int32_t
 * - Shift derecha 15 bits para recuperar valor real
 */

int32_t applyIIRFilter(int32_t input) {
  int32_t tmp, out_s1, out_s2;
  
  // ===== SECCIÓN 1 =====
  // Calcular: y[n] = (b0*x[n] + b1*x[n-1] + b2*x[n-2]) - (a1*y[n-1] + a2*y[n-2])
  
  tmp = ((int32_t)b0_s1 * input);    // b0 * x[n]
  tmp += ((int32_t)b1_s1 * x1_s1);   // + b1 * x[n-1]
  tmp += ((int32_t)b2_s1 * x2_s1);   // + b2 * x[n-2]
  tmp -= ((int32_t)a1_s1 * y1_s1);   // - a1 * y[n-1]
  tmp -= ((int32_t)a2_s1 * y2_s1);   // - a2 * y[n-2]
  
  // Shift right 15 para convertir de Q15 a valor real
  out_s1 = tmp >> 15;
  
  // Saturar si es necesario (evitar overflow)
  if(out_s1 > 32767) out_s1 = 32767;
  if(out_s1 < -32768) out_s1 = -32768;
  
  // Actualizar estados para próxima iteración
  x2_s1 = x1_s1;
  x1_s1 = input;
  y2_s1 = y1_s1;
  y1_s1 = out_s1;
  
  // ===== SECCIÓN 2 =====
  // La entrada es la salida de la sección 1
  
  tmp = ((int32_t)b0_s2 * out_s1);   // b0 * x[n]
  tmp += ((int32_t)b1_s2 * x1_s2);   // + b1 * x[n-1]
  tmp += ((int32_t)b2_s2 * x2_s2);   // + b2 * x[n-2]
  tmp -= ((int32_t)a1_s2 * y1_s2);   // - a1 * y[n-1]
  tmp -= ((int32_t)a2_s2 * y2_s2);   // - a2 * y[n-2]
  
  out_s2 = tmp >> 15;
  
  // Saturar si es necesario
  if(out_s2 > 32767) out_s2 = 32767;
  if(out_s2 < -32768) out_s2 = -32768;
  
  // Actualizar estados para próxima iteración
  x2_s2 = x1_s2;
  x1_s2 = out_s1;
  y2_s2 = y1_s2;
  y1_s2 = out_s2;
  
  // Retornar salida final filtrada
  return out_s2;
}

// ============================================================
// FUNCIÓN: ESCRIBIR EN DAC R2R
// ============================================================
/*
 * Convierte byte de 8 bits en salida paralela
 * 
 * Conexión:
 * Pin 2 (MSB) → bit 7
 * Pin 3      → bit 6
 * Pin 4      → bit 5
 * Pin 5      → bit 4
 * Pin 6      → bit 3
 * Pin 7      → bit 2
 * Pin 8      → bit 1
 * Pin 9 (LSB)→ bit 0
 */

void writeDAC(uint8_t value) {
  digitalWrite(DAC_BIT7, (value >> 7) & 1);  // MSB
  digitalWrite(DAC_BIT6, (value >> 6) & 1);
  digitalWrite(DAC_BIT5, (value >> 5) & 1);
  digitalWrite(DAC_BIT4, (value >> 4) & 1);
  digitalWrite(DAC_BIT3, (value >> 3) & 1);
  digitalWrite(DAC_BIT2, (value >> 2) & 1);
  digitalWrite(DAC_BIT1, (value >> 1) & 1);
  digitalWrite(DAC_BIT0,  value       & 1);  // LSB
}

// ============================================================
// FIN
// ============================================================
