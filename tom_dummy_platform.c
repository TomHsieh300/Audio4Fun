// tom_dummy_platform.c - PCM platform for Tom Dummy ASoC card

#include <linux/module.h>
#include <linux/platform_device.h>

#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "tom_dummy.h"

static const struct snd_pcm_hardware tom_dummy_pcm_hardware = {
    .info = SNDRV_PCM_INFO_MMAP |
            SNDRV_PCM_INFO_INTERLEAVED |
            SNDRV_PCM_INFO_MMAP_VALID,
    .formats = SNDRV_PCM_FMTBIT_S16_LE,
    .rates = (SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000),
    .rate_min = 48000,
    .rate_max = 48000,
    .channels_min = 2,
    .channels_max = 2,
    .buffer_bytes_max = 512 * 1024,
    .period_bytes_min = 64,
    .period_bytes_max = 64 * 1024,
    .periods_min = 2,
    .periods_max = 1024,
};

/* ==== PCM callbacks (new soc-component API) ==== */
static int tom_dummy_platform_pcm_construct(struct snd_soc_component *component,
                                            struct snd_soc_pcm_runtime *rtd)
{
    int ret;

    pr_info("tom_platform: pcm_construct (pcm=%s)\n", rtd->pcm->name);

    /*
     * 在 PCM 建立時，設定這個 PCM 所有 substreams 的 buffer。
     * 使用 VMALLOC buffer，不綁定實體 DMA device。
     */
    ret = snd_pcm_set_managed_buffer_all(rtd->pcm,
                                         SNDRV_DMA_TYPE_VMALLOC,
                                         NULL,           /* dev */
                                         64 * 1024,      /* min size */
                                         512 * 1024);    /* max size */
    if (ret < 0)
        dev_err(component->dev,
                "tom_platform: set_managed_buffer_all failed: %d\n", ret);

    return ret;
}


static int tom_dummy_platform_open(struct snd_soc_component *component,
                                   struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;

    pr_info("tom_platform: open (stream=%d)\n", substream->stream);

    runtime->hw = tom_dummy_pcm_hardware;

    return 0;
}


static int tom_dummy_platform_close(struct snd_soc_component *component,
                                    struct snd_pcm_substream *substream)
{
    pr_info("tom_platform: close (stream=%d)\n", substream->stream);
    return 0;
}

static int tom_dummy_platform_hw_params(struct snd_soc_component *component,
                                        struct snd_pcm_substream *substream,
                                        struct snd_pcm_hw_params *params)
{
    pr_info("tom_platform: hw_params buffer=%u period=%u\n",
            params_buffer_bytes(params), params_period_bytes(params));
    return 0;
}

static int tom_dummy_platform_hw_free(struct snd_soc_component *component,
                                      struct snd_pcm_substream *substream)
{
    pr_info("tom_platform: hw_free\n");
    return 0;
}

static int tom_dummy_platform_prepare(struct snd_soc_component *component,
                                      struct snd_pcm_substream *substream)
{
    pr_info("tom_platform: prepare\n");
    return 0;
}

static int tom_dummy_platform_trigger(struct snd_soc_component *component,
                                      struct snd_pcm_substream *substream,
                                      int cmd)
{
    pr_info("tom_platform: trigger cmd=%d\n", cmd);

    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        return 0;
    default:
        return -EINVAL;
    }
}

static snd_pcm_uframes_t
tom_dummy_platform_pointer(struct snd_soc_component *component,
                           struct snd_pcm_substream *substream)
{
    /* 練習用，先簡單回 0 */
    return 0;
}

/* ===== component driver ===== */

static const struct snd_soc_component_driver tom_dummy_platform_component = {
    .name      = TOM_DUMMY_PLATFORM_DRV_NAME,

    .pcm_construct = tom_dummy_platform_pcm_construct,

    .open      = tom_dummy_platform_open,
    .close     = tom_dummy_platform_close,
    .hw_params = tom_dummy_platform_hw_params,
    .hw_free   = tom_dummy_platform_hw_free,
    .prepare   = tom_dummy_platform_prepare,
    .trigger   = tom_dummy_platform_trigger,
    .pointer   = tom_dummy_platform_pointer,
};

/* ===== platform driver ===== */

static int tom_dummy_platform_probe(struct platform_device *pdev)
{
    pr_info("tom_platform: probe\n");

    return devm_snd_soc_register_component(&pdev->dev,
                                           &tom_dummy_platform_component,
                                           NULL, 0);
}

static struct platform_driver tom_dummy_platform_driver = {
    .driver = {
        .name  = TOM_DUMMY_PLATFORM_DRV_NAME,
        .owner = THIS_MODULE,
    },
    .probe = tom_dummy_platform_probe,
};

static struct platform_device *tom_dummy_platform_pdev;

static int __init tom_dummy_platform_init(void)
{
    int ret;

    pr_info("tom_platform: init\n");

    ret = platform_driver_register(&tom_dummy_platform_driver);
    if (ret)
        return ret;

    tom_dummy_platform_pdev =
        platform_device_register_simple(TOM_DUMMY_PLATFORM_DRV_NAME,
                                        -1, NULL, 0);
    if (IS_ERR(tom_dummy_platform_pdev)) {
        ret = PTR_ERR(tom_dummy_platform_pdev);
        platform_driver_unregister(&tom_dummy_platform_driver);
        return ret;
    }

    return 0;
}

static void __exit tom_dummy_platform_exit(void)
{
    pr_info("tom_platform: exit\n");

    platform_device_unregister(tom_dummy_platform_pdev);
    platform_driver_unregister(&tom_dummy_platform_driver);
}

module_init(tom_dummy_platform_init);
module_exit(tom_dummy_platform_exit);

MODULE_DESCRIPTION("Tom Dummy PCM Platform (ASoC, managed buffer)");
MODULE_AUTHOR("Tom Hsieh");
MODULE_LICENSE("GPL");

