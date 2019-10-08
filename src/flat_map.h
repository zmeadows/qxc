#pragma once

/*
An open addressing hash table using robin hood hashing
ex usage:
in a .c file:
DECL_HASH_TABLE(MyMap, int, char)
--> creates type: struct MyMap
--> creates functions:
     MyMap__init
     MyMap__free
     MyMap__lookup
     MyMap__insert

 #define DECL_HASH_TABLE(                                                            \
   NEW_TYPE, KEY_TYPE, VALUE_TYPE) #struct NEW_TYPE{#KEY_TYPE * keys;                \
                                                    #VALUE_TYPE * values;            \
                                                    #size_t count; #size_t capacity; \
                                                    #size_t longest_probe;           \
                                                    #double max_load_factor; #};     \
   #struct NEW_TYPE * NEW_TYPE##__init(size_t init_capacity);                        \
   #void NEW_TYPE##__free(struct NEW_TYPE * table);                                  \
   #VALUE_TYPE * NEW_TYPE##__lookup(struct NEW_TYPE * table, KEY_TYPE lookup_id);    \
   #void NEW_TYPE##__insert(struct NEW_TYPE * table, KEY_TYPE new_id,                \
                            KEY_VALUE new_val);

 #define IMPL_HASH_TABLE(NEW_TYPE, KEY_TYPE, VALUE_TYPE, HASH_FUNC, EQU_FUNC, \
                         EMPTY_SENTINEL, TOMBSTONE_SENTINEL) \
                                                                                        \
     static inline void NEW_TYPE##__swap_keys(KEY_TYPE* keyA, KEY_TYPE* keyB) \
     { \
         KEY_TYPE tmp = *keyA; \
         *keyA = *keyB; \
         *keyB = tmp; \
     } \
                                                                                        \
     static inline void NEW_TYPE##__swap_values(VALUE_TYPE* valA, VALUE_TYPE* valB) \
     { \
         VALUE_TYPE tmp = *valA; \
         *valA = *valB; \
         *valB = tmp; \
     } \
                                                                                        \
     static struct NEW_TYPE* NEW_TYPE##__init(size_t init_capacity) \
     { \
         struct NEW_TYPE* new_table = malloc(sizeof(struct NEW_TYPE)); \
         new_table->keys = (KEY_TYPE*)malloc(sizeof(KEY_TYPE) * init_capacity); \
         new_table->values = (VALUE_TYPE*)malloc(sizeof(VALUE_TYPE) * init_capacity); \
                                                                                        \
         if (!new_table->keys || !new_table->values) { \
             free(new_table->keys); \
             free(new_table->values); \
             free(new_table); \
             fprintf(stderr, \
                     "Failed to allocate memory for a hash table with capacity: %zu\n",
                     \
                     init_capacity); \
             return NULL; \
         } \
                                                                                        \
         new_table->count = 0; \
         new_table->capacity = init_capacity; \
         new_table->longest_probe = 0; \
         new_table->max_load_factor = 0.5; \
                                                                                        \
         for (size_t i = 0; i < new_table->capacity; i++) { \
             new_table->keys[i] = EMPTY_SENTINEL; \
         } \
                                                                                        \
         return new_table; \
     } \
                                                                                        \
     static void NEW_TYPE##__free(struct NEW_TYPE* table) \
     { \
         free(table->keys); \
         free(table->values); \
         table->count = 0; \
         table->capacity = 0; \
         table->longest_probe = 0; \
         free(table); \
     } \
                                                                                        \
     static VALUE_TYPE* NEW_TYPE##__lookup(struct NEW_TYPE* table, KEY_TYPE lookup_id)
     \
     { \
         const uint64_t N = table->capacity - 1; \
         uint64_t probe_index = HASH_FUNC(lookup_id) & N; \
         uint64_t distance_from_initial_bucket = 0; \
         while (true) { \
             const KEY_TYPE probed_id = table->keys[probe_index]; \
             if (EQU_FUNC(probed_id, lookup_id)) { \
                 return table->values + probe_index; \
             } \
             else if (EQU_FUNC(probed_id, EMPTY_SENTINEL)) { \
                 return NULL; \
             } \
             probe_index = (probe_index + 1) & N; \
             distance_from_initial_bucket++; \
             if (distance_from_initial_bucket > table->longest_probe) { \
                 return NULL; \
             } \
         } \
     } \
                                                                                        \
     static void NEW_TYPE##__insert(struct NEW_TYPE* table, KEY_TYPE new_key, \
                                    VALUE_TYPE new_val) \
     { \
         const uint64_t N = m_capacity - 1; \
         uint64_t probe_index = HASH_FUNC(new_key) & N; \
         uint64_t dib = 0; \
                                                                                        \
         if ((double)table->count / (double)table->capacity > m_max_load_factor) { \
             NEW_TYPE##__reshash(table, table->capacity * 2); \
         } \
                                                                                        \
         while (true) { \
             const KEY_TYPE probed_id = table->keys[probe_index]; \
             if (EQU_FUNC(probed_id, TOMBSTONE_SENTINEL) || \
                 EQU_FUNC(probed_id, EMPTY_SENTINEL)) { \
                 table->keys[probe_index] = new_key; \
                 NEW_TYPE##__swap_values(&new_val, &table->values[probe_index]); \
                 table->count++; \
                 if (dib > table->longest_probe) { \
                     table->longest_probe = dib; \
                 } \
             } \
             else if (EQU_FUNC(probed_id, new_key)) { \
                 NEW_TYPE##__swap_values(&m_values[probe_index], &new_value); \
             } \
             else { \
                 const uint64_t probed_dib = \
                     probe_index - (detail::hash_id(probed_id) & N); \
                 if (probed_dib < dib) { \
                     NEW_TYPE##__swap_keys(&probed_id, &new_key); \
                     NEW_TYPE##__swap_values(&table->values[probe_index], &new_value);
                     \
                     m_longest_probe = dib > m_longest_probe ? dib : m_longest_probe; \
                     dib = probed_dib; \
                 } \
             } \
             probe_index = (probe_index + 1) & N; \
             dib++; \
         } \
     } \
                                                                                        \
     void NEW_TYPE##__rehash(struct NEW_TYPE* old_table, size_t new_capacity) \
     { \
         assert(new_capacity > table->capacity); \
         assert((n & (n - 1)) == 0 && \
                "EntityMap: Table capacity must be a power of two!"); \
         struct NEW_TYPE* new_table = NEW_TYPE##__init(new_capacity); \
                                                                                        \
         for (size_t i = 0; i < old_table->capacity; i++) { \
             KEY_TYPE k = old_table->keys[i]; \
             if (k != EMPTY_SENTINEL && k != TOMBSTONE_SENTINEL) { \
                 NEW_TYPE##__insert(k, table->values[i]); \
             } \
         } \
         std::swap(m_keys, new_table.m_keys); \
         std::swap(m_values, new_table.m_values); \
         std::swap(m_capacity, new_table.m_capacity); \
         std::swap(m_longest_probe, new_table.m_longest_probe); \
     }

*/
