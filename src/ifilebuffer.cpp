
#include <asr/ifilebuffer>

namespace asr {

    bool IFileBuffer::fill_request(int n_min, int n_max, bool inquiry)
    {
        char buffer[512];

        if (fp == nullptr || fsize - ftell(fp) < n_min)
            return false;

        if (inquiry)
            return true;

        if (!n_max) n_max = space_available();

        while (n_max > 0 && !feof(fp))
        {
            int i = fread(buffer, 1, n_max > 512 ? 512 : n_max, fp);
            if (!i) break;

            fill(buffer, i);

            n_max -= i;
            n_min -= i;
        }

        return n_min <= 0;
    }

    IFileBuffer::IFileBuffer(const char *filepath, int mode, int buffer_size) : Buffer(buffer_size)
    {
        fp = fopen(filepath, mode == O_BINARY ? "rb" : "rt");
        if (fp == nullptr) return;

        fseek(fp, 0, SEEK_END);
        fsize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        fill_request(0);
        fp_owned = true;
    }

    IFileBuffer::IFileBuffer(FILE *fp, int buffer_size, bool fp_owned) : Buffer(buffer_size)
    {
        this->fp = fp;
        this->fp_owned = fp_owned;

        long pos = ftell(fp);
        fseek(fp, 0, SEEK_END);
        fsize = ftell(fp) - pos;
        fseek(fp, pos, SEEK_SET);

        fill_request(0);
    }

    IFileBuffer::~IFileBuffer() {
        close();
    }

    void IFileBuffer::close()
    {
        if (fp != nullptr)
        {
            if (fp_owned) fclose (fp);
            fp = nullptr;
        }
    }

};
