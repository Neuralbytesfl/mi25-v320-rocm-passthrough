# License and Compliance Notes

This repository contains notes, small helper scripts, and patch files used to
document a local ROCm passthrough setup.

## Repository License

Original documentation and helper scripts in this repository are licensed under
the repository's MIT license unless a file says otherwise.

## Third-Party Source Patches

The patch files under `patches/` modify third-party projects and should be
treated as derivative source patches of those projects:

```text
patches/qemu/0001-pcie-root-port-atomic-completion.patch
```

This modifies QEMU source files. QEMU is primarily distributed under GPL-2.0-or-
later terms, with some components under other compatible free-software licenses.
The touched files include GPL-2.0-or-later license headers. If redistributed or
applied to QEMU source, keep the patch available under the same terms as the
surrounding QEMU source.

```text
patches/amdgpu/0001-vega10-mi25-passthrough-init-stability.patch
```

This modifies AMDGPU driver source from the AMD DKMS package. The code is part
of the Linux kernel/AMDGPU driver family and should be treated under the
licensing terms of the source package being patched. If redistributed or applied
to that driver source, keep the patch available under the same terms as the
surrounding driver source.

## What Is Not Included

The repository intentionally does not include:

- Full QEMU source code.
- Full AMDGPU DKMS source code.
- VBIOS ROM images.
- PowerPlay binary table dumps.
- VM disk images.
- ISO images.
- Host or guest passwords.
- Private SSH keys.
- Private network addresses or Wi-Fi credentials.

## VBIOS and PowerPlay Tables

Runtime PowerPlay table changes are documented as field names and values. Binary
PowerPlay table files and ROM images are not committed because they may be tied
to a specific card, firmware image, or vendor distribution.

## Benchmark Links

The public Geekbench result URL is included. The private Geekbench claim URL/key
is not included.
