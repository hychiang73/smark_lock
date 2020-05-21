# Introduction

The source code is based on the Nordic RF52840 development board that supports NFC and BLE functionaility and the lastest SDK version 16.0.

We developed it following on the sample code provided by Nordic SDK, which can be found in the path `examples/ble_peripheral/experimental/ble_nfc_pairing_reference`.

# Explanation

The basic way to implement the communcations via BLE between the board and the device is to create a new customised service in GATT. We define a series of string to tell the board when and how to control the relay to lock or unlock a eletronic locker. Th string consists of command, access code, phone id and timestamp :

`|cmd | Length of access code | access code | Phone ID | timestamp |`

# File

* `blm.c` :  Bluetooth stack initialization, hanlding BLE/UART events.
* `buttons_m.c` : Handle the button events.
* `main.c` : Initialize BLE and NFC service.
* `pm_m.c` : Handle Peer management events.
* `smartlock.c` : Parse commands and deal with the controller of eletronic locker.
* `uarts.c` : Create and config a new service in GATT.

# References

[Nordic SDK 16.0](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fstruct_sdk%2Fstruct%2Fsdk_nrf5_latest.html&cp=7_1)

[Nordic DevZone](https://devzone.nordicsemi.com/)
