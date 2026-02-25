# SOLUCIÓN RÁPIDA - Prueba estas 3 versiones EN ORDEN

## 1️⃣ PRIMERO: Prueba `arduino_ema_filter.ino` (MÁS SIMPLE)

❌ Si el filtro anterior no funcionaba, es porque hay un problema con los coeficientes IIR o Q15.

✅ **EMA (Exponential Moving Average)** es mucho más simple:
- Solo 2 variables de estado
- Sin multiplicaciones complicadas
- Funciona perfectamente para remover baseline wandering

**Qué hacer:**
1. Copia **`arduino_ema_filter.ino`** al Arduino IDE
2. Compila y sube
3. Observa en el osciloscopio si la señal ahora se ve filtrada
4. Mira la salida serial (115200 baud): deberías ver valores FILTERED diferentes de RAW

---

## 2️⃣ SI EMA FUNCIONA BIEN:

Entonces tu Arduino y DAC van bien. El problema fue con los coeficientes IIR.

---

## 3️⃣ SI QUIERES VOLVER AL IIR (Más preciso):

Prueba **`arduino_iir_float.ino`** (usa punto flotante, sin Q15):
- Más lento pero da certeza
- Si esto funciona, sabes que los coeficientes son correctos
- Entonces volvemos a la versión Q15 pero calculada correctamente

---

## 4️⃣ PARÁMETRO IMPORTANTE EN EMA:

```cpp
#define EMA_ALPHA_NUM 1
#define EMA_ALPHA_DEN 32    // Cambiar según necesidad
```

Valores sugeridos:
- `1/16` = Filtrado medio (menos agresivo)
- `1/32` = Filtrado fuerte (remueve más baseline)
- `1/64` = Filtrado muy fuerte

Prueba `1/32` primero, si no gusta, intenta `1/16`.

---

## Reporte:

Prueba y dime cuál funciona y qué ves en la salida serial 📊
