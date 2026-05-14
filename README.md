# AMD MI25 / Radeon PRO V320 ROCm Passthrough Notes

This repository documents a reproducible setup for running an AMD Vega 10
Instinct MI25-family card, identified as a Radeon PRO V320, inside an Ubuntu
22.04 QEMU/libvirt guest with ROCm.

The useful discovery is that the card's `110 W` firmware power limit can be
tested safely with a reversible Linux soft PowerPlay table. No VBIOS flash is
required to test higher caps such as `130 W` or `150 W`.

## Hardware Observed

```text
PCI device:        1002:6860
Subsystem:         1002:0c35
ROCm card series:  Vega 10 [Radeon Instinct MI25]
ROCm card model:   Radeon PRO V320
VBIOS:             113-D0513500-N00
VBIOS SKU:         D0513500
Compute units:     56
VRAM:              16 GiB HBM2
PCIe:              3.0 x16
```

The full AMD Instinct MI25 reference spec is `64 CU`, `12.29 TFLOPS FP32`,
`484 GB/s` HBM2 bandwidth, and `300 W` peak TDP. This V320-profile card exposes
`56 CU` and a default `110 W` PowerPlay limit.

## Guest Environment

```text
Guest OS:       Ubuntu 22.04.5 LTS
Kernel:         5.15.0-177-generic
ROCm:           5.7.1
amdgpu DKMS:    6.2.4-1664922.22.04
GPU target:     gfx900:xnack+
```

Ubuntu's HWE 6.8 kernel and the stock in-tree driver failed GPU initialization
for this card in passthrough. The stable setup used Ubuntu 22.04's GA 5.15
kernel plus AMD's ROCm 5.7.1 driver stack.

## Host/VM Requirements

The host uses libvirt/QEMU with PCI passthrough. Important host-side pieces:

- IOMMU enabled.
- GPU bound to `vfio-pci` on the host.
- `vendor-reset` loaded on the host for reliable VM restarts.
- QEMU patched so the PCIe root port advertises atomic completion capability.
- VM Secure Boot disabled.

The guest root port must expose PCIe atomic completion. Without this, the guest
driver reports that PCIe atomic ops are unsupported.

The exact QEMU patch used for the local build is included in:

```text
patches/qemu/0001-pcie-root-port-atomic-completion.patch
```

The VM used:

```text
-global pcie-root-port.x-atomic-completion=on
```

## Guest Driver Patches

Two guest-side amdgpu DKMS source changes were required for stable GPU init.
The minimal patch is included in:

```text
patches/amdgpu/0001-vega10-mi25-passthrough-init-stability.patch
```

See [docs/patch-details.md](docs/patch-details.md) for a more explicit record
of the affected files, fields, runtime arguments, verification output, and
license notes.

### PSP Delay

Patch `amd/amdgpu/psp_v3_1.c` so the first two bootloader delay points in:

```text
psp_v3_1_bootloader_load_sysdrv()
psp_v3_1_bootloader_load_sos()
```

use a longer delay:

```diff
- mdelay(20);
+ mdelay(500);
```

Only the bootloader sysdrv/sos delay points were changed.

### Avoid BACO Reset During Init

Patch `amd/amdgpu/soc15.c` so Vega10 passthrough does not auto-select BACO for
initialization reset. Use MODE1 instead.

```diff
- baco_reset = amdgpu_dpm_is_baco_supported(adev);
+ baco_reset = false;
```

After this patch, the guest showed:

```text
MODE1 reset
psp mode1 reset succeed
kfd: added device 1002:6860
Initialized amdgpu ... for 0000:05:00.0
```

## Reversible Soft PowerPlay

The live table is available at:

```text
/sys/class/drm/card1/device/pp_table
```

The original table contained:

```text
SocketPowerLimit: 110
BatteryPowerLimit: 110
SmallPowerLimit: 110
TdcLimit: 300
```

Using `upp`, only these three 16-bit fields were changed for higher runtime
caps:

```text
PowerTuneTable.SocketPowerLimit
PowerTuneTable.BatteryPowerLimit
PowerTuneTable.SmallPowerLimit
```

Tested runtime tables:

```text
110 W - original
130 W - balanced sustained-clock setting
150 W - higher raw compute, hotter
```

The recommended persistent setting from testing was `130 W`.

## Current Persistent Runtime Setup

Install the soft tables under:

```text
/etc/mi25-powerplay/
```

Use `/etc/mi25-powerplay.conf`:

```text
POWER_W=130
PROFILE=COMPUTE
PERF_LEVEL=high
```

Apply through a oneshot systemd service:

```text
/usr/local/sbin/mi25-powerplay-apply
/etc/systemd/system/mi25-powerplay.service
```

Switching to another tested cap is reversible:

```bash
sudo sed -i 's/^POWER_W=.*/POWER_W=150/' /etc/mi25-powerplay.conf
sudo systemctl restart mi25-powerplay.service
```

Disable soft PowerPlay entirely:

```bash
sudo systemctl disable --now mi25-powerplay.service
```

## Benchmark Summary

See [docs/benchmarks.md](docs/benchmarks.md) for measured benchmark results and
public comparison links.

## Training Ideas

See [docs/training-ideas.md](docs/training-ideas.md) for practical ML training
directions that fit this older `gfx900` ROCm setup, especially synthetic-data
generation and learned cellular automata.

## Multi-GPU Plan

A second card can be passed through to the same VM later if the host can isolate
it cleanly with IOMMU/vfio. See [docs/multi-gpu.md](docs/multi-gpu.md) for the
checklist and notes about per-card PowerPlay tables.

## License and Compliance

See [docs/compliance.md](docs/compliance.md). In short:

- Original documentation and helper scripts here are MIT licensed.
- QEMU patch files should be treated under QEMU's GPL-2.0-or-later terms.
- AMDGPU patch files should be treated under the source package's driver/kernel
  licensing terms.
- Binary VBIOS images and binary PowerPlay table dumps are intentionally not
  committed.

## Safety Notes

- Do not jump directly to `300 W`.
- No VBIOS flash is required for the useful runtime tests.
- Soft PowerPlay resets after driver reload/reboot unless the systemd service
  reapplies it.
- Keep a known-good original VBIOS backup before considering any firmware work.
- A hardware SPI programmer is recommended before flashing modified ROMs.

## Sources

- AMD Instinct MI25 specifications: https://www.amd.com/en/support/downloads/drivers.html/accelerators/instinct/instinct-mi-series/instinct-mi25.html
- Geekbench OpenCL chart: https://browser.geekbench.com/opencl-benchmarks
- `upp` PowerPlay table tool: https://github.com/sibradzic/upp
- PSP delay discussion: https://superuser.com/questions/1747738/amd-radeon-instinct-mi25-fails-to-initialize-drmamdgpu-device-fw-loading-amd
