# PlatformIO LoRaWAN example 

Derived from [mbed-os-example-lorawan](https://github.com/ARMmbed/mbed-os-example-lorawan), this code should make it issue to reproduce an issue with the mbed-os LoRaWAN stack in which no downlink is received when transmitting at SF10 or higher, due to missed RX windows.
# Settings 

See https://github.com/maxgerhardt/mbedos-lorawan-no-dl-reproduction/blob/e198b2146e6c2eadf9ac46ad031fef166af051de/src/main.cpp#L61-L64 and https://github.com/maxgerhardt/mbedos-lorawan-no-dl-reproduction/blob/master/src/lora_radio_helper.h. You must change the pin settings and LoRa keys (ABP parameters) for this to work.

# TTN notice

mbed-os listens by default on SF12 for the RX2 window. Since TTN sends RX2 traffic on SF9, your node will hear
nothing. If you want to receive data when using TTN and ABP, see https://github.com/ARMmbed/mbed-os/issues/9761

Change [this line](https://github.com/ARMmbed/mbed-os/blob/ffbd92c5a98e907c778a20bbd1cef35dbddf271c/features/lorawan/lorastack/phy/LoRaPHYEU868.cpp#L179) from 

```cpp
#define EU868_RX_WND_2_DR          DR_0
```

to SF9 (aka data rate 3)

```cpp3
#define EU868_RX_WND_2_DR          DR_3
```


# Credits

Radio driver code is courtesy of the people at https://github.com/ARMmbed/mbed-semtech-lora-rf-drivers
