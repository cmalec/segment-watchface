/* In-memory persist + AppMessage stubs for host tests. */
#include "pebble.h"

#define MAX_KV 64
static struct { uint32_t key; uint8_t data[PERSIST_DATA_MAX_LENGTH]; size_t len; int used; } s_kv[MAX_KV];

int persist_exists(uint32_t key) {
  for (int i = 0; i < MAX_KV; i++) if (s_kv[i].used && s_kv[i].key == key) return 1;
  return 0;
}
int persist_read_data(uint32_t key, void *buffer, size_t buffer_size) {
  for (int i = 0; i < MAX_KV; i++) {
    if (s_kv[i].used && s_kv[i].key == key) {
      size_t n = s_kv[i].len < buffer_size ? s_kv[i].len : buffer_size;
      memcpy(buffer, s_kv[i].data, n);
      return (int)n;
    }
  }
  return -1;
}
int persist_write_data(uint32_t key, const void *data, size_t size) {
  if (size > PERSIST_DATA_MAX_LENGTH) return -3;
  for (int i = 0; i < MAX_KV; i++) {
    if (s_kv[i].used && s_kv[i].key == key) {
      memcpy(s_kv[i].data, data, size); s_kv[i].len = size; return (int)size;
    }
  }
  for (int i = 0; i < MAX_KV; i++) {
    if (!s_kv[i].used) {
      s_kv[i].used = 1; s_kv[i].key = key; memcpy(s_kv[i].data, data, size); s_kv[i].len = size;
      return (int)size;
    }
  }
  return -4;
}
int persist_delete(uint32_t key) {
  for (int i = 0; i < MAX_KV; i++) if (s_kv[i].used && s_kv[i].key == key) { s_kv[i].used = 0; return 0; }
  return -1;
}
int heap_bytes_free(void) { return 64 * 1024; }
void app_message_register_inbox_received(void *cb) { (void)cb; }
void app_message_register_inbox_dropped(void *cb) { (void)cb; }
int app_message_open(uint32_t inbound, uint32_t outbound) { (void)inbound; (void)outbound; return 0; }

/* ---- AppMessage iterator mock: tests queue tuples and settings_inbox reads
 * them back. Values are stored inline so the exported Tuple pointers stay
 * valid while the queue is walked. ---- */
#define MOCK_ITER_MAX 8
static struct MockTuple {
  uint32_t key;
  union { uint8_t uint8; int8_t int8; uint16_t uint16; int32_t int32; const char *cstring; } value;
} mock_tuples[MOCK_ITER_MAX];
static Tuple mock_export[MOCK_ITER_MAX];
static int mock_iter_len = 0;
static int mock_iter_pos = 0;

void mock_iter_reset(void) { mock_iter_len = 0; mock_iter_pos = 0; }

static void mock_iter_push(uint32_t key) {
  if (mock_iter_len >= MOCK_ITER_MAX) return;
  mock_export[mock_iter_len].key = key;
  mock_export[mock_iter_len].value = (void *)&mock_tuples[mock_iter_len].value;
  mock_iter_len++;
}

void mock_iter_uint(uint32_t key, uint8_t value) {
  mock_iter_push(key);
  mock_tuples[mock_iter_len - 1].value.uint8 = value;
}

void mock_iter_cstring(uint32_t key, const char *value) {
  mock_iter_push(key);
  mock_tuples[mock_iter_len - 1].value.cstring = value;
}

Tuple *dict_read_first(DictionaryIterator *iter) {
  (void)iter;
  mock_iter_pos = 0;
  return mock_iter_len > 0 ? &mock_export[0] : NULL;
}
Tuple *dict_read_next(DictionaryIterator *iter) {
  (void)iter;
  mock_iter_pos++;
  return mock_iter_pos < mock_iter_len ? &mock_export[mock_iter_pos] : NULL;
}
