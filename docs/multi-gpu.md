# Multi-GPU Passthrough Notes

A second Radeon PRO V320 / MI25-family card should be passable into the same VM,
but it should be treated as a separate passthrough device with separate thermal
and power management.

## Host Requirements

Before attaching a second card, check:

```bash
find /sys/kernel/iommu_groups -type l | sort
lspci -nn
```

Each GPU should either be in its own IOMMU group or be isolated in a group that
contains only devices safe to pass through together.

The host should bind both cards to `vfio-pci`. Example IDs may be the same for
both cards:

```text
1002:6860
```

Do not assume both cards have identical subsystem IDs or VBIOS tables. Dump and
inspect each card separately.

## VM XML

Add one `<hostdev>` block per GPU. The PCI addresses will differ per host.

The guest should see two GPUs, usually as:

```text
/sys/class/drm/card1/device
/sys/class/drm/card2/device
```

The exact numbering can change across boots, so persistent scripts should find
cards by PCI ID or by walking `/sys/bus/pci/devices`.

## PowerPlay Per Card

Each card has its own runtime table:

```text
/sys/class/drm/card*/device/pp_table
```

Each card also has its own hwmon path:

```text
/sys/class/drm/card*/device/hwmon/hwmon*
```

For two cards, the one-card systemd helper in this repository should be extended
to iterate over all matching `1002:6860` devices and apply the selected table to
each one. Do not blindly write the same table to a card until its original
PowerPlay table has been dumped and checked.

## Cooling and Power

Two passive accelerator cards need directed airflow. Re-test thermals after
adding the second card; do not reuse single-card limits without a new soak test.

Recommended process:

```text
1. Bring up both cards at firmware default power.
2. Confirm ROCm sees both `gfx900` devices.
3. Run a low-power test on each card individually.
4. Run both cards together at 110 W.
5. Re-sweep 120/130/140/150 W with both cards loaded.
6. Pick a limit based on sustained clocks and memory temperature, not only peak score.
```
