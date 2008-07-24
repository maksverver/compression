#include <stdio.h>
#include <stdbool.h>
#include <string.h>

typedef unsigned char byte;

typedef struct MTF
{
    byte symbol[256];
} MTF;

void mtf_init(MTF *mtf)
{
    int n;
    for(n = 0; n < 256; ++n)
        mtf->symbol[n] = (byte)n;
}

bool mtf_encode(MTF *mtf, byte *buf, size_t len)
{
    byte c;
    int n;

    while(len--)
    {
        c = *buf;
        for(n = 0; n < 256; ++n)
        {
            if(mtf->symbol[n] == c)
                break;
        }
        if(n == 256)
            return false;
        *buf = (byte)n;
        while(n)
        {
            mtf->symbol[n] = mtf->symbol[n - 1];
            n = n - 1;
        }
        mtf->symbol[0] = c;
        ++buf;
    }

    return true;
}

void mtf_decode(MTF *mtf, byte *buf, size_t len)
{
    int n;

    while(len--)
    {
        n = *buf;
        *buf = mtf->symbol[n];
        while(n)
        {
            mtf->symbol[n] = mtf->symbol[n - 1];
            n = n - 1;
        }
        mtf->symbol[0] = *buf;
        ++buf;
    }
}

int main(int argc, char *argv[])
{
    MTF mtf;
    byte buf[1024];
    size_t len;
    bool decoding = (argc > 1 && strcmp(argv[1], "-d") == 0);

    mtf_init(&mtf);
    while((len = fread(buf, 1, sizeof(buf), stdin)))
    {
        if(decoding)
            mtf_decode(&mtf, buf, len);
        else
            mtf_encode(&mtf, buf, len);
        fwrite(buf, 1, len, stdout);
    }

    return 0;
}
