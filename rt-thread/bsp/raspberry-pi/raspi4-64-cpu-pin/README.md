# RT-Thread SMP多核处理器绑定样例

该文件夹包含样例代码，RT-Thread配置和RT-Thread 5.3.0版本中64位树莓派4B板级支持包（BSP）的一部分（`drivers`文件夹）。RT-Thread版本更新时`drivers`文件夹可能更新，请确认上游`rt-thread/bsp/raspberry-pi/raspi4-64`文件夹。

样例按照宏定义的线程数创建线程，头三个线程会依次与处理器核心0、1、2绑定，其余线程自由执行。每秒线程均会唤醒并打印执行的处理器核心号。树莓派4B为4核处理器，预设定义为4线程，其中线程1到3会绑定核心0到2。

## 1. 编译说明

编译前需安装64位ARM none-elf工具链[1]，并在`rtconfig.py`中更改`EXEC_PATH`指向工具链位置，例如：

```
EXEC_PATH = r'/opt/arm-gnu-toolchain-15.3.rel2-x86_64-aarch64-none-elf/bin'
```

也可定义环境变量`RTT_EXEC_PATH`，不更改`rtconfig.py`。

将整个文件夹复制到`rt-thread/bsp/raspberry-pi`中即可准备编译。

在命令行下进入到目录中，运行以下命令：

```
scons
```

来编译这个板级支持包。如果编译正确无误，会产生`rtthread.elf`，`rtthread.bin`文件。

如需全新编译，清理编译输出：

```
scons -c
```

## 2. 环境搭建
### 2.1 准备好串口线

目前版本是使用raspi4的GPIO 14, GPIO 15来作路口输出，连线情况如下图所示[2]：

![raspiconsole](./misc/raspberrypi-console.png)

串口参数：115200 8N1，硬件和软件流控为关。

### 2.2 SD卡运行

BSP不支持树莓派直接加载运行，需要使用u-boot加载。

**1.准备SD卡**

不同样例SD卡准备只需要做一次。

参考树莓派Imager工具[3]直接格式化和准备SD卡。

查看`lsblk`输出，确定SD卡路径，如`/dev/sdb`。

卸载SD卡上已挂载的文件系统，例如`sudo umount /dev/sdb1`。

通过fdisk修改分区表，创建一个256MB的`BOOT`文件系统，其余空间创建第二个`ROOTFS`文件系统：

```
$ sudo fdisk /dev/sdb
[sudo] password for <user>: 

Welcome to fdisk (util-linux 2.39.3).
Changes will remain in memory only, until you decide to write them.
Be careful before using the write command.

This disk is currently in use - repartitioning is probably a bad idea.
It's recommended to umount all file systems, and swapoff all swap
partitions on this disk.


Command (m for help): o

Created a new DOS (MBR) disklabel with disk identifier 0x0c610a8c.
The device contains 'gpt' signature and it will be removed by a write command. See fdisk(8) man page and --wipe option for more details.

Command (m for help): n
Partition type
   p   primary (0 primary, 0 extended, 4 free)
   e   extended (container for logical partitions)
Select (default p): p
Partition number (1-4, default 1): 1
First sector (2048-62521343, default 2048): 
Last sector, +/-sectors or +/-size{K,M,G,T,P} (2048-62521343, default 62521343): +256M

Created a new partition 1 of type 'Linux' and of size 256 MiB.
Partition #1 contains a vfat signature.

Do you want to remove the signature? [Y]es/[N]o: Y

The signature will be removed by a write command.

Command (m for help): t
Selected partition 1
Hex code or alias (type L to list all): c
Changed type of partition 'Linux' to 'W95 FAT32 (LBA)'.

Command (m for help): n
Partition type
   p   primary (1 primary, 0 extended, 3 free)
   e   extended (container for logical partitions)
Select (default p): p
Partition number (2-4, default 2): 2
First sector (526336-62521343, default 526336): 
Last sector, +/-sectors or +/-size{K,M,G,T,P} (526336-62521343, default 62521343): 

Created a new partition 2 of type 'Linux' and of size 29.6 GiB.

Command (m for help): w
The partition table has been altered.
Failed to remove partition 2 from system: Device or resource busy
Failed to update system information about partition 1: Device or resource busy
Failed to add partition 2 to system: Device or resource busy

The kernel still uses the old partitions. The new table will be used at the next reboot. 
Syncing disks.
```

格式化分区：

```
sudo mkfs.vfat -F 32 -n "BOOT" /dev/sdb1
sudo mkfs.ext4 -L "ROOTFS" /dev/sdb2
```

接下来拔插SD卡，让系统自动挂载文件系统。

**2.复制u-boot和树莓派文件**

此bsp的`misc`下可以找到 [u-boot64.bin](./misc/u-boot64.bin)和[config.txt](./misc/config.txt)两个文件，将它们复制到`BOOT`文件系统中（例如`/media/<user>/BOOT`）。

下载树莓派4B的固件文件[4]到`BOOT`文件系统中，需要的文件：`bcm2711-rpi-4-b.dtb`，`fixup4.dat`和`start4.elf`。

**3.配置u-boot自动加载**

将`misc`下的`boot.cmd`文件转换为`boot.scr`脚本文件放入`BOOT`文件系统中：

```
mkimage -A arm64 -O linux -T script -C none -d ./misc/boot.cmd /media/<usr>/BOOT/boot.scr
```

接下来第4步首次启动u-boot时输入回车键打断，执行下面的命令：

```
U-Boot> setenv bootcmd "fatload mmc 0:1 0x3000000 boot.scr; source 0x3000000"
U-Boot> saveenv
Saving Environment to FAT... OK
```

这样u-boot就能够每次启动都自动加载`rtthread.bin`。

运行reset命令重置。

## 3. 运行

每次编译后，将`rtthread.bin`复制到SD卡`BOOT`中。将SD卡插入树莓派背面的卡槽，上电。

运行输出样例：

```
 \ | /
- RT -     Thread Operating System
 / | \     5.3.0 build Oct 24 2026 16:33:39
 2006 - 2024 Copyright by RT-Thread team
lwIP-2.1.2 initialized!
<0x1b>[32m[I/phy.bcm54213] PHY link down at boot, defer to link event / retry work<0x1b>[0m
<0x1b>[31m[E/fb.bcm2708] Firmware reported no displays<0x1b>[0m
<0x1b>[32m[I/pci.ofw] pcie@7d500000: No "bus-range" found, using [0x0, 0xff]<0x1b>[0m
<0x1b>[32m[I/rtdm.pci] Bus Memory region(0):<0x1b>[0m
<0x1b>[32m[I/rtdm.pci]   cpu:      [0x0000000600000000, 0x000000063fffffff]<0x1b>[0m
<0x1b>[32m[I/rtdm.pci]   physical: [0x00000000c0000000, 0x00000000ffffffff]<0x1b>[0m
<0x1b>[33m[W/eth.bcmgenet] Link down<0x1b>[0m
<0x1b>[32m[I/sal.skt] Socket Abstraction Layer initialize success.<0x1b>[0m
<0x1b>[32m[I/utest] utest is initialize success.<0x1b>[0m
<0x1b>[32m[I/utest] total utest testcase num: (0)<0x1b>[0m
<0x1b>[32m[I/rtdm.mnt] File system initialization done<0x1b>[0m
<0x1b>[sh//>/cpu.aa64] Call cpu 1 on success<0x1b>[0m
<0x1b>[32m[I/cpu.aa64] Call cpu 2 on success<0x1b>[0m
<0x1b>[32m[I/cpu.aa64] Call cpu 3 on success<0x1b>[0m
<0x1b>[32m[I/SDIO] SD card capacity 31260672 KB.<0x1b>[0m
<0x1b>[32m[I/SDIO] SD card switch to High Speed / SDR25 mode<0x1b>[0m
found part[0], begin: 1048576, size: 256.0MB
found part[1], begin: 269484032, size: 29.575GB
Demo for SMP CPU binding
Created thread 0
Binded thread 0 to CPU 0
Created thread 1
Binded thread 1 to CPU 1
Created thread 2
Binded thread 2 to CPU 2
Created thread 3
Thread 2 is running on core 1, count: 0
Thread 3 is running on core 2, count: 0
Thread 4 is running on core 3, count: 0
Thread 1 is running on core 0, count: 0
Thread 2 is running on core 1, count: 1
Thread 3 is running on core 2, count: 1
Thread 4 is running on core 0, count: 1
Thread 1 is running on core 0, count: 1
```

## 4. 参考链接

[1] https://gitlab.arm.com/tooling/gnu-toolchains-for-arm

[2] https://github.com/RT-Thread/rt-thread/tree/master/bsp/raspberry-pi/raspi4-64#readme

[3] https://www.raspberrypi.com/software/

[4] https://github.com/raspberrypi/firmware/tree/master/boot
