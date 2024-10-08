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

namespace asr
{
    // Memory tracking variables.
    uint32_t memblocks = 0;
    uint32_t memsize = 0;
    uint32_t peak_memsize = 0;

    // Enables trace() messages to be sent to STDOUT instead of the log file.
    bool stdout_trace_enabled = false;

    /**
     * @brief Returns the current time in milliseconds.
     * @return time_t
     */
    time_t millis() {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
        return elapsed.count();
    }


    /**
     * @brief Allocates a block of memory of the given size. Never returns NULL because when no memory is
     * left an internal error will occur.
     * 
     * @param size Amount of bytes to allocate.
     * @return void*
     */
    void *alloc (uint32_t size)
    {
        #if ASR_TRACK_MEMORY == 1
            uint8_t* buff = (uint8_t*)malloc(5+size);
            if (buff == nullptr) {
                trace ("ERROR: Not enough memory to allocate block of size %u.\n", size);
                exit(1);
            }

            *buff = 'A';
            *((uint32_t *)(buff+1)) = size;

            memblocks++;
            memsize += 5+size;
            if (memsize > peak_memsize)
                peak_memsize = memsize;

            return (void*)(buff+5);

        #else
            return malloc(size);
        #endif
    }


    /**
     * @brief Deallocates a previously allocated block of memory. Detects if the block is correct to avoid
     * memory errors.
     * 
     * @param block 
     */
    void dealloc (void *block)
    {
        if (block == nullptr) {
            trace("ERROR: Deallocating NULL block.\n");
            exit(1);
            return;
        }

        #if ASR_TRACK_MEMORY == 1
            uint8_t *buff = ((uint8_t *)block) - 5;

            if (*buff != 'A') {
                trace("ERROR: Deallocating invalid or already deallocated block: %08lx\n", buff);
                exit(1);
                return;
            }

            *buff = 'D';
            memblocks--;
            memsize -= *((uint32_t *)(buff+1)) + 5;
            free (buff);

        #else
            free (block);
        #endif
    }


    /**
     * @brief Copies the specified string into a newly allocated buffer.
     * 
     * @param value String to copy.
     * @param length Bytes to copy. If 0, the length is calculated using strlen.
     * @return char*
     */
    char *strclone (const char *value, int length)
    {
        if (length <= 0)
            length = strlen(value)+1;

        return (char *)memcpy((char *)asr::alloc(length), value, length);
    }


    /**
     * @brief Returns the next available temporal buffer from the circular queue of temporal buffers. These
     * are dynamically created (up to 32 buffers of TEMPBUFF_SIZE bytes each), when the `clear` parameter is
     * true, all buffers will be released and `nullptr` will be returned.
     * 
     * @param clear Optional parameter to clear all buffers if `true`.
     * @return char*
     */
    char *tempbuff (bool clear)
    {
        static char *buffer[32] = { nullptr };
        static int i = 0;

        if (clear) {
            for (int j = 0; j < 32; j++) {
                if (buffer[j] != nullptr) {
                    asr::dealloc(buffer[j]);
                    buffer[j] = nullptr;
                }
            }

            return nullptr;
        }

        i = ++i & 31;

        if (buffer[i] == nullptr)
            buffer[i] = (char *)asr::alloc(TEMPBUFF_SIZE);

        return buffer[i];
    }


    /**
     * @brief Converts the given value to string and returns it in a temporal buffer.
     * @param value 
     * @return char*
     */
    const char *c_str (int value) {
        char *buffer = tempbuff(); sprintf(buffer, "%d", value); return buffer;
    }

    const char *c_str (uint32_t value) {
        char *buffer = tempbuff(); sprintf(buffer, "%u", value); return buffer;
    }

    const char *c_str (float value) {
        char *buffer = tempbuff(); sprintf(buffer, "%.2f", value); return buffer;
    }

    const char *c_str (double value) {
        char *buffer = tempbuff(); sprintf(buffer, "%g", value); return buffer;
    }

    /**
     * Variables required for the trace functions.
     */
    static FILE *trace_fp = NULL;
    static bool is_unicode = false;
    static bool trace_exit_attached = false;

    /**
    */
    void trace_off() {
        if (trace_fp != nullptr) fclose(trace_fp);
        trace_fp = nullptr;
    }

    /**
     * @brief Enables/disables sending the trace messages to STDOUT.
     */
    void stdtrace (bool value) {
        stdout_trace_enabled = value;
    }


    /**
     * @brief Writes a message to the log file.
     */
    void trace (const char *msg, ...)
    {
        FILE *fp = trace_fp;

        if (fp != nullptr && is_unicode) {
            fclose(fp);
            fp = nullptr;
        }

        if (stdout_trace_enabled) fp = stdout;

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


    /**
     * @brief Writes a message to the log file without the new line character at the end (aka "raw").
     */
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


    /**
     * @brief Writes a message to the log file (UNICODE).
     */
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
     * @brief Enables ANSI Escape codes and UTF8 in the console.
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
     * @brief Allocates a new console or attaches to the parent's console.
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

/**
 * Global override of the "new" operator to match the ASR's memory protection functions.
 */
__attribute__((used)) void *operator new (size_t size) {
    return asr::alloc(size);
}

__attribute__((used)) void *operator new[] (size_t size) {
    return asr::alloc(size);
}


/**
 * Global override of the "delete" operator to match the ASR's memory protection functions.
 */
__attribute__((used)) void operator delete (void *block) noexcept {
    asr::dealloc(block);
}

__attribute__((used)) void operator delete[] (void *block) noexcept {
    asr::dealloc(block);
}

#endif

/**
 * Prevent glob expansion on MinGW.
 */
int _CRT_glob = 0;
