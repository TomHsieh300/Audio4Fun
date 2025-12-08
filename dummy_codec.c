#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>

#define DUMMY_CODEC_DRV_NAME   "dummy-codec"
#define DUMMY_CODEC_DAI_NAME   "dummy-codec-dai"
#define DEFAULT_VOLUME 20

static struct platform_device *dummy_pdev;
static int audio_volume = DEFAULT_VOLUME;

static int dummy_vol_get(struct snd_kcontrol *kcontrol,
                         struct snd_ctl_elem_value *ucontrol)
{
    ucontrol->value.integer.value[0] = audio_volume;

    return 0;
}

static int dummy_vol_put(struct snd_kcontrol *kcontrol,
                         struct snd_ctl_elem_value *ucontrol)
{
    int user_value = ucontrol->value.integer.value[0];

    if (user_value != audio_volume) {
        audio_volume = user_value;
        pr_info("update audio_volume to: %d\n", audio_volume);
        return 1;
    }

    return 0;
}

static const struct snd_kcontrol_new dapm_controls[] = {
    SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 0),
};

static const struct snd_kcontrol_new dummy_controls[] = {
    SOC_SINGLE_EXT("Master Playback Volume", SND_SOC_NOPM, 0, 100, 0,
                    dummy_vol_get, dummy_vol_put),
};

static const struct snd_soc_dapm_widget dummy_dapm_widgets[] = {
    SND_SOC_DAPM_DAC("Dummy DAC", "Master Playback", SND_SOC_NOPM, 0, 0),
    SND_SOC_DAPM_OUTPUT("Dummy Out"),
    SND_SOC_DAPM_SWITCH("Master Playback", SND_SOC_NOPM, 0, 0, &dapm_controls[0]),
};

static const struct snd_soc_dapm_route dummy_dapm_routes[] = {
    { "Master Playback", "Switch", "Dummy DAC" },
    { "Dummy Out", NULL, "Master Playback" },
};

static const struct snd_soc_component_driver dummy_component_driver = {
    .name               = DUMMY_CODEC_DRV_NAME,
    .dapm_widgets       = dummy_dapm_widgets,
    .num_dapm_widgets   = ARRAY_SIZE(dummy_dapm_widgets),
    .dapm_routes        = dummy_dapm_routes,
    .num_dapm_routes    = ARRAY_SIZE(dummy_dapm_routes),

    .controls = dummy_controls,
    .num_controls = ARRAY_SIZE(dummy_controls),

    .idle_bias_on       = 1,
    .use_pmdown_time    = 1,
    .endianness         = 1,
};

static struct snd_soc_dai_driver dummy_dai = {
    .name = DUMMY_CODEC_DAI_NAME,
    .playback = {
        .stream_name = "Master Playback",
        .channels_min = 1,
        .channels_max = 2,
        .rates = SNDRV_PCM_RATE_8000_192000,
        .formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
    },
};


static int dummy_codec_probe(struct platform_device *pdev)
{
    return devm_snd_soc_register_component(&pdev->dev,
                                           &dummy_component_driver,
                                           &dummy_dai,
                                           1);
}

static struct platform_driver dummy_codec_driver = {
    .driver = {
        .name = DUMMY_CODEC_DRV_NAME,
        .owner = THIS_MODULE,
    },
    .probe = dummy_codec_probe,
};

static int __init my_dummy_init(void)
{
    int err;
    err = platform_driver_register(&dummy_codec_driver);
    if(err < 0)
        return err;
    
    struct platform_device *device;
    
    device = platform_device_register_simple(DUMMY_CODEC_DRV_NAME, -1, NULL, 0);
    
    if (IS_ERR(device)) {
        platform_driver_unregister(&dummy_codec_driver);
        return PTR_ERR(device);
    }
    
    dummy_pdev = device;

    return 0;
}

static void __exit my_dummy_exit(void)
{
    platform_device_unregister(dummy_pdev);
    platform_driver_unregister(&dummy_codec_driver);
}

module_init(my_dummy_init)
module_exit(my_dummy_exit)

MODULE_DESCRIPTION("ASoC Dummy Codec Driver");
MODULE_AUTHOR("Tom Hsieh");
MODULE_LICENSE("GPL");
