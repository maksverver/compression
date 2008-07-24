#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/*  Data format:

    128 bytes containing the lengths of the codes:
        byte[n] == (len(2*n) - 1) | ((len(2*n + 1) - 1) << 4)
    (Each length is between 1 and 16, inclusive.)

*/

typedef unsigned char byte;

void clear_model(unsigned *freqs)
{
    int n;

    for(n = 0; n < 257; ++n)
        freqs[n] = 1;   /* FIXME: make this 0? */
}

void update_model(unsigned *freqs, byte *buf, size_t len)
{
    while(len--)
        ++freqs[*buf++&255];
}


struct HuffmanNode
{
    struct HuffmanNode *next, *left, *right;
    unsigned freq;
    int symbol;
};

/* Inserts a node into a list ordered by frequency.
   Sets node->next and updates *list if necessary. */
void insert_node(struct HuffmanNode **list, struct HuffmanNode *node)
{
    if(*list == NULL || node->freq < (*list)->freq)
    {
        node->next = *list;
        *list = node;
    }
    else
    {
        struct HuffmanNode *it = *list;
        while(it->next && node->freq >= it->next->freq)
            it = it->next;
        node->next = it->next;
        it->next = node;
    }
}

void assign_codelengths(byte lengths[257], struct HuffmanNode *node, int depth)
{
    if(node->left)
    {
        assign_codelengths(lengths, node->left,  depth + 1);
        assign_codelengths(lengths, node->right, depth + 1);
    }
    else
    {
        lengths[node->symbol] = depth;
    }
}

void compute_codelengths(unsigned freqs[257], byte lengths[257])
{
    int n;
    struct HuffmanNode nodes[513], *list;

    for(n = 0; n < 257; ++n)
    {
        freqs[n] /= 4;    /* HACK */
        if(freqs[n] == 0)
            freqs[n] = 1;
    }

    /* FIXME: should ensure codelengths are all <= 16 */

    list = NULL;
    for(n = 0; n < 257; ++n)
    {
        nodes[n].left = NULL;
        nodes[n].freq = freqs[n];
        nodes[n].symbol = n;
        insert_node(&list, &nodes[n]);
    }

    for( ; n < 513; ++n)
    {
        nodes[n].left  = list;
        nodes[n].right = list->next;
        nodes[n].freq  = list->freq + list->next->freq;
        list = list->next->next;
        insert_node(&list, &nodes[n]);
    }

    assign_codelengths(lengths, list, 0);
}

/* FIXME: make sure this doesn't crash if input is invalid! */
void assign_codes(byte lengths[257], unsigned codes[257])
{
    int len, n;
    unsigned num;

    len = 0;
    for(n = 0; n < 257; ++n)
        if(lengths[n] > len)
            len = lengths[n];

    if(len > 16)
    {
        fprintf(stderr, "Code length >16!\n");
        abort();
    }

    for(num = 0; len != 0; --len)
    {
        for(n = 0; n < 257; ++n)
            if(lengths[n] == len)
                codes[n] = num++;
        num >>= 1;
    }
}

void huffman_encode( byte lengths[257], unsigned codes[257],
    byte *in_buf, size_t in_len, byte *out_buf, size_t out_len,
    size_t *enc_bytes )
{
    int pos, offset, len;
    unsigned code;

    *enc_bytes = 0;
    *out_buf = 0;
    pos = offset = 0;
    do {
        code = codes[in_len ? *in_buf : 256];
        len  = lengths[in_len ? *in_buf : 256];
        ++in_buf;

        /* Bytes are filled from high to low bits
          'offset' is the number of bits filled in *out_buf
        */

        while(len >= 8 - offset)
        {
            /* Only room for '8 - offset' more bits... */
            *out_buf |= (byte)(code >> (len - 8 + offset));
            len -= 8 - offset;
            code &= (1<<len)-1;

            /* Move to next byte */
            ++out_buf;
            *out_buf = 0, offset = 0;
            ++*enc_bytes;
        }
        if(len)
        {
            *out_buf |= (byte)(code << (8 - offset - len));
            offset += len;
        }
    } while(in_len--);

    if(offset)
        ++*enc_bytes;
}

/* Note: out_buf and/or dec_bytes may be NULL */
bool huffman_decode( byte lengths[257], unsigned codes[257],
    byte *in_buf, size_t in_len, byte *out_buf, size_t out_len,
    size_t *dec_bytes )
{
    unsigned short symbol[65536];
    int n, m, pos, word;

    if(in_len == 0)
        return false;

    for(n = 0; n < 257; ++n)
    {
        pos = 16 - lengths[n];
        for(m = 0; m < (1 << pos); ++m)
            symbol[(codes[n] << pos) + m] = n;
    }

    if(dec_bytes)
        *dec_bytes = 0;
    n = 0;
    while(true)
    {
        word = (in_buf[0] << ( 8 + n));
        if(in_len > 1)
            word |= (in_buf[1] << n);
        if(in_len > 2)
            word |= (in_buf[2] >> (8 - n));
        word &= 65535;

        if(symbol[word] == 256)
            return true;

        if(out_buf)
        {
            if(out_len == 0)
                return false;   /* no more space in output buffer */
            *out_buf = (byte)symbol[word];
            ++out_buf, --out_len;
        }
        if(dec_bytes)
            ++*dec_bytes;

        /* Move bit pointer */
        n += lengths[symbol[word]];
        while(n > 8)
        {
            ++in_buf;
            if(--in_len == 0)
                return false;
            n -= 8;
        }
    }
    return true;
}

byte *read_all_data(FILE *fp, byte **buf, size_t *len)
{
    size_t capacity = 8192;
    byte *nbuf;

    *buf = malloc(capacity);
    if(*buf == NULL)
        return NULL;

    *len = 0;
    while(true)
    {
        *len += fread(*buf + *len, 1, capacity - *len, fp);
        if(*len < capacity)
            break;

        capacity *= 2;
        nbuf = realloc(*buf, capacity);
        if(nbuf == NULL)
        {
            free(buf);
            return NULL;
        }
        *buf = nbuf;
    };

    return *buf;
}

void print_huffman_table(byte *lengths, unsigned *codes, FILE *fp)
{
    int n, m;
    for(n = 0; n < 257; ++n)
    {
        fprintf(fp, "%d (len %d):\t", n, lengths[n]);
        for(m = lengths[n] - 1; m >= 0; --m)
            fputc('0' + ((codes[n] >> m)&1), fp);
        fputc('\n', fp);
    }
}

/* TODO: check for valid data to avoid crashes/buffer overflows! */
int main(int argc, char *argv[])
{
    bool decoding = (argc > 1 && strcmp(argv[1], "-d") == 0);
    byte *in_buf;
    size_t in_len;
    int n;
    byte lengths[257];
    unsigned codes[257];
    byte *out_buf;
    size_t out_len;

    if(read_all_data(stdin, &in_buf, &in_len) == NULL)
    {
        fprintf(stderr, "Could not read input into memory!\n");
        return 1;
    }

    if(decoding)
    {
        if(in_len < 130)
        {
            fprintf(stderr, "Input data is too short!\n");
            return 0;
        }
        for(n = 0; n < 128; ++n)
        {
            lengths[2*n + 0] = (byte)(1 + (in_buf[n]&15));
            lengths[2*n + 1] = (byte)(1 + ((in_buf[n]>>4)&15));
        }
        lengths[256] = 1 + (in_buf[128]&15);
        in_buf += 129;
        in_len -= 129;

        assign_codes(lengths, codes);
        /* print_huffman_table(lengths, codes, stderr); */

        if(!huffman_decode(lengths, codes, in_buf, in_len, NULL, 0, &out_len))
        {
            fprintf(stderr, "Could not decode input data!\n");
            return 1;
        }

        out_buf = malloc(out_len);
        if(out_buf == NULL)
        {
            fprintf(stderr, "Could not allocate output buffer!\n");
            return 1;
        }
        huffman_decode(lengths, codes, in_buf, in_len, out_buf, out_len, NULL);
        fwrite(out_buf, 1, out_len, stdout);
    }
    else
    {
        unsigned freqs[257];
        size_t enc_bytes;

        out_len = in_len;
        out_buf = malloc(out_len);
        if(out_buf == NULL)
        {
            fprintf(stderr, "Could not allocate output buffer!\n");
            return 1;
        }

        clear_model(freqs);
        update_model(freqs, in_buf, in_len);
        compute_codelengths(freqs, lengths);
        assign_codes(lengths, codes);
        /* print_huffman_table(lengths, codes, stderr); */

        huffman_encode(lengths, codes, in_buf, in_len, out_buf, out_len, &enc_bytes);

        for(n = 0; n < 128; ++n)
            fputc((lengths[2*n] - 1) | ((lengths[2*n + 1] - 1) << 4), stdout);
        fputc(lengths[256] - 1, stdout);
        fwrite(out_buf, 1, enc_bytes, stdout);
    }

    return 0;
}
