#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "lib_crc.h"

#define SAMPLE_RATE     (16)
#define PREAMBLE_BITS   (10)
#define PREAMBLE_SIZE   (2 * SAMPLE_RATE * PREAMBLE_BITS)
#define MAX_MSG_SIZE    (29)
#define BUFFER_SIZE     (1 << 13) /* must be power of 2, and larger than the expected message */

/*
 * nRF905 preamble is Manchester-encoded 0x3F6 (0xAAA66):
 * '10101010101001100110'
 */
static const uint8_t hi_map[PREAMBLE_BITS] = { 1, 3, 5, 7, 9, 11, 12, 15, 16, 19 };
static const uint8_t lo_map[PREAMBLE_BITS] = { 0, 2, 4, 6, 8, 10, 13, 14, 17, 18 };

void process_stream(FILE *stream) {
    int16_t readbuf[BUFFER_SIZE];
    uint16_t c;
    size_t len;

    static double xv[4], yv[4];

    static int16_t buffer[BUFFER_SIZE];
    static uint16_t buffer_end = 0;
    static uint16_t buffer_skip = PREAMBLE_SIZE + 2 * SAMPLE_RATE * MAX_MSG_SIZE * CHAR_BIT;
    static int32_t sum = 0;
    #define BUF(i) (buffer[(buffer_end + i) & (BUFFER_SIZE - 1)])

    static uint8_t msg[MAX_MSG_SIZE];

    int16_t threshold;
    uint16_t i, j, k;
    uint8_t msg_len;
    uint8_t m1, m2;
    uint16_t crc16;

    double timestamp, rms;
    struct timeval tv;

    while (!feof(stream)) {
        len = fread(readbuf, sizeof(int16_t), BUFFER_SIZE, stream);

        for (c = 0; c < len; c++) {
            /* Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
               Command line: /www/usr/fisher/helpers/mkfilter -Bu -Lp -o 3 -a 6.2500000000e-02 0.0000000000e+00 -l */

            xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3];
            xv[3] = readbuf[c] / 1.886646578e+02;
            yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3];
            yv[3] = (xv[0] + xv[3]) + 3 * (xv[1] + xv[2])
                + (  0.4535459334 * yv[0])
                + ( -1.7151178300 * yv[1])
                + (  2.2191686183 * yv[2]);

            buffer[buffer_end++] = yv[2];
            buffer_end &= BUFFER_SIZE - 1;

            sum -= BUF(BUFFER_SIZE);
            sum += BUF(PREAMBLE_SIZE);

            if (buffer_skip) {
                buffer_skip--;
            } else {
                threshold = sum / PREAMBLE_SIZE;

                j = 0;
                for (i = 0; i < PREAMBLE_BITS; i++) {
                    if (BUF(hi_map[i] * SAMPLE_RATE) > threshold)
                        j++;
                    if (BUF(lo_map[i] * SAMPLE_RATE) < threshold)
                        j++;
                }

                if (j == 2 * PREAMBLE_BITS) {
                    memset(msg, 0, MAX_MSG_SIZE);
                    crc16 = 0xffff;
                    for (i = PREAMBLE_SIZE, j = 0; i < BUFFER_SIZE; i += SAMPLE_RATE * 2, j++) {
                        msg_len = j >> 3;
                        m1 = BUF(i) < threshold;
                        m2 = BUF(i + SAMPLE_RATE) < threshold;
                        if (m1 == m2)
                            break;
                        else if (m1)
                            msg[msg_len] |= 1 << ((CHAR_BIT - 1) - (j & (CHAR_BIT - 1)));

                        if ((j & (CHAR_BIT - 1)) == (CHAR_BIT - 1)) {
                            crc16 = update_crc_ccitt(crc16, msg[msg_len]);
                            if (crc16 == 0) {
                                gettimeofday(&tv, NULL);
                                timestamp = tv.tv_sec + tv.tv_usec / 1e6;
                                timestamp -= (BUFFER_SIZE / SAMPLE_RATE) * 2e-5;
                                printf("%.06f\t", timestamp);

                                for (k = 0, rms = 0; k < i; k++)
                                    rms += BUF(k) * BUF(k);
                                rms /= i;
                                printf("%.01f\t", 20.0 * log10(sqrt(rms) / SHRT_MAX));

                                for (k = 0; k <= msg_len; k++)
                                    printf("%02x", msg[k]);
                                printf("\n");

                                buffer_skip = PREAMBLE_SIZE + 2 * SAMPLE_RATE * (msg_len + 1) * CHAR_BIT;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

int main(int argc, char **argv) {
    process_stream(stdin);
    return 0;
}
