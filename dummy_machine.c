#include <linux/module.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>

#define MACHINE_NAME "dummy-audio-card"

SND_SOC_DAILINK_DEFS(dummy_link,
    DAILINK_COMP_ARRAY(COMP_CPU("snd-soc-dummy-dai")),
    DAILINK_COMP_ARRAY(COMP_CODEC("dummy-codec", "dummy-codec-dai")),
    DAILINK_COMP_ARRAY(COMP_PLATFORM("snd-soc-dummy")));

static struct snd_soc_dai_link dummy_dai_link = {
    .name = "Dummy Audio Link",
    .stream_name = "Dummy Playback",
    .dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
    
    SND_SOC_DAILINK_REG(dummy_link),
};

static struct snd_soc_card dummy_card = {
    .name = "Dummy ASoC Card",
    .owner = THIS_MODULE,
    .dai_link = &dummy_dai_link,
    .num_links = 1,
};

static int dummy_machine_probe(struct platform_device *pdev)
{
    dummy_card.dev = &pdev->dev;
    return devm_snd_soc_register_card(&pdev->dev, &dummy_card);
}

static struct platform_driver dummy_machine_driver = {
    .driver = {
        .name = MACHINE_NAME,
        .owner = THIS_MODULE,
    },
    .probe = dummy_machine_probe,
};

static struct platform_device *dummy_machine_pdev;

static int __init dummy_machine_init(void)
{
    int err;
    
    err = platform_driver_register(&dummy_machine_driver);
    if (err < 0) return err;

    dummy_machine_pdev = platform_device_register_simple(MACHINE_NAME, -1, NULL, 0);
    if (IS_ERR(dummy_machine_pdev)) {
        platform_driver_unregister(&dummy_machine_driver);
        return PTR_ERR(dummy_machine_pdev);
    }

    return 0;
}

static void __exit dummy_machine_exit(void)
{
    platform_device_unregister(dummy_machine_pdev);
    platform_driver_unregister(&dummy_machine_driver);
}

module_init(dummy_machine_init);
module_exit(dummy_machine_exit);

MODULE_DESCRIPTION("ASoC Dummy Machine Driver");
MODULE_LICENSE("GPL");
