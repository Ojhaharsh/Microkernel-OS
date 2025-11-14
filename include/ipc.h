#ifndef IPC_H
#define IPC_H

#include <stddef.h>
#include <stdint.h>

// Simple fixed-size message payload
#define IPC_MSG_MAX 128

// Initialize IPC subsystem (mailboxes)
void ipc_init(void);

// Send a message to destination task id. Blocks if receiver mailbox is full.
// Returns number of bytes sent (<= IPC_MSG_MAX) or negative on error.
int ipc_send(int dst_task_id, const char* buf, size_t len);

// Receive a message for the current task. Blocks if no message available.
// On success, writes sender id to *from, length to *out_len, and copies data to buf (up to maxlen).
// Returns 0 on success, negative on error.
int ipc_recv(int* from, char* buf, size_t maxlen, size_t* out_len);

#endif // IPC_H
