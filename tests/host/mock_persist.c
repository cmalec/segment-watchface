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
Tuple *dict_read_first(DictionaryIterator *iter) { (void)iter; return NULL; }
Tuple *dict_read_next(DictionaryIterator *iter) { (void)iter; return NULL; }
