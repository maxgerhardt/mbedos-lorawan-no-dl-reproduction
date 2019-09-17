#include "LoRaLinkCheck.h"
#include "LoRaWAN_Stack.h"
#include <global_objects.h>
#include <EventQueue.h>
#include <LoRaWANInterface.h>
#include <EventQueue.h>
#include <psg_debug.h>
#include <mbed_trace.h>

using namespace events;

LoRaLinkCheckResult LoRaLinkCheck::linkCheckResult = LORA_LINKCHECK_RESULT_NO_CONNECTION;

extern void trace_printf(const char* str);

/**
 * Save result in variable and stop processing the LoRa event queue.
 */
#define END_LINKCHECK(result) \
    linkCheckResult = (result); \
    lorawan->disconnect();

bool LoRaLinkCheck::Init(int maxUplinkTries, int sf) {
    /*mbed_trace_init();
    mbed_trace_config_set(TRACE_ACTIVE_LEVEL_ALL);
    mbed_trace_print_function_set(&trace_printf);
     */

    // Initialize LoRaWAN stack
    if (lorawan->initialize(&lora_ev_queue) != LORAWAN_STATUS_OK) {
        psg_printf(LOG_FATAL, "LoRa initialization failed! \r\n");
        return false;
    }
    psg_printf(LOG_SUCCESS, "Mbed LoRaWANStack initialized \r\n");
    // prepare application callbacks
    callbacks.events = mbed::callback(&LoRaLinkCheck::EventHandler);
    callbacks.link_check_resp = mbed::callback(&LoRaLinkCheck::LinkCheckResponse);
    lorawan->add_app_callbacks(&callbacks);

    // Set number of retries in case of CONFIRMED messages
    if (lorawan->set_confirmed_msg_retries(maxUplinkTries)
        != LORAWAN_STATUS_OK) {
        psg_printf(LOG_FATAL,
                   "\r\n set_confirmed_msg_retries failed! \r\n\r\n");
        return false;
    }

    psg_printf(LOG_INFO, "CONFIRMED message retries : %d \r\n", maxUplinkTries);

    uint8_t dr = LoRaWAN_ConvertSFToDR(sf);
    if (lorawan->set_datarate(dr) != LORAWAN_STATUS_OK) {
        psg_printf(LOG_FATAL, "failed to set dataratate \r\n");
        return false;
    }

    //Either use all available TTN channels, or use only those
    //which are enabled by the channel mask setting.
#ifdef LORA_USE_CHANNEL_MASK
    LoRa_AddChannelWithMask((uint8_t) LORA_USE_CHANNEL_MASK);
#else
    LoRa_AddTTNChannels();
#endif
    return true;
}

LoRaLinkCheckResult LoRaLinkCheck::DoLinkCheck(lorawan_connect_t* connectParams) {
    lorawan_status_t retcode;
    retcode = lorawan->connect(*connectParams);
    if (retcode == LORAWAN_STATUS_OK ||
        retcode == LORAWAN_STATUS_CONNECT_IN_PROGRESS) {
    } else {
        psg_printf(LOG_FATAL, "Connection error, code = %d \r\n", retcode);
        return LORA_LINKCHECK_RESULT_INIT_FAILED;
    }
    psg_printf(LOG_SUCCESS, "Connection - In Progress ...\r\n");

    //start event queue until break is requested
    lora_ev_queue.dispatch_forever();
    //remove eventually added link check request
    lorawan->remove_link_check_request();
    return linkCheckResult;
}

void LoRaLinkCheck::EventHandler(lorawan_event_t event) {
    lorawan_tx_metadata txMeta{};
    lorawan_rx_metadata rxMeta{};
    int16_t retcode = 0;
    uint8_t fport = 0;
    int flags = MSG_CONFIRMED_FLAG | MSG_UNCONFIRMED_FLAG;
    switch (event) {
        case CONNECTED:
            psg_printf(LOG_SUCCESS, "Connection - Successful \r\n");
            LoRaLinkCheck::QueuePacket();
            break;
        case DISCONNECTED:
            psg_printf(LOG_SUCCESS, "Disconnected Successfully \r\n");
            lora_ev_queue.break_dispatch();
            break;
        case TX_DONE:
            lorawan->get_tx_metadata(txMeta);
            psg_printf(LOG_SUCCESS,
                       "TX DONE ToA: %d channel %d power %d retransmissions %d\n",
                       (int) txMeta.tx_toa,
                       (int) txMeta.channel, (int) txMeta.tx_power,
                       (int) txMeta.nb_retries);
            psg_printf(LOG_SUCCESS, "Message Sent to Network Server \r\n");
            break;
        case TX_TIMEOUT:
        case TX_CRYPTO_ERROR:
        case TX_SCHEDULING_ERROR:
            psg_printf(LOG_FATAL, "Transmission Error - EventCode = %d \r\n",
                       event);
            END_LINKCHECK(LORA_LINKCHECK_RESULT_TX_FAILED);
            break;
        case TX_ERROR:
            psg_printf(LOG_FATAL, "Uplink retries exhausted without getting confirmed downlink.. \r\n",
                       event);
            END_LINKCHECK(LORA_LINKCHECK_RESULT_NO_CONNECTION);
            break;
        case RX_DONE:
            psg_printf(LOG_SUCCESS,
                       "Received message from Network Server \r\n");
            lorawan->get_rx_metadata(rxMeta);
            retcode = lorawan->receive(LoRa_RX_Buf,
                                       (uint16_t) sizeof(LoRa_RX_Buf),
                                       fport,
                                       flags);
            psg_printf(LOG_SUCCESS, "RX %d byte RSSI %d SNR %d Fport %d \n",
                       (int) retcode,
                       (int) rxMeta.rssi,
                       (int) rxMeta.snr,
                       (int) fport);
            break;
            //RX failures lead to not calling the receive hook, which is ok.
            //the post TX/RX hook will always be called.
        case RX_TIMEOUT:
        case RX_ERROR:
            psg_printf(LOG_FATAL, "Error in reception - Code = %d \r\n",
                       event);
            END_LINKCHECK(LORA_LINKCHECK_RESULT_RX_FAILED);
            break;
        case JOIN_FAILURE:
            psg_printf(LOG_FATAL, "OTAA Failed - Check Keys \r\n");
            END_LINKCHECK(LORA_LINKCHECK_RESULT_CONNECT_FAILED);
            break;
        case UPLINK_REQUIRED:
            QueuePacket();
            break;
        default:
            MBED_ASSERT("Unknown Event");
    }
}

void LoRaLinkCheck::LinkCheckResponse(uint8_t demodulationMargin, uint8_t numGateways) {
    psg_printf(LOG_SUCCESS, "Link check OK. Demodulation margin: %d "
                            "Number of GWs: %d\n",
               (int) demodulationMargin,
               (int) numGateways);
    END_LINKCHECK(LORA_LINKCHECK_RESULT_OK);
}

bool LoRaLinkCheck::QueuePacket() {
    uint8_t port = 223;
    const uint8_t payload[] = {0xAB};

    int retcode = lorawan->add_link_check_request();
    if (retcode != LORAWAN_STATUS_OK) {
        psg_printf(LOG_FATAL, "Requesting a link check failed: %d\n", retcode);
        END_LINKCHECK(LORA_LINKCHECK_RESULT_TX_FAILED);
        return false;
    }

    retcode = lorawan->send(port, payload, sizeof(payload), MSG_CONFIRMED_FLAG);

    if (retcode < 0) {
        if (retcode == LORAWAN_STATUS_WOULD_BLOCK) {
            psg_printf(LOG_WARNING,
                       "send - WOULD BLOCK\r\n");
        } else {
            psg_printf(LOG_FATAL,
                       "\r\n send() - Error code %d \r\n",
                       retcode);
        }
        END_LINKCHECK(LORA_LINKCHECK_RESULT_TX_FAILED);
        return false;
    }
    return true;
}
