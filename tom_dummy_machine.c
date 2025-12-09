// tom_dummy_machine.c - Machine driver for Tom Dummy ASoC card

#include <linux/module.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "tom_dummy.h"

/* ====== DAI link ====== */

static int tom_dummy_machine_hw_params(struct snd_pcm_substream *substream,
                                       struct snd_pcm_hw_params *params)
{
    int rate   = params_rate(params);
    int width  = snd_pcm_format_width(params_format(params));
    int ch     = params_channels(params);

    dev_info(substream->pcm->card->dev,
             "tom_machine: hw_params rate=%d width=%d ch=%d\n",
             rate, width, ch);

    return 0;
}

static const struct snd_soc_ops tom_dummy_machine_ops = {
    .hw_params = tom_dummy_machine_hw_params,
};

SND_SOC_DAILINK_DEFS(tom_dummy_link,
    DAILINK_COMP_ARRAY(COMP_CPU(TOM_DUMMY_CPU_DAI_NAME)),
    DAILINK_COMP_ARRAY(COMP_CODEC(TOM_DUMMY_CODEC_DRV_NAME,
                                  TOM_DUMMY_CODEC_DAI_NAME)),
    DAILINK_COMP_ARRAY(COMP_PLATFORM(TOM_DUMMY_PLATFORM_DRV_NAME))
);

static struct snd_soc_dai_link tom_dummy_dai_link = {
    .name          = "Tom Dummy Link",
    .stream_name   = "Dummy Playback",
    .playback_only = 1,
    .ops           = &tom_dummy_machine_ops,

    SND_SOC_DAILINK_REG(tom_dummy_link),
};

/* ====== Card ====== */

static struct snd_soc_card tom_dummy_card = {
    .name      = TOM_DUMMY_CARD_NAME,
    .owner     = THIS_MODULE,
    .dai_link  = &tom_dummy_dai_link,
    .num_links = 1,
};

/* ====== Machine platform driver ====== */

static int tom_dummy_machine_probe(struct platform_device *pdev)
{
    pr_info("tom_machine: probe\n");

    tom_dummy_card.dev = &pdev->dev;
    return devm_snd_soc_register_card(&pdev->dev, &tom_dummy_card);
}

static struct platform_driver tom_dummy_machine_driver = {
    .driver = {
        .name  = "tom-dummy-machine",
        .owner = THIS_MODULE,
    },
    .probe  = tom_dummy_machine_probe,
};

static struct platform_device *tom_dummy_machine_pdev;

static int __init tom_dummy_machine_init(void)
{
    int ret;

    pr_info("tom_machine: init\n");

    ret = platform_driver_register(&tom_dummy_machine_driver);
    if (ret)
        return ret;

    tom_dummy_machine_pdev =
        platform_device_register_simple("tom-dummy-machine",
                                        -1, NULL, 0);
    if (IS_ERR(tom_dummy_machine_pdev)) {
        ret = PTR_ERR(tom_dummy_machine_pdev);
        platform_driver_unregister(&tom_dummy_machine_driver);
        return ret;
    }

    return 0;
}

static void __exit tom_dummy_machine_exit(void)
{
    pr_info("tom_machine: exit\n");

    platform_device_unregister(tom_dummy_machine_pdev);
    platform_driver_unregister(&tom_dummy_machine_driver);
}

module_init(tom_dummy_machine_init);
module_exit(tom_dummy_machine_exit);

MODULE_DESCRIPTION("Tom Dummy ASoC virtual audio card (machine)");
MODULE_AUTHOR("Tom Hsieh");
MODULE_LICENSE("GPL");

