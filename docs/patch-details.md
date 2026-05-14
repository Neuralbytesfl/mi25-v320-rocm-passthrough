# Patch Details

This page records the local code changes used during the working setup.

## QEMU Atomic Completion

Patch file:

```text
patches/qemu/0001-pcie-root-port-atomic-completion.patch
```

Purpose:

```text
Expose PCIe 32-bit and 64-bit atomic completion capability on the emulated
pcie-root-port so ROCm/amdgpu can initialize the Vega GPU in the guest.
```

Files touched:

```text
hw/pci-bridge/gen_pcie_root_port.c
hw/pci/pcie.c
include/hw/pci/pcie_port.h
```

Behavior added:

```text
x-atomic-completion boolean property on pcie-root-port
PCI_EXP_DEVCAP2_ATOMIC_COMP32
PCI_EXP_DEVCAP2_ATOMIC_COMP64
```

Runtime VM argument:

```text
-global pcie-root-port.x-atomic-completion=on
```

Guest verification:

```text
AtomicOpsCap: Routing- 32bit+ 64bit+ 128bitCAS-
```

License:

```text
QEMU source is primarily GPL-2.0-or-later. The touched files include GPL
license headers. Treat this patch under the same terms as QEMU source.
```

## AMDGPU PSP and Reset

Patch file:

```text
patches/amdgpu/0001-vega10-mi25-passthrough-init-stability.patch
```

Purpose:

```text
Let the Radeon PRO V320 / MI25-family Vega10 card initialize reliably under
passthrough by giving PSP firmware handshakes more time and avoiding BACO reset
during guest driver initialization.
```

Files touched:

```text
amd/amdgpu/psp_v3_1.c
amd/amdgpu/soc15.c
```

Changes:

```text
psp_v3_1_bootloader_load_sysdrv(): mdelay(20) -> mdelay(500)
psp_v3_1_bootloader_load_sos():    mdelay(20) -> mdelay(500)
soc15_asic_reset_method():         force non-VEGA20 Vega10 path away from BACO
```

Expected guest dmesg after patch:

```text
MODE1 reset
GPU mode1 reset
GPU psp mode1 reset
psp mode1 reset succeed
reserve ... for PSP TMR
kfd: added device 1002:6860
Initialized amdgpu ... for 0000:05:00.0
```

License:

```text
The patch modifies AMDGPU driver source from the AMD DKMS package. Treat it
under the same licensing terms as the source package and surrounding driver
files.
```

## Soft PowerPlay

This is not a source-code patch. It is a runtime table modification applied
through the Linux amdgpu sysfs interface:

```text
/sys/class/drm/card*/device/pp_table
```

Fields changed from the original table:

```text
PowerTuneTable.SocketPowerLimit
PowerTuneTable.BatteryPowerLimit
PowerTuneTable.SmallPowerLimit
```

Tested values:

```text
110 W original
130 W balanced
150 W higher raw compute, hotter
```

The repository includes the systemd helper but does not include binary
PowerPlay table dumps.
