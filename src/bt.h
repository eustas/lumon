#ifndef LUMON_BT_H
#define LUMON_BT_H

#include <stdint.h>

/*

Glossary (from https://software-dl.ti.com/lprf/sdg-latest/html/ble-stack-3.x/)

 BT - Bluetooth

 CYW43 - BT/WiFi chip, used in the Raspberry Pi Pico W

 GAP - Generic Access Profile
  The GAP layer of the Bluetooth low energy protocol stack is responsible for
  connection functionality. This layer handles the access modes and procedures
  of the device including device discovery, link establishment,
  link termination, initiation of security features, and device configuration.

 GATT - Generic Attribute Profile
  Just as the GAP layer handles most connection-related functionality, the GATT
  layer of the Bluetooth low energy protocol stack is used by the application
  for data communication between two connected devices. Data is passed and
  stored in the form of characteristics which are stored in memory on the
  Bluetooth ow energy device.

 HCI - Host Controller Interface
  The host controller interface (HCI) layer is a thin layer which transports
  commands and events between the host and controller elements of the Bluetooth
  protocol stack. In a pure network processor application (that is, the
  host_test project), the HCI layer is implemented through a transport protocol
  such as SPI or UART.

 L2CAP - Logical Link Control and Adaptation Layer Protocol
  The L2CAP layer sits on top of the HCI layer on the host side and transfers
  data between the upper layers of the host (GAP, GATT, application) and the
  lower layer protocol stack. This layer is responsible for protocol
  multiplexing capability, segmentation, and reassembly operation for data
  exchanged between the host and the protocol stack. L2CAP permits higher-level
  protocols and applications to transmit and receive upper layer data packets
  (L2CAP service data units, SDU) up to 64KB long.

*/

void init_bt(void);

// MUST be a power of two.
#define BT_DATA_WRAP 64
// Ring buffer; when BT data arrives it is appended (if there is enough space),
// or dropped. Client is responsible in timely data reading (moving
// bt_data_start towards bt_data_end).
extern uint8_t bt_data_buf[BT_DATA_WRAP];
extern uint32_t bt_data_start;
extern uint32_t bt_data_end;

#endif  // LUMON_BT_H
