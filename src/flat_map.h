#pragma once

namespace {

template <typename T>
inline constexpr bool is_power_of_two(T n)
{
    return (n & (n - 1)) == 0;
}

template <typename T>
uint8_t* raw_alloc(size_t count)
{
    return reinterpret_cast<uint8_t*>(aligned_alloc(alignof(T), sizeof(T) * count));
}

template <typename T>
struct DefaultSentinels {
    static_assert(
        sizeof(T) == -1,
        "Default sentinels don't exist for this key type, use custom implementation!");
    static T empty;
    static T tombstone;
};

template <typename T>
struct DefaultSentinels<T*> {
    static T* empty = (T*)0x0;
    static T* tombstone = (T*)0x1;
};

template <typename T>
struct DefaultHasher {
    static_assert(
        sizeof(T) == -1,
        "Default Hasher doesn't exist for this key type, use custom implementation!");
    uint64_t hash(T key);
};

// pointer hashing
template <typename T>
struct DefaultHasher<T*> {
    uint64_t hash(T* key_ptr)
    {
        auto key = reinterpret_cast<uintptr_t>(key_ptr);
        key = (~key) + (key << 21);  // key = (key << 21) - key - 1;
        key = key ^ (key >> 24);
        key = (key + (key << 3)) + (key << 8);  // key * 265
        key = key ^ (key >> 14);
        key = (key + (key << 2)) + (key << 4);  // key * 21
        key = key ^ (key >> 28);
        key = key + (key << 31);
        return static_cast<uint64_t>(key);
    }
};

}  // namespace

template <typename K, typename V, class Hasher = DefaultHasher<K>,
          K EMPTY_SENTINEL = DefaultSentinels<K>::empty,
          K TOMBSTONE_SENTINEL = DefaultSentinels<K>::tombstone>
class DenseHashTable {
    uint8_t* m_keys;
    uint8_t* m_values;
    size_t m_count;
    size_t m_capacity;
    size_t m_longest_probe;
    double m_max_load_factor;

    inline K* cast_keys(void) { return reinterpret_cast<K*>(m_keys); }
    inline V* cast_values(void) { return reinterpret_cast<V*>(m_values); }

    void rehash(size_t new_capacity)
    {
        assert(new_capacity > m_capacity);
        assert(is_power_of_two(new_capacity));

        DenseHashTable new_table(new_capacity);

        // TODO: just keep these as a class member?
        K* keys = this->cast_keys();
        V* values = this->cast_values();

        for (size_t i = 0; i < m_capacity; i++) {
            const K& k = keys[i];
            if (k != EMPTY_SENTINEL && k != TOMBSTONE_SENTINEL) {
                new_table.insert(k, std::move(values[i]));
            }
        }

        assert(m_count == new_table.m_count);
        std::swap(m_keys, new_table.m_keys);
        std::swap(m_values, new_table.m_values);
        std::swap(m_capacity, new_table.m_capacity);
        std::swap(m_longest_probe, new_table.m_longest_probe);
    }

public:
    // TODO: write nearest_power_of_two function so user can't screw up initial capacity
    DenseHashTable(size_t initial_capacity)
        : m_keys(raw_alloc<K>(initial_capacity)),
          m_values(raw_alloc<V>(initial_capacity)),
          m_count(0),
          m_capacity(initial_capacity),
          m_longest_probe(0),
          m_max_load_factor(0.5)
    {
        assert(initial_capacity > 0);
        assert(is_power_of_two(initial_capacity));
        assert(m_keys);
        assert(m_values);

        K* keys_cast = this->cast_keys();

        for (size_t i = 0; i < m_capacity; i++) {
            keys_cast[i] = EMPTY_SENTINEL;
        }
    }

    DenseHashTable() : DenseHashTable(8) {}

    DenseHashTable& operator=(const DenseHashTable&) = delete;
    DenseHashTable& operator=(DenseHashTable&&) = delete;
    DenseHashTable(DenseHashTable&&) = delete;
    DenseHashTable(const DenseHashTable&) = delete;

    ~DenseHashTable(void)
    {
        K* keys_cast = this->cast_keys();
        V* values_cast = this->cast_values();

        for (size_t i = 0; i < m_capacity; i++) {
            K& key = keys_cast[i];
            if (key != EMPTY_SENTINEL && key != TOMBSTONE_SENTINEL) {
                values_cast[i].~V();
            }
            key.~K();
        }

        free(m_keys);
        free(m_values);
        m_keys = nullptr;
        m_values = nullptr;
        m_count = 0;
        m_capacity = 0;
        m_longest_probe = 0;
    }

    V& operator[](K id)
    {
        V* result = lookup(id);
        assert(result != nullptr);
        return *result;
    }

    const V& operator[](K id) const
    {
        const V* result = lookup(id);
        assert(result != nullptr);
        return *result;
    }

    inline double load_factor(void)
    {
        return static_cast<double>(m_count) / static_cast<double>(m_capacity);
    }

    inline size_t size(void) const { return m_count; }

    V* lookup(K lookup_id) const
    {
        const uint64_t N = m_capacity - 1;
        uint64_t probe_index = Hasher::hash(lookup_id) & N;
        uint64_t distance_from_initial_bucket = 0;

        K* keys = this->cast_keys();
        V* values = this->cast_values();

        while (true) {
            const K& probed_id = keys[probe_index];

            if (probed_id == lookup_id) {
                return values + probe_index;
            }
            else if (probed_id == EMPTY_SENTINEL) {
                return nullptr;
            }

            probe_index = (probe_index + 1) & N;
            distance_from_initial_bucket++;

            if (distance_from_initial_bucket > m_longest_probe) {
                return nullptr;
            }
        }
    }

    template <typename... Args>
    V* insert(K new_key, Args&&... args)
    {
        if (load_factor() > m_max_load_factor) {
            rehash(m_capacity * 2);
        }

        const uint64_t N = m_capacity - 1;
        uint64_t probe_index = Hasher::hash(new_key) & N;
        uint64_t dib = 0;  // 'd'istance from 'i'nitial 'b'ucket

        V new_value(std::forward<Args>(args)...);

        K* keys = this->cast_keys();
        V* values = this->cast_values();

        while (true) {
            K& probed_id = keys[probe_index];

            if (probed_id == TOMBSTONE_SENTINEL || probed_id == EMPTY_SENTINEL) {
                probed_id = new_key;
                std::swap(new_value, values[probe_index]);
                m_count++;
                m_longest_probe = dib > m_longest_probe ? dib : m_longest_probe;
                return values + probe_index;
            }
            else if (probed_id == new_key) {
                std::swap(values[probe_index], new_value);
                return values + probe_index;
            }
            else {
                // TODO: is this handling wrap-around properly?
                const uint64_t probed_dib = probe_index - (Hasher::hash(probed_id) & N);
                if (probed_dib < dib) {
                    std::swap(probed_id, new_key);
                    std::swap(values[probe_index], new_value);
                    m_longest_probe = dib > m_longest_probe ? dib : m_longest_probe;
                    dib = probed_dib;
                }
            }

            probe_index = (probe_index + 1) & N;
            dib++;
        }
    }

    bool remove(K id)
    {
        const uint64_t N = m_capacity - 1;
        uint64_t probe_index = Hasher::hash(id) & N;
        uint64_t dib = 0;

        K* keys = this->cast_keys();
        V* values = this->cast_values();

        while (true) {
            K& probed_id = keys[probe_index];

            if (probed_id == id) {
                probed_id = TOMBSTONE_SENTINEL;
                values[probe_index].~V();
                m_count--;
                return true;
            }
            else if (probed_id == EMPTY_SENTINEL) {
                return false;
            }

            probe_index = (probe_index + 1) & N;
            dib++;

            if (dib > m_longest_probe) {
                return false;
            }
        }
    }
};
