#ifndef _CANCOMUNICATIONS_H_
#define _CANCOMUNICATIONS_H_

#include "mcal/can.h"
#include <stdint.h>
#include <stdbool.h>



/**
 * @brief Inicializa el módulo de comunicación CAN y los periféricos asociados.
 * @return false si hubo un error en la inicialización, true si se configuró correctamente.
 */
bool CANCom_Init(void);

/**
 * @brief Verifica la disponibilidad de nuevos mensajes en el bus CAN.
 * @return false si no hay datos nuevos disponibles, true si se recibió y copió un nuevo mensaje.
 */
bool CANReceived(void);

/**
 * @brief Evalúa la semántica del mensaje y gestiona los LEDs en caso de tramas RGB.
 * @return false si el mensaje procesado fue un comando RGB, true si es un mensaje de posición válido.
 */
bool isMsgPosition(void); // Devuelve false si es RGB, se encarga en caso de RGB de cambiar color

/**
 * @brief Formatea manualmente la última trama CAN recibida en un string compatible con la GUI.
 * @param USBData Puntero al buffer de memoria (char array) donde se guardará el string resultante.
 * @return void
 */
uint8_t USBUpdate(char * CANData, char * CANDataType); // Devuelve Grupo

/**
 * @brief Empaqueta y transmite datos en formato ASCII de longitud variable hacia el bus CAN.
 * @param CANData Puntero a la cadena de caracteres ASCII que contiene el valor numérico del ángulo.
 * @param CANDataType Carácter que define el tipo de medición ('R' para Roll, 'C' para Pitch, 'O' para Yaw).
 * @return false si el buffer de transmisión de FlexCAN estaba lleno, true si el mensaje se cargo con éxito.
 */
bool SendCAN(char * CANData, char CANDataType, uint8_t size);

#endif // _CANCOMUNICATIONS_H_
