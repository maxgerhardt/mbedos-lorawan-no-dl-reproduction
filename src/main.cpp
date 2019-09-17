/**
 * Copyright (c) 2017, Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <global_objects.h>
#include <stdio.h>
#include <mbed_config.h>
#include <mbed.h>

#include "lorawan/LoRaWANInterface.h"
#include "lorawan/system/lorawan_data_structures.h"
#include "events/EventQueue.h"

// Application helpers
#include "trace_helper.h"
#include "lora_radio_helper.h"
#include "LoRaLinkCheck.h"


/**
 * Maximum number of events for the event queue.
 * 10 is the safe number for the stack events, however, if application
 * also uses the queue for whatever purposes, this number should be increased.
 */
#define MAX_NUMBER_OF_EVENTS            10

/**
* This event queue is the global event queue for both the
* application and stack. To conserve memory, the stack is designed to run
* in the same thread as the application and the application is responsible for
* providing an event queue to the stack that will be used for ISR deferment as
* well as application information event queuing.
*/
EventQueue lora_ev_queue(MAX_NUMBER_OF_EVENTS * EVENTS_EVENT_SIZE);

/**
 * Constructing Mbed LoRaWANInterface and passing it the radio object from lora_radio_helper.
 */
LoRaWANInterface* lorawan;

/**
 * Application specific callbacks
 */
lorawan_app_callbacks_t callbacks;

#define NWKSKEY_SIZE 16
#define APPSKEY_SIZE 16

/* PLACE TTN ABP KEYS HERE */
#define NWSKEY {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define APPSKEY {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, }
#define DEV_ADDR 0x00000000

uint8_t nwk_skey[NWKSKEY_SIZE] = NWSKEY;
uint8_t app_skey[APPSKEY_SIZE] = APPSKEY;
lorawan_connect_t cparm;
uint8_t LoRa_RX_Buf[LORAMAC_PHY_MAXPAYLOAD];

void do_tests() {
    uint32_t lora_devaddr = DEV_ADDR;
    printf("Using dev addr 0x%08lx\n", lora_devaddr);

    cparm.connect_type = LORAWAN_CONNECTION_ABP;
    cparm.connection_u.abp.nwk_id = ((uint32_t)lora_devaddr & 0xFE000000u) >> 25u;
    cparm.connection_u.abp.dev_addr = lora_devaddr;
    cparm.connection_u.abp.nwk_skey = (uint8_t *)nwk_skey;
    cparm.connection_u.abp.app_skey = (uint8_t *)app_skey;

    for (int sf = 7; sf <= 12; sf++) {
        for(int retries = 0; retries < 3; retries++) {
            printf("Doing LoRa LinkCheck for sf = %d retry = %d\n", sf, retries);
            LoRaLinkCheck::Init(LORA_LINKCHECK_MAX_UPLINKS, sf);
            LoRaLinkCheckResult res = LoRaLinkCheck::DoLinkCheck(&cparm);
            if (res == LORA_LINKCHECK_RESULT_OK) {
                printf("LoRa Connectivity Check SUCCESSFUL\n");
                break;
            } else {
                if (res == LORA_LINKCHECK_RESULT_NO_CONNECTION) {
                    printf("No connectivity.\n");
                } else {
                    printf("Link check error: %d\n",
                               (int) res);
                }
            }
        }
        printf("LinkCheck done\n");
    }
    while (true) { wait(1.0f); }
}

int main()
{
    lorawan = new LoRaWANInterface(radio);
    do_tests();
    return 0;
}

// EOF
