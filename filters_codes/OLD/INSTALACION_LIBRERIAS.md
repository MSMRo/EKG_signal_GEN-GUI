# Instalación de Librerías para Arduino

## Librería de Filtro IIR - ArduinoIIRFilter

### ¿Qué es ArduinoIIRFilter?
Es una librería pública de Arduino que implementa filtros IIR de forma optimizada y confiable, eliminando la necesidad de implementar manualmente la aritmética de punto fijo Q15.

### Ventajas:
- ✓ Código optimizado y probado en producción
- ✓ Mejor manejo de overflow/underflow
- ✓ Soporta diferentes tipos de filtros (paso alto, paso bajo, paso banda)
- ✓ Fácil de usar y mantener
- ✓ Mejor rendimiento que implementación manual

### Instalación Manual:

#### Opción 1: Desde Arduino IDE
1. Abre Arduino IDE
2. Ve a **Sketch → Include Library → Manage Libraries**
3. Busca: `ArduinoIIRFilter`
4. Instala la versión más reciente
5. También instala: `DSP library for Arduino`

#### Opción 2: Descarga directa
1. Ve a: https://github.com/rfetick/ArduinoIIRFilter
2. Descarga como ZIP
3. En Arduino IDE: **Sketch → Include Library → Add .ZIP Library**
4. Selecciona el ZIP descargado

#### Opción 3: Copiar manualmente
1. Descarga el repositorio
2. Copia la carpeta `ArduinoIIRFilter` a:
   - Windows: `Documents\Arduino\libraries\`
   - Linux: `~/Arduino/libraries/`
   - Mac: `~/Documents/Arduino/libraries/`

### Verificación de instalación:
```cpp
#include <ArduinoIIRFilter.h>

// Si compila sin errores, está correctamente instalada
```

### Filtro Butterworth configurado para ECG:
```cpp
// Filtro paso alto para eliminar baseline wandering
// Orden 4, Frecuencia de corte 0.5 Hz, Frecuencia de muestreo 250 Hz

IIRFilter iirFilter(
  IIRFilter::HIGH_PASS,    // Tipo: paso alto
  0.5 / 125.0,            // Frecuencia normalizada (0.5 Hz / 125 Hz Nyquist)
  0.707,                  // Damping (Q factor)
  4                       // Orden del filtro
);
```

### Uso en el código:
```cpp
// Lectura del sensor
int rawValue = analogRead(INPUT_PIN);

// Aplicar filtro
int filteredValue = iirFilter.apply(rawValue);

// Escalar a DAC de 8 bits
int dacValue = constrain((filteredValue >> 2) + 128, 0, 255);
writeDAC8Bit(dacValue);
```

## Alternativas de librerías:

### 1. **DSP Library** (ARM CMSIS-DSP)
- Para microcontroladores más potentes (ARM Cortex-M)
- Mayor compatibilidad con SAM, Arduino MKR, etc.
- https://github.com/ARM-software/CMSIS_5

### 2. **Filters** (Analog Devices)
- Librería oficial de Analog Devices
- Muy optimizada pero más compleja
- https://github.com/analogdevicesinc/Arduino

### 3. **Simple IIR Filter**
- Implementación minimalista
- Buena para aplicaciones simples
- Menos overhead de memoria

## Recomendación:
Para este proyecto ECG con Arduino UNO/Nano, **ArduinoIIRFilter** es la mejor opción:
- Tamaño optimizado
- Sin dependencias adicionales
- Fácil configuración
- Código limpio y mantenible
