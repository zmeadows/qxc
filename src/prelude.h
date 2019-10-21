#pragma once

#include <stdbool.h>
#include <stdlib.h>

#define DEBUG

#ifdef DEBUG
#define debug_print(...)                                               \
    do {                                                               \
        fprintf(stderr, "%s:%d:%s(): ", __FILE__, __LINE__, __func__); \
        fprintf(stderr, __VA_ARGS__);                                  \
        fprintf(stderr, "\n");                                         \
    } while (0)
#else
#define debug_print(...) \
    do {                 \
    } while (0)
#endif

#define QXC_FATAL_ERROR(msg)                                       \
    fprintf(stderr, "%s:%d:%s(): ", __FILE__, __LINE__, __func__); \
    fprintf(stderr, "fatal compiler error:\n%s\n", msg);           \
    exit(EXIT_FAILURE);

#define QXC_UNREACHABLE() QXC_FATAL_ERROR("Supposedly unreachable path encountered")

#define qxc_snprintf(buf, buf_size, ...)                     \
    if (snprintf(buf, buf_size, __VA_ARGS__) > buf_size) {   \
        QXC_FATAL_ERROR("Buffer overflow in string buffer"); \
    }

bool strs_are_equal(const char* strA, const char* strB);

void strip_ext(char* fname);

const char* mk_tmp_dir(void);
void rm_tmp_dir(const char* tmp_path);

size_t max(size_t a, size_t b);

void print_file(const char* filepath);

void qxc_compiler_error_halt(const char* msg);

#include <cstdio>
#include <memory>

template <typename F>
class defer_finalizer {
    F f;
    bool moved;

public:
    template <typename T>
    defer_finalizer(T&& f_) : f(std::forward<T>(f_)), moved(false)
    {
    }

    defer_finalizer(const defer_finalizer&) = delete;

    defer_finalizer(defer_finalizer&& other) : f(std::move(other.f)), moved(other.moved)
    {
        other.moved = true;
    }

    ~defer_finalizer()
    {
        if (!moved) f();
    }
};

struct deferrer {
    template <typename F>
    defer_finalizer<F> operator<<(F&& f)
    {
        return defer_finalizer<F>(std::forward<F>(f));
    }
};

#define TOKENPASTE(x, y) x##y
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)
#define defer auto TOKENPASTE2(__deferred_lambda_call, __COUNTER__) = deferrer() << [&]

