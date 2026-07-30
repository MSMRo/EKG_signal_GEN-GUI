/*
 * ECG Signal Reader - VERSIÓN SIMPLIFICADA
 * ===========================================
 * 
 * Filtrado de baseline wandering sin distorsión
 * 
 * Idea:
 * - Usar un filtro lowpass MUY simple (moving average o IIR de 1er orden)
 * - para estimar la línea base (baseline)
 * - Restar la línea base de la señal original
 * - Resultado: ECG limpio sin baseline wandering
 * 
 * Ventaja:
 * - No distorsiona la forma de la onda ECG
 * - Muy simple de implementar
 * - Bajo procesamiento
 * - Mejor resultado visual
 */

// ============================================================================
// CONFIGURACIÓN
// ============================================================================

#define INPUT_PIN      A0        
#define DAC_BIT0       9         
#define DAC_BIT1       8         
#define DAC_BIT2       7         
#define DAC_BIT3       6         
#define DAC_BIT4       5         
#define DAC_BIT5       4         
#define DAC_BIT6       3         
#define DAC_BIT7       2         

#define BAUD_RATE      9600      
#define SAMPLE_RATE    250       
#define SAMPLE_TIME    4         

// Parámetro de filtro exponencial para la línea base
// Más bajo = cambios más lentos = baseline más estable
// Rango: 1-100
// Recomendación ECG: 5-15
#define ALPHA_BASELINE 8         // Factor de suavizado (1-255, más bajo = más suave)

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

int32_t rawValue        = 0;
int32_t baseline_smooth = 512;   // Estimación de la línea base (inicia en 512)
int32_t filtered_output = 0;
uint8_t dacValue        = 128;
unsigned long lastSampleTime = 0;
uint16_t sampleCount    = 0;

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  // Configurar pines
  pinMode(INPUT_PIN, INPUT);
  pinMode(DAC_BIT0, OUTPUT);
  pinMode(DAC_BIT1, OUTPUT);
  pinMode(DAC_BIT2, OUTPUT);
  pinMode(DAC_BIT3, OUTPUT);
  pinMode(DAC_BIT4, OUTPUT);
  pinMode(DAC_BIT5, OUTPUT);
  pinMode(DAC_BIT6, OUTPUT);
  pinMode(DAC_BIT7, OUTPUT);
  
  Serial.begin(BAUD_RATE);
  
  Serial.println("=== ECG Signal Reader - SIMPLIFIED ===");
  Serial.print("Sampling Rate: ");
  Serial.print(SAMPLE_RATE);
  Serial.println(" Hz");
  Serial.println("Filter: Moving Average Baseline Removal");
  Serial.println("Starting...");
}

// ============================================================================
// LOOP PRINCIPAL
// ============================================================================

void loop() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastSampleTime >= SAMPLE_TIME) {
    lastSampleTime = currentTime;
    
    // ========== PASO 1: LECTURA ==========
    rawValue = analogRead(INPUT_PIN);
    
    // ========== PASO 2: ACTUALIZAR ESTIMACIÓN DE LÍNEA BASE ==========
    // Usar filtro exponencial de 1er orden para suavizar la línea base
    // Fórmula: baseline_new = baseline_old + alpha * (raw - baseline_old)
    // alpha pequeño = cambios lentos = baseline muy estable
    // alpha grande = cambios rápidos = baseline sigue más de cerca
    
    int32_t error = rawValue - baseline_smooth;
    baseline_smooth += (error * ALPHA_BASELINE) / 256;
    
    // Saturar baseline para evitar valores fuera de rango
    if (baseline_smooth < 0) baseline_smooth = 0;
    if (baseline_smooth > 1023) baseline_smooth = 1023;
    
    // ========== PASO 3: RESTAR LÍNEA BASE ==========
    // Señal_limpia = Señal_cruda - Línea_base_estimada
    // Esto elimina el baseline wandering
    int32_t signal_no_baseline = rawValue - baseline_smooth;  // Rango: -1023 a +1023
    
    // Centrar en 512 para que oscile alrededor del centro del rango DAC
    filtered_output = signal_no_baseline + 512;  // Rango: -511 a +1535
    
    // Saturar al rango válido del DAC
    if (filtered_output < 0) filtered_output = 0;
    if (filtered_output > 1023) filtered_output = 1023;
    
    // ========== PASO 4: MAPEAR A 8 BITS ==========
    // Convertir de rango 0-1023 a 0-255 para el DAC de 8 bits
    dacValue = filtered_output >> 2;  // Divide por 4
    
    // ========== PASO 5: ESCRIBIR EN DAC R2R ==========
    writeDAC8Bit(dacValue);
    
    // ========== PASO 6: MONITOREO ==========
    sampleCount++;
    if (sampleCount >= 10) {
      sampleCount = 0;
      
      Serial.print("Raw:");
      Serial.print(rawValue, DEC);
      Serial.print(",Base:");
      Serial.print(baseline_smooth, DEC);
      Serial.print(",out:");
      Serial.print(filtered_output, DEC);
      Serial.print(",DAC:");
      Serial.println(dacValue, DEC);
    }
  }
}

// ============================================================================
// FUNCIÓN PARA ESCRIBIR EN DAC R2R DE 8 BITS
// ============================================================================

void writeDAC8Bit(uint8_t value) {
  digitalWrite(DAC_BIT7, (value >> 7) & 1);
  digitalWrite(DAC_BIT6, (value >> 6) & 1);
  digitalWrite(DAC_BIT5, (value >> 5) & 1);
  digitalWrite(DAC_BIT4, (value >> 4) & 1);
  digitalWrite(DAC_BIT3, (value >> 3) & 1);
  digitalWrite(DAC_BIT2, (value >> 2) & 1);
  digitalWrite(DAC_BIT1, (value >> 1) & 1);
  digitalWrite(DAC_BIT0,  value       & 1);
}

// ============================================================================
// EXPLICACIÓN DEL ALGORITMO
// ============================================================================
/*
 
PROBLEMA:
- El filtro IIR Butterworth highpass de orden 4 es MUY agresivo
- Distorsiona la forma de la onda ECG
- Produce señal "fea" en el osciloscopio

SOLUCIÓN:
- En lugar de eliminar todo lo que esté bajo 0.5 Hz,
- Solo estimamos y removemos la línea base (baseline)
- El baseline wandering típicamente cambia MUY lentamente (~0.1 Hz)
- Usamos una media móvil exponencial para estimarlo
- Y lo restamos de la señal original

VENTAJAS:
✓ Preserva completamente la forma de la onda ECG
✓ Solo elimina el drift lento
✓ Mucho más simple que IIR orden 4
✓ Menos distorsión
✓ Mejor visual en osciloscopio

PARÁMETRO CLAVE: ALPHA_BASELINE
- Muy bajo (1-3): Baseline muy estable, pero lento para adaptarse a cambios
- Medio (5-15): Balance: estable pero responde a cambios lentos
- Alto (20-50): Responde rápido, pero menos estable

RECOMENDACIÓN PARA ECG:
- ALPHA_BASELINE = 8 (por defecto)
- Si hay mucho baseline drift lento: aumentar a 10-15
- Si la línea base es muy ruidosa: bajar a 3-5

MONITOREO SERIAL:
Raw: Entrada ADC (0-1023)
Base: Estimación de la línea base suavizada
out: Señal sin baseline (output antes de mapear a DAC)
DAC: Valor final para el convertidor D/A (0-255)

VISUALIZACIÓN:
- Raw debe mostrar oscilaciones ECG CON baseline drift
- Base debe ser una línea suave que sigue lentamente el drift
- out debe mostrar las oscilaciones ECG SIN el drift (centrado en ~512)
- DAC es out pero en rango 0-255
 
*/
