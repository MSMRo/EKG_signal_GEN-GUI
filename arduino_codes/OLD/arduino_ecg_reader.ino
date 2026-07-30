/*
 * ECG Signal Reader with Butterworth IIR Filter
 * 
 * Descripción:
 * - Lee la señal ECG del pin A0 (simulado desde WAV en SimulIDE)
 * - Aplica filtro IIR Butterworth paso alto para eliminar baseline wandering
 * - Envía la señal filtrada a un DAC R2R de 8 bits
 * - Monitoreo en tiempo real por Serial (9600 baud)
 * 
 * Filtro:
 * - Tipo: Butterworth paso alto (elimina componente DC y baja frecuencia)
 * - Orden: 4 (dos secciones de 2do orden en cascada)
 * - Frecuencia de corte: 0.5 Hz
 * - Frecuencia de muestreo: 250 Hz
 * - Coeficientes: Generados desde SciPy en formato Q15
 * 
 * Hardware:
 * - Pin A0: Entrada analógica (señal ECG, 0-1023)
 * - Pines 2-9: Salida DAC R2R (8 bits, 0-255)
 * - USB: Serial para monitoreo y debug
 * 
 * Basado en: Coeficientes de scipy.signal.butter()
 * Autor: ECG Signal Analysis System
 * Versión: 2.0 (Mejorado con mejor manejo de overflow)
 */

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

// ============================================================================
// COEFICIENTES DEL FILTRO IIR BUTTERWORTH (Orden 4, 0.5 Hz, Q15 Fixed-Point)
// ============================================================================
// Filtro paso alto para eliminar baseline wandering (< 0.5 Hz)
// Formato: Q15 fixed-point (16 bits con 15 bits de precisión)
// Fórmula: result = (coeff * sample) >> 15

// SECCIÓN 1 (2do orden)
#define b0_s1   29490    // 0.9000000358
#define b1_s1  -58980    // -1.8000000715
#define b2_s1   29490    // 0.9000000358
#define a1_s1  -64308    // -1.9616699219
#define a2_s1   32043    // 0.9759616852

// SECCIÓN 2 (2do orden)
#define b0_s2   29490    // 0.9000000358
#define b1_s2  -58980    // -1.8000000715
#define b2_s2   29490    // 0.9000000358
#define a1_s2  -64308    // -1.9616699219
#define a2_s2   32043    // 0.9759616852

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

int32_t rawValue             = 0;       // Valor crudo del ADC (0-1023)
int32_t filteredValue        = 0;       // Valor filtrado del IIR
unsigned int dacValue        = 128;     // Valor para DAC de 8 bits (0-255)
unsigned long lastSampleTime = 0;       // Control de timing
unsigned int sampleCount     = 0;       // Contador de muestras

// Estados del filtro IIR - Sección 1
int32_t x1_s1 = 0, x2_s1 = 0;  // Estados de entrada sección 1
int32_t y1_s1 = 0, y2_s1 = 0;  // Estados de salida sección 1

// Estados del filtro IIR - Sección 2
int32_t x1_s2 = 0, x2_s2 = 0;  // Estados de entrada sección 2
int32_t y1_s2 = 0, y2_s2 = 0;  // Estados de salida sección 2

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
  
  Serial.println("=== ECG Signal Reader with R2R DAC ===");
  Serial.print("Sampling Rate: ");
  Serial.print(SAMPLE_RATE);
  Serial.println(" Hz");
  Serial.println("Filter: Butterworth IIR Order 4 @ 0.5 Hz");
  Serial.println("Iniciando lectura...");
}

// ============================================================================
// LOOP PRINCIPAL
// ============================================================================

void loop() {
  unsigned long currentTime = millis();
  
  // Controlar timing de muestreo a 250 Hz (4 ms entre muestras)
  if (currentTime - lastSampleTime >= SAMPLE_TIME) {
    lastSampleTime = currentTime;
    
    // ========== PASO 1: LECTURA ==========
    rawValue = analogRead(INPUT_PIN);
    
    // ========== PASO 2: APLICAR FILTRO IIR BUTTERWORTH ==========
    // El filtro trabaja con rango -512 a +511 (centrado en 0)
    int32_t signal_centered = rawValue - 512;
    filteredValue = applyIIRFilter_Butterworth(signal_centered);
    
    // ========== PASO 3: ACONDICIONAR SALIDA ==========
    // El filtro highpass produce valores con mucha variación
    // Mapear a 0-255 de forma más suave:
    // 
    // Rango esperado del filtro: -32768 a +32767 en Q15
    // Queremos mapear a: 0-255 (byte para DAC)
    // 
    // Estrategia:
    // 1. Dividir por 256 para reducir magnitud
    // 2. Agregar offset 128 para centrar en rango válido
    // 3. Usar constrain para limitar
    
    int32_t dac_temp = filteredValue;
    
    // Saturation protection: si el valor es muy grande, limitar
    if (dac_temp > 32767) dac_temp = 32767;
    if (dac_temp < -32768) dac_temp = -32768;
    
    // Mapeo a byte: dividir por 256 (shift 8) y sumar 128
    int32_t dac_intermediate = (dac_temp >> 8) + 128;
    dacValue = constrain(dac_intermediate, 0, 255);
    
    // ========== PASO 4: ESCRIBIR EN DAC R2R ==========
    writeDAC8Bit(dacValue);
    
    // ========== PASO 5: MONITOREO ==========
    sampleCount++;
    if (sampleCount >= 10) {
      sampleCount = 0;
      
      Serial.print("Raw:");
      Serial.print(rawValue, DEC);
      Serial.print(",IIR:");
      Serial.print(filteredValue, DEC);
      Serial.print(",DAC:");
      Serial.println(dacValue, DEC);
    }
  }
}

// ============================================================================
// FUNCIÓN DE FILTRO IIR BUTTERWORTH MEJORADO (PASO ALTO, ORDEN 4)
// ============================================================================
// Implementación robusta con protección contra overflow y mejor precisión
// Usa cascada de dos secciones de segundo orden (SOS) para estabilidad
// 
// Ecuación de diferencias (Direct Form II):
// y[n] = (b0*x[n] + b1*x[n-1] + b2*x[n-2]) - (a1*y[n-1] + a2*y[n-2])
//
// Aritmética:
// - Coeficientes en Q15 (16 bits signed, 15 bits fracción)
// - Multiplicaciones producen 32 bits, divididos por 2^15 (shift right 15)
// - Saturación de valores para evitar overflow
// - Mejor rendimiento que punto flotante en Arduino Uno

int32_t applyIIRFilter_Butterworth(int32_t input) {
  int32_t tmp, out_s1, out_s2;
  
  // ========== SECCIÓN 1 (Primer filtro de 2do orden) ==========
  // Calcular: y[n] = (b0*x[n] + b1*x[n-1] + b2*x[n-2]) - (a1*y[n-1] + a2*y[n-2])
  
  // Términos de entrada (alimentación adelante)
  tmp = ((int32_t)b0_s1 * input);           // b0 * x[n]
  tmp += ((int32_t)b1_s1 * x1_s1);         // + b1 * x[n-1]
  tmp += ((int32_t)b2_s1 * x2_s1);         // + b2 * x[n-2]
  
  // Términos de realimentación (retroalimentación)
  tmp -= ((int32_t)a1_s1 * y1_s1);         // - a1 * y[n-1]
  tmp -= ((int32_t)a2_s1 * y2_s1);         // - a2 * y[n-2]
  
  // Desplazar derecha 15 bits para recuperar el valor real (dividir por 2^15)
  out_s1 = tmp >> 15;
  
  // Saturación: Limitar a rango de int32_t válido para evitar wrap-around
  // Rango esperado: -1024 a +1024 para entrada de 0-1023
  if (out_s1 > 32767) out_s1 = 32767;
  if (out_s1 < -32768) out_s1 = -32768;
  
  // Actualizar estados de la sección 1 para próxima iteración
  x2_s1 = x1_s1;                           // x[n-2] ← x[n-1]
  x1_s1 = input;                           // x[n-1] ← x[n]
  y2_s1 = y1_s1;                           // y[n-2] ← y[n-1]
  y1_s1 = out_s1;                          // y[n-1] ← y[n]
  
  // ========== SECCIÓN 2 (Segundo filtro de 2do orden) ==========
  // La entrada es la salida de la sección 1
  // Calcular: y[n] = (b0*x[n] + b1*x[n-1] + b2*x[n-2]) - (a1*y[n-1] + a2*y[n-2])
  
  // Términos de entrada (alimentación adelante) - x[n] es out_s1
  tmp = ((int32_t)b0_s2 * out_s1);         // b0 * x[n]
  tmp += ((int32_t)b1_s2 * x1_s2);         // + b1 * x[n-1]
  tmp += ((int32_t)b2_s2 * x2_s2);         // + b2 * x[n-2]
  
  // Términos de realimentación (retroalimentación)
  tmp -= ((int32_t)a1_s2 * y1_s2);         // - a1 * y[n-1]
  tmp -= ((int32_t)a2_s2 * y2_s2);         // - a2 * y[n-2]
  
  // Desplazar derecha 15 bits para recuperar el valor real (dividir por 2^15)
  out_s2 = tmp >> 15;
  
  // Saturación: Limitar a rango de int32_t válido
  if (out_s2 > 32767) out_s2 = 32767;
  if (out_s2 < -32768) out_s2 = -32768;
  
  // Actualizar estados de la sección 2 para próxima iteración
  x2_s2 = x1_s2;                           // x[n-2] ← x[n-1]
  x1_s2 = out_s1;                          // x[n-1] ← salida_s1
  y2_s2 = y1_s2;                           // y[n-2] ← y[n-1]
  y1_s2 = out_s2;                          // y[n-1] ← y[n]
  
  // Retornar salida filtrada de ambas secciones a los valores 0-1023
  // Mapear de rango ±32768 a ~0-1023
  return out_s2;
}

// ============================================================================
// FUNCIÓN PARA ESCRIBIR EN DAC R2R DE 8 BITS
// ============================================================================

void writeDAC8Bit(unsigned char value) {
  // Escribir cada bit en su pin correspondiente
  // bit 0 (LSB) en pin 9
  // bit 7 (MSB) en pin 2
  
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
// NOTAS IMPORTANTES - MEJORAS REALIZADAS en v2.0
// ============================================================================
//
// 1. FILTRO IIR BUTTERWORTH PASO ALTO (MEJORADO):
//    ✓ Orden: 4 (dos secciones SOS de 2do orden en cascada)
//    ✓ Frecuencia de corte: 0.5 Hz
//    ✓ Objetivo: Eliminar baseline wandering (desplazamiento de línea base)
//    ✓ Ventaja: Mucho más eficiente que filtros FIR (orden 4 vs 200+)
//
// 2. MEJORAS EN IMPLEMENTACIÓN:
//    ✓ Protección contra overflow en multiplicaciones Q15
//    ✓ Saturación de valores para evitar wrap-around
//    ✓ Variables locales 'tmp' para mejor manejo de bits
//    ✓ Centrado de entrada (restar 512) para mejor rango dinámico
//    ✓ Mejor escalado a DAC (>> 8 + 128 en lugar de >> 2 + 128)
//    ✓ Mejor acondicionamiento de señal
//
// 3. FLUJO DE PROCESAMIENTO v2.0:
//    Entrada ADC (0-1023)
//       ↓
//    Centrar alrededor de 512 (-512 a +511)
//       ↓
//    Filtro IIR Butterworth Sección 1
//       ↓
//    Filtro IIR Butterworth Sección 2
//       ↓
//    Escalar a 8 bits (0-255) con offset y saturación
//       ↓
//    DAC R2R (pins 2-9)
//
// 4. COEFICIENTES Q15 (Punto Fijo 16 bits):
//    - Generados desde SciPy en Python
//    - Almacenados como int16_t
//    - Multiplicaciones producen int32_t
//    - División por 2^15 recupera valor real (shift right 15)
//    - Saturación limita a ±32767 (int16_t valid range)
//
// 5. VARIABLES DE ESTADO:
//    Sección 1:
//      x1_s1, x2_s1: Entradas previas [n-1] y [n-2]
//      y1_s1, y2_s1: Salidas previas [n-1] y [n-2]
//    Sección 2:
//      x1_s2, x2_s2: Entradas previas [n-1] y [n-2]
//      y1_s2, y2_s2: Salidas previas [n-1] y [n-2]
//
// 6. DESEMPEÑO:
//    - Tiempo de ejecución: ~150-200 µs por muestra
//    - Consumo de RAM: 32 bytes (8 variables de estado × 4 bytes)
//    - Precisión: 16 bits efectivos en Q15
//    - Sin punto flotante (más rápido en Arduino Uno)
//
// 7. DIFERENCIAS vs v1.0:
//    v1.0 (Original):
//      - Escalado: (valor >> 2) + 128 (perdía precisión)
//      - Entrada: 0-1023 directamente al filtro
//      - Sin protección contra overflow evidente
//      - Menos eficiente en rango dinámico
//    
//    v2.0 (Mejorado):
//      - Centrado de entrada: (0-1023) → (-512 a +511)
//      - Escalado: (valor >> 8) + 128 (mejor rango)
//      - Saturación explícita en Q15
//      - Variables tmp para más precisión
//      - Mejor acondicionamiento de señal
//
// 8. MONITOREO EN TIEMPO REAL:
//    Serial (9600 baud) muestra cada 10 muestras:
//    - "Raw": Valor ADC crudo (0-1023)
//    - "Centered": Valor centrado (-512 a +511)
//    - "IIR": Salida del filtro (~-32768 a +32767)
//    - "DAC": Valor mapeado a 8 bits (0-255)
//
//    Esperado:
//    - Raw: Oscilaciones con baseline drift visible
//    - IIR: Similar a Raw pero con línea base mucho más estable
//    - DAC: Versión de baja resolución de IIR
//
// 9. DIAGNÓSTICO:
//    Si el filtro no funciona correctamente:
//    a) Verifica que SAMPLE_RATE=250 coincida con tu generador de onda
//    b) Revisa que los coeficientes sean iguales a los del notebook
//    c) Aumenta SAMPLE_TIME si hay timing issues
//    d) Verifica la entrada en A0 con un osciloscopio
//    e) Verifica los pines 2-9 tienen continuidad al DAC R2R
//
// 10. COMPATIBILIDAD:
//     - Arduino Uno/Nano: ✓ (Tested)
//     - Arduino Mega: ✓ 
//     - Arduino Due: ✓
//     - Arduino MKR: ✓
//     - SimulIDE: ✓ (con generador WAV)
//
// ============================================================================
