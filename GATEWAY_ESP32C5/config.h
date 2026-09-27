#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// SINGFAST ESP32 - CONFIGURATION
// ============================================================


// ============================================================
// 1. WIFI ACCESS POINT
// ============================================================

#define WIFI_SSID       "SINGFAST_CAR"
#define WIFI_PASSWORD   "HuHuKhecKhec889!"

// ESP32 Access Point IP
#define WIFI_IP         192,168,4,1
#define WIFI_GATEWAY    192,168,4,1
#define WIFI_SUBNET     255,255,255,0


// ============================================================
// 2. WEB SERVER
// ============================================================

#define WEB_SERVER_PORT     80
#define WEBSOCKET_PORT      81


// ============================================================
// 3. STM32 UART
// ============================================================

// ESP32 UART2
#define STM32_UART_BAUDRATE    115200

// ESP32 GPIO
#define STM32_UART_RX_PIN      24
#define STM32_UART_TX_PIN      23


// ============================================================
// 4. COMMUNICATION TIMEOUT
// ============================================================

// Thời gian tối đa chờ STM32 phản hồi
#define STM32_TIMEOUT_MS       500

// Nếu không nhận command mới trong khoảng thời gian này,
// ESP32 có thể gửi STOP để đảm bảo an toàn.
#define COMMAND_TIMEOUT_MS     300


// ============================================================
// 5. HEARTBEAT
// ============================================================

// Khoảng thời gian ESP32 gửi heartbeat tới STM32
#define HEARTBEAT_INTERVAL_MS  500

// Số lần heartbeat có thể mất trước khi xem STM32 mất kết nối
#define HEARTBEAT_MAX_MISSED   3


// ============================================================
// 6. INTERNAL LATENCY
// ============================================================

// Khoảng thời gian đo internal latency
#define LATENCY_INTERVAL_MS    1000


// ============================================================
// 7. SERIAL DEBUG
// ============================================================

// Serial Monitor của ESP32
#define DEBUG_BAUDRATE         115200


// ============================================================
// 8. WEBSOCKET
// ============================================================

// Số lượng WebSocket client tối đa
#define MAX_WEBSOCKET_CLIENTS  1


// ============================================================
// 9. SYSTEM
// ============================================================

// Phiên bản protocol giữa Web / ESP32 / STM32
#define PROTOCOL_VERSION       1


#endif // CONFIG_H