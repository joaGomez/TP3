#include "can.h"
#include "gpio.h"
#include "hardware.h"
#include "MK64F12.h"

#define PIN_CAN0_TX    PORTNUM2PIN(PB, 18)
#define PIN_CAN0_RX    PORTNUM2PIN(PB, 19)

#define PRESDIV ((((__CORE_CLOCK__)/2) / ((CAN_BAUDRATE) * (TOTAL_TQ))) - 1)

bool CAN0_Init(void) {
	
    
    gpioInit(PIN_CAN0_TX, PORT_mAlt2);
	gpioInit(PIN_CAN0_RX, PORT_mAlt2);

    SIM->SCGC6 |= SIM_SCGC6_FLEXCAN0_MASK;
	//CAN0->MCR &= ~(CAN_MCR_MDIS(1));		// Enables FLEXCAN
    CAN0->MCR |= CAN_MCR_MDIS_MASK;
    CAN0->CTRL1 |= CAN_CTRL1_CLKSRC(1);

	CAN0->MCR = ~(CAN_MCR_MDIS(1));

    while (CAN0->MCR & CAN_MCR_LPMACK_MASK);


	
    while (!(CAN0->MCR & CAN_MCR_FRZACK_MASK));


	CAN0->CTRL1 = CAN_CTRL1_PRESDIV(PRESDIV)  	| 
                  CAN_CTRL1_PROPSEG(2)   		| // 3 Tq (Segmento Propagación)
                  CAN_CTRL1_PSEG1(5)     		| // 6 Tq (Fase 1)
                  CAN_CTRL1_PSEG2(5)     		| // 6 Tq (Fase 2)
                  CAN_CTRL1_RJW(1)       		| // Salto de resincronización (2 Tq)
                  CAN_CTRL1_CLKSRC(1);

	

	CAN0->RXMGMASK = CAN_RXMGMASK_MG(CAN_ID_STD(0x7F8));

    CAN0->MB[0].CS = CAN_CS_CODE(8);

	CAN0->MB[1].ID = CAN_ID_STD(0x100); 
    CAN0->MB[1].CS = CAN_CS_CODE(4);   

    
    for (int i = 2; i < 16; i++) {
        CAN0->MB[i].CS = CAN_CS_CODE(0);
    }

	

	CAN0->IFLAG1 = 0xFFFFFFFF;
	CAN0->IMASK1 |= (1 << 1); 
	NVIC_EnableIRQ(CAN0_ORed_Message_buffer_IRQn);

	CAN0->MCR &= ~(CAN_MCR_HALT_MASK);

    while (CAN0->MCR & CAN_MCR_FRZACK_MASK);
	return true;
}




bool CAN0_WriteMessage(uint32_t id, uint8_t* data, uint8_t size) {
    
	if ((CAN0->MB[0].CS & CAN_CS_CODE_MASK) != CAN_CS_CODE(8)) {
        return false; 
    }
	//CAN0->MB[0].CS = CAN_CS_CODE(8);

   
    CAN0->IFLAG1 = (1 << 0);
    
    CAN0->MB[0].ID = CAN_ID_STD(id);
	
	
	uint32_t words[2] = {0, 0}; 

    for (int i = 0; i < size; i++) {
        words[i / 4] |= (uint32_t)data[i] << (8 * (3 - (i % 4)));
    }

    CAN0->MB[0].WORD0 = words[0];
    CAN0->MB[0].WORD1 = words[1];
   
    
    CAN0->MB[0].CS = CAN_CS_CODE(12) | CAN_CS_DLC(size) | CAN_CS_SRR_MASK;

	return true;

}


// Variables globales para comunicación entre ISR y la APP
volatile bool nuevo_dato_disponible = false;
uint8_t datos_recibidos[8];
uint32_t id_recibido;
uint8_t  dlc_recibido;

void CAN0_ORed_Message_buffer_IRQHandler(void) {
    
    if (CAN0->IFLAG1 & (1 << 1)) {
        
        dlc_recibido = (uint8_t)((CAN0->MB[1].CS & CAN_CS_DLC_MASK) >> CAN_CS_DLC_SHIFT);
        id_recibido = (CAN0->MB[1].ID & CAN_ID_STD_MASK) >> CAN_ID_STD_SHIFT;
        
        
        uint32_t w0 = CAN0->MB[1].WORD0;
        uint32_t w1 = CAN0->MB[1].WORD1;
        
        datos_recibidos[0] = (uint8_t)(w0 >> 24);
        datos_recibidos[1] = (uint8_t)(w0 >> 16);
        datos_recibidos[2] = (uint8_t)(w0 >> 8);
        datos_recibidos[3] = (uint8_t)(w0);
        datos_recibidos[4] = (uint8_t)(w1 >> 24);
        datos_recibidos[5] = (uint8_t)(w1 >> 16);
        datos_recibidos[6] = (uint8_t)(w1 >> 8);
        datos_recibidos[7] = (uint8_t)(w1);
       
         // 4. DESBLOQUEAR EL MB
        (void)CAN0->TIMER;
       
        CAN0->MB[1].CS = CAN_CS_CODE(4);    

        CAN0->IFLAG1 = (1 << 1);
         nuevo_dato_disponible = true; 
       
    }
}




bool CAN0_MsgGetter(uint32_t *id_out, uint8_t *data_out, uint8_t *size) {
    if (!nuevo_dato_disponible) {
        return false; 
    }

    __disable_irq(); 

    *size = dlc_recibido;
    *id_out = id_recibido;
    for(uint8_t i = 0; i < dlc_recibido; i++) {
        data_out[i] = datos_recibidos[i];
    }
    nuevo_dato_disponible = false; 

    __enable_irq(); 

    return true;
}




