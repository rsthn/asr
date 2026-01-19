#include <asr/defs>

#ifdef _WIN32
#include <windows.h>
#endif

#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <chrono>
#include <new>

namespace asr
{
    // Memory tracking variables.
    uint32_t memblocks = 0;
    uint32_t memsize = 0;
    uint32_t peak_memsize = 0;

    // Enables trace() messages to be sent to STDOUT instead of the log file.
    bool stdout_trace_enabled = false;

    time_t millis() {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()
		);
        return elapsed.count();
    }

    // Unified header layout (16 bytes before user pointer):
    // [0-7] raw_ptr (void*) - pointer returned by malloc
    // [8-11] total_size (uint32_t) - total bytes malloc'd
    // [12] marker: 'A' = regular, 'B' = aligned
    // [13-15] padding
    static constexpr size_t HEADER_SIZE = 16;

    void* alloc (uint32_t size)
    {
        #if ASR_TRACK_MEMORY == 1
            uint32_t total = HEADER_SIZE + size;
            uint8_t* raw = (uint8_t*)std::malloc(total);
            if (!raw) {
                trace("ERROR: Not enough memory to allocate block of size %u.\n", size);
                exit(1);
            }

            uint8_t* header = raw;
            uint8_t* user_ptr = raw + HEADER_SIZE;

            *((void**)header) = raw;
            *((uint32_t*)(header + 8)) = total;
            *(header + 12) = 'A';

            memblocks++;
            memsize += total;
            if (memsize > peak_memsize)
                peak_memsize = memsize;

            return user_ptr;
        #else
            return std::malloc(size);
        #endif
    }

    void dealloc (void* block)
    {
        if (!block) {
            trace("ERROR: Deallocating NULL block.\n");
            exit(1);
            return;
        }

        #if ASR_TRACK_MEMORY == 1
            uint8_t* header = ((uint8_t*)block) - HEADER_SIZE;
            uint8_t marker = *(header + 12);

            if (marker != 'A' && marker != 'B') {
                trace("ERROR: Deallocating invalid or already deallocated block: %p\n", block);
                exit(1);
                return;
            }

            void* raw = *((void**)header);
            uint32_t total = *((uint32_t*)(header + 8));

            *(header + 12) = 'D';
            memblocks--;
            memsize -= total;
            std::free(raw);
        #else
            std::free(block);
        #endif
    }

    bool memblock_alive (void* block)
    {
        if (block == nullptr)
            return false;

        #if ASR_TRACK_MEMORY == 1
            uint8_t* header = ((uint8_t*)block) - HEADER_SIZE;
            uint8_t marker = *(header + 12);
            return marker == 'A' || marker == 'B';
        #else
            return true;
        #endif
    }

    void* alloc_aligned (size_t size, size_t alignment)
    {
        if (alignment <= alignof(std::max_align_t))
            return alloc(size);

        // Allocate extra space for alignment and header
        size_t total = size + alignment + HEADER_SIZE;
        uint8_t* raw = (uint8_t*)std::malloc(total);
        if (!raw) {
            asr::trace("ERROR: Not enough memory to allocate aligned block of size %zu.\n", size);
            throw std::bad_alloc();
        }

        // Align the user pointer
        uintptr_t raw_addr = (uintptr_t)(raw + HEADER_SIZE);
        uintptr_t aligned_addr = (raw_addr + alignment - 1) & ~(alignment - 1);
        uint8_t* user_ptr = (uint8_t*)aligned_addr;

        // Store header
        uint8_t* header = user_ptr - HEADER_SIZE;
        *((void**)header) = raw;
        *((uint32_t*)(header + 8)) = (uint32_t)total;
        *(header + 12) = 'B';

        memblocks++;
        memsize += total;
        if (memsize > peak_memsize)
            peak_memsize = memsize;

        return user_ptr;
    }

    void dealloc_aligned (void* block, size_t) noexcept {
        dealloc(block);
    }

    /**
     * Variables required for the trace functions.
     */
    static FILE *trace_fp = NULL;
    static bool is_unicode = false;
    static bool trace_exit_attached = false;

    void trace_off() {
        if (trace_fp != nullptr) fclose(trace_fp);
        trace_fp = nullptr;
    }

    void stdtrace (bool value) {
        stdout_trace_enabled = value;
    }

    void trace (const char *msg, ...)
    {
        FILE *fp = trace_fp;

        if (fp != nullptr && is_unicode) {
            fclose(fp);
            fp = nullptr;
        }

        if (stdout_trace_enabled)
            fp = stdout;

        if (fp == nullptr)
        {
            trace_fp = fp = fopen("system.log", "a+t");
            if (!fp) return;

            if (!trace_exit_attached) {
                atexit(&trace_off);
                trace_exit_attached = true;
            }

            is_unicode = false;
        }

        va_list args;
        va_start (args, msg);
        vfprintf (fp, msg, args);
        fprintf (fp, "\n");
        fflush (fp);
    }

    void rtrace (const char *msg, ...)
    {
        FILE *fp = trace_fp;

        if (fp != nullptr && is_unicode) {
            fclose(fp);
            fp = nullptr;
        }

        if (fp == nullptr)
        {
            trace_fp = fp = fopen ("system.log", "a+t");
            if (!fp) return;

            if (!trace_exit_attached) {
                atexit (&trace_off);
                trace_exit_attached = true;
            }

            is_unicode = false;
        }

        va_list args;
        va_start (args, msg);
        vfprintf (fp, msg, args);
        fflush (fp);
    }

    void wtrace (const wchar_t *msg, ...)
    {
        FILE *fp = trace_fp;

        if (fp != nullptr && !is_unicode) {
            fclose(fp);
            fp = nullptr;
        }

        if (fp == nullptr)
        {
            bool write_bom = false;

            fp = fopen ("wsystem.log", "rb");
            if (fp == nullptr) write_bom = true; else fclose(fp);

            trace_fp = fp = fopen ("wsystem.log", "a+b");
            if (!fp) return;

            if (write_bom) {
                putc(0xFF, fp);
                putc(0xFE, fp);
            }

            if (!trace_exit_attached) {
                atexit (&trace_off);
                trace_exit_attached = true;
            }

            is_unicode = true;
        }

        va_list args;
        va_start (args, msg);
        vfwprintf (fp, msg, args);
        fwprintf (fp, L"\r\n");
        fflush (fp);
    }

#ifdef _WIN32

    #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
    #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 4
    #endif

    /**
     * Enables ANSI Escape codes and UTF8 in the console.
     */
    int initConsole() {
        HANDLE hOut = GetStdHandle (STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode (hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode (hOut, dwMode);
        SetConsoleOutputCP(CP_UTF8);
        return 0;
    }

    /**
     * Allocates a new console or attaches to the parent's console.
     */
    void obtainConsole (int attachToParent)
    {
        static FILE *fp;

        if (attachToParent) {
            if (!AttachConsole(-1)) {
                initConsole();
                return;
            }
        }
        else {
            if (!AllocConsole()) {
                initConsole();
                return;
            }
        }

        freopen_s (&fp, "CONOUT$", "w", stdout);
        initConsole();
    }

    /**
     * Automatically enable ANSI escape codes in the console.
     */
    static int _initConsole = initConsole();

#endif
};

#if ASR_TRACK_MEMORY == 1

// Standard allocation/deallocation (C++11)
[[nodiscard]] void* operator new (std::size_t size) {
    void* p = asr::alloc(size);
    if (!p) throw std::bad_alloc();
    return p;
}

[[nodiscard]] void* operator new[] (std::size_t size) {
    void* p = asr::alloc(size);
    if (!p) throw std::bad_alloc();
    return p;
}

void operator delete (void* ptr) noexcept {
    asr::dealloc(ptr);
}

void operator delete[] (void* ptr) noexcept {
    asr::dealloc(ptr);
}

// Sized deallocation (C++14)
void operator delete (void* ptr, std::size_t) noexcept {
    asr::dealloc(ptr);
}

void operator delete[] (void* ptr, std::size_t) noexcept
{
    asr::dealloc(ptr);
}

// Aligned allocation and deallocation (C++17)
[[nodiscard]] void* operator new (std::size_t size, std::align_val_t align) {
    return asr::alloc_aligned(size, static_cast<std::size_t>(align));
}

[[nodiscard]] void* operator new[] (std::size_t size, std::align_val_t align) {
    return asr::alloc_aligned(size, static_cast<std::size_t>(align));
}

void operator delete (void* ptr, std::align_val_t align) noexcept {
    asr::dealloc_aligned(ptr, static_cast<std::size_t>(align));
}

void operator delete[] (void* ptr, std::align_val_t align) noexcept {
    asr::dealloc_aligned(ptr, static_cast<std::size_t>(align));
}

// Sized + aligned deallocation (C++17)
void operator delete (void* ptr, std::size_t, std::align_val_t align) noexcept {
    asr::dealloc_aligned(ptr, static_cast<std::size_t>(align));
}

void operator delete[] (void* ptr, std::size_t, std::align_val_t align) noexcept {
    asr::dealloc_aligned(ptr, static_cast<std::size_t>(align));
}

// Nothrow versions
[[nodiscard]] void* operator new (std::size_t size, const std::nothrow_t&) noexcept {
    try {
        return asr::alloc(size);
    } catch (...) {
        return nullptr;
    }
}

[[nodiscard]] void* operator new[] (std::size_t size, const std::nothrow_t&) noexcept {
    try {
        return asr::alloc(size);
    } catch (...) {
        return nullptr;
    }
}

void operator delete (void* ptr, const std::nothrow_t&) noexcept {
    asr::dealloc(ptr);
}

void operator delete[] (void* ptr, const std::nothrow_t&) noexcept {
    asr::dealloc(ptr);
}

// Aligned nothrow versions (C++17)
[[nodiscard]] void* operator new (std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept {
    try {
        return asr::alloc_aligned(size, static_cast<std::size_t>(align));
    } catch (...) {
        return nullptr;
    }
}

[[nodiscard]] void* operator new[] (std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept {
    try {
        return asr::alloc_aligned(size, static_cast<std::size_t>(align));
    } catch (...) {
        return nullptr;
    }
}

void operator delete (void* ptr, std::align_val_t align, const std::nothrow_t&) noexcept {
    asr::dealloc_aligned(ptr, static_cast<std::size_t>(align));
}

void operator delete[] (void* ptr, std::align_val_t align, const std::nothrow_t&) noexcept {
    asr::dealloc_aligned(ptr, static_cast<std::size_t>(align));
}

#endif

/**
 * Prevent glob expansion on MinGW.
 */
int _CRT_glob = 0;
