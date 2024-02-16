# USB DFU bootloader

USB DFU bootloader for the lpc1756. Supports writing binary files using dfu-util 0.11. Has a override input that prevents the bootloader from starting the application. 

:warning: Currently the bootloader only checks if the flash is blank and does not check if a valid user application is present. This can cause the target to get stuck until a power cycle, as the bootloader does not initialize any watchdogs.

The bootloader should be easy to change to a different target. (As long as it has the USB driver, flash driver and pin_in working)

### Compiling
The bootloader uses [klib](https://github.com/itzandroidtab/klib). This repo can be cloned in the klib project folder. See [build.yml](./.github/workflows/build.yml) for more info on compiling this project.

### Example usage dfu-util
```bash
dfu-util -D ./klib.bin
```
Currently dfuSe is not supported. This means the content of `klib.bin` will be written at location `app_vector_address`. This also prevents dfu-util from overwriting the bootloader.

### Size optimization
Size can be optimized further by removing the CRP section in the linkerscript (decreasing the size to ≈3800 bytes. This makes it fit in the first sector).

### User application
The bootloader expects the user application to start at the address specified in `app_vector_address`. This means the user application needs to modify the linkerscript so the vector table is in this location.

The bootloader expects the normal vector table that includes the stack, reset handler and the other interrupts. The bootloader will 
setup the stack and move the vector table to `app_vector_address`. This allows the user application to trigger interrupts without moving the vector table to ram. After this setup the bootloader will call the reset handler. 

:warning: If the stack entry or the reset handler is incorrect the bootloader will crash as there is no check for a valid application at the moment.
