/*
 * Copyright (C) 2014 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BLUEKITCHEN
 * GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at
 * contact@bluekitchen-gmbh.com
 *
 */

#define BTSTACK_FILE__ "gatt_counter.c"

// *****************************************************************************
/* EXAMPLE_START(gatt_counter): GATT Server - Heartbeat Counter over GATT
 *
 * @text All newer operating systems provide GATT Client functionality.
 * The LE Counter examples demonstrates how to specify a minimal GATT Database
 * with a custom GATT Service and a custom Characteristic that sends periodic
 * notifications.
 */
 // *****************************************************************************

 // System headers
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Pico headers
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

// BT stack headers
#include "btstack.h"
#include "btstack_run_loop.h"
#include "ble/gatt-service/battery_service_server.h"
#ifdef WANT_HCI_DUMP
#include "platform/embedded/hci_dump_embedded_stdout.h"
#endif

// App headers
#include "pico_demo_gatt_service.h"
#include "led.h"

#define HEARTBEAT_PERIOD_MS 1000

/* @section Main Application Setup
 *
 * @text Listing MainConfiguration shows main application code.
 * It initializes L2CAP, the Security Manager and configures the ATT Server with the pre-compiled
 * ATT Database generated from $le_counter.gatt$.
 * Additionally, it enables the Battery Service Server with the current battery level.
 * Finally, it configures the advertisements
 * and the heartbeat handler and boots the Bluetooth stack.
 * In this example, the Advertisement contains the Flags attribute and the device name.
 * The flag 0x06 indicates: LE General Discoverable Mode and BR/EDR not supported.
 */

/* LISTING_START(MainConfiguration): Init L2CAP SM ATT Server and start heartbeat timer */
static int le_notification_enabled;
static btstack_timer_source_t heartbeat;
static btstack_packet_callback_registration_t hci_event_callback_registration;
static hci_con_handle_t con_handle;
static uint8_t battery = 100;

#ifdef ENABLE_GATT_OVER_CLASSIC
static uint8_t gatt_service_buffer[70];
#endif

static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static uint16_t att_read_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size);
static int att_write_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
static void  heartbeat_handler(struct btstack_timer_source *ts);
static void beat(void);

// Flags general discoverable, BR/EDR supported (== not supported flag not set) when ENABLE_GATT_OVER_CLASSIC is enabled
#ifdef ENABLE_GATT_OVER_CLASSIC
#define APP_AD_FLAGS 0x02
#else
#define APP_AD_FLAGS 0x06
#endif

const uint8_t adv_data[] = {
    // Flags general discoverable
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, APP_AD_FLAGS,
    // Name
    0x0b, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'P', 'i', 'c', 'o', ' ', 'B', 'L', 'E',
    // Incomplete List of 16-bit Service Class UUIDs -- FF10 - only valid for testing!
    0x03, BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 0x10, 0xff,
};
const uint8_t adv_data_len = sizeof(adv_data);

// Setup our GATT service
static void setup_gatt_service(void){

    l2cap_init();

    // setup SM: Display only
    sm_init();

#ifdef ENABLE_GATT_OVER_CLASSIC
    // init SDP, create record for GATT and register with SDP
    sdp_init();
    memset(gatt_service_buffer, 0, sizeof(gatt_service_buffer));
    gatt_create_sdp_record(gatt_service_buffer, sdp_create_service_record_handle(), ATT_SERVICE_GATT_SERVICE_START_HANDLE, ATT_SERVICE_GATT_SERVICE_END_HANDLE);
    btstack_assert(de_get_len( gatt_service_buffer) <= sizeof(gatt_service_buffer));
    sdp_register_service(gatt_service_buffer);

    // configure Classic GAP
    gap_set_local_name("GATT Counter BR/EDR 00:00:00:00:00:00");
    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_DISPLAY_YES_NO);
    gap_discoverable_control(1);
#endif

    // setup ATT server
    att_server_init(profile_data, att_read_callback, att_write_callback);

    // setup battery service
    battery_service_server_init(battery);

    // setup advertisements
    uint16_t adv_int_min = 0x0030;
    uint16_t adv_int_max = 0x0030;
    uint8_t adv_type = 0;
    bd_addr_t null_addr;
    memset(null_addr, 0, 6);
    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
    gap_advertisements_enable(1);

    // register for HCI events
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // register for ATT event
    att_server_register_packet_handler(packet_handler);

    // set one-shot timer
    heartbeat.process = &heartbeat_handler;
    btstack_run_loop_set_timer(&heartbeat, HEARTBEAT_PERIOD_MS);
    btstack_run_loop_add_timer(&heartbeat);

    // beat once
    beat();
}
/* LISTING_END */

/*
 * @section Heartbeat Handler
 *
 * @text The heartbeat handler updates the value of the single Characteristic provided in this example,
 * and request a ATT_EVENT_CAN_SEND_NOW to send a notification if enabled see Listing heartbeat.
 */

 /* LISTING_START(heartbeat): Hearbeat Handler */
static int  counter = 0;
static char counter_string[30];
static int  counter_string_len;
static char led_string[10];
static int led_string_len;

static void refresh_led_status() {
    led_string_len = snprintf(led_string, sizeof(led_string), (led_status()) ? "ON" : "OFF");
    puts(led_string);
}

static void beat(void){
    counter++;
    counter_string_len = snprintf(counter_string, sizeof(counter_string), "BTstack counter %04u", counter);
    puts(counter_string);

    refresh_led_status();
}

static void heartbeat_handler(struct btstack_timer_source *ts){
    if (le_notification_enabled) {
        beat();
        att_server_request_can_send_now_event(con_handle);
    }

    // simulate battery drain
    battery--;
    if (battery < 50) {
        battery = 100;
    }
    battery_service_server_set_battery_value(battery);

    btstack_run_loop_set_timer(ts, HEARTBEAT_PERIOD_MS);
    btstack_run_loop_add_timer(ts);
}
/* LISTING_END */

/*
 * @section Packet Handler
 *
 * @text The packet handler is used to:
 *        - stop the counter after a disconnect
 *        - send a notification when the requested ATT_EVENT_CAN_SEND_NOW is received
 */

/* LISTING_START(packetHandler): Packet Handler */
static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);

    printf("packet_handler - packet_type %d\n", packet_type);

    if (packet_type != HCI_EVENT_PACKET) return;

    printf("packet_handler - event_packet_type %d\n", hci_event_packet_get_type(packet));

    switch (hci_event_packet_get_type(packet)) {
        case BTSTACK_EVENT_STATE:
        {
            bd_addr_t local_addr;
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
            gap_local_bd_addr(local_addr);
            printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));
            led_off();
        }
        break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            le_notification_enabled = 0;
            printf("HCI_EVENT_DISCONNECTION_COMPLETE\n");
            break;
        case ATT_EVENT_CAN_SEND_NOW:
            att_server_notify(con_handle, ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE, (uint8_t*) counter_string, counter_string_len);
            att_server_notify(con_handle, ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE, (uint8_t*) led_string, led_string_len);
            break;
        default:
            break;
    }
}

/* LISTING_END */

/*
 * @section ATT Read
 *
 * @text The ATT Server handles all reads to constant data. For dynamic data like the custom characteristic, the registered
 * att_read_callback is called. To handle long characteristics and long reads, the att_read_callback is first called
 * with buffer == NULL, to request the total value length. Then it will be called again requesting a chunk of the value.
 * See Listing attRead.
 */

/* LISTING_START(attRead): ATT Read */

// ATT Client Read Callback for Dynamic Data
// - if buffer == NULL, don't copy data, just return size of value
// - if buffer != NULL, copy data and return number bytes copied
// @param offset defines start of attribute value
static uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size){
    UNUSED(connection_handle);

    printf("att_handle %#06x\n", att_handle);

    // Counter characteristic
    if (att_handle == ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE){
        return att_read_callback_handle_blob((const uint8_t *)counter_string, counter_string_len, offset, buffer, buffer_size);
    }
    if (att_handle == ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_USER_DESCRIPTION_HANDLE) {
        return att_read_callback_handle_blob((const uint8_t *)"Counter", 7, offset, buffer, buffer_size);
    }

    // LED characteristic
    if (att_handle == ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE) {
        refresh_led_status();
        return att_read_callback_handle_blob((const uint8_t *)led_string, led_string_len, offset, buffer, buffer_size);
    }
    if (att_handle == ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_USER_DESCRIPTION_HANDLE) {
        return att_read_callback_handle_blob((const uint8_t *)"LED Status and Control", 22, offset, buffer, buffer_size);
    }

    return 0;
}
/* LISTING_END */


/*
 * @section ATT Write
 *
 * @text The only valid ATT writes in this example are to the Client Characteristic Configuration, which configures notification
 * and indication and to the the Characteristic Value.
 * If the ATT handle matches the client configuration handle, the new configuration value is stored and used
 * in the heartbeat handler to decide if a new value should be sent.
 * If the ATT handle matches the characteristic value handle, we print the write as hexdump
 * See Listing attWrite.
 */

/* LISTING_START(attWrite): ATT Write */
static int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size){
    switch (att_handle){
        case ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE:
            le_notification_enabled = little_endian_read_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
            con_handle = connection_handle;
            break;
        case ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE:
            printf("Write: transaction mode %u, offset %u, data (%u bytes): ", transaction_mode, offset, buffer_size);
            printf_hexdump(buffer, buffer_size);
            break;
        case ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE:
            le_notification_enabled = little_endian_read_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
            con_handle = connection_handle;
            break;
        case ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE:
            printf("Value 2 write");
            printf("Write: transaction mode %u, offset %u, data (%u bytes): ", transaction_mode, offset, buffer_size);
            printf_hexdump(buffer, buffer_size);

            buffer[buffer_size] = 0;
            if (!strcmp(buffer, "OFF")) {
                led_off();
            } else if (!strcmp(buffer, "ON")) {
                led_on();
            }
            break;
        default:
            break;
    }
    return 0;
}

/**
 * Entry point
 */
int main()
{
    // Setup IO first
    stdio_init_all();

    // Initialize CYW43 driver architecture (will enable BT if/because CYW43_ENABLE_BLUETOOTH == 1)
    if (cyw43_arch_init())
    {
        printf("failed to initialise cyw43_arch\n");
        return -1;
    }

    // Turn on LED while HCI is powering up
    led_on();

    // Setup our GATT service
    setup_gatt_service();

    // Turn on Bluetooth HCI
    hci_power_control(HCI_POWER_ON);

#if WANT_HCI_DUMP
    hci_dump_init(hci_dump_embedded_stdout_get_instance());
#endif

    // Run our Bluetooth app
    btstack_run_loop_execute();
}
