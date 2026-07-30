/*
 * ===========================================================================
 * VERSIÓN ALTERNATIVA CON LIBRERÍA EXTERNA - ArduinoIIRFilter
 * ===========================================================================
 * 
 * Este archivo muestra cómo usar la librería ArduinoIIRFilter para un
 * filtrado más robusto y profesional.
 * 
 * INSTALACIÓN:
 * 1. Arduino IDE → Sketch → Include Library → Manage Libraries
 * 2. Busca: "ArduinoIIRFilter" por rfetick
 * 3. Click en Install
 * 4. Cierra y reabre Arduino IDE
 * 
 * VENTAJAS:
 * ✓ Código más limpio y legible
 * ✓ Mejor manejo de errores numéricos
 * ✓ Menos bugs de overflow
 * ✓ Mantenida por comunidad
 * ✓ Documentación completa
 * ✓ Funciona en múltiples plataformas Arduino
 * 
 * ===========================================================================
 */

#include <ArduinoIIRFilter.h>

// ============================================================================
// CONFIGURACIÓN
// ============================================================================

#define INPUT_PIN      A0        // Pin de entrada analógica
#define DAC_BIT0       9         // LSB del DAC R2R (bit 0)
#define DAC_BIT1       8         // bit 1
#define DAC_BIT2       7         // bit 2
#define DAC_BIT3       6         // bit 3
#define DAC_BIT4       5         // bit 4
#define DAC_BIT5       4         // bit 5
#define DAC_BIT6       3         // bit 6
#define DAC_BIT7       2         // MSB del DAC R2R (bit 7)

#define BAUD_RATE      9600      // Velocidad serial
#define SAMPLE_RATE    250       // Frecuencia de muestreo (debe coincidir con WAV)
#define SAMPLE_TIME    4         // Tiempo entre muestras en ms (1000/SAMPLE_RATE)

#define FILTER_CUTOFF  0.5       // Frecuencia de corte en Hz (baseline wandering < 0.5 Hz)
#define FILTER_ORDER   4         // Orden del filtro

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

// Crear instancia del filtro IIR Butterworth
// Parámetros:
// - Tipo: HIGH_PASS (para eliminar baseline wandering)
// - Frecuencia normalizada: cutoff_freq / (sample_rate / 2)
// - Damping: Factor de amortiguamiento (0.707 para Butterworth)
// - Orden: 4 (dos cascadas de 2do orden)

float normalized_freq = FILTER_CUTOFF / (SAMPLE_RATE / 2.0);  // 0.5 / 125 = 0.004
IIRFilter iirFilter(IIRFilter::HIGH_PASS, normalized_freq, 0.707, FILTER_ORDER);

int32_t rawValue        = 0;
int32_t filteredValue   = 0;
uint8_t dacValue        = 128;
unsigned long lastSampleTime = 0;
uint16_t sampleCount    = 0;

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  // Configurar pines de entrada
  pinMode(INPUT_PIN, INPUT);
  
  // Configurar pines de salida del DAC R2R (8 bits)
  pinMode(DAC_BIT0, OUTPUT);
  pinMode(DAC_BIT1, OUTPUT);
  pinMode(DAC_BIT2, OUTPUT);
  pinMode(DAC_BIT3, OUTPUT);
  pinMode(DAC_BIT4, OUTPUT);
  pinMode(DAC_BIT5, OUTPUT);
  pinMode(DAC_BIT6, OUTPUT);
  pinMode(DAC_BIT7, OUTPUT);
  
  // Inicializar comunicación serial
  Serial.begin(BAUD_RATE);
  
  Serial.println("=== ECG Signal Reader with IIR Filter (Library) ===");
  Serial.print("Sampling Rate: ");
  Serial.print(SAMPLE_RATE);
  Serial.println(" Hz");
  Serial.print("Filter Type: Butterworth High-Pass");
  Serial.print(" Cutoff: ");
  Serial.print(FILTER_CUTOFF);
  Serial.print(" Hz, Order: ");
  Serial.println(FILTER_ORDER);
  Serial.println("Starting acquisition...");
  
  // Inicializar el filtro
  iirFilter.reset();
}

// ============================================================================
// LOOP PRINCIPAL
// ============================================================================

void loop() {
  unsigned long currentTime = millis();
  
  // Controlar timing de muestreo a 250 Hz
  if (currentTime - lastSampleTime >= SAMPLE_TIME) {
    lastSampleTime = currentTime;
    
    // ========== PASO 1: LECTURA DEL SENSOR ==========
    rawValue = analogRead(INPUT_PIN);
    
    // ========== PASO 2: CENTRAR ALREDEDOR DE 512 ==========
    // Ayuda al filtro a trabajar mejor (rango: -512 a +511)
    int32_t centered = rawValue - 512;
    
    // ========== PASO 3: APLICAR FILTRO IIR (LIBRERÍA) ==========
    // Mucho más simple que implementación manual
    // La librería maneja todos los casos de overflow automáticamente
    filteredValue = iirFilter.apply(centered);
    
    // ========== PASO 4: ESCALAR A RANGO DAC DE 8 BITS ==========
    // Mapear desde rango filtrado a 0-255
    int32_t dacScaled = (filteredValue >> 8) + 128;
    dacValue = constrain(dacScaled, 0, 255);
    
    // ========== PASO 5: ESCRIBIR AL DAC R2R ==========
    writeDAC8Bit(dacValue);
    
    // ========== PASO 6: MONITOREO EN TIEMPO REAL ==========
    sampleCount++;
    if (sampleCount >= 10) {
      sampleCount = 0;
      
      Serial.print("Raw: ");
      Serial.print(rawValue, DEC);
      Serial.print(" | Centered: ");
      Serial.print(centered, DEC);
      Serial.print(" | IIR: ");
      Serial.print(filteredValue, DEC);
      Serial.print(" | DAC: ");
      Serial.print(dacValue, DEC);
      Serial.print(" (0x");
      Serial.print(dacValue, HEX);
      Serial.println(")");
    }
  }
}

// ============================================================================
// FUNCIÓN PARA ESCRIBIR EN DAC R2R DE 8 BITS
// ============================================================================

void writeDAC8Bit(uint8_t value) {
  // Escribir cada bit en su pin correspondiente
  // Standard binary to digital logic conversion
  
  digitalWrite(DAC_BIT7, (value >> 7) & 1);  // Pin 2 - MSB
  digitalWrite(DAC_BIT6, (value >> 6) & 1);  // Pin 3
  digitalWrite(DAC_BIT5, (value >> 5) & 1);  // Pin 4
  digitalWrite(DAC_BIT4, (value >> 4) & 1);  // Pin 5
  digitalWrite(DAC_BIT3, (value >> 3) & 1);  // Pin 6
  digitalWrite(DAC_BIT2, (value >> 2) & 1);  // Pin 7
  digitalWrite(DAC_BIT1, (value >> 1) & 1);  // Pin 8
  digitalWrite(DAC_BIT0,  value       & 1);  // Pin 9 - LSB
}

// ============================================================================
// COMPARACIÓN: VERSIÓN MANUAL vs LIBRERÍA
// ============================================================================
//
// VERSIÓN MANUAL (arduino_ecg_reader.ino):
// ─────────────────────────────────────
// Pros:
//   + Sin dependencias externas
//   + Control total del código
//   + Menor uso de RAM (solo 8 variables de estado)
// 
// Contras:
//   - Más líneas de código
//   - Mayor riesgo de errores numéricos
//   - Menos legible
//   - Difícil de debugging
//
// VERSIÓN CON LIBRERÍA (Este archivo):
// ──────────────────────────────────────
// Pros:
//   + Código mucho más simple y limpio
//   + Mejor manejo de errores numéricos
//   + Fácil cambiar parámetros del filtro
//   + Mejor documentación
//   + Probado en múltiples plataformas
// 
// Contras:
//   - Requiere instalación de librería
//   - Un poco más de RAM
//   - Menos control de bajo nivel
//
// ============================================================================
// RECOMENDACIÓN
// ============================================================================
//
// Para este proyecto ECG, RECOMENDAMOS usar la VERSIÓN CON LIBRERÍA porque:
// 1. Es más confiable
// 2. Mucho más fácil de entender
// 3. Menor probabilidad de bugs
// 4. Más fácil de mantener en el futuro
// 5. Profesional y documentado
//
// SI NECESITAS máxima libertad sin dependencias, usa la VERSIÓN MANUAL.
//
// ============================================================================
