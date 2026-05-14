#ifndef BTSTACK_CONFIG_H
#define BTSTACK_CONFIG_H

#define ENABLE_BLE
#define ENABLE_LE_PERIPHERAL
#define ENABLE_LOG_INFO
#define ENABLE_LOG_ERROR

#define HCI_ACL_PAYLOAD_SIZE            255  /* must be >= 255 for CC256X 255-byte patch commands */
#define HCI_INCOMING_PRE_BUFFER_SIZE    4
#define MAX_NR_HCI_CONNECTIONS          1
#define MAX_NR_L2CAP_SERVICES           2
#define MAX_NR_L2CAP_CHANNELS           4
#define MAX_NR_SM_LOOKUP_ENTRIES        3
#define MAX_NR_WHITELIST_ENTRIES        4
#define MAX_NR_LE_DEVICE_DB_ENTRIES     4
/* NVM_NUM_DEVICE_DB_ENTRIES intentionally NOT defined — use le_device_db_memory.c (static RAM) */

#define HAVE_EMBEDDED_TIME_MS

#endif /* BTSTACK_CONFIG_H */
