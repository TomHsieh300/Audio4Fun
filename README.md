# Audio4Fun: Linux ASoC Dummy Driver Implementation

## Overview
Audio4Fun is a specialized project designed to demonstrate and explore the Linux ALSA System on Chip (ASoC) subsystem. It implements a minimal yet functional set of drivers—a **Dummy Codec** and a **Dummy Machine** driver—to simulate audio card registration and data paths without requiring physical audio hardware.

This project serves as a reference for:
* Understanding ASoC architecture (Machine vs. Codec vs. Platform).
* Learning DAPM (Dynamic Audio Power Management) widget configuration.
* Practicing Linux Kernel Module compilation and signing.

## Project Structure

* **dummy_codec.c**: Implements the codec driver, defining DAI (Digital Audio Interface) capabilities, DAPM widgets ("Dummy DAC", "Dummy Out"), and routes.
* **dummy_machine.c**: Implements the machine driver, responsible for the `snd_soc_card` instantiation and linking the dummy codec to the CPU DAI.
* **Makefile**: Kbuild-compliant makefile with support for module signing.

## Prerequisites

* Linux Kernel Headers (matching your current running kernel)
* GCC Toolchain
* Make

## Build Instructions

1.  **Compile the modules:**
    ```bash
    make
    ```
    This command builds `dummy_codec.ko` and `dummy_machine.ko` against `/lib/modules/$(uname -r)/build`.

2.  **Sign the modules (Optional):**
    If your system enforces module signing (Secure Boot), use the provided target:
    ```bash
    make sign
    ```
    *Note: This uses the local `signing_key.pem` and `signing_key.x509`.*

3.  **Clean build artifacts:**
    ```bash
    make clean
    ```

## Usage

### 1. Load the Modules
Load the codec driver first, followed by the machine driver:

```bash
sudo insmod dummy_codec.ko
sudo insmod dummy_machine.ko
