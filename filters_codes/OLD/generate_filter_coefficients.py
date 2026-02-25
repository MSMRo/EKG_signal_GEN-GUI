#!/usr/bin/env python3
"""
Generador de Coeficientes de Filtro IIR para Arduino
=====================================================

Herramienta para generar y validar coeficientes de filtro Butterworth
en formato Q15 fixed-point para uso en Arduino.

Uso:
    python3 generate_filter_coefficients.py

Permite:
    - Cambiar frecuencia de corte, orden, tipo de filtro
    - Generar coeficientes en formato Q15
    - Exportar directamente a código C/Arduino
    - Visualizar respuesta en frecuencia
    - Análisis de estabilidad (polos y ceros)
"""

import numpy as np
from scipy import signal
import sys

def generate_butterworth_coefficients(
    fs=250,
    cutoff_freq=0.5,
    filter_order=4,
    filter_type='high',
    plot=False
):
    """
    Genera coeficientes de filtro Butterworth en formato Q15
    
    Parámetros:
        fs: Frecuencia de muestreo (Hz)
        cutoff_freq: Frecuencia de corte (Hz)
        filter_order: Orden del filtro
        filter_type: 'high' o 'low'
        plot: Si True, muestra gráficos
    
    Retorna:
        dict: Coeficientes SOS normalizados y en Q15
    """
    
    # Nyquist frequency
    nyquist = fs / 2
    normalized_cutoff = cutoff_freq / nyquist
    
    print(f"\n{'='*70}")
    print(f"GENERADOR DE COEFICIENTES DE FILTRO IIR BUTTERWORTH")
    print(f"{'='*70}")
    print(f"Frecuencia de muestreo (fs): {fs} Hz")
    print(f"Frecuencia de Nyquist: {nyquist} Hz")
    print(f"Frecuencia de corte: {cutoff_freq} Hz")
    print(f"Frecuencia normalizada: {normalized_cutoff:.6f}")
    print(f"Tipo de filtro: {filter_type.upper()}")
    print(f"Orden: {filter_order}")
    print(f"{'='*70}\n")
    
    # Generar coeficientes usando scipy
    btype_map = {'high': 'highpass', 'low': 'lowpass'}
    sos = signal.butter(
        filter_order,
        normalized_cutoff,
        btype=btype_map.get(filter_type, 'highpass'),
        output='sos'
    )
    
    print(f"Coeficientes SOS (scipy.signal.butter):")
    print(f"Forma: {sos.shape} (n_secciones=2, coeff=[b0,b1,b2,a0,a1,a2])\n")
    
    # Extraer y normalizar cada sección
    q15_scale = 2**15  # 32768 para Q15
    all_coeffs = []
    
    for idx, section in enumerate(sos, 1):
        b0, b1, b2, a0, a1, a2 = section
        
        # Normalizar por a0
        b0_norm = b0 / a0
        b1_norm = b1 / a0
        b2_norm = b2 / a0
        a1_norm = a1 / a0
        a2_norm = a2 / a0
        
        # Convertir a Q15
        b0_q15 = int(np.round(b0_norm * q15_scale))
        b1_q15 = int(np.round(b1_norm * q15_scale))
        b2_q15 = int(np.round(b2_norm * q15_scale))
        a1_q15 = int(np.round(a1_norm * q15_scale))
        a2_q15 = int(np.round(a2_norm * q15_scale))
        
        all_coeffs.append({
            'section': idx,
            'b0': b0_norm, 'b1': b1_norm, 'b2': b2_norm,
            'a1': a1_norm, 'a2': a2_norm,
            'b0_q15': b0_q15, 'b1_q15': b1_q15, 'b2_q15': b2_q15,
            'a1_q15': a1_q15, 'a2_q15': a2_q15,
        })
        
        print(f"SECCIÓN {idx}:")
        print(f"  Coeficientes normalizados:")
        print(f"    b0 = {b0_norm:.10f}")
        print(f"    b1 = {b1_norm:.10f}")
        print(f"    b2 = {b2_norm:.10f}")
        print(f"    a1 = {a1_norm:.10f}")
        print(f"    a2 = {a2_norm:.10f}")
        print(f"\n  Q15 Fixed-Point (16 bits):")
        print(f"    b0_s{idx} = {b0_q15:6d}  // {b0_norm:.10f}")
        print(f"    b1_s{idx} = {b1_q15:6d}  // {b1_norm:.10f}")
        print(f"    b2_s{idx} = {b2_q15:6d}  // {b2_norm:.10f}")
        print(f"    a1_s{idx} = {a1_q15:6d}  // {a1_norm:.10f}")
        print(f"    a2_s{idx} = {a2_q15:6d}  // {a2_norm:.10f}\n")
    
    # Generar código C
    print(f"\n{'='*70}")
    print("CÓDIGO C PARA ARDUINO:")
    print(f"{'='*70}\n")
    
    print("// COEFICIENTES DEL FILTRO IIR BUTTERWORTH (Q15 Fixed-Point)")
    print(f"// Frecuencia de corte: {cutoff_freq} Hz | Orden: {filter_order} | fs: {fs} Hz\n")
    
    for coeff in all_coeffs:
        section = coeff['section']
        print(f"// SECCIÓN {section} (2do orden)")
        print(f"#define b0_s{section}   {coeff['b0_q15']:6d}    // {coeff['b0']:.10f}")
        print(f"#define b1_s{section}  {coeff['b1_q15']:6d}    // {coeff['b1']:.10f}")
        print(f"#define b2_s{section}   {coeff['b2_q15']:6d}    // {coeff['b2']:.10f}")
        print(f"#define a1_s{section}  {coeff['a1_q15']:6d}    // {coeff['a1']:.10f}")
        print(f"#define a2_s{section}   {coeff['a2_q15']:6d}    // {coeff['a2']:.10f}\n")
    
    # Análisis de estabilidad
    print(f"\n{'='*70}")
    print("ANÁLISIS DE ESTABILIDAD:")
    print(f"{'='*70}\n")
    
    # Obtener polos y ceros
    z, p, k = signal.butter(
        filter_order,
        normalized_cutoff,
        btype=btype_map.get(filter_type, 'highpass'),
        output='zpk'
    )
    
    print(f"Polos (deben estar dentro del círculo unitario para estabilidad):")
    for idx, pole in enumerate(p, 1):
        magnitude = np.abs(pole)
        stable = "✓ ESTABLE" if magnitude < 1.0 else "✗ INESTABLE"
        print(f"  p{idx} = {pole:.6f}  |  Magnitud: {magnitude:.6f}  {stable}")
    
    print(f"\nCeros:")
    for idx, zero in enumerate(z, 1):
        magnitude = np.abs(zero)
        print(f"  z{idx} = {zero:.6f}  |  Magnitud: {magnitude:.6f}")
    
    all_poles_stable = all(np.abs(pole) < 1.0 for pole in p)
    print(f"\n{'✓ FILTRO ESTABLE' if all_poles_stable else '✗ FILTRO INESTABLE'}")
    
    # Respuesta en frecuencia
    print(f"\n{'='*70}")
    print("RESPUESTA EN FRECUENCIA:")
    print(f"{'='*70}\n")
    
    w, h = signal.sosfreqz(sos, fs=fs, worN=1000)
    magnitude_db = 20 * np.log10(np.abs(h) + 1e-10)
    
    # Puntos de interés
    freq_point_1 = 0.1  # 0.1 Hz
    freq_point_2 = cutoff_freq  # cutoff
    freq_point_3 = 1.0  # 1 Hz
    
    def get_magnitude_at_freq(freq):
        idx = np.argmin(np.abs(w - freq))
        return magnitude_db[idx]
    
    print(f"Atenuación @ 0.1 Hz: {get_magnitude_at_freq(0.1):.2f} dB")
    print(f"Atenuación @ {cutoff_freq} Hz: {get_magnitude_at_freq(cutoff_freq):.2f} dB")
    print(f"Atenuación @ 1.0 Hz: {get_magnitude_at_freq(1.0):.2f} dB")
    print(f"Atenuación @ 10 Hz: {get_magnitude_at_freq(10.0):.2f} dB")
    
    # Visualizar si se solicita
    if plot:
        try:
            import matplotlib.pyplot as plt
            
            fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(14, 10))
            
            # Respuesta en frecuencia (magnitud)
            ax1.semilogx(w, magnitude_db, 'b-', linewidth=2)
            ax1.grid(True, alpha=0.3)
            ax1.set_xlabel('Frecuencia (Hz)')
            ax1.set_ylabel('Magnitud (dB)')
            ax1.set_title(f'Respuesta en Frecuencia (Magnitud)')
            ax1.axvline(cutoff_freq, color='r', linestyle='--', label=f'Cutoff: {cutoff_freq} Hz')
            ax1.legend()
            
            # Polos y Ceros
            ax2.plot(np.real(z), np.imag(z), 'bo', markersize=8, label='Ceros')
            ax2.plot(np.real(p), np.imag(p), 'rx', markersize=10, markeredgewidth=2, label='Polos')
            circle = plt.Circle((0, 0), 1, fill=False, linestyle='--', color='k', linewidth=1)
            ax2.add_patch(circle)
            ax2.set_xlim(-1.5, 1.5)
            ax2.set_ylim(-1.5, 1.5)
            ax2.set_aspect('equal')
            ax2.grid(True, alpha=0.3)
            ax2.set_xlabel('Real')
            ax2.set_ylabel('Imaginario')
            ax2.set_title('Diagrama de Polos y Ceros')
            ax2.legend()
            
            # Fase
            phase = np.unwrap(np.angle(h)) * 180 / np.pi
            ax3.semilogx(w, phase, 'g-', linewidth=2)
            ax3.grid(True, alpha=0.3)
            ax3.set_xlabel('Frecuencia (Hz)')
            ax3.set_ylabel('Fase (grados)')
            ax3.set_title('Respuesta en Fase')
            
            # Respuesta al escalón
            t = np.arange(0, 2, 1/fs)
            step_input = np.ones_like(t)
            step_response = signal.sosfilt(sos, step_input)
            ax4.plot(t, step_response, 'b-', linewidth=2)
            ax4.grid(True, alpha=0.3)
            ax4.set_xlabel('Tiempo (s)')
            ax4.set_ylabel('Amplitud')
            ax4.set_title('Respuesta al Escalón')
            ax4.set_xlim(0, 2)
            
            plt.tight_layout()
            plt.savefig('filter_analysis.png', dpi=150)
            print("\nGráfico guardado como: filter_analysis.png")
            plt.show()
        except ImportError:
            print("\n(matplotlib no está instalado. Omitir gráficos.)")
    
    return all_coeffs


def main():
    """Interfaz interactiva para generar coeficientes"""
    
    print("\n╔════════════════════════════════════════════════════════════════════╗")
    print("║  GENERADOR DE COEFICIENTES DE FILTRO IIR BUTTERWORTH PARA ARDUINO  ║")
    print("╚════════════════════════════════════════════════════════════════════╝")
    
    try:
        # Parámetros por defecto (ECG baseline wandering)
        fs = 250                    # Frecuencia de muestreo
        cutoff_freq = 0.5          # Frecuencia de corte
        filter_order = 4           # Orden del filtro
        filter_type = 'high'       # Tipo de filtro
        
        print("\nParámetros por defecto (ECG - Baseline Wandering):")
        print(f"  Frecuencia de muestreo: {fs} Hz")
        print(f"  Frecuencia de corte: {cutoff_freq} Hz")
        print(f"  Orden del filtro: {filter_order}")
        print(f"  Tipo: {filter_type.upper()}")
        
        use_defaults = input("\n¿Usar estos parámetros? (s/n, default=s): ").strip().lower()
        
        if use_defaults != 'n':
            # Usar parámetros por defecto
            print("\nUsando parámetros por defecto...")
        else:
            # Solicitar parámetros personalizados
            fs = float(input("\nFrecuencia de muestreo (Hz) [250]: ") or 250)
            cutoff_freq = float(input("Frecuencia de corte (Hz) [0.5]: ") or 0.5)
            filter_order = int(input("Orden del filtro [4]: ") or 4)
            filter_type_input = input("Tipo (high/low) [high]: ").strip().lower() or 'high'
            filter_type = 'high' if 'h' in filter_type_input else 'low'
        
        # Generar coeficientes
        coeffs = generate_butterworth_coefficients(
            fs=fs,
            cutoff_freq=cutoff_freq,
            filter_order=filter_order,
            filter_type=filter_type,
            plot=False  # Cambiar a True si quieres gráficos
        )
        
        # Guardar código C en archivo
        output_file = "filter_coefficients.h"
        with open(output_file, 'w') as f:
            f.write(f"// Coeficientes de Filtro IIR Butterworth\n")
            f.write(f"// fs={fs} Hz, cutoff={cutoff_freq} Hz, orden={filter_order}\n")
            f.write(f"// Generado automáticamente - NO EDITAR MANUALMENTE\n\n")
            
            f.write("#ifndef FILTER_COEFFICIENTS_H\n")
            f.write("#define FILTER_COEFFICIENTS_H\n\n")
            
            for coeff in coeffs:
                section = coeff['section']
                f.write(f"// SECCIÓN {section}\n")
                f.write(f"#define b0_s{section}   {coeff['b0_q15']:6d}\n")
                f.write(f"#define b1_s{section}  {coeff['b1_q15']:6d}\n")
                f.write(f"#define b2_s{section}   {coeff['b2_q15']:6d}\n")
                f.write(f"#define a1_s{section}  {coeff['a1_q15']:6d}\n")
                f.write(f"#define a2_s{section}   {coeff['a2_q15']:6d}\n\n")
            
            f.write("#endif  // FILTER_COEFFICIENTS_H\n")
        
        print(f"\n✓ Código C guardado en: {output_file}")
        print(f"\nPuedes copiar el contenido de {output_file} a tu código Arduino.")
        
    except Exception as e:
        print(f"\n✗ Error: {e}")
        return 1
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
