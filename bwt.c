#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define BLOCKSIZE   1048576

typedef unsigned char byte;
static int  L;                  /* String length */
static byte S[BLOCKSIZE];       /* Input string of length L */
static byte T[BLOCKSIZE];       /* Output string of length L */
static int  I[BLOCKSIZE];
static int  Z;                  /* Sentinel position */

int suffix_compare(const void *a, const void *b)
{
    int i, j, r;

    i = *(const int*)a;
    j = *(const int*)b;
    r = memcmp(S + i, S + j, L - (i > j ? i : j));
    return r ? r : j - i;   /* NB: sentinel preceeds other characters */
}

int char_compare(const void *a, const void *b)
{
    int i, j, r;

    i = *(const int*)a;
    j = *(const int*)b;
    r = S[i] - S[j];
    return r ? r : i - j;
}

bool write_int(FILE *fp, int i)
{
    unsigned char buf[4] = { (i>>24)&255, (i>>16)&255, (i>>8)&255, i&255 };
    return fwrite(buf, 4, 1, fp) == 1;
}

bool read_int(FILE *fp, int *i)
{
    unsigned char buf[4];
    if(fread(buf, 4, 1, fp) != 1)
        return false;
    *i = (buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | buf[3];
    return true;
}

bool decode()
{
    int n, p;

    if(Z < 0 || Z > L)
        return false;

    if(L == 0)
        return true;

    n = 0, p = I[Z];
    while(n < L)
    {
        T[n++] = S[p];
        if(p == 0)
            return n == L;
        p = p > Z ? I[p] : I[p - 1];
    }
    return false;
}

int main(int argc, char *argv[])
{
    int n, m;
    bool decoding = (argc > 1 && strcmp(argv[1], "-d") == 0);

    while(true)
    {
        if(decoding)
        {
            if(!read_int(stdin, &Z))
                break;

            L = (int)fread(S, 1, BLOCKSIZE, stdin);

            for(n = 0; n < L; ++n)
                I[n] = n;
            qsort(I, L, sizeof(*I), char_compare);
            if(!decode())
            {
                fprintf(stderr, "Invalid input!\n");
                return 1;
            }

            fwrite(T, 1, L, stdout);
        }
        else
        {
            L = (int)fread(S, 1, BLOCKSIZE, stdin);
            if(L == 0)
                break;
            for(n = 0; n < L; ++n)
                I[n] = n;
            qsort(I, L, sizeof(*I), suffix_compare);

            if(L == 0)
            {
                Z = 0;
            }
            else
            {
                T[0] = S[L - 1];
                for(m = 1, n = 0; n < L; ++n)
                {
                    if(I[n] == 0)
                        Z = n;
                    else
                        T[m++] = S[I[n] - 1];
                }
            }

            if(!write_int(stdout, Z) || fwrite(T, 1, L, stdout) != L)
            {
                fprintf(stderr, "Write error!\n");
                return 1;
            }
        }
    }

    return 0;
}
