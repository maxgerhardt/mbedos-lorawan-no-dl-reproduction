#ifndef LORATLSV2_GLOBAL_OBJECTS_H
#define LORATLSV2_GLOBAL_OBJECTS_H

#include <stdint.h>
#include "../lib/mbed-lora-radio-drv/SX1276/SX1276_LoRaRadio.h"
#include "../../.platformio/packages/framework-mbed/features/lorawan/LoRaWANInterface.h"
#include "../../.platformio/packages/framework-mbed/events/EventQueue.h"
#include <LoRaWANInterface.h>
#include <events/EventQueue.h>


/* Make objects constructed in main.cpp visible without having
 * to hack "extern .." everywhere.
 * */
using namespace events;

extern SX1276_LoRaRadio radio;
extern LoRaWANInterface* lorawan;
extern lorawan_connect_t cparm;
extern EventQueue lora_ev_queue;
extern lorawan_app_callbacks_t callbacks;
extern uint8_t LoRa_RX_Buf[LORAMAC_PHY_MAXPAYLOAD];

#endif //LORATLSV2_GLOBAL_OBJECTS_H
