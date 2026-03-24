#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H
#include "zf_common_headfile.h"

// Connect to proxy server.
bool websocket_client_connect(void);
// Send payload as a WebSocket client text frame.
bool websocket_client_send(const uint8_t* data, uint32_t len);
// Receive and decode server WebSocket frames.
bool websocket_client_receive(uint8_t* buffer);
// Close proxy socket connection.
void websocket_client_close();
// Build a WebSocket frame.
uint32_t websocket_create_frame(uint8_t* frame, const uint8_t* payload, uint64_t payload_len, uint8_t type, bool mask);

#endif // WEBSOCKET_CLIENT_H
