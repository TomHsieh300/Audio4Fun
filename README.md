# Audio4Fun: Linux ASoC Dummy Driver Implementation

## Overview

Audio4Fun is a specialized project designed to demonstrate and explore the Linux ALSA System on Chip (ASoC) subsystem. It implements a complete set of drivers—**CPU DAI**, **Platform (PCM)**, **Codec**, and **Machine** drivers—to simulate audio card registration and data paths without requiring physical audio hardware.

This project serves as a reference for:
* Understanding ASoC architecture (Machine vs. Codec vs. CPU DAI vs. Platform).
* Learning DAPM (Dynamic Audio Power Management) widget configuration.
* **Simulating bidirectional audio streams (Playback & Capture).**
* Exploring PCM buffer management with VMALLOC buffers.
* Practicing Linux Kernel Module compilation, signing, and **stress testing**.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Tom Dummy ASoC Card                      │
│                   (tom_dummy_machine.c)                     │
├─────────────────────────────────────────────────────────────┤
│                        DAI Link                             │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐  │
│  │   CPU DAI    │◄──►│   Codec DAI  │◄──►│   Platform   │  │
│  │ (tom_dummy   │    │ (tom_dummy   │    │ (tom_dummy   │  │
│  │  _cpu.c)     │    │  _codec.c)   │    │  _platform.c)│  │
│  └──────────────┘    └──────────────┘    └──────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## Project Structure

| File | Description |
|------|-------------|
| `tom_dummy.h` | Common definitions and driver/DAI name macros |
| `tom_dummy_cpu.c` | CPU DAI driver, defines the CPU-side digital audio interface |
| `tom_dummy_platform.c` | PCM Platform driver, handles buffer management and PCM operations |
| `tom_dummy_codec.c` | Codec driver, defines DAI capabilities, DAPM widgets, and mixer controls |
| `tom_dummy_machine.c` | Machine driver, creates `snd_soc_card` and links all components together |
| `test_audio_driver.sh`| **Stress test script for concurrent Playback/Capture open/close cycles** |
| `Makefile` | Kbuild-compliant makefile with module signing support |

## Module Details

### CPU DAI (`tom_dummy_cpu.ko`)
- Registers a virtual CPU DAI component.
- Supports **Full Duplex** (Playback & Capture).
- Channels: 2 (Stereo).
- Sample rates: 44100 Hz, 48000 Hz.
- Format: S16_LE (16-bit signed little-endian).

### Platform (`tom_dummy_platform.ko`)
- **Virtual PCM Engine**: Implements a software-based DMA simulation using `hrtimer`. It consumes/produces data in real-time and generates virtual period interrupts.
- **Capture Simulation**: For capture streams, the driver fills the DMA buffer with silence (memset 0) to simulate incoming data, preventing uninitialized memory reads.
- **Buffer Management**: Uses `SNDRV_DMA_TYPE_VMALLOC` for continuous buffer allocation.
- **Concurrency & Stability**: Features robust locking mechanisms to handle race conditions during concurrent `trigger`, `pointer`, and `close` operations.
- Buffer size: 64KB ~ 512KB.
- Period size: 1024B ~ 64KB.

### Codec (`tom_dummy_codec.ko`)
- DAPM widgets: `Dummy DAC`, `Dummy Out`, `Dummy ADC`, `Dummy In`, `Playback Path` (switch).
- Mixer controls:
  - `Master Playback Volume` (range: 0-100, default: 20)
  - `Playback Switch` (DAPM switch for audio path control)

### Machine (`tom_dummy_machine.ko`)
- Card name: "Tom Dummy ASoC Card"
- Links CPU DAI, Codec DAI, and Platform together.
- Configured for both Playback and Capture streams.

## Prerequisites

* Linux Kernel Headers (matching your current running kernel)
* GCC Toolchain
* Make

## Build Instructions

1. **Compile the modules:**
   ```bash
   make
   ```
   This builds four kernel modules:
   - `tom_dummy_cpu.ko`
   - `tom_dummy_platform.ko`
   - `tom_dummy_codec.ko`
   - `tom_dummy_machine.ko`

2. **Sign the modules (Optional):**
   If your system enforces module signing (Secure Boot), use:
   ```bash
   make sign
   ```
   *Note: Requires `signing_key.pem` and `signing_key.x509` in the project directory.*

3. **Clean build artifacts:**
   ```bash
   make clean
   ```

## Usage

### 1. Load the Modules

Load modules in the correct order (dependencies first):

```bash
sudo insmod tom_dummy_cpu.ko
sudo insmod tom_dummy_platform.ko
sudo insmod tom_dummy_codec.ko
sudo insmod tom_dummy_machine.ko
```

### 2. Verify Installation

Check the kernel log to confirm successful probing:

```bash
dmesg | tail -20
```

Expected output:
```
tom_cpu_dai: init
tom_cpu_dai: probe
tom_platform: init
tom_platform: probe
tom_codec: init
tom_codec: probe
tom_machine: init
tom_machine: probe
```

Verify the sound card registration:

```bash
cat /proc/asound/cards
# or
aplay -l
arecord -l
```

### 3. Playback & Capture Test
Generate some dummy audio traffic to verify the PCM path and buffer management.

**Playback:**

```bash
# Play random noise (or any wav file) to the card
# Replace '-D plughw:X' with your card number (e.g., plughw:0)
aplay -D plughw:0 -f cd /dev/urandom --duration=5
```

**Capture**

```bash
# Record 5 seconds of audio (will be silence/zero-filled data)
arecord -D plughw:0 -f cd -d 5 /tmp/test_capture.wav
```

### 4. Stress Testing

Use the provided script to verify driver stability under high load and rapid open/close cycles.

```bash
chmod +x test_audio_driver.sh
./test_audio_driver.sh
```
*This script will simulate 100 iterations of concurrent playback and recording, forcibly killing streams to test driver resource cleanup.*

### 5. Mixer Control

Use `amixer` (part of `alsa-utils`) to interact with mixer controls.

**List all controls:**

```bash
amixer -c <card_number> contents
```

**Control the Playback Switch (DAPM):**

```bash
# Turn Off (Mute) - Disconnects DAC from Output
amixer -c <card_number> cset name='Playback Switch' 0

# Turn On (Unmute) - Connects DAC to Output
amixer -c <card_number> cset name='Playback Switch' 1
```

**Control the Master Volume:**

The volume range is 0-100 (default: 20). Changes are logged to the kernel ring buffer.

```bash
# Set volume to 80%
amixer -c <card_number> cset name='Master Playback Volume' 80

# Check the log for the update message
dmesg | tail
# Expected: "tom_codec: update audio_volume to 80"
```

### 6. Unload Modules

Unload in reverse order to avoid dependency issues:

```bash
sudo rmmod tom_dummy_machine
sudo rmmod tom_dummy_codec
sudo rmmod tom_dummy_platform
sudo rmmod tom_dummy_cpu
```

## License

This project is licensed under the GPL License.

## Author

Tom Hsieh
