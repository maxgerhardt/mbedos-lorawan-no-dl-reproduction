#ifndef LORATLSV2_LORALINKCHECK_H
#define LORATLSV2_LORALINKCHECK_H

#include <stdint.h>
#include <stddef.h>
#include <lorawan_types.h>

#define LORA_LINKCHECK_MAX_UPLINKS 3

/**
 * Enum describing link check outcomes
 */
enum LoRaLinkCheckResult {
    LORA_LINKCHECK_RESULT_INIT_FAILED,
    LORA_LINKCHECK_RESULT_CONNECT_FAILED,
    LORA_LINKCHECK_RESULT_TX_FAILED,
    LORA_LINKCHECK_RESULT_RX_FAILED,
    LORA_LINKCHECK_RESULT_OK,
    LORA_LINKCHECK_RESULT_NO_CONNECTION,
};

/**
 * Class responsible for doing a link quality check.
 */
class LoRaLinkCheck {
public:
    static bool Init(int maxUplinkTries = LORA_LINKCHECK_MAX_UPLINKS, int sf = 9);

    static LoRaLinkCheckResult DoLinkCheck(lorawan_connect_t* connectParams);

private:
    static void EventHandler(lorawan_event_t event);
    static void LinkCheckResponse(uint8_t demodulationMargin, uint8_t numGateways);
    static bool QueuePacket();
    static LoRaLinkCheckResult linkCheckResult;
};


#endif //LORATLSV2_LORALINKCHECK_H
