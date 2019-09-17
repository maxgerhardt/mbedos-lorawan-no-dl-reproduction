#include "LoRaLinkCheck.h"
#include <global_objects.h>
#include <EventQueue.h>
#include <LoRaWANInterface.h>
#include <EventQueue.h>
#include <mbed_trace.h>

using namespace events;

LoRaLinkCheckResult LoRaLinkCheck::linkCheckResult = LORA_LINKCHECK_RESULT_NO_CONNECTION;

//just replace physec debug log lib functions with normal printf for now..
#define psg_printf(level, args...) printf(args)
uint8_t LoRaWAN_ConvertSFToDR(int sf);
void LoRaWAN_SetChannelPlan(const lorawan_channelplan_t &channelPlan);
void LoRa_AddChannelWithMask(uint8_t channelMask);

/**
 * Save result in variable and stop processing the LoRa event queue.
 */
#define END_LINKCHECK(result) \
    linkCheckResult = (result); \
    lorawan->disconnect();

bool LoRaLinkCheck::Init(int maxUplinkTries, int sf) {
    /*mbed_trace_init();
    mbed_trace_config_set(TRACE_ACTIVE_LEVEL_ALL);
    //mbed_trace_print_function_set(&trace_printf);
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
    lorawan->disable_adaptive_datarate();

    psg_printf(LOG_INFO, "CONFIRMED message retries : %d \r\n", maxUplinkTries);

    uint8_t dr = LoRaWAN_ConvertSFToDR(sf);
    lorawan_status_t ret;
    if ((ret = lorawan->set_datarate(dr)) != LORAWAN_STATUS_OK) {
        psg_printf(LOG_FATAL, "failed to set dataratate: %d\r\n", (int) ret);
        return false;
    }

    //Either use all available TTN channels, or use only those
    //which are enabled by the channel mask setting.
#define LORA_USE_CHANNEL_MASK 0xff
#ifdef LORA_USE_CHANNEL_MASK
    LoRa_AddChannelWithMask((uint8_t) LORA_USE_CHANNEL_MASK);
#else
    LoRa_AddTTNChannels();
#endif
    return true;
}

LoRaLinkCheckResult LoRaLinkCheck::DoLinkCheck(lorawan_connect_t* connectParams) {
    lorawan_status_t retcode;
    linkCheckResult = LORA_LINKCHECK_RESULT_NO_CONNECTION;
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
            if(linkCheckResult != LORA_LINKCHECK_RESULT_OK) {
                linkCheckResult = LORA_LINKCHECK_RESULT_NO_CONNECTION;
            }
            lorawan->disconnect();
            break;
        case TX_TIMEOUT:
        case TX_CRYPTO_ERROR:
        case TX_SCHEDULING_ERROR:
            psg_printf(LOG_FATAL, "Transmission Error - EventCode = %d \r\n",
                       event);
            END_LINKCHECK(LORA_LINKCHECK_RESULT_TX_FAILED);
            break;
        case TX_ERROR:
            psg_printf(LOG_FATAL, "Uplink retries exhausted without getting confirmed downlink.. \r\n");
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

    retcode = lorawan->send(port, payload, sizeof(payload), MSG_UNCONFIRMED_FLAG);

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

uint8_t LoRaWAN_ConvertSFToDR(int sf) {
    uint8_t lorawan_dr = DR_0;
    switch (sf) {
        case 12:
            lorawan_dr = DR_0;
            break;
        case 11:
            lorawan_dr = DR_1;
            break;
        case 10:
            lorawan_dr = DR_2;
            break;
        case 9:
            lorawan_dr = DR_3;
            break;
        case 8:
            lorawan_dr = DR_4;
            break;
        case 7:
            lorawan_dr = DR_5;
            break;
        default:
            break;
    }
    return lorawan_dr;
}

/* https://www.thethingsnetwork.org/docs/lorawan/frequency-plans.html#eu863-870 */
static loramac_channel_t ttnChannels[] = {
        {0, {868100000, 0, {(DR_5 << 4) | DR_0}, 1}},
        {1, {868300000, 0, {(DR_5 << 4) | DR_0}, 1}},
        {2, {868500000, 0, {(DR_5 << 4) | DR_0}, 1}},
        {3, {867100000, 0, {(DR_5 << 4) | DR_0}, 0}},
        {4, {867300000, 0, {(DR_5 << 4) | DR_0}, 0}},
        {5, {867500000, 0, {(DR_5 << 4) | DR_0}, 0}},
        {6, {867700000, 0, {(DR_5 << 4) | DR_0}, 0}},
        {7, {867900000, 0, {(DR_5 << 4) | DR_0}, 0}}
};

void LoRa_AddChannelWithMask(uint8_t channelMask) {
    lorawan_channelplan_t channelPlan{};
    //To supress the warning "variable escapes local scope"
    // this doesn't logically happen anyways
    loramac_channel_t channels[8];
    channelPlan.channels = channels;
    channelPlan.channels = (loramac_channel_t*) channels;

    //Parse new channels
    uint8_t usedChannels = 0;
    for (uint8_t i = 0; i < 8; i++) {
        //Is this bit set?
        if (channelMask & (1u << i)) {
            //Then take a copy of this channel and
            //put it in the channel plan
            channels[usedChannels] = ttnChannels[i];
            usedChannels++;
        }
    }
    channelPlan.nb_channels = usedChannels;
    LoRaWAN_SetChannelPlan(channelPlan);
}

void LoRaWAN_SetChannelPlan(const lorawan_channelplan_t &channelPlan) {
    lorawan_status_t ret;
    psg_printf(LOG_INFO, "Setting channel plan with %d channels.\n",
               (int) channelPlan.nb_channels);
    //remove previously set channel plan
    if ((ret = lorawan->remove_channel_plan()) != LORAWAN_STATUS_OK) {
        psg_printf(LOG_FATAL, "[+] Removing old channels failed: %d\n", (int) ret);
        //still try to set new channel plan
    }
    if ((ret = lorawan->set_channel_plan(channelPlan)) != LORAWAN_STATUS_OK) {
        psg_printf(LOG_FATAL,
                   "[-] Failed to set TTN channels! Debug return code: %d\n", (int) ret);
    }
}