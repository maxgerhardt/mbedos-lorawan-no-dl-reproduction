//
// Created by max on 14.05.18.
//

#ifndef LORATLSV2_GLOBAL_OBJECTS_H
#define LORATLSV2_GLOBAL_OBJECTS_H

#include "mbed-semtech-lora-rf-drivers-physec/SX1276/SX1276_LoRaRadio.h"
#include "mbed-semtech-lora-rf-drivers-physec/SX1272/SX1272_LoRaRadio.h"
#include "LoRaWANInterface.h"
#include "events/EventQueue.h"
#include <RawSerial.h>
#include <TI_Integration/SessionManager.h>
#include <TI_Integration/LoRaCommunicationAdapter.h>
#include <TI/TI.h>
#include <Application/Generic/AppThread.h>
#include <WMSApp.h>
#include <WMCApp.h>
#include <provisioning_data.h>

typedef enum {
    MAIN_STATE_PROVISIONING = 0x00u,
    MAIN_STATE_LORA_PING_PONG = 0x01u,
    MAIN_STATE_NORMAL = 0x02u
} MAIN_STATE_VAL;

/* Make objects constructed in main.cpp visible without having
 * to hack "extern .." everywhere.
 * */
using namespace events;

#ifdef GWBOX_VARIANT
extern SX1272_LoRaRadio radio;
#else
extern SX1276_LoRaRadio radio;
#endif
extern LoRaWANInterface* lorawan;
extern lorawan_connect_t cparm;
extern EventQueue lora_ev_queue;
extern lorawan_app_callbacks_t callbacks;
extern uint8_t LoRa_RX_Buf[LORAMAC_PHY_MAXPAYLOAD];

extern mbed::RawSerial pairingSerial;
extern bool kill_switch_installed;

extern LoRaCommunicationAdapter loRaCommunicationAdapter;
extern SessionManager sessionManager;
extern TI* ti;
extern AppThread appThread;
extern WMSApp wmsApp;
extern WMCApp wmcApp;

extern DigitalOut pairingLed;

extern uint8_t main_state;

void set_main_state(uint8_t state);
uint8_t get_main_state(void);

#if PROVISIONING_ENABLED
extern PHYSEC::ProvisioningData* provisioning_data_global;
#endif

#endif //LORATLSV2_GLOBAL_OBJECTS_H
