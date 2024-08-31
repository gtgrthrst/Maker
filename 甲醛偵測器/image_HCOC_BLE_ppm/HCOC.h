#ifndef HCOC_H
#define HCOC_H

#include <stdint.h>

typedef struct {
    const uint16_t *data;
    uint16_t width;
    uint16_t height;
} tImage;

extern const tImage HCOC;

#endif // HCOC_H