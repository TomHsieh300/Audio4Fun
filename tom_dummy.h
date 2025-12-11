#ifndef __TOM_DUMMY_H__
#define __TOM_DUMMY_H__

#include <linux/platform_device.h>
#include <sound/soc.h>

#define TOM_DUMMY_CARD_NAME          "Tom Dummy ASoC Card"

/* CPU */
#define TOM_DUMMY_CPU_DRV_NAME       "tom-dummy-cpu"
#define TOM_DUMMY_CPU_DAI_NAME       "tom-dummy-cpu-dai"

/* Codec */
#define TOM_DUMMY_CODEC_DRV_NAME     "tom-dummy-codec"
#define TOM_DUMMY_CODEC_DAI_NAME     "tom-dummy-codec-dai"

/* Platform (PCM) */
#define TOM_DUMMY_PLATFORM_DRV_NAME  "tom-dummy-platform"


/* 1kHz Sine Wave @ 48kHz Sample Rate (48 samples per cycle) */
static const s16 sine_1k_48k_table[48] = {
    0, 2616, 5195, 7702, 10099, 12350, 14421, 16280, 17898, 19248, 20309, 21062,
    21493, 21598, 21376, 20830, 19967, 18803, 17357, 15654, 13725, 11606, 9336, 6956,
    4510, 2043, -402, -2786, -5070, -7215, -9186, -10952, -12484, -13755, -14742, -15433,
    -15814, -15882, -15635, -15077, -14217, -13069, -11654, -10000, -8142, -6116, -3962, -1728
};

#endif /* __TOM_DUMMY_H__ */

