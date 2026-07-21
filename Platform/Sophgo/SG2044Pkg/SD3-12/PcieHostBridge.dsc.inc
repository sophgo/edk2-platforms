## SD3-12 PCIe host bridge configuration (PcdPcieHostBridgeTable).
## Included from SD3-12.dsc inside [PcdsFixedAtBuild.common].
## Field layout and how-to: skill sg2044-pcie-bar.

  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.NumOfControllers|6

  # ---- Controller 0 (domain 0) ----
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Domain|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Flag.Mem32Support|TRUE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Flag.Pmem32Support|FALSE
  # 64bit non-prefetchable: set Mem64Support TRUE + fill Mem64.* to enable
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Flag.Mem64Support|FALSE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Flag.Pmem64Support|TRUE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Flag.IoSupport|TRUE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Bus.RootBusBase|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Bus.RootBusLimit|0xFF
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Bus.RootBusTranslation|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Reg.DbiBase|0x6C00400000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Reg.DbiSize|0x1000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Reg.CtrBase|0x6C00780000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Reg.CtrSize|0x1000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Reg.AtuBase|0x6C00700000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Reg.AtuSize|0x4000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Reg.CfgBase|0x4000000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Reg.CfgSize|0x1000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Pmem32.CpuAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Pmem32.PciAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Pmem32.RangeSize|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Mem32.CpuAddr|0x40000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Mem32.PciAddr|0x40000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Mem32.RangeSize|0x10000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Pmem64.CpuAddr|0x4800000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Pmem64.PciAddr|0x4800000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Pmem64.RangeSize|0x800000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Mem64.CpuAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Mem64.PciAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Mem64.RangeSize|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Io.CpuAddr|0x4010000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Io.PciAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Io.RangeSize|0x100000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Space32Start|0x40000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Space32End|0x4FFFFFFF
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Space64Start|0x4000000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].Space64End|0x4FFFFFFFFF

  # ---- Controller 1 (domain 2) ----
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Domain|2
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Flag.Mem32Support|TRUE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Flag.Pmem32Support|FALSE
  # 64bit non-prefetchable: set Mem64Support TRUE + fill Mem64.* to enable
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Flag.Mem64Support|FALSE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Flag.Pmem64Support|TRUE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Flag.IoSupport|TRUE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Bus.RootBusBase|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Bus.RootBusLimit|0xFF
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Bus.RootBusTranslation|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Reg.DbiBase|0x6C00000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Reg.DbiSize|0x1000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Reg.CtrBase|0x6C000C0000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Reg.CtrSize|0x1000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Reg.AtuBase|0x6C00300000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Reg.AtuSize|0x4000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Reg.CfgBase|0x5000000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Reg.CfgSize|0x1000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Pmem32.CpuAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Pmem32.PciAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Pmem32.RangeSize|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Mem32.CpuAddr|0x50000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Mem32.PciAddr|0x50000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Mem32.RangeSize|0x10000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Pmem64.CpuAddr|0x5800000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Pmem64.PciAddr|0x5800000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Pmem64.RangeSize|0x800000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Mem64.CpuAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Mem64.PciAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Mem64.RangeSize|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Io.CpuAddr|0x5010000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Io.PciAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Io.RangeSize|0x100000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Space32Start|0x50000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Space32End|0x5FFFFFFF
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Space64Start|0x5000000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].Space64End|0x5FFFFFFFFF

  # ---- Controller 2 (domain 4) ----
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Domain|4
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Flag.Mem32Support|TRUE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Flag.Pmem32Support|FALSE
  # 64bit non-prefetchable: set Mem64Support TRUE + fill Mem64.* to enable
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Flag.Mem64Support|FALSE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Flag.Pmem64Support|TRUE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Flag.IoSupport|TRUE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Bus.RootBusBase|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Bus.RootBusLimit|0xFF
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Bus.RootBusTranslation|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Reg.DbiBase|0x6C04400000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Reg.DbiSize|0x1000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Reg.CtrBase|0x6C04780000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Reg.CtrSize|0x1000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Reg.AtuBase|0x6C04700000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Reg.AtuSize|0x4000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Reg.CfgBase|0x7800000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Reg.CfgSize|0x1000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Pmem32.CpuAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Pmem32.PciAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Pmem32.RangeSize|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Mem32.CpuAddr|0x20000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Mem32.PciAddr|0x20000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Mem32.RangeSize|0x8000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Pmem64.CpuAddr|0x7900000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Pmem64.PciAddr|0x7900000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Pmem64.RangeSize|0x100000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Mem64.CpuAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Mem64.PciAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Mem64.RangeSize|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Io.CpuAddr|0x7810000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Io.PciAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Io.RangeSize|0x100000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Space32Start|0x20000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Space32End|0x27FFFFFF
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Space64Start|0x7800000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].Space64End|0x79FFFFFFFF

  # ---- Controller 3 (domain 5) ----
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Domain|5
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Flag.Mem32Support|TRUE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Flag.Pmem32Support|FALSE
  # 64bit non-prefetchable: set Mem64Support TRUE + fill Mem64.* to enable
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Flag.Mem64Support|FALSE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Flag.Pmem64Support|TRUE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Flag.IoSupport|TRUE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Bus.RootBusBase|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Bus.RootBusLimit|0xFF
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Bus.RootBusTranslation|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Reg.DbiBase|0x6C04800000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Reg.DbiSize|0x1000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Reg.CtrBase|0x6C04B80000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Reg.CtrSize|0x1000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Reg.AtuBase|0x6C04B00000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Reg.AtuSize|0x4000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Reg.CfgBase|0x7A00000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Reg.CfgSize|0x1000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Pmem32.CpuAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Pmem32.PciAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Pmem32.RangeSize|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Mem32.CpuAddr|0x28000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Mem32.PciAddr|0x28000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Mem32.RangeSize|0x8000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Pmem64.CpuAddr|0x7B00000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Pmem64.PciAddr|0x7B00000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Pmem64.RangeSize|0x100000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Mem64.CpuAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Mem64.PciAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Mem64.RangeSize|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Io.CpuAddr|0x7A10000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Io.PciAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Io.RangeSize|0x100000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Space32Start|0x28000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Space32End|0x2FFFFFFF
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Space64Start|0x7A00000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].Space64End|0x7BFFFFFFFF

  # ---- Controller 4 (domain 6) ----
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Domain|6
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Flag.Mem32Support|TRUE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Flag.Pmem32Support|FALSE
  # 64bit non-prefetchable: set Mem64Support TRUE + fill Mem64.* to enable
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Flag.Mem64Support|FALSE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Flag.Pmem64Support|TRUE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Flag.IoSupport|TRUE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Bus.RootBusBase|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Bus.RootBusLimit|0xFF
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Bus.RootBusTranslation|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Reg.DbiBase|0x6C04000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Reg.DbiSize|0x1000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Reg.CtrBase|0x6C040C0000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Reg.CtrSize|0x1000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Reg.AtuBase|0x6C04300000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Reg.AtuSize|0x4000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Reg.CfgBase|0x7C00000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Reg.CfgSize|0x1000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Pmem32.CpuAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Pmem32.PciAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Pmem32.RangeSize|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Mem32.CpuAddr|0x30000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Mem32.PciAddr|0x30000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Mem32.RangeSize|0x10000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Pmem64.CpuAddr|0x7E00000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Pmem64.PciAddr|0x7E00000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Pmem64.RangeSize|0x200000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Mem64.CpuAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Mem64.PciAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Mem64.RangeSize|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Io.CpuAddr|0x7C10000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Io.PciAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Io.RangeSize|0x100000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Space32Start|0x30000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Space32End|0x3FFFFFFF
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Space64Start|0x7C00000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].Space64End|0x7FFFFFFFFF

  # ---- Controller 5 (domain 8) ----
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Domain|8
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Flag.Mem32Support|TRUE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Flag.Pmem32Support|FALSE
  # 64bit non-prefetchable: set Mem64Support TRUE + fill Mem64.* to enable
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Flag.Mem64Support|FALSE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Flag.Pmem64Support|TRUE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Flag.IoSupport|TRUE
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Bus.RootBusBase|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Bus.RootBusLimit|0xFF
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Bus.RootBusTranslation|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Reg.DbiBase|0x6C08400000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Reg.DbiSize|0x1000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Reg.CtrBase|0x6C08780000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Reg.CtrSize|0x1000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Reg.AtuBase|0x6C08700000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Reg.AtuSize|0x4000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Reg.CfgBase|0x6000000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Reg.CfgSize|0x1000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Pmem32.CpuAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Pmem32.PciAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Pmem32.RangeSize|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Mem32.CpuAddr|0x10000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Mem32.PciAddr|0x10000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Mem32.RangeSize|0x10000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Pmem64.CpuAddr|0x6400000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Pmem64.PciAddr|0x6400000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Pmem64.RangeSize|0x400000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Mem64.CpuAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Mem64.PciAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Mem64.RangeSize|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Io.CpuAddr|0x6010000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Io.PciAddr|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Io.RangeSize|0x100000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Space32Start|0x10000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Space32End|0x1FFFFFFF
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Space64Start|0x6000000000
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].Space64End|0x67FFFFFFFF

  # ---- PcieInitPei bring-up data (PEI PHY/link training) ----
  # Per-controller CtrlInit: C2cId/WrapperId/PhyId locate the controller in
  # the SoC C2C/wrapper/PHY hierarchy (FSBL numbering, c2c in {0,1,4}; CXP=4).
  # GenSpeed=5 (PCIE_GEN_5). LaneCount=8 (x8) or 4 (x4+x4 split). PERST is not
  # stored here -- it is driven by the SoC-fixed perst_gpio[10] table replicated
  # in PcieInitPei (FSBL sg2260_pcie.c:1558), indexed c2c*4+wrapper*2+phy with
  # c2c==4 folded to 2, so PerstGpio is left at 0.
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].CtrlInit.C2cId|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].CtrlInit.WrapperId|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].CtrlInit.PhyId|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].CtrlInit.GenSpeed|5
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].CtrlInit.LaneCount|8
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[0].CtrlInit.PerstGpio|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].CtrlInit.C2cId|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].CtrlInit.WrapperId|1
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].CtrlInit.PhyId|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].CtrlInit.GenSpeed|5
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].CtrlInit.LaneCount|8
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[1].CtrlInit.PerstGpio|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].CtrlInit.C2cId|1
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].CtrlInit.WrapperId|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].CtrlInit.PhyId|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].CtrlInit.GenSpeed|5
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].CtrlInit.LaneCount|4
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[2].CtrlInit.PerstGpio|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].CtrlInit.C2cId|1
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].CtrlInit.WrapperId|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].CtrlInit.PhyId|1
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].CtrlInit.GenSpeed|5
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].CtrlInit.LaneCount|4
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[3].CtrlInit.PerstGpio|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].CtrlInit.C2cId|1
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].CtrlInit.WrapperId|1
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].CtrlInit.PhyId|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].CtrlInit.GenSpeed|5
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].CtrlInit.LaneCount|8
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[4].CtrlInit.PerstGpio|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].CtrlInit.C2cId|2
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].CtrlInit.WrapperId|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].CtrlInit.PhyId|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].CtrlInit.GenSpeed|5
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].CtrlInit.LaneCount|8
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Controller[5].CtrlInit.PerstGpio|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.NumOfPhys|5
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Phy[0].C2cId|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Phy[0].WrapperId|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Phy[0].SerdesMode|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Phy[1].C2cId|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Phy[1].WrapperId|1
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Phy[1].SerdesMode|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Phy[2].C2cId|1
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Phy[2].WrapperId|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Phy[2].SerdesMode|1
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Phy[3].C2cId|1
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Phy[3].WrapperId|1
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Phy[3].SerdesMode|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Phy[4].C2cId|2
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Phy[4].WrapperId|0
  gSophgoSG2044PlatformPkgTokenSpaceGuid.PcdPcieHostBridgeTable.Phy[4].SerdesMode|0
