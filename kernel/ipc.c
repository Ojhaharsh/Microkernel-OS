// Week 6: Simple per-task single-slot mailboxes with blocking send/recv
#include <stddef.h>
#include <stdint.h>
#include "ipc.h"
#include "scheduler.h"

#define MAX_TASKS 8

typedef struct mailbox_t {
	volatile int full;       // 0 = empty, 1 = full
	int from;                // sender task id
	size_t len;              // bytes valid in data
	char data[IPC_MSG_MAX];
} mailbox_t;

static mailbox_t mboxes[MAX_TASKS];

void ipc_init(void) {
	for (int i = 0; i < MAX_TASKS; ++i) {
		mboxes[i].full = 0;
		mboxes[i].from = -1;
		mboxes[i].len = 0;
	}
}

int ipc_send(int dst_task_id, const char* buf, size_t len) {
	if (dst_task_id < 0 || dst_task_id >= MAX_TASKS) return -1;
	if (!buf) return -2;
	if (len > IPC_MSG_MAX) len = IPC_MSG_MAX;

	int self = sched_current_id();
	if (self < 0) return -3;

	// Block until receiver mailbox is empty
	while (mboxes[dst_task_id].full) {
		sched_block_current();
	}

	// Enter critical section: avoid preemption during mailbox fill
	__asm__ __volatile__("cli");

	// Copy message into receiver mailbox
	for (size_t i = 0; i < len; ++i) mboxes[dst_task_id].data[i] = buf[i];
	mboxes[dst_task_id].len = len;
	mboxes[dst_task_id].from = self;
	mboxes[dst_task_id].full = 1;
	__asm__ __volatile__("sti");

	// Wake receiver if it was blocked waiting to receive, then prefer to run it next
	sched_unblock(dst_task_id);
	sched_prefer_next(dst_task_id);
	yield();
	return (int)len;
}

int ipc_recv(int* from, char* buf, size_t maxlen, size_t* out_len) {
	int self = sched_current_id();
	if (self < 0) return -1;
	if (!buf) return -2;
	if (maxlen == 0) return -3;

	// Block until we have a message
	while (!mboxes[self].full) {
		sched_block_current();
	}

	__asm__ __volatile__("cli");

	size_t n = mboxes[self].len;
	if (n > maxlen) n = maxlen;
	for (size_t i = 0; i < n; ++i) buf[i] = mboxes[self].data[i];
	if (out_len) *out_len = n;
	if (from) *from = mboxes[self].from;

	// Mark empty and potentially wake any sender waiting on us
	mboxes[self].full = 0;
	__asm__ __volatile__("sti");

	// Unblock all tasks; simple approach lets senders retry
	for (int i = 0; i < MAX_TASKS; ++i) sched_unblock(i);
	return 0;
}
