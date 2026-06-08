#include "hardware.h"
#include "uart.h"
#include "gpio.h"


#define UART_DEFAULT_BAUDRATE 115200

#define PIN_UART0_TX 	PORTNUM2PIN(PB, 17)
#define PIN_UART0_RX 	PORTNUM2PIN(PB, 16)

void UART_SetBaudRate (UART_Type *uart, uint32_t baudrate);
void UART_Send_Data(unsigned char tx_data);

#define BUF_SIZE 128

typedef struct {
    uint8_t data[BUF_SIZE];
    volatile uint16_t head; // Índice de escritura
    volatile uint16_t tail; // Índice de lectura
} RingBuffer_t;

static RingBuffer_t rx_buffer = {{0}, 0, 0};
static RingBuffer_t tx_buffer = {{0}, 0, 0};


void UART_Init (void)
{
    // Habilitar Clocks de Puertos y Periféricos
    gpioInit(PIN_UART0_TX, PORT_mAlt3);
    gpioInit(PIN_UART0_RX, PORT_mAlt3);

    SIM->SCGC4 |= SIM_SCGC4_UART0_MASK;
    SIM->SCGC4 |= SIM_SCGC4_UART1_MASK;


    PORTB->PCR[PIN2NUM(PIN_UART0_RX)] |= PORT_PCR_IRQC(PORT_eDisabled);
    PORTB->PCR[PIN2NUM(PIN_UART0_TX)] |= PORT_PCR_IRQC(PORT_eDisabled);

    // Desactivar Transmisor y Receptor para configurar parámetros críticos
    UART0->C2 &= ~(UART_C2_TE_MASK | UART_C2_RE_MASK);

    // Configurar Baudrate
    UART_SetBaudRate(UART0, UART_DEFAULT_BAUDRATE);

    // ---------------------
    // CONFIGURACIÓN DE FIFO
    // ---------------------

    // Habilitar FIFOs de Transmisión y Recepción
    UART0->PFIFO |= (UART_PFIFO_TXFE_MASK | UART_PFIFO_RXFE_MASK);

    // Watermarks
    UART0->RWFIFO = 1; // Interrumpir cuando haya 1 byte acumulado en RX  
                       // Sino necesito enviar varias veces la data de los leds
    UART0->TWFIFO = 2; // Interrumpir cuando queden 2 espacios o menos en TX

    // Limpiar (Flush) FIFOs para descartar basura previa
    UART0->CFIFO |= (UART_CFIFO_TXFLUSH_MASK | UART_CFIFO_RXFLUSH_MASK);

    // Habilitar Interrupciones en el NVIC
    NVIC_EnableIRQ(UART0_RX_TX_IRQn);
    NVIC_EnableIRQ(UART1_RX_TX_IRQn);

    // Habilitar UART y activar Interrupción de Recepción (RIE)
    // La interrupción de Transmisión (TIE) se activará solo cuando tengamos datos para enviar.
    UART0->C2 |= (UART_C2_TE_MASK | UART_C2_RE_MASK | UART_C2_RIE_MASK);
}


void UART_SetBaudRate (UART_Type *uart, uint32_t baudrate)
{
	uint16_t sbr, brfa;
	uint32_t clock;

	clock = ((uart == UART0) || (uart == UART1))?(__CORE_CLOCK__):(__CORE_CLOCK__ >> 1);

	baudrate = ((baudrate == 0)?(UART_DEFAULT_BAUDRATE):
			((baudrate > 0x1FFF)?(UART_DEFAULT_BAUDRATE):(baudrate)));

	sbr = clock / (baudrate << 4);               // sbr = clock/(Baudrate x 16)
	brfa = (clock << 1) / baudrate - (sbr << 5); // brfa = 2*Clock/baudrate - 32*sbr

	uart->BDH = UART_BDH_SBR(sbr >> 8);
	uart->BDL = UART_BDL_SBR(sbr);
	uart->C4 = (uart->C4 & ~UART_C4_BRFA_MASK) | UART_C4_BRFA(brfa);
}



// Envía datos: Los pone en el buffer de RAM y activa la interrupción
void UART_Send_Data(uint8_t data) {
    uint16_t next = (tx_buffer.head + 1) % BUF_SIZE;
    while (next == tx_buffer.tail); // Esperar solo si el buffer de RAM está lleno

    tx_buffer.data[tx_buffer.head] = data;
    tx_buffer.head = next;

    // IMPORTANTE: Activar interrupción de TX para que el ISR empiece a vaciar la RAM al FIFO
    UART0->C2 |= UART_C2_TIE_MASK;
}


bool UART_Receive_Data(uint8_t *byte) {
    // Si la cabeza y la cola coinciden, la interrupción no guardó nada nuevo
    if (rx_buffer.head == rx_buffer.tail) {
        return false; // Salimos inmediatamente sin bloquear el flujo
    }

    // Extraemos el dato apuntado por la cola (tail)
    *byte = rx_buffer.data[rx_buffer.tail];
    
    // Avanzamos la cola de forma circular
    rx_buffer.tail = (rx_buffer.tail + 1) % BUF_SIZE;
    
    return true; // Avisamos que devolvimos un dato válido
}

void UART_SendString(char* str) {
    while(*str) {
        UART_Send_Data((unsigned char)*str);
        str++;
    }
}


void UART0_RX_TX_IRQHandler(void) {
    uint8_t s1 = UART0->S1;

    // --- AGREGADO DE SEGURIDAD ---
    // Si hay flags de error activos (OR: Overrun, NF: Ruido, FE: Framing, PF: Paridad)
    if (s1 & (UART_S1_OR_MASK | UART_S1_NF_MASK | UART_S1_FE_MASK | UART_S1_PF_MASK)) {
        // En los Kinetis, los flags de error de S1 se limpian leyendo S1 (ya lo hiciste arriba)
        // y luego realizando una lectura "dummy" del registro de datos D.
        volatile uint8_t dummy_read = UART0->D;
        (void)dummy_read; // Evita el warning de variable no usada
    }

    // CASO 1: Recepción (El FIFO de RX alcanzó el Watermark)
    if (s1 & UART_S1_RDRF_MASK) {
        // Mientras haya datos en el FIFO de hardware, moverlos a RAM
        while (UART0->RCFIFO > 0) {
            uint8_t data = UART0->D;
            uint16_t next = (rx_buffer.head + 1) % BUF_SIZE;
            if (next != rx_buffer.tail) { // Si hay espacio en RAM
                rx_buffer.data[rx_buffer.head] = data;
                rx_buffer.head = next;
            }
        }
    }

    // CASO 2: Transmisión (El FIFO de TX tiene espacio)
    if (UART0->C2 & UART_C2_TIE_MASK) { // Si la interrupción de TX está habilitada
        if (s1 & UART_S1_TDRE_MASK) {
            // Llenar el FIFO de hardware con lo que haya en la RAM
            while (UART0->TCFIFO < 8 && tx_buffer.head != tx_buffer.tail) {
                UART0->D = tx_buffer.data[tx_buffer.tail];
                tx_buffer.tail = (tx_buffer.tail + 1) % BUF_SIZE;
            }

            // Si ya no hay nada más que enviar en la RAM, apagar interrupción de TX
            if (tx_buffer.head == tx_buffer.tail) {
                UART0->C2 &= ~UART_C2_TIE_MASK;
            }
        }
    }
}
