#include <linux/module.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "tom_dummy.h"

#define DEFAULT_VOLUME 20

static int audio_volume = DEFAULT_VOLUME;

static int tom_dummy_codec_startup(struct snd_pcm_substream *substream,
                                   struct snd_soc_dai *dai)
{
    pr_info("tom_codec: startup (stream=%d)\n", substream->stream);
    return 0;
}

static int tom_dummy_codec_hw_params(struct snd_pcm_substream *substream,
                                     struct snd_pcm_hw_params *params,
                                     struct snd_soc_dai *dai)
{
    int rate   = params_rate(params);
    int width  = snd_pcm_format_width(params_format(params));
    int ch     = params_channels(params);

    pr_info("tom_codec: hw_params rate=%d width=%d ch=%d\n",
            rate, width, ch);

    return 0;
}

static const struct snd_soc_dai_ops tom_dummy_codec_dai_ops = {
    .startup   = tom_dummy_codec_startup,
    .hw_params = tom_dummy_codec_hw_params,
};

static int tom_dummy_vol_get(struct snd_kcontrol *kcontrol,
                             struct snd_ctl_elem_value *ucontrol)
{
    ucontrol->value.integer.value[0] = audio_volume;
    return 0;
}

static int tom_dummy_vol_put(struct snd_kcontrol *kcontrol,
                             struct snd_ctl_elem_value *ucontrol)
{
    int user_value = ucontrol->value.integer.value[0];

    if (user_value != audio_volume) {
        audio_volume = user_value;
        pr_info("tom_codec: update audio_volume to %d\n", audio_volume);
        return 1;
    }

    return 0;
}

static const struct snd_kcontrol_new tom_dummy_controls[] = {
    SOC_SINGLE_EXT("Master Playback Volume",
                   SND_SOC_NOPM, 0, 100, 0,
                   tom_dummy_vol_get, tom_dummy_vol_put),
};

static const struct snd_kcontrol_new tom_dummy_dapm_controls[] = {
    SOC_DAPM_SINGLE("Playback Switch", SND_SOC_NOPM, 0, 1, 0),
};

static const struct snd_soc_dapm_widget tom_dummy_dapm_widgets[] = {
    SND_SOC_DAPM_DAC("Dummy DAC", "Dummy Playback", SND_SOC_NOPM, 0, 0),
    SND_SOC_DAPM_OUTPUT("Dummy Out"),
    SND_SOC_DAPM_SWITCH("Playback Path", SND_SOC_NOPM, 0, 0,
                        &tom_dummy_dapm_controls[0]),
};

static const struct snd_soc_dapm_route tom_dummy_dapm_routes[] = {
    { "Playback Path", "Playback Switch", "Dummy DAC" },
    { "Dummy Out", NULL, "Playback Path" },
};

static struct snd_soc_dai_driver tom_dummy_codec_dai = {
    .name = TOM_DUMMY_CODEC_DAI_NAME,

    .playback = {
        .stream_name  = "Dummy Playback",
        .channels_min = 2,
        .channels_max = 2,
        .rates        = (SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000),
        .formats      = SNDRV_PCM_FMTBIT_S16_LE,
    },

    .capture= {
        .stream_name  = "Dummy Capture",
        .channels_min = 2,
        .channels_max = 2,
        .rates        = (SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000),
        .formats      = SNDRV_PCM_FMTBIT_S16_LE,
    },

    .ops = &tom_dummy_codec_dai_ops,
};

static const struct snd_soc_component_driver tom_dummy_codec_component = {
    .name              = TOM_DUMMY_CODEC_DRV_NAME,
    .dapm_widgets      = tom_dummy_dapm_widgets,
    .num_dapm_widgets  = ARRAY_SIZE(tom_dummy_dapm_widgets),
    .dapm_routes       = tom_dummy_dapm_routes,
    .num_dapm_routes   = ARRAY_SIZE(tom_dummy_dapm_routes),
    .controls          = tom_dummy_controls,
    .num_controls      = ARRAY_SIZE(tom_dummy_controls),
    .idle_bias_on      = 1,
    .use_pmdown_time   = 1,
};

static int tom_dummy_codec_probe(struct platform_device *pdev)
{
    pr_info("tom_codec: probe\n");

    return devm_snd_soc_register_component(&pdev->dev,
                                           &tom_dummy_codec_component,
                                           &tom_dummy_codec_dai, 1);
}

static struct platform_driver tom_dummy_codec_driver = {
    .driver = {
        .name  = TOM_DUMMY_CODEC_DRV_NAME,
        .owner = THIS_MODULE,
    },
    .probe  = tom_dummy_codec_probe,
};

static struct platform_device *tom_dummy_codec_pdev;

static int __init tom_dummy_codec_init(void)
{
    int ret;

    pr_info("tom_codec: init\n");

    ret = platform_driver_register(&tom_dummy_codec_driver);
    if (ret)
        return ret;

    tom_dummy_codec_pdev =
        platform_device_register_simple(TOM_DUMMY_CODEC_DRV_NAME,
                                        -1, NULL, 0);
    if (IS_ERR(tom_dummy_codec_pdev)) {
        ret = PTR_ERR(tom_dummy_codec_pdev);
        platform_driver_unregister(&tom_dummy_codec_driver);
        return ret;
    }

    return 0;
}

static void __exit tom_dummy_codec_exit(void)
{
    pr_info("tom_codec: exit\n");

    platform_device_unregister(tom_dummy_codec_pdev);
    platform_driver_unregister(&tom_dummy_codec_driver);
}

module_init(tom_dummy_codec_init);
module_exit(tom_dummy_codec_exit);

MODULE_DESCRIPTION("Tom Dummy Codec Driver");
MODULE_AUTHOR("Tom Hsieh");
MODULE_LICENSE("GPL");

