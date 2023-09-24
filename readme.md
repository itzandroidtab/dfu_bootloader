# DFU bootloader

DFU bootloader for the lpc1756. Supports writing binary files using dfu-util 0.11. Has a override input that prevents the bootloader from starting the application.

The bootloader should be easy to change to a different target. (As long as it has the usb driver, flash driver and pin_in working)

### Compiling
The bootloader uses [klib](https://github.com/itzandroidtab/klib). This repo can be cloned in the klib project folder. See [build.yml](./.github/workflows/build.yml) for more info on compiling this project.

### Example usage dfu-util
```bash
dfu-util -D ./klib.bin
```

### Size optimization
Size can be optimized further by removing the CRP section in the linkerscript (decreasing the size to ≈3800 bytes. This makes it fit in 1 sector).

