#pragma once

/*

template <typename T>
inline constexpr bool is_power_of_two(T n)
{
    return (n & (n - 1)) == 0;
}

template <typename K, typename V, typename HashFunc, typename CmpFunc, K EmptySentinel,
          K TombstoneSentinel>
class DenseHashTable {
    uint8_t* m_keys;
    uint8_t* m_values;
    size_t m_count;
    size_t m_capacity;
    size_t m_longest_probe;
    double m_max_load_factor;

public:
    EntityMap(size_t initial_capacity)
        : keys((K*)malloc(sizeof(K) * initial_capacity)),
          values((V*)malloc(sizeof(V) * initial_capacity)),
          count(0),
          capacity(initial_capacity),
          longest_probe(0),
          max_load_factor(0.5)
    {
        ARK_ASSERT(initial_capacity > 0, "initial capacity must be greater than 0.");
        ARK_ASSERT(detail::is_power_of_two(initial_capacity),
                   "capacity must always be a power of two!");
        ARK_ASSERT(keys, "Failed to initialize memory for EntityMap keys");
        ARK_ASSERT(values, "Failed to initialize memory for EntityMap values");

        for (size_t i = 0; i < capacity; i++) {
            keys[i] = EMPTY_SENTINEL;
        }
    }

    EntityMap() : EntityMap(64) {}

    EntityMap& operator=(const EntityMap&) = delete;
    EntityMap& operator=(EntityMap&&) = delete;
    EntityMap(EntityMap&&) = delete;
    EntityMap(const EntityMap&) = delete;

    ~EntityMap(void)
    {
        for (size_t i = 0; i < capacity; i++) {
            const K id = keys[i];
            if (id != EMPTY_SENTINEL && id != TOMBSTONE_SENTINEL) {
                values[i].~V();
            }
        }

        free(keys);
        free(values);
        keys = nullptr;
        values = nullptr;
        count = 0;
        capacity = 0;
        longest_probe = 0;
    }

    inline double load_factor(Self* table)
    {
        return static_cast<double>(table->count) / static_cast<double>(table->capacity);
    }

    inline size_t size(void) const { return count; }

    V* lookup(K lookup_id) const
    {
        const uint64_t N = capacity - 1;
        uint32_t probe_index = detail::hash_id(lookup_id) & N;
        uint64_t distance_froinitial_bucket = 0;

        while (true) {
            const K probed_id = keys[probe_index];

            if (probed_id == lookup_id) {
                return values + probe_index;
            }
            else if (probed_id == EMPTY_SENTINEL) {
                return nullptr;
            }

            probe_index = (probe_index + 1) & N;
            distance_froinitial_bucket++;

            if (distance_froinitial_bucket > longest_probe) {
                return nullptr;
            }
        }
    }

    template <typename... Args>
    V* insert(K new_id, Args&&... args)
    {
        if (load_factor() > max_load_factor) {
            rehash(capacity * 2);
        }

        const uint64_t N = capacity - 1;
        uint32_t probe_index = detail::hash_id(new_id) & N;

        uint64_t dib = 0;  // 'd'istance from 'i'nitial 'b'ucket

        V new_value(std::forward<Args>(args)...);

        while (true) {
            K& probed_id = keys[probe_index];

            if (probed_id == TOMBSTONE_SENTINEL || probed_id == EMPTY_SENTINEL) {
                probed_id = new_id;
                std::swap(new_value, values[probe_index]);
                count++;
                longest_probe = dib > longest_probe ? dib : longest_probe;
                return values + probe_index;
            }
            else if (probed_id == new_id) {
                std::swap(values[probe_index], new_value);
                return values + probe_index;
            }
            else {
                // TODO: is this handling wrap-around properly?
                const uint64_t probed_dib =
                    probe_index - (detail::hash_id(probed_id) & N);
                if (probed_dib < dib) {
                    std::swap(probed_id, new_id);
                    std::swap(values[probe_index], new_value);
                    longest_probe = dib > longest_probe ? dib : longest_probe;
                    dib = probed_dib;
                }
            }

            probe_index = (probe_index + 1) & N;
            dib++;
        }
    }

    bool remove(K id)
    {
        const uint64_t N = capacity - 1;
        uint64_t probe_index = detail::hash_id(id) & N;

        uint64_t dib = 0;

        while (true) {
            K& probed_id = keys[probe_index];

            if (probed_id == id) {
                probed_id = TOMBSTONE_SENTINEL;
                values[probe_index].~V();
                count--;
                return true;
            }
            else if (probed_id == EMPTY_SENTINEL) {
                return false;
            }

            probe_index = (probe_index + 1) & N;
            dib++;

            if (dib > longest_probe) {
                return false;
            }
        }
    }

    void rehash(size_t new_capacity)
    {
        assert(new_capacity > capacity);

        assert(detail::is_power_of_two(new_capacity) &&
               "EntityMap: Table capacity must be a power of two!");

        EntityMap new_table(new_capacity);

        for (size_t i = 0; i < capacity; i++) {
            K id = keys[i];
            if (id != EMPTY_SENTINEL && id != TOMBSTONE_SENTINEL) {
                new_table.insert(id, std::move(values[i]));
            }
        }

        std::swap(keys, new_table.keys);
        std::swap(values, new_table.values);
        std::swap(capacity, new_table.capacity);
        std::swap(longest_probe, new_table.longest_probe);
    }

    V& operator[](K id)
    {
        V* result = lookup(id);
        ARK_ASSERT(result != nullptr,
                   "EntityMap: Called operator[] with non-existent K!");
        return *result;
    }

    const V& operator[](K id) const
    {
        const V* result = lookup(id);
        ARK_ASSERT(result != nullptr,
                   "EntityMap: Called operator[] with non-existent K: " << id);
        return *result;
    }
};
*/
