# Tom Dummy ASoC Driver — Architecture Overview

This document describes the architecture and internal structure of the Dummy ASoC Audio Driver implemented in:

- `tom_dummy_machine.c`
- `tom_dummy_cpu.c`
- `tom_dummy_codec.c`
- `tom_dummy_platform.c`

The design follows the **standard ALSA SoC (ASoC) architecture** and maps closely to real-world SoC audio subsystems (e.g., Rockchip I2S + ES7202 Codec + DMA Engine).

This project intentionally separates **CPU DAI / Codec / Platform / Machine** drivers to match how real audio drivers behave in the Linux kernel.

---

## 1. High-Level Architecture

```text
Userspace
   └── aplay → ALSA PCM API
           ↓
      ASoC PCM Core
           ↓
 +---------+---------+---------+
 |       Machine Driver        |
 +-----------------------------+
     |             |           |
     v             v           v
 CPU DAI      Codec DAI     Platform PCM Engine
 (I2S)        (Codec IC)     (Software DMA)
```

Each component has a clearly defined responsibility:

| Component | File | Responsibility |
| :--- | :--- | :--- |
| **Machine** | `tom_dummy_machine.c` | Defines overall card topology & DAI links |
| **CPU DAI** | `tom_dummy_cpu.c` | Represents SoC I2S controller capabilities |
| **Codec** | `tom_dummy_codec.c` | Represents audio codec (volume, DAPM, format) |
| **Platform** | `tom_dummy_platform.c` | PCM engine (buffer, period, pointer, trigger) |

## 2. Component Responsibilities

This section details what each file is responsible for and how they interact.

### 2.1 Machine Driver (`tom_dummy_machine.c`)
**Role: System Topology / Card Assembler**

The machine driver defines:
- Which CPU DAI connects to which Codec DAI
- Which Platform driver handles PCM
- DAI format (I2S, LJ, RJ, master/slave)
- Card name, routing, and `hw_params` override

**Key responsibilities:**
- Register `struct snd_soc_card`
- Create DAI link with `SND_SOC_DAILINK_DEFS`
- Override `hw_params()` to inspect or modify settings
- Assemble the whole audio system

> **Matches real world:**
> - Rockchip board machine drivers (e.g., `rk3588_snd_card.c`)
> - Simple-audio-card in device tree

### 2.2 CPU DAI (`tom_dummy_cpu.c`)
**Role: SoC I2S controller abstraction**

Defines:
- Supported sample rates
- Channels / formats (S16, S24)
- I2S mode, master/slave
- Startup & `hw_params` callbacks

**Responsibilities:**
- `hw_params()` handles SoC I2S configuration
- `startup()` prepares controller
- Provide `snd_soc_dai_driver` to Machine driver

> **Matches real world:**
> - Rockchip I2S controller (`rockchip_i2s.c`)
> - Qualcomm, MTK, NXP SoC DAI drivers

### 2.3 Codec Driver (`tom_dummy_codec.c`)
**Role: Audio Codec IC (volume, DAPM, signal path)**

Defines:
- DAPM widgets (DAC, output)
- DAPM routes (DAC → Switch → Out)
- ALSA mixer controls (Volume control)
- Codec DAI capabilities (rates, formats)
- Responds to `hw_params()` and `startup()`

**Responsibilities:**
- Implement mixer get/put for volume
- Implement codec DAI callbacks
- Define codec audio path using DAPM

> **Matches real world:**
> - ES7202, RT5640, WM8960 codec drivers
> - Any I2C/SPI driven codec IC

### 2.4 Platform Driver (PCM Engine) (`tom_dummy_platform.c`)
**Role: PCM engine = who moves audio data**

This is the core of the entire driver.
In real hardware, this would be a **DMA engine**.
In this project, it is implemented using **hrtimer + software buffer movement**.

**Responsibilities:**
- Allocate buffer & periods (`hw_params`)
- Implement `pointer()` to report hardware position
- Implement `trigger()`:
    - **START** → start hrtimer
    - **STOP** → stop hrtimer
- Run hrtimer callback:
    - Advance `hw_ptr`
    - Call `snd_pcm_period_elapsed()`

> **Matches real world:**
> - Rockchip DMA engine (`rk_dmaengine_pcm.c`)
> - ALSA generic dmaengine PCM
> - Any hardware PCM engine

## 3. ASoC Playback Call Flow (Detailed)

```text
aplay
 |
 | open()
 v
tom_platform.open()
tom_cpu.startup()
tom_codec.startup()
 |
 | hw_params()
 v
tom_machine.hw_params()
tom_cpu.hw_params()
tom_codec.hw_params()
tom_platform.hw_params()
 |
 | prepare()
 v
tom_platform.prepare()
 |
 | trigger(START)
 v
tom_platform.trigger() → start hrtimer
 |
 | Every period:
 v
hrtimer_cb()
  → update hw_ptr
  → snd_pcm_period_elapsed()
 |
 | trigger(STOP)
 v
tom_platform.trigger() stop hrtimer
 |
 | close()
 v
tom_platform.close()
```

## 4. PCM Lifecycle (ALSA Standard Flow)

```text
open →
  hw_params →
    prepare →
      trigger(START) →
        pointer (repeated) →
      trigger(STOP) →
close
```

**Stage Explanations:**

- **`open()`**: Initialize PCM runtime.
- **`hw_params()`**: Allocate buffer, set sample rate, channels, format, periods.
- **`prepare()`**: Reset DMA (or simulated DMA) and runtime state.
- **`trigger(START)`**: Start DMA engine (or hrtimer).
- **`pointer()`**: Report hardware pointer back to ALSA.
- **`trigger(STOP)`**: Stop DMA / timer.
- **`close()`**: Free resources.

## 5. DAI Link Deep Analysis

The Machine Driver’s DAI link defines who connects to who.

**Example:**
```c
{
    .name = "Dummy Playback",
    .stream_name = "Playback",
    .cpu_dai_name = "tom-dummy-cpu-dai",
    .codec_dai_name = "tom-dummy-codec-dai",
    .codec_name = "tom-dummy-codec",
    .platform_name = "tom-dummy-platform",
    .dai_fmt = SND_SOC_DAIFMT_I2S |
               SND_SOC_DAIFMT_NB_NF |
               SND_SOC_DAIFMT_CBS_CFS,
}
```

**Field Breakdown**
| Field             | Meaning                             |
| ----------------- | ----------------------------------- |
| `.cpu_dai_name`   | SoC I2S controller                  |
| `.codec_dai_name` | Codec DAI interface                 |
| `.codec_name`     | Codec device name                   |
| `.platform_name`  | PCM engine driver                   |
| `.dai_fmt`        | I2S/LJ/RJ + polarity + master/slave |

**Common dai_fmt Flags**
| Flag                     | Meaning                    |
| ------------------------ | -------------------------- |
| `SND_SOC_DAIFMT_I2S`     | I2S format                 |
| `SND_SOC_DAIFMT_LEFT_J`  | Left-justified             |
| `SND_SOC_DAIFMT_RIGHT_J` | Right-justified            |
| `SND_SOC_DAIFMT_CBS_CFS` | Codec is BCLK/LRCLK slave  |
| `SND_SOC_DAIFMT_CBM_CFM` | Codec is BCLK/LRCLK master |
| `SND_SOC_DAIFMT_NB_NF`   | Normal clock polarity      |

The DAI link is the centerpiece of any machine driver.

## 6. ASoC Four-Layer Model (Conceptual Diagram)
```text
 +----------------------------------------------------+
 |                  Machine Driver                    |
 |     - Card definition                              |
 |     - DAI links (CPU ↔ Codec ↔ Platform)           |
 +----------------------------------------------------+
 |      CPU DAI                Codec DAI              |
 |  (I2S/TDM Controller)   (External Codec IC)        |
 +----------------------------------------------------+
 |                Platform PCM Engine                 |
 |       (DMA engine / Software DMA simulator)        |
 +----------------------------------------------------+
 |                     ALSA Core                      |
 +----------------------------------------------------+
 |                  User Space (aplay)                |
 +----------------------------------------------------+
```

This model is essential—every ASoC system fits this structure.

## 7. File-by-File Mapping Table

| File                   | Function Types                          | Purpose                     |
| :--------------------- | :-------------------------------------- | :-------------------------- |
| `tom_dummy_machine.c`  | probe, hw_params, DAI link              | Board topology              |
| `tom_dummy_cpu.c`      | startup, hw_params, DAI ops             | SoC I2S abstraction         |
| `tom_dummy_codec.c`    | DAPM, mixer controls, codec DAI         | Codec behavior              |
| `tom_dummy_platform.c` | PCM ops (open, close, pointer, trigger) | PCM engine / DMA simulation |

## 8. Glossary (ASoC Technical Terms)

| Term               | Definition                            |
| ------------------ | ------------------------------------- |
| **ASoC**           | ALSA System-on-Chip                   |
| **DAI**            | Digital Audio Interface               |
| **CPU DAI**        | SoC audio controller                  |
| **Codec DAI**      | External codec interface              |
| **Platform**       | PCM engine (DMA)                      |
| **Machine Driver** | Board-level topology                  |
| **DAPM**           | Dynamic Audio Power Management        |
| **hw_params**      | PCM hardware config stage             |
| **prepare**        | Stream initialization                 |
| **trigger**        | Start/stop PCM                        |
| **pointer**        | Reports playback progress             |
| **period**         | Chunk of samples triggering interrupt |
| **buffer**         | Full PCM ring buffer                  |
| **hw_ptr**         | Hardware pointer                      |
| **DMA**            | Direct Memory Access                  |
| **aplay**          | ALSA playback utility                 |

## 9. Summary

`tom_dummy` is a fully modular ASoC implementation that demonstrates:

- ✔ **Clear CPU / Codec / Platform / Machine separation**
- ✔ **Realistic ALSA PCM lifecycle**
- ✔ **Fully functional timer-based DMA simulator**
- ✔ **Clean ASoC architecture identical to real SoC drivers**

This project forms an ideal foundation for porting to:

- Real codecs (ES7202, WM8960, RT5640)
- SoC I2S hardware (Rockchip, MTK, Qualcomm)
- Real DMA engines
