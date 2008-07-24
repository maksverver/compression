#include <stdio.h>
#include <math.h>

int main()
{
    char buffer[1024];
    size_t read, total = 0, freq[256] = { }, n;
    double H;

    while((read = fread(buffer, 1, sizeof(buffer), stdin)))
    {
        total += read;
        for(n = 0; n < read; ++n)
            ++freq[buffer[n]&255];
    }

    H = 0;
    for(n = 0; n < 256; ++n)
        if(freq[n])
            H -= freq[n]*log((double)freq[n]/total);
    H /= log(256);
    printf("%f %f\n", H, H/total);
    return 0;
}
