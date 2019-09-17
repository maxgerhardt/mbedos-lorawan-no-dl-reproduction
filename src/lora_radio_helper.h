#include "lorawan/LoRaRadio.h"

#ifndef APP_LORA_RADIO_HELPER_H_
#define APP_LORA_RADIO_HELPER_H_

#include "SX1272_LoRaRadio.h"
#include "SX1276_LoRaRadio.h"

#define PIN_RADIO_NSS D10       /* Not Slave Select */
#define PIN_RADIO_MOSI D11      /* MOSI */
#define PIN_RADIO_MISO D12      /* MISO */
#define PIN_RADIO_SCLK D13      /* SCLK */
#define PIN_RADIO_RST A0        /* RST */
#define PIN_RADIO_RXTX A4       /* RXTX */
#define PIN_RADIO_DIO0 D2       /* DIO 0 */
#define PIN_RADIO_DIO1 D3       /* DIO 1 */
#define PIN_RADIO_DIO2 D4       /* DIO 2 */

SX1276_LoRaRadio radio(PIN_RADIO_MOSI, PIN_RADIO_MISO, PIN_RADIO_SCLK,
                       PIN_RADIO_NSS, PIN_RADIO_RST, PIN_RADIO_DIO0,
                       PIN_RADIO_DIO1, PIN_RADIO_DIO2, NC, NC, NC,
                       NC, NC, NC, NC, NC, NC, NC);

#endif /* APP_LORA_RADIO_HELPER_H_ */
