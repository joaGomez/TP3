/***************************************************************************//**
  @file     fsk_demod.c
  @brief    Demodulador FSK Bell 202 por software (cadena de streaming, V1).

  Cadena (un sample por llamada a fsk_demod_process_sample):
     ADC(12b) -> centrar -> x[n]*x[n-D] -> FIR LPF (Q15) ->
     comparador con histeresis -> recuperacion de bits estilo UART
     (oversampling) -> framing 8O1 -> callback de byte.

  Convencion FSK (doc TP3): mark = 1 = 1200 Hz, space = 0 = 2200 Hz.
  Con D=11 @ 24 kHz: d(mark)=0.5A^2*cos(w_L*d) < 0 ; d(space) > 0.
  => y < 0  -> mark (bit 1) ;  y > 0 -> space (bit 0).
 ******************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ---- Parametros (generados/validados en host, ver design_fir.py) -------- */
#define DEMOD_FS        24000
#define DEMOD_SPB       20      /* samples por bit (24000/1200)              */
#define DEMOD_DELAY     11      /* retardo de la delay-line (~458us)         */
#define FIR_NTAPS       25
static const int16_t FIR_Q15[FIR_NTAPS] = {
    0, -2, 0, 30, 124, 331, 698, 1243, 1939, 2701, 3398, 3890, 4067, 3890,
    3398, 2701, 1939, 1243, 698, 331, 124, 30, 0, -2, 0
};

/* Umbral de histeresis del comparador. Nivel tipico |y| ~ A^2/2 ~ 1.6e6
   con A~1800 LSB; 200k ~ 12% del nivel: rechaza el rizado de 2*fL. */
#define COMP_HYST       200000

typedef void (*fsk_demod_byte_cb_t)(uint8_t byte);

/* ----------------------------- estado ------------------------------------ */
static int16_t  xline[DEMOD_DELAY + 1];        /* muestras centradas        */
static uint8_t  xidx;
static int32_t  pline[FIR_NTAPS];              /* productos x[n]*x[n-D]      */
static uint8_t  pidx;

static uint8_t  cmp_state;                     /* 1=mark(idle), 0=space      */
static uint8_t  cmp_prev;

/* recuperacion de bits / framing */
static uint8_t  rx_active;                      /* dentro de una trama        */
static int16_t  rx_count;                       /* cuenta regresiva al mid-bit*/
static uint8_t  rx_bitpos;                      /* 0=start,1..8=data,9=par,10=stop */
static uint8_t  rx_data;
static uint8_t  rx_parity_ones;

static fsk_demod_byte_cb_t rx_cb;

void fsk_demod_init(fsk_demod_byte_cb_t cb)
{
    for (uint8_t i = 0; i <= DEMOD_DELAY; i++) xline[i] = 0;
    for (uint8_t i = 0; i < FIR_NTAPS; i++)    pline[i] = 0;
    xidx = 0; pidx = 0;
    cmp_state = 1; cmp_prev = 1;          /* linea en reposo = mark           */
    rx_active = 0; rx_count = 0; rx_bitpos = 0; rx_data = 0; rx_parity_ones = 0;
    rx_cb = cb;
}

/* devuelve odd-parity bit esperado para 'data' (total de 1s impar) */
static inline uint8_t odd_parity_bit(uint8_t data)
{
    uint8_t c = 0;
    for (uint8_t i = 0; i < 8; i++) c += (data >> i) & 1u;
    return (c & 1u) ? 0u : 1u;
}

static inline void framing_step(uint8_t bit)
{
    switch (rx_bitpos) {
    case 0:                                   /* start: debe ser space (0)   */
        if (bit != 0) { rx_active = 0; return; }  /* falso start              */
        rx_data = 0; rx_parity_ones = 0;
        break;
    default:                                   /* 1..8 = datos (LSB primero)  */
        if (rx_bitpos <= 8) {
            if (bit) { rx_data |= (uint8_t)(1u << (rx_bitpos - 1)); rx_parity_ones++; }
        } else if (rx_bitpos == 9) {           /* paridad (impar)             */
            uint8_t expect = (rx_parity_ones & 1u) ? 0u : 1u;
            if (bit != expect) { rx_active = 0; return; }  /* error de paridad */
        } else { /* rx_bitpos == 10 : stop, debe ser mark(1) */
            if (bit == 1 && rx_cb) rx_cb(rx_data);
            rx_active = 0;
            return;
        }
        break;
    }
    rx_bitpos++;
    rx_count = DEMOD_SPB;                       /* proximo mid-bit            */
}

void fsk_demod_process_sample(uint16_t adc_code)
{
    /* 1) centrar a +-2047 (12 bits, 1.65V -> 2048) */
    int32_t c = (int32_t)adc_code - 2048;

    /* 2) delay line: x[n] y x[n-D] */
    xline[xidx] = (int16_t)c;
    uint8_t didx = (uint8_t)((xidx + 1) % (DEMOD_DELAY + 1)); /* el mas viejo = x[n-D] */
    int32_t xdel = xline[didx];
    xidx = (uint8_t)((xidx + 1) % (DEMOD_DELAY + 1));

    /* 3) producto x[n]*x[n-D] */
    int32_t prod = c * xdel;
    pline[pidx] = prod;

    /* 4) FIR LPF (acumulador 64b: pico ~1.6e6*32768 > int32) */
    int64_t acc = 0;
    uint8_t k = pidx;
    for (uint8_t t = 0; t < FIR_NTAPS; t++) {
        acc += (int64_t)FIR_Q15[t] * (int64_t)pline[k];
        k = (k == 0) ? (FIR_NTAPS - 1) : (uint8_t)(k - 1);
    }
    pidx = (uint8_t)((pidx + 1) % FIR_NTAPS);
    int32_t y = (int32_t)(acc >> 15);

    /* 5) comparador con histeresis: y<0 -> mark(1), y>0 -> space(0) */
    cmp_prev = cmp_state;
    if      (y < -COMP_HYST) cmp_state = 1;
    else if (y >  COMP_HYST) cmp_state = 0;
    /* zona muerta: mantiene el estado anterior */

    /* 6) recuperacion de bits estilo UART (oversampling) */
    if (!rx_active) {
        if (cmp_prev == 1 && cmp_state == 0) {     /* flanco mark->space = start */
            rx_active = 1;
            rx_bitpos = 0;
            rx_count  = DEMOD_SPB / 2;             /* muestrear en el centro del start */
        }
    } else {
        if (--rx_count <= 0) {
            framing_step(cmp_state);
        }
    }
}

/* ============================ AUTO-TEST (host) ============================ */
#ifdef DEMOD_TEST
#include <stdio.h>
#include <stdlib.h>

static uint8_t got[256]; static int ngot = 0;
static void cb(uint8_t b){ if (ngot < 256) got[ngot++] = b; }

int main(int argc, char** argv)
{
    if (argc < 2) { fprintf(stderr, "usage: %s samples.txt\n", argv[0]); return 2; }
    FILE* f = fopen(argv[1], "r");
    if (!f) { perror("open"); return 2; }
    int n; if (fscanf(f, "%d", &n) != 1) return 2;

    fsk_demod_init(cb);
    for (int i = 0; i < n; i++) {
        int code; if (fscanf(f, "%d", &code) != 1) break;
        fsk_demod_process_sample((uint16_t)code);
    }
    fclose(f);

    printf("decoded %d bytes: ", ngot);
    for (int i = 0; i < ngot; i++) printf("%02X ", got[i]);
    printf("\n");
    return 0;
}
#endif
