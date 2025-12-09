#include <linux/module.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "tom_dummy.h"

static int tom_dummy_cpu_startup(struct snd_pcm_substream *substream,
                                 struct snd_soc_dai *dai)
{
    pr_info("tom_cpu_dai: startup (stream=%d)\n", substream->stream);
    return 0;
}

static int tom_dummy_cpu_hw_params(struct snd_pcm_substream *substream,
                                   struct snd_pcm_hw_params *params,
                                   struct snd_soc_dai *dai)
{
    int rate   = params_rate(params);
    int width  = snd_pcm_format_width(params_format(params));
    int ch     = params_channels(params);

    pr_info("tom_cpu_dai: hw_params rate=%d width=%d ch=%d\n",
            rate, width, ch);

    return 0;
}

static const struct snd_soc_dai_ops tom_dummy_cpu_dai_ops = {
    .startup   = tom_dummy_cpu_startup,
    .hw_params = tom_dummy_cpu_hw_params,
};

static struct snd_soc_dai_driver tom_dummy_cpu_dai = {
    .name = TOM_DUMMY_CPU_DAI_NAME,
    .playback = {
        .stream_name  = "Tom CPU Playback",
        .channels_min = 2,
        .channels_max = 2,
        .rates        = (SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000),
        .formats      = SNDRV_PCM_FMTBIT_S16_LE,
    },
    .ops = &tom_dummy_cpu_dai_ops,
};

static const struct snd_soc_component_driver tom_dummy_cpu_component = {
    .name = TOM_DUMMY_CPU_DRV_NAME,
};

static int tom_dummy_cpu_probe(struct platform_device *pdev)
{
    pr_info("tom_cpu_dai: probe\n");

    return devm_snd_soc_register_component(&pdev->dev,
                                           &tom_dummy_cpu_component,
                                           &tom_dummy_cpu_dai, 1);
}

static struct platform_driver tom_dummy_cpu_driver = {
    .driver = {
        .name  = TOM_DUMMY_CPU_DRV_NAME,
        .owner = THIS_MODULE,
    },
    .probe  = tom_dummy_cpu_probe,
};

static struct platform_device *tom_dummy_cpu_pdev;

static int __init tom_dummy_cpu_init(void)
{
    int ret;

    pr_info("tom_cpu_dai: init\n");

    ret = platform_driver_register(&tom_dummy_cpu_driver);
    if (ret)
        return ret;

    tom_dummy_cpu_pdev =
        platform_device_register_simple(TOM_DUMMY_CPU_DRV_NAME,
                                        -1, NULL, 0);
    if (IS_ERR(tom_dummy_cpu_pdev)) {
        ret = PTR_ERR(tom_dummy_cpu_pdev);
        platform_driver_unregister(&tom_dummy_cpu_driver);
        return ret;
    }

    return 0;
}

static void __exit tom_dummy_cpu_exit(void)
{
    pr_info("tom_cpu_dai: exit\n");

    platform_device_unregister(tom_dummy_cpu_pdev);
    platform_driver_unregister(&tom_dummy_cpu_driver);
}

module_init(tom_dummy_cpu_init);
module_exit(tom_dummy_cpu_exit);

MODULE_DESCRIPTION("Tom Dummy CPU DAI driver");
MODULE_AUTHOR("Tom Hsieh");
MODULE_LICENSE("GPL");

