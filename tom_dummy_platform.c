#include <linux/math64.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/ktime.h>

#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "tom_dummy.h"

struct tom_dummy_runtime {
    struct snd_pcm_substream *substream;
    struct hrtimer timer;
    spinlock_t lock;

    snd_pcm_uframes_t buffer_size;
    snd_pcm_uframes_t period_size;
    snd_pcm_uframes_t hw_ptr;

    unsigned int rate;
    ktime_t period_ktime;

    bool running;
};

static const struct snd_pcm_hardware tom_dummy_pcm_hardware = {
    .info = SNDRV_PCM_INFO_MMAP |
            SNDRV_PCM_INFO_INTERLEAVED |
            SNDRV_PCM_INFO_MMAP_VALID,
    .formats = SNDRV_PCM_FMTBIT_S16_LE,
    .rates = SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000,
    .rate_min = 44100,
    .rate_max = 48000,
    .channels_min = 2,
    .channels_max = 2,
    .buffer_bytes_max = 512 * 1024,
    .period_bytes_min = 1024,
    .period_bytes_max = 64 * 1024,
    .periods_min = 2,
    .periods_max = 1024,
};

static enum hrtimer_restart tom_dummy_hrtimer_cb(struct hrtimer *timer)
{
    struct tom_dummy_runtime *prtd =
        container_of(timer, struct tom_dummy_runtime, timer);
    struct snd_pcm_substream *substream;
    struct snd_pcm_runtime *runtime;
    unsigned long flags;
    snd_pcm_uframes_t old_hw_ptr;
    snd_pcm_uframes_t period, buf_frames;
    bool is_capture;

    spin_lock_irqsave(&prtd->lock, flags);

    if (!prtd->running) {
        spin_unlock_irqrestore(&prtd->lock, flags);
        return HRTIMER_NORESTART;
    }

    substream = prtd->substream;
    if (!substream || !substream->runtime) {
        prtd->running = false;
        spin_unlock_irqrestore(&prtd->lock, flags);
        return HRTIMER_NORESTART;
    }

    runtime = substream->runtime;
    is_capture = (substream->stream == SNDRV_PCM_STREAM_CAPTURE);

    old_hw_ptr = prtd->hw_ptr;
    period     = prtd->period_size;
    buf_frames = prtd->buffer_size;

    prtd->hw_ptr = old_hw_ptr + period;

    while (prtd->hw_ptr >= buf_frames)
        prtd->hw_ptr -= buf_frames;

    spin_unlock_irqrestore(&prtd->lock, flags);

    if (is_capture && runtime->dma_area) {
        snd_pcm_uframes_t first = period;
        snd_pcm_uframes_t second = 0;

        if (old_hw_ptr + period > buf_frames) {
            first  = buf_frames - old_hw_ptr;
            second = period - first;
        }

        memset(runtime->dma_area + frames_to_bytes(runtime, old_hw_ptr),
               0, frames_to_bytes(runtime, first));

        if (second)
            memset(runtime->dma_area, 0, frames_to_bytes(runtime, second));
    }

    snd_pcm_period_elapsed(substream);

    if (hrtimer_forward_now(timer, prtd->period_ktime) == 0) {
        pr_warn_ratelimited("tom_platform: hrtimer_forward_now returned 0!\n");
    }
    return HRTIMER_RESTART;
}

static int tom_dummy_platform_open(struct snd_soc_component *component,
                                   struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct tom_dummy_runtime *prtd;

    pr_info("tom_platform: open (stream=%d)\n", substream->stream);

    prtd = kzalloc(sizeof(*prtd), GFP_KERNEL);
    if (!prtd)
        return -ENOMEM;

    runtime->private_data = prtd;
    prtd->substream = substream;
    prtd->running = false;
    prtd->hw_ptr = 0;

    spin_lock_init(&prtd->lock);

    hrtimer_init(&prtd->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_SOFT);
    prtd->timer.function = tom_dummy_hrtimer_cb;

    runtime->hw = tom_dummy_pcm_hardware;

    return 0;
}

static int tom_dummy_platform_close(struct snd_soc_component *component,
                                    struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct tom_dummy_runtime *prtd = runtime->private_data;
    unsigned long flags;

    pr_info("tom_platform: close (stream=%d)\n", substream->stream);

    if (prtd) {
        hrtimer_cancel(&prtd->timer);

        spin_lock_irqsave(&prtd->lock, flags);
        prtd->running = false;
        prtd->substream = NULL;
        spin_unlock_irqrestore(&prtd->lock, flags);

        runtime->private_data = NULL;
        kfree(prtd);
    }

    return 0;
}

static int tom_dummy_platform_hw_params(struct snd_soc_component *component,
                                        struct snd_pcm_substream *substream,
                                        struct snd_pcm_hw_params *params)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct tom_dummy_runtime *prtd = runtime->private_data;
    unsigned int rate = params_rate(params);
    snd_pcm_uframes_t buffer_size = params_buffer_size(params);
    snd_pcm_uframes_t period_size = params_period_size(params);
    u64 nsecs;

    pr_info("tom_platform: hw_params buffer=%u period=%u rate=%u\n",
            params_buffer_bytes(params), params_period_bytes(params), rate);

    prtd->buffer_size = buffer_size;
    prtd->period_size = period_size;
    prtd->rate = rate;
    prtd->hw_ptr = 0;

    nsecs = (u64)period_size * NSEC_PER_SEC;
    do_div(nsecs, rate);
    prtd->period_ktime = ns_to_ktime(nsecs);

    pr_info("tom_platform: period_ktime=%lld ns\n", (long long)nsecs);

    return 0;
}

static int tom_dummy_platform_hw_free(struct snd_soc_component *component,
                                      struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct tom_dummy_runtime *prtd = runtime->private_data;
    unsigned long flags;

    pr_info("tom_platform: hw_free\n");

    if (prtd) {
        spin_lock_irqsave(&prtd->lock, flags);
        prtd->running = false;
        spin_unlock_irqrestore(&prtd->lock, flags);

        hrtimer_cancel(&prtd->timer);
    }

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
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct tom_dummy_runtime *prtd = runtime->private_data;
    unsigned long flags;

    // pr_info("tom_platform: trigger cmd=%d\n", cmd);

    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        spin_lock_irqsave(&prtd->lock, flags);
        if (cmd == SNDRV_PCM_TRIGGER_START)
            prtd->hw_ptr = 0;
        prtd->running = true;
        spin_unlock_irqrestore(&prtd->lock, flags);

        hrtimer_start(&prtd->timer, prtd->period_ktime, HRTIMER_MODE_REL_SOFT);
        break;

    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        spin_lock_irqsave(&prtd->lock, flags);
        prtd->running = false;
        spin_unlock_irqrestore(&prtd->lock, flags);
        break;

    default:
        return -EINVAL;
    }

    return 0;
}

static snd_pcm_uframes_t
tom_dummy_platform_pointer(struct snd_soc_component *component,
                           struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct tom_dummy_runtime *prtd = runtime->private_data;
    snd_pcm_uframes_t ptr;
    unsigned long flags;

    if (!prtd)
        return 0;

    spin_lock_irqsave(&prtd->lock, flags);
    ptr = prtd->hw_ptr;
    spin_unlock_irqrestore(&prtd->lock, flags);

    return ptr;
}

static int tom_dummy_platform_pcm_construct(struct snd_soc_component *component,
                                            struct snd_soc_pcm_runtime *rtd)
{
    int ret;

    pr_info("tom_platform: pcm_construct (pcm=%s)\n", rtd->pcm->name);

    ret = snd_pcm_set_managed_buffer_all(rtd->pcm,
                                         SNDRV_DMA_TYPE_VMALLOC,
                                         NULL,
                                         64 * 1024,
                                         512 * 1024);
    if (ret < 0)
        dev_err(component->dev,
                "tom_platform: set_managed_buffer_all failed: %d\n", ret);

    return ret;
}

static const struct snd_soc_component_driver tom_dummy_platform_component = {
    .name          = TOM_DUMMY_PLATFORM_DRV_NAME,
    .pcm_construct = tom_dummy_platform_pcm_construct,

    .open      = tom_dummy_platform_open,
    .close     = tom_dummy_platform_close,
    .hw_params = tom_dummy_platform_hw_params,
    .hw_free   = tom_dummy_platform_hw_free,
    .prepare   = tom_dummy_platform_prepare,
    .trigger   = tom_dummy_platform_trigger,
    .pointer   = tom_dummy_platform_pointer,
};

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

MODULE_DESCRIPTION("Tom Dummy PCM Platform with software PCM engine");
MODULE_AUTHOR("Tom Hsieh");
MODULE_LICENSE("GPL");

