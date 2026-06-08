#include "CANComunications.h"
#include "leds.h"

#define GROUPSHIFT 4
#define GROUPMASK  0x70
#define GROUP(x) (uint8_t)(((x) & GROUPMASK) >> GROUPSHIFT)
#define COLORMASK  0x07
#define COLOR(x) (uint8_t)((x) & COLORMASK)

#define DIGITS_RECEIVED(x) ((x) - 1)
#define CANT0(x) (4 - (DIGITS_RECEIVED(x)))

#define CAN_GROUP_MASK   0x00000007U



char msg[8];
uint32_t id;
uint8_t msglength;



bool CANCom_Init(void){
    CAN0_Init();
    return true;
}

bool CANReceived(void){
    bool newmsgflag = CAN0_MsgGetter( &id, (uint8_t*)msg, &msglength);
    return newmsgflag;
}




bool isMsgPosition(void){

     if((msg[0] & 0x80)){
        if((GROUP(msg[0]) == GRUPO_NUM)){
            ledOff(WHITE);
            ledOn(COLOR(msg[0]));
        }
        return false;
     }

    return true;
}

/*
void USBUpdate(char * USBData){

    
    // Usamos la máscara para quedarnos solo con los 3 bits más bajos (0 a 7)
    uint8_t ngroup = (uint8_t)(id & CAN_GROUP_MASK) - 1;
    
    // Mapeamos la letra del formato de mensajes CAN ('R', 'C', 'O') al número de ángulo que quiere la GUI (0, 1, 2)
    char angleId = '0'; 
    if (msg[0] == 'C') {
        angleId = '1'; // Pitch
    } else if (msg[0] == 'O') {
        angleId = '2'; // Yaw
    } // Si es 'R', queda en 0 (Roll)

    uint8_t i = 0;
    
    USBData[i++] = '>';
    USBData[i++] = 'S';
    USBData[i++] = (char)(ngroup + '0'); // Convertimos el entero (0-6) a su carácter ASCII ('0'-'6')
    USBData[i++] = ',';
    USBData[i++] = 'A';
    USBData[i++] = angleId;       // Ya es un carácter ASCII
    USBData[i++] = ',';
    USBData[i++] = 'V';

    // Copiamos los caracteres del número que ya están en formato ASCII 
    for(uint8_t j = 1; j < msglength; j++) {
        USBData[i++] = msg[j];
    }
    
    // Completamos el salto de linea y el EOS
    USBData[i++] = '\n';
    USBData[i]   = '\0';

    return;
}*/



uint8_t USBUpdate(char * CANData, char * CANDataType){

    *CANDataType = msg[0];
    
    uint8_t cant0 = CANT0(msglength);
    uint8_t i = 0;
    uint8_t j = 1;
    int8_t sign = 1;

    if (msg[j] == '-') {
    	sign *= -1;
    	CANData[i++] = msg[1];
    	j++;
    }



    /*for(uint8_t k=0; k < cant0; k++){
        CANData[i++] = '0';
    }*/

    while(j<msglength) {
    	CANData[i++]= msg[j];
    	j++;
    }

    for(j; j< msglength; j++){
        CANData[i++]= msg[j];
    }
    CANData[i] = '\0';

    uint8_t group = (uint8_t)(id & CAN_GROUP_MASK);

    return group;
}


bool SendCAN(char * CANData, char CANDataType, uint8_t size){
    


    if (CANDataType == 'L')
    {
        return CAN0_WriteMessage(MY_ID, (uint8_t*)CANData , 1);
    }
    

    char angle[size+1];

    angle[0] = CANDataType;

    for(uint8_t i = 0; i < size; i++){
        angle[1+i] = CANData[i];
    }
    
    
    return CAN0_WriteMessage(MY_ID, (uint8_t*)angle , size+1);
}

