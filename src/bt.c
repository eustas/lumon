#include "bt.h"

// System
#include <stdint.h>
#include <string.h>

// Board
#include "btstack.h"
#include "pico/cyw43_arch.h"

// Project
#include "io.h"
#include "lumo.h" // Generated from lumo.gatt

uint8_t bt_data_buf[BT_DATA_WRAP];
uint32_t bt_data_start = 0;
uint32_t bt_data_end = 0;

// TODO(eustas): explain
int le_notification_enabled;

// TODO(eustas): explain
hci_con_handle_t con_handle;

// TODO(eustas): explain
#define APP_AD_FLAGS 0x06
static uint8_t adv_data[] = {
  // Flags general discoverable
  0x02, BLUETOOTH_DATA_TYPE_FLAGS,
  APP_AD_FLAGS,
  // Name
  0x17, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, // 0x17 = 0x13 + ___
  'L', 'u', 'm', 'o',                            //               ^^^
  ' ', '0', '0', ':', '0', '0', ':', '0', '0',
  ':', '0', '0', ':', '0', '0', ':', '0', '0',
  0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS,
  0x1a, 0x18,
};
static const uint8_t adv_data_len = sizeof(adv_data);

static int heartbeat_period_ms = 500;
static btstack_timer_source_t heartbeat;

static btstack_packet_callback_registration_t hci_event_callback_registration;

static void heartbeat_handler(struct btstack_timer_source *ts) {
  static uint32_t counter = 0;
  counter++;

  // Update the temp every 10s
  //if (counter % 10 == 0) {
  //  if (le_notification_enabled) {
  //    att_server_request_can_send_now_event(con_handle);
  //  }
  //}

  // Invert the led
  static int led_on = true;
  led_on = !led_on;
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);

  // Restart timer
  btstack_run_loop_set_timer(ts, heartbeat_period_ms);
  btstack_run_loop_add_timer(ts);
}

// Shortcut.
#define IO_VALUE_HANDLE \
  ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_STRING_01_VALUE_HANDLE

void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet,
                    uint16_t size) {
  (void)size;
  (void)channel;
  // TODO(eustas): drop?
  bd_addr_t local_addr;
  if (packet_type != HCI_EVENT_PACKET)
    return;

  uint8_t event_type = hci_event_packet_get_type(packet);
  switch (event_type) {
  case BTSTACK_EVENT_STATE:
    if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING)
      return;
    gap_local_bd_addr(local_addr);
    // printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));

    // setup advertisements
    uint16_t adv_int_min = 800;
    uint16_t adv_int_max = 800;
    uint8_t adv_type = 0;
    bd_addr_t null_addr;
    memset(null_addr, 0, 6);
    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0,
                                  null_addr, 0x07, 0x00);
    assert(adv_data_len <= 31); // ble limitation
    gap_advertisements_set_data(adv_data_len, (uint8_t *)adv_data);
    gap_advertisements_enable(1);
    break;

  case HCI_EVENT_DISCONNECTION_COMPLETE:
    le_notification_enabled = 0;
    break;

  //case ATT_EVENT_CAN_SEND_NOW:
  //  att_server_notify(con_handle, HANDLE(VALUE), (uint8_t *)&magic_smoke,
  //                    sizeof(magic_smoke));
  //  break;

  default:
    break;
  }
}

uint16_t att_read_callback(hci_con_handle_t connection_handle,
                           uint16_t att_handle, uint16_t offset,
                           uint8_t *buffer, uint16_t buffer_size) {
  (void)connection_handle;

  if (att_handle == IO_VALUE_HANDLE) {
    const char str[] = "Abracadabra";
    return att_read_callback_handle_blob((const uint8_t *)&str,
                                         sizeof(str), offset, buffer,
                                         buffer_size);
  }
  return 0;
}

int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle,
                       uint16_t transaction_mode, uint16_t offset,
                       uint8_t *buffer, uint16_t buffer_size) {
  (void)transaction_mode;

  // Technically offset can be any, but its semantics is unclear. Given what
  // I see in the implementation, it should always be zero.
  if (offset != 0) {
    return 0;
  }

  if (att_handle == IO_VALUE_HANDLE) {
    if (buffer_size == 0) {
      // Empty message?
      return 0;
    }
    if (bt_data_end + buffer_size > bt_data_start + BT_DATA_WRAP) {
      // Report error?
      return 0;
    }
    uint32_t offset = bt_data_end & (BT_DATA_WRAP - 1);
    if (offset + buffer_size <= BT_DATA_WRAP) {
      memcpy(bt_data_buf + offset, buffer, buffer_size);
    } else {
      uint32_t head = BT_DATA_WRAP - offset;
      memcpy(bt_data_buf + offset, buffer, head);
      memcpy(bt_data_buf, buffer, buffer_size - head);
    }
    bt_data_end += buffer_size;
    return 0;
  }

  return 0;
}

void init_bt(void) {
  if (cyw43_arch_init()) {
    sos();
  }

  // Set up L2CAP and register L2CAP with HCI layer.
  l2cap_init();

  // Initializes the Security Manager, connects to L2CAP.
  sm_init();

  // Setup (G)ATT server.
  // profile_data is generated from .gatt file
  att_server_init(profile_data, att_read_callback, att_write_callback);

  // Inform about BTstack state.
  hci_event_callback_registration.callback = &packet_handler;
  hci_add_event_handler(&hci_event_callback_registration);

  // Register for (G)ATT event.
  att_server_register_packet_handler(packet_handler);

  // set one-shot btstack timer
  heartbeat.process = &heartbeat_handler;
  btstack_run_loop_set_timer(&heartbeat, heartbeat_period_ms);
  btstack_run_loop_add_timer(&heartbeat);

  // turn on bluetooth!
  hci_power_control(HCI_POWER_ON);

  // btstack_run_loop_execute is only required when using the 'polling' method
  // (e.g. using pico_cyw43_arch_poll library). Here we use the 'threadsafe
  // background` method, where BT work is handled in a low priority  IRQ, so it
  // is fine to call bt_stack_run_loop_execute() but equally we can continue
  // executing user code.
}
