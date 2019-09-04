/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Ayke van Laethem
 * Copyright (c) 2019 Jim Mussared
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef MICROPY_INCLUDED_EXTMOD_MODBLUETOOTH_H
#define MICROPY_INCLUDED_EXTMOD_MODBLUETOOTH_H

#include <stdbool.h>

#include "py/obj.h"
#include "py/objlist.h"
#include "py/ringbuf.h"

// Port specific configuration.
#ifndef MICROPY_PY_BLUETOOTH_RINGBUF_SIZE
#define MICROPY_PY_BLUETOOTH_RINGBUF_SIZE (128)
#endif

#ifndef MICROPY_PY_BLUETOOTH_CALLBACK_ALLOC
#define MICROPY_PY_BLUETOOTH_CALLBACK_ALLOC (0)
#endif

#ifndef MICROPY_PY_BLUETOOTH_ENABLE_CENTRAL_MODE
#define MICROPY_PY_BLUETOOTH_ENABLE_CENTRAL_MODE (0)
#endif

// Common constants.
#ifndef MP_BLUETOOTH_MAX_ATTR_SIZE
#define MP_BLUETOOTH_MAX_ATTR_SIZE (20)
#endif

// Advertisement packet lengths
#define MP_BLUETOOTH_GAP_ADV_MAX_LEN (32)

#define MP_BLUETOOTH_CHARACTERISTIC_FLAG_READ     (1 << 1)
#define MP_BLUETOOTH_CHARACTERISTIC_FLAG_WRITE    (1 << 3)
#define MP_BLUETOOTH_CHARACTERISTIC_FLAG_NOTIFY   (1 << 4)

// Type value also doubles as length.
#define MP_BLUETOOTH_UUID_TYPE_16  (2)
#define MP_BLUETOOTH_UUID_TYPE_32  (4)
#define MP_BLUETOOTH_UUID_TYPE_128 (16)

// Address types (for the addr_type params).
// Ports will need to map these to their own values.
#define MP_BLUETOOTH_ADDR_PUBLIC                        (0x00)  // Public (identity) address. (Same as NimBLE and NRF SD)
#define MP_BLUETOOTH_ADDR_RANDOM_STATIC                 (0x01)  // Random static (identity) address. (Same as NimBLE and NRF SD)
#define MP_BLUETOOTH_ADDR_PUBLIC_ID                     (0x02)  // (Same as NimBLE)
#define MP_BLUETOOTH_ADDR_RANDOM_ID                     (0x03)  // (Same as NimBLE)
#define MP_BLUETOOTH_ADDR_RANDOM_PRIVATE_RESOLVABLE     (0x12) // Random private resolvable address. (NRF SD 0x02)
#define MP_BLUETOOTH_ADDR_RANDOM_PRIVATE_NON_RESOLVABLE (0x13) // Random private non-resolvable address. (NRF SD 0x03)

// Event codes for the IRQ handler.
// Can also be combined to pass to the trigger param to select which events you are interested in.
// Note this is currently stored in a uint16_t (trigger, event), so one bit free.
#define MP_BLUETOOTH_IRQ_CENTRAL_CONNECT                  (1 << 1)
#define MP_BLUETOOTH_IRQ_CENTRAL_DISCONNECT               (1 << 2)
#define MP_BLUETOOTH_IRQ_CHARACTERISTIC_WRITE             (1 << 3)
#define MP_BLUETOOTH_IRQ_CHARACTERISTIC_READ_REQUEST      (1 << 4)
#define MP_BLUETOOTH_IRQ_SCAN_RESULT                      (1 << 5)
#define MP_BLUETOOTH_IRQ_SCAN_COMPLETE                    (1 << 6)
#define MP_BLUETOOTH_IRQ_PERIPHERAL_CONNECT               (1 << 7)
#define MP_BLUETOOTH_IRQ_PERIPHERAL_DISCONNECT            (1 << 8)
#define MP_BLUETOOTH_IRQ_PERIPHERAL_SERVICE_RESULT        (1 << 9)
#define MP_BLUETOOTH_IRQ_PERIPHERAL_CHARACTERISTIC_RESULT (1 << 10)
#define MP_BLUETOOTH_IRQ_PERIPHERAL_DESCRIPTOR_RESULT     (1 << 11)
#define MP_BLUETOOTH_IRQ_PERIPHERAL_READ_RESULT           (1 << 12)
#define MP_BLUETOOTH_IRQ_PERIPHERAL_WRITE_STATUS          (1 << 13)
#define MP_BLUETOOTH_IRQ_PERIPHERAL_NOTIFY                (1 << 14)
#define MP_BLUETOOTH_IRQ_PERIPHERAL_INDICATE              (1 << 15)
#define MP_BLUETOOTH_IRQ_ALL                              (0xffff)

/*
These aren't included in the module for space reasons, but can be used
in your Python code if necessary.

from micropython import const
IRQ_CENTRAL_CONNECT                  = const(1 << 1)
IRQ_CENTRAL_DISCONNECT               = const(1 << 2)
IRQ_CHARACTERISTIC_WRITE             = const(1 << 3)
IRQ_SCAN_RESULT                      = const(1 << 4)
IRQ_SCAN_COMPLETE                    = const(1 << 5)
IRQ_PERIPHERAL_CONNECT               = const(1 << 6)
IRQ_PERIPHERAL_DISCONNECT            = const(1 << 7)
IRQ_PERIPHERAL_SERVICE_RESULT        = const(1 << 8)
IRQ_PERIPHERAL_CHARACTERISTIC_RESULT = const(1 << 9)
IRQ_PERIPHERAL_DESCRIPTOR_RESULT     = const(1 << 10)
IRQ_PERIPHERAL_READ_RESULT           = const(1 << 11)
IRQ_PERIPHERAL_WRITE_STATUS          = const(1 << 12)
IRQ_PERIPHERAL_NOTIFY                = const(1 << 13)
IRQ_PERIPHERAL_INDICATE              = const(1 << 14)
IRQ_ALL                              = const(0xffff)
*/

// Main bluetooth module type
typedef struct {
    mp_obj_base_t base;
    mp_obj_t irq_handler;
    uint16_t irq_trigger;
    ringbuf_t ringbuf;
    mp_obj_dict_t char_handles;
} mp_obj_bluetooth_t;

// Common UUID type.
// Ports are expected to map this to their own internal UUID types.
typedef struct {
    mp_obj_base_t base;
    uint8_t type;
    union {
        uint16_t _16;
        uint32_t _32;
        uint8_t _128[16];
    } uuid;
} mp_obj_bluetooth_uuid_t;


// Memory allocation for bluetooth
void *m_malloc_bluetooth(size_t size);
#define m_new_bluetooth(type, num) ((type*)m_malloc_bluetooth(sizeof(type) * (num)))

//////////////////////////////////////////////////////////////
// API implemented by ports (i.e. called from modbluetooth.c):

// TODO: At the moment this only allows for a single `Bluetooth` instance to be created.
// Ideally in the future we'd be able to have multiple instances or to select a specific BT driver or HCI UART.
// So these global methods should be replaced with a struct of function pointers (like the machine.I2C implementations).

// Any method returning an int returns errno on failure, otherwise zero.

// Performs any initial setup for stack.
int mp_bluetooth_init(void);

// Enables the Bluetooth stack.
int mp_bluetooth_enable(void);

// Disables the Bluetooth stack. Is a no-op when not enabled.
void mp_bluetooth_disable(void);

// Returns true when the Bluetooth stack is enabled.
bool mp_bluetooth_is_enabled(void);

// Gets the MAC addr of this device in LSB format.
void mp_bluetooth_get_addr(uint8_t *addr);

// Start advertisement. Will re-start advertisement when already enabled.
// Returns errno on failure.
int mp_bluetooth_advertise_start(bool connectable, uint16_t interval_ms, const uint8_t *adv_data, size_t adv_data_len, const uint8_t *sr_data, size_t sr_data_len);

// Stop advertisement. No-op when already stopped.
void mp_bluetooth_advertise_stop(void);

// // Add a service with the given list of characteristics.
int mp_bluetooth_add_service(mp_obj_bluetooth_uuid_t *service_uuid, mp_obj_bluetooth_uuid_t **characteristic_uuids, uint8_t *characteristic_flags, uint16_t *value_handles, size_t characteristic_len);

// Read the value from the local gatts db (likely this has been written by a central).
int mp_bluetooth_characteristic_value_read(uint16_t value_handle, uint8_t *value, size_t *value_len);
// Write the value to the local gatts db (ready to be queried by a central).
int mp_bluetooth_characteristic_value_write(uint16_t value_handle, const uint8_t *value, size_t *value_len);
// Notify the central that it should do a read.
int mp_bluetooth_characteristic_value_notify(uint16_t conn_handle, uint16_t value_handle);
// Notify the central, including a data payload. (Note: does not set the gatts db value).
int mp_bluetooth_characteristic_value_notify_send(uint16_t conn_handle, uint16_t value_handle, const uint8_t *value, size_t *value_len);
// Indicate the central.
int mp_bluetooth_characteristic_value_indicate(uint16_t conn_handle, uint16_t value_handle);

// Disconnect from a central or peripheral.
int mp_bluetooth_disconnect(uint16_t conn_handle);

#if MICROPY_PY_BLUETOOTH_ENABLE_CENTRAL_MODE
// Start a discovery (scan). Set duration to zero to run continuously.
int mp_bluetooth_scan_start(int32_t duration_ms);

// Stop discovery (if currently active).
int mp_bluetooth_scan_stop(void);

// Connect to a found peripheral.
int mp_bluetooth_peripheral_connect(uint8_t addr_type, const uint8_t *addr, int32_t duration_ms);

// Find all primary services on the connected peripheral.
int mp_bluetooth_peripheral_discover_primary_services(uint16_t conn_handle);

// Find all characteristics on the specified service on a connected peripheral.
int mp_bluetooth_peripheral_discover_characteristics(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle);

// Find all descriptors on the specified characteristic on a connected peripheral.
int mp_bluetooth_peripheral_discover_descriptors(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle);

// Initiate read of a value from the remote peripheral.
int mp_bluetooth_peripheral_read_characteristic(uint16_t conn_handle, uint16_t value_handle);

// Write the value to the remote peripheral.
int mp_bluetooth_peripheral_write_characteristic(uint16_t conn_handle, uint16_t value_handle, const uint8_t *value, size_t *value_len);
#endif

/////////////////////////////////////////////////////////////////////////////
// API implemented by modbluetooth (called by port-specific implementations):

// Notify modbluetooth that a central has connected.
void mp_bluetooth_central_connected(uint16_t conn_handle, uint8_t addr_type, const uint8_t *addr);

// Notify modbluetooth that a central has disconnected.
void mp_bluetooth_central_disconnected(uint16_t conn_handle);

// Call this when a characteristic is written to.
void mp_bluetooth_characteristic_on_write(uint16_t conn_handle, uint16_t value_handle);

#if MICROPY_PY_BLUETOOTH_ENABLE_CENTRAL_MODE
// Notify modbluetooth that scan has finished, either timeout, manually, or by some other action (e.g. connecting).
void mp_bluetooth_scan_complete(void);

// Notify modbluetooth of a scan result.
void mp_bluetooth_scan_result(uint8_t addr_type, const uint8_t *addr, bool connectable, const int8_t rssi, const uint8_t *data, size_t data_len);

// Notify modbluetooth that a peripheral connected.
void mp_bluetooth_peripheral_connected(uint16_t conn_handle, uint8_t addr_type, const uint8_t *addr);

// Notify modbluetooth that a peripheral disconnected.
void mp_bluetooth_peripheral_disconnected(uint16_t conn_handle);

// Notify modbluetooth that a service was found (either by discover-all, or discover-by-uuid).
void mp_bluetooth_peripheral_primary_service_result(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle, mp_obj_bluetooth_uuid_t *service_uuid);

// Notify modbluetooth that a characteristic was found (either by discover-all-on-service, or discover-by-uuid-on-service).
void mp_bluetooth_peripheral_characteristic_result(uint16_t conn_handle, uint16_t def_handle, uint16_t value_handle, uint8_t properties, mp_obj_bluetooth_uuid_t *characteristic_uuid);

// Notify modbluetooth that a descriptor was found.
void mp_bluetooth_peripheral_descriptor_result(uint16_t conn_handle, uint16_t handle, mp_obj_bluetooth_uuid_t *descriptor_uuid);

// Notify modbluetooth that a read has completed with data (or notify/indicate data available, use `event` to disambiguate).
void mp_bluetooth_peripheral_characteristic_data_available(uint16_t event, uint16_t conn_handle, uint16_t value_handle, const uint8_t *data, size_t data_len);

// Notify modbluetooth that a write has completed.
void mp_bluetooth_peripheral_characteristic_write_status(uint16_t conn_handle, uint16_t value_handle, uint16_t status);
#endif

#endif // MICROPY_INCLUDED_EXTMOD_MODBLUETOOTH_H
