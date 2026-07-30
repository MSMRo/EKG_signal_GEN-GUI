# Mejoras Implementadas en el Filtro IIR - v2.0

## 📋 Resumen de Cambios

El código Arduino para el filtrado ECG ha sido **mejorado significativamente** en la versión 2.0. Se han solucionado problemas de precisión numérica y overflow que causaban un filtrado deficiente.

---

## 🔧 Problemas Identificados en v1.0

1. **Escalado incorrecto**: `(filteredValue >> 2) + 128` perdía demasiada precisión
2. **Sin centrado de entrada**: La señal 0-1023 no era tratada como señal bipolar
3. **Saturación insuficiente**: No había protección explícita contra overflow
4. **Rango de salida**: El mapeo a 0-255 no era óptimo

---

## ✅ Mejoras Implementadas en v2.0

### 1. **Centrado de Entrada**
```cpp
// v1.0 (INCORRECTO)
filteredValue = applyIIRFilter_Butterworth(rawValue);  // 0-1023

// v2.0 (CORRECTO)
int32_t centered = rawValue - 512;  // -512 a +511
filteredValue = applyIIRFilter_Butterworth(centered);
```

**Por qué es importante:**
- El filtro IIR está diseñado para señales que oscilan alrededor de cero
- Entrada 0-1023 hace que el filtro se sature o pierda efectividad
- Centrados en 512, el filtro trabaja en su rango óptimo: -512 a +511

### 2. **Mejor Escalado Q15**
```cpp
// v1.0 (INCORRECTO)
int32_t scaled = (filteredValue >> 2) + 128;  // Pierde precisión

// v2.0 (CORRECTO)
int32_t dacScaled = (filteredValue >> 8) + 128;  // Mejor rango
dacValue = constrain(dacScaled, 0, 255);
```

**Diferencia:**
- `>> 2` divide por 4 (demasiado pequeño)
- `>> 8` divide por 256 (correcto para Q15 a byte)

### 3. **Saturación Explícita en la Función IIR**
```cpp
// v1.0 (SIN PROTECCIÓN)
out_s1 = tmp >> 15;
out_s2 = tmp >> 15;

// v2.0 (CON SATURACIÓN)
out_s1 = tmp >> 15;
if (out_s1 > 32767) out_s1 = 32767;  // Saturar máximo
if (out_s1 < -32768) out_s1 = -32768;  // Saturar mínimo
```

**Beneficio:** Evita que valores muy grandes causen "wrap-around" (inversión de signo)

### 4. **Mejor Precisión con Variables Temporales**
```cpp
// v1.0 (SUMA DIRECTA)
out_s1 = ((int32_t)b0_s1 * input) >> 15;
out_s1 += ((int32_t)b1_s1 * x1_s1) >> 15;  // Posible pérdida de precisión

// v2.0 (SUMA AL FINAL)
tmp = ((int32_t)b0_s1 * input);
tmp += ((int32_t)b1_s1 * x1_s1);
out_s1 = tmp >> 15;  // Un solo shift al final
```

### 5. **Monitoreo Mejorado**
```cpp
// v1.0
Serial.print(rawValue, DEC);
Serial.print(" | IIR: ");
Serial.print(filteredValue, DEC);

// v2.0
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
```

---

## 📊 Impacto de las Mejoras

| Métrica | v1.0 | v2.0 | Mejora |
|---------|------|------|--------|
| Rango dinámico entrada | 0-1023 | -512 a +511 | ✓ Óptimo |
| Escalado DAC | 0-255 (impreciso) | 0-255 (preciso) | ✓ Mejor |
| Protección overflow | No | Sí | ✓ Seguro |
| Precisión Q15 | Baja | Alta | ✓ Exacto |
| Detección problemas | Difícil | Fácil | ✓ Debug |

---

## 🚀 Cómo Usar

### Opción A: Versión Mejorada Manual (RECOMENDADO)
```bash
# Archivo: arduino_ecg_reader.ino
# ✓ Sin dependencias externas
# ✓ Totalmente controlado
# ✓ Optimizado para Arduino Uno
```

**Pasos:**
1. Abre Arduino IDE
2. Carga: `arduino_ecg_reader.ino`
3. Selecciona tu placa (Arduino Uno, Nano, etc.)
4. Sube el código

### Opción B: Versión con Librería (ALTERNATIVA)
```bash
# Archivo: arduino_ecg_reader_CON_LIBRERIA.ino
# ✓ Código más simple
# ✓ Mejor manejo de errores
# ✓ Más fácil de modificar
```

**Pasos:**
1. Arduino IDE → Sketch → Include Library → Manage Libraries
2. Busca: `ArduinoIIRFilter` (por rfetick)
3. Instala
4. Carga: `arduino_ecg_reader_CON_LIBRERIA.ino`
5. Sube el código

---

## 📈 Comportamiento Esperado

### En el Monitor Serial (9600 baud):
```
Raw: 512 | Centered: 0 | IIR: 0 | DAC: 128 (0x80)
Raw: 525 | Centered: 13 | IIR: 8 | DAC: 128 (0x80)
Raw: 538 | Centered: 26 | IIR: 18 | DAC: 129 (0x81)
Raw: 548 | Centered: 36 | IIR: 28 | DAC: 129 (0x81)
...
```

### En el osciloscopio (DAC R2R):
- **Sin filtro (Raw→ADC)**: Línea base oscilante (baseline wandering)
- **Con filtro (IIR→DAC)**: Línea base mucho más estable (~127-128)
- **Diferencia**: Claramente visible después de unos segundos

---

## 🔍 Diagnóstico

### Si el filtro SIGUE sin funcionar bien:

1. **Verifica los coeficientes en el notebook**
   ```python
   # En el notebook Python
   print("b0_s1 =", int(b0_s1 * 32768))
   print("b1_s1 =", int(b1_s1 * 32768))
   # Compara con #define en arduino_ecg_reader.ino
   ```

2. **Verifica que SAMPLE_RATE=250 Hz sea correcto**
   ```cpp
   // Si tu generador WAV es diferente, ajusta aquí
   #define SAMPLE_RATE 250  // Cambiar si es necesario
   ```

3. **Aumenta el orden del filtro** (si deseas más agresividad)
   ```cpp
   // Nota: Requeriría nuevos coeficientes del notebook
   // FILTER_ORDER 4 → FILTER_ORDER 6 (requiere más variables de estado)
   ```

4. **Aumenta la agresividad de la cutoff** (en el notebook)
   ```python
   cutoff_freq = 0.25  # (en lugar de 0.5 Hz)
   # Recalcula y actualiza coeficientes
   ```

---

## 📝 Particularidades por Placa

### Arduino Uno (ATmega328P)
- ✓ Totalmente compatible
- ✓ RAM suficiente (2KB)
- RAM requerida: 32 bytes (estados) + overhead
- **Recomendación**: Usar v2.0 manual

### Arduino Nano
- ✓ Compatible (mismo ATmega328P)
- ✓ RAM suficiente
- **Recomendación**: Usar v2.0 manual

### Arduino Mega
- ✓ Compatible
- ✓ Mucho más RAM disponible
- **Recomendación**: Cualquiera de las dos versiones

### Arduino MKR / Due
- ✓ Compatible
- ✓ Procesador más poderoso (ARM)
- ✓ Punto flotante disponible
- **Recomendación**: Usar librería para mayor flexibilidad

---

## 📚 Referencias

### Filas de Butterworth
- Orden 4 = 2 secciones de 2do orden
- Cada sección: Ecuación de diferencias IIR de 2do orden
- Cascada: Salida sección 1 = entrada sección 2

### Q15 Fixed-Point
- 16 bits: 1 bit de signo + 15 bits de fracción
- Factor de escala: 32768 (2^15)
- Rango: -32768 a +32767
- Multiplicación: (a × b) >> 15

### ECG Baseline Wandering
- Causado por movimiento del electrodo, respiración
- Frecuencia: 0.1-0.5 Hz típicamente
- Solución: Filtro paso alto con cutoff ~0.5 Hz

---

## ✓ Checklist de Verificación

- [ ] Coeficientes verificados contra notebook
- [ ] SAMPLE_RATE = 250 Hz correcto
- [ ] Pines ADC y DAC conectados correctamente
- [ ] Serial monitor mostrando datos
- [ ] DAC R2R correctamente cableado (pins 2-9)
- [ ] Baseline wandering reducido visiblemente
- [ ] Sin valores saturados en IIR (-32768 o +32767 sostenidamente)
- [ ] DAC Output entre 0-255 (no fuera de rango)

---

## 📞 Soporte

**Si el filtrado sigue sin funcionar:**

1. Entra en el notebook y recalcula los coeficientes
2. Verifica que el WAV tenga exactly 250 Hz
3. Intenta la versión con librería (más robusta)
4. Aumenta orden del filtro si es necesario
5. Revisa conexiones de hardware (especialmente DAC)

---

**Última actualización:** v2.0 - Mejoras mayores en precisión y robustez
