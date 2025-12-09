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

#endif /* __TOM_DUMMY_H__ */

