This an emulator for a MAI basic four 2000 (code name Eagle)

History
=
I have worked with that machine in the 80s and got a working one in 2011 out of the US. It is an 110 Volt model so i have to run it with a transformer from 230 to 100 V. 
As i had a lot of documentation (scanned and made available at bitsavers/pdf/mai) i decided to start writing an emulator for it. Not a very good idea because i has no experience with 68000 assembler at all.

I started with the basics, got the excellent [Musashi 68k](https://github.com/kstenerud/Musashi) emulator, implemented memory in supervisor mode and
made the tape drive working to be able to load diagnostics from tape.
That was a challenge as there was no documentation for the tape controller available. The rescue was a utility that came with the diags, mcsfs, that one had a lot of information in that made it possible to write an emulation for it, of cause, it was not perfect but it was able load the diagnostics from tape.

As i had already done some basic support for the 2 serials on the CMB, i tried to implement the harddisk controller wd0. I tried it used in diags fixing test for test but i struggled fixing the required status register changes. Mainly due to my lack of 68000 assembler knowledge.

I also started implementing the floppy disk controller but never finished it.

So the project was in the state:
- boots diags from tape
- nvram was there
- wd/fd non working
- no mmu as i never got out of supervisor mode
- diag passed all tape tests except parity
- wd passed some tests
- fd passed some tests
- a lot of bugs

I also compiled gcc and wrote some rudimentary runtime to allow console i/o that could be uploaded via the 2'nd console port and the internal debugger to check the wd status registers but i never succeeded with the status registers of wd.  

And that was in the Year 2011. Some years later i put that unfinished project on github.

And than came Enrique
=
And than, 15 years later, came Enrique. He send me an eMail "I am a collector of vintage OS. I like to see them running in emulation." with some screenshots of a booted MAI 2000. The screenshot had the serial number that my one had, i was not sure if that screenshot was from a real or an emulated machine. But later he sated that his work was based on my unfinished emulator.

He did an amazing work on adding all the missing peaces like the mmu, the fourway controller output, implemented wd and fixed a lot of badly errors in my code, only to mention parts of his work, see [NOTES.md](https://github.com/ardiehl/MAI-Basic-Four-System-2000-emulator/blob/main/NOTES.md) for details.

So i started again on working on that project, added ip support, added input/output translation, fourway input, 2nd wd support, finished floppy support but still work to do.

The emulator is now at a stage where it can not only boot the diagnostics from tape but also boot a full system from harddisk including entering multi user mode and having multiple terminals connected via telnet.

How to run it
=
Compile should work on linux systems, simply run make. The only library required is readline so you may have to install the dev version of it. For Fedora/Redhat it is
```
dnf install readline-devel
```

After compiling you could directly start ./eagleemu, type g <enter> to start the cpu. After the self test is completed, you will get an error message as there is no harddisk image attached and the prompt
```
    Boot Device:
```
As we have not attached a harddisk nor a floppy disk we can only boot from tape as the diagnostics tape is part of the source directory. Valid boot devices are
```
    wdN harddisk, N=0 or N=1 for disk 0 or 1
    fdN floppy, N=0 or N=1 for floppy drive 0 or 1
    cs tape
```
Enter cs to boot the diagnostics from tape.

## Booting the operating system from disk

For booting BOSS/IX (a licensed version of Charles River Data Systems (CDRS) UNOS from about 1984) you need a disk image, you can download it from basicfour.de 
[bossix_micropolis_2011.dsk.bz2](http://www.basicfour.de/download/mai2000/bossix_micropolis_2011.dsk.bz2) is an image that boots into single user mode and allows multi user after pressing ^D
http://www.basicfour.de/download/mai2000/bossix_micropolis_2011.dsk.multi.bz2 boots directly into multi user mode
Create a directory "wd" in the directory of eagleemu and place the extracted images there.
There is a bash (cmd for windows) script included that allows a simplified start, try ./boot -?.
If you have placed bossix_micropolis_2011.dsk in ./wd you can start the operating system with

    ./boot -0 -1 -g
This will start the emulator with one xterm on the console (-0) and one xterm on the first port of the fourway controller (-1). The -g parameter will start the cpu and boot the system. Use ^x in the emulator window to stop and "quit" to exit the emulator.
## Documentation
I have scanned all the documentation i have and made it available on [bitsavers](http://bitsavers.org/pdf/mai/).
To get started, use:
[MAI 2000 User Guide](http://bitsavers.org/pdf/mai/M6201A_MAI2000_UserGuide_Aug1987.pdf)

[BOSS/IX command line reference (Technical Reference Manual)](http://bitsavers.org/pdf/mai/M6225A_MAI2000_TechnicalReferenceManaual_Aug1985.pdf)

[Diagnostics and Error Log Manual](https://bitsavers.org/pdf/mai/M6204C_BOSS_IX_DiagnosticsErrorlogManual_1989.pdf)

[Business Basic86 Refence Manual](https://bitsavers.org/pdf/mai/M6262A_Business_Basic86_RefenceManual_Apr87.pdf)

## How it works
The design is fully based on debugging. It is aligned to the internal debugger that is included in the 2000 boot roms for stepping and breakpoints.
The boot script simply builds commands that are executed in the emulator. An unlimited number of commands can be specified when starting eagleemu, e.g.:
```
./eagleemu "dev wd image wd/bossix_micropolis_2011.dsk" "dev fd image fd/unos_boot.img" "dev nv fd" g
```
? will show the available commands
```Help for the 68010 emulator debugger
 ====================================
 an         aX - change register A0 to A7
 break      BrkNum address [count] - set breakpoint 0 to 3
 watch      WatchNum [address] [len] - log writes to an address
 cwatch     WatchNum [address] [len] [rw,r or w] [1 = virtual]- log cpu access to an address
 history    [count] - show recently executed instructions
 msave      [file] - dump all RAM to a file
 traptrace  [0|1] - log TRAP instructions executed in user mode
 bus        {0|1} disable/enable break on bus error
 clr        clear all breakpoints
 db         address [count] - change/display count byte(s)
 dc         address [count] - change/display count char(s)
 device     device (fw|wd|scc|cmb|nw) device_command
 dl         address [count] - change/display count long(s)
 dn         dX - change register D0 to D7
 dis        address [num instructions] - disassemble
 dump       fromAdr len    - display memory dump
 dup        {0|1} disable/enable showing of duplicate messages
 dw         [count] - change/display count word(s)
 exec       command - start a new process
 execa      command - start a new process
 go         [address] - run, optional from address
 image      save|load [filename] save/load current state to/from file
 int        generate interrupt n
 load       filename [mem offset] - load s-record file
 nmi        generate NMI and continue execution
 over       step over next instruction
 mbreak     set break on messages with break flag on/off
 msg        set message level, {source|all} {-|+|{+|-}warn | {+|-}err | {+|-}info}
 pc         change pc
 regs       show registers
 reset      reset cpu
 rm         Run until (enabled) message from emu
 step       step one or more instructions
 type       fromAdr toAddr - display memory dump
 translate  translate virtual to physical - address
 colors     color to list color 0 to disable, color err|notimp|warn|info|fatal|func colorName
 vector     show vector table
 help       show this help
 quit       terminate emulator
args can be hex values, decimal values if started with # or register values
if started with -. + at end makes value a pointer.
A0+: pointer to addess 0xA0, -A0+: pointer to contents of A0
-A0: contents of A0
By default addresses will be translated in user mode, override in any mode by appending ! or @ for do not/do translation.
You can break into the simulator debugger with control x or by by sending
SIGINT to eagleemu.
```
When not debugging, you only need
go	to start
^x	to break
quit	to exit the emulator

The more useful command for non developers is the dev command. It provides device based command as setting the image file for the harddisk or the directory for tape files. There are also commands for redirecting ports to the local console or tcp to connect via telnet to.
```
dev ? lists valid devices
dev device ? lists valid command for a device
```
samples (set directory for tape files and attach harddisk image)
=
```
<dbg>dev cs ?

 cs help commands
 
 directory  set directory for tape files
 filemask   set filemask for tape files, e.g. %sF%05d
 iopb       show iopb, up to the number of given in last param
 registers  show cs registers
 setiopb    set iopb address
 setiopbw   set iopb word address
 size       show/change tape size (MB), use #xxx as 2nd param
 status     change status register to last param
 istatus    change status in current IOPB to last param
 istat1     change status1 in current IOPB to last param
 help       show this help

<dbg>dev cs dir diag
'diag' does not exist
<dbg>dev cs dir cs/diag

<dbg>dev wd help

 wd help commands
 ================
 image      image <file> [unit] - attach a raw 512 byte per block disk image
 registers  show wd registers
 ?          show this help
 help       show this help

<dbg>dev wd im wd/bossix_micropolis_2011.dsk
wd0: attached 'wd/bossix_micropolis_2011.dsk', 139264 blocks (68.0 MB)
```
You can specify commands on the command line, for example if you want
to start and run with the harddisk image "wd/bossix_micropolis_2011.dsk" and
the tape directory "cs/diag" you can start the emulator with
```
./eagleemu "dev wd img wd/bossix_micropolis_2011.dsk" "dev cs dir cs/diag" g
```
Redirecting terminals
=

## Output translation

By default, the port A of the main board is the console, the other ports are listening for telnet connections starting from port 4000. You can change the starting portnumber with the command line parameter -p.
Port numbers are:
```
    0: scc0 - console port, 1 st serial port on cmb
    1: scc1 - second serial port on cmb (mainboard)
    2: fw0:0 - first port on first fourway serial controller
    ...
    6:fw1:0 - first port on second fourway serial controller
```
The emulator will translate basic four evdt escape sequences send to a terminal into ANSI/VT sequences by default for port 0 (console) and the first 4 serial ports. This can be changed by the
```
    dev sock outtrans
```
command. Example, enable output translation for the first port of the second 4-way controller:
```
    dev sock outt 6 1
```
The current settings can by inspected by "dev sock status":
```
    dbg>dev sock sta
```

```
    portNum   fd revents  Status              Telnet init  Trans out in   port
    ==========================================================================
          0   -1 00000000 STAT_CLOSED                   1          1  1   4000
          1   -1 00000000 STAT_CLOSED                   1          1  1   4001
          2   -1 00000000 STAT_CLOSED                   1          1  1   4002
          3   -1 00000000 STAT_CLOSED                   1          1  1   4003
          4   -1 00000000 STAT_CLOSED                   1          1  1   4004
          5   -1 00000000 STAT_CLOSED                   1          1  1   4005
          6   -1 00000000 STAT_CLOSED                   0          0  0   4006
          7   -1 00000000 STAT_CLOSED                   0          0  0   4007
          8   -1 00000000 STAT_CLOSED                   0          0  0   4008
          9   -1 00000000 STAT_CLOSED                   0          0  0   4009
```
Basic programs often use input masks displayed in dimmed characters while the input is shown non dimmed. The mnemnotic 'CF' (Clear Foreground Characters) is used to clear the input data. This is supported but requires a VT220 compatible telnet client, the only one i found working is xterm.
## Input translation
By default, input translation is enabled for ports 0 to 5.
ANSI/VT keycodes will be translated into keycodes expected by ved (the os editor) and the basic EDIT command. The F12 key can be used to switch backspace to work with the command line (the default) or ved/EDIT. F1..F4 are mapped to MB I .. MB IV.
Input translation can be set for each port via the

    dev sock intrans

command.
## Port redirection
The Serial ports of the 2 4-way controllers are always on TCP starting with port 4002, port 0 (console) is by default on the terminal. The 2 ports on the cmb (device scc) can be changed via the

    dev scc socketio

command where the first parameter is the scc number (0 or 1) and the second parameter is 0 for disable and 1 for enable
e.g. redirect the console to tcp:

    dev scc sock 0 1

By default, the port A of the main board is the console, the other ports are listening for telnet 

You can change that with the

    dev scc

command.

## Compile for Windows 32/64
Compile for Windows is currently only supported on linux using mingw32/mingw64.
Use 
    make PLATFORM=win64
or
    make PLATFORM=win32
to build for Windows.


# The System Boots

```
eaglesim 0.3.45 (ad Sun 30-Aug-2026)
Control x will break into the command line
<dbg> dev wd im wd/bossix_micropolis_2011.dsk
wd0: attached 'wd/bossix_micropolis_2011.dsk', 139264 blocks (68.0 MB)
<dbg>g

            MAI Basic Four Inc.
                 MAI 2000

System Self Test B4.3: SSN 2000-97894
cmb                                   pass
memory   [size=1536 kbytes]           pass
fd                                    pass
fw       [modules= 0,1]               pass
wd       [modules= 0]                 pass
cs                                    pass

Booting from wd00
Loading /sys/bossix
.........
Loading /etc/conf
........
Executing /sys/bossix,/etc/conf

   **************************************************************************
   *                                  NOTICE                                *
   *                                                                        *
   *  THIS SOFTWARE INCLUDES PROPRIETARY INFORMATION AND IS PROTECTED BY    *
   *  COPYRIGHT AND TRADE SECRET LAWS.  UNAUTHORIZED COPYING OR DISCLOSURE  *
   *  OF THIS SOFTWARE MAY RESULT IN CIVIL AND CRIMINAL PENALTIES.          *
   *  POSSESSION AND USE OF THIS SOFTWARE IS ALSO SUBJECT TO A LICENSE      *
   *  AGREEMENT WITH MAI BASIC FOUR, INC.  UNDER THE LICENSE AGREEMENT,     *
   *  USE OF THIS SOFTWARE IS RESTRICTED TO A SINGLE DESIGNATED CENTRAL     *
   *  PROCESSING UNIT.  ALL OTHER USE, PUBLICATION, REPRODUCTION AND        *
   *  TRANSMISSION OF THIS SOFTWARE IS PROHIBITED.  FAILURE TO COMPLY WITH  *
   *  THE LAW AND THE LICENSE AGREEMENT MAY RESULT IN TERMINATION OF THE    *
   *  LICENSE AGREEMENT, CANCELLATION OF THE RIGHT TO USE THIS SOFTWARE,    *
   *  AND CIVIL AND CRIMINAL PENALTIES.                                     *
   *                                                                        *
   *  COPYRIGHT 1984, REV. 1991 MAI BASIC FOUR, INC.  ALL RIGHTS RESERVED.  *
   *  MAI AND BASIC FOUR ARE REGISTERED TRADEMARKS OF MAI BASIC FOUR, INC.  *
   *                                                                        *
   *  UNOS (C) 1981 BY CHARLES RIVER DATA SYSTEMS, INC.                     *
   **************************************************************************

System name: MAI 2000                         System serial number: 2000-97894
Operating System: EOS5B22, BOSS/IX release 7.5B*22 (Jan  4 1991 18:22)
009:17 am, 11/16/11.  Update clock: 23210000 083026
<multi-user mode>
Multi-user startup in progress.
  Cleaning temporary directory '/tmp'...
  Starting system 'update' and 'errlog' processes...
  Starting network remote service manager...
Multi-user startup completed.
Wed Nov 16 2011 09:19:18

                          M A I   B A S I C   F O U R

         BBBBBBB    OOOOOO    SSSSSS    SSSSSS         //  IIIIII  XX    XX
         BB   BB  OO    OO  SS    SS  SS    SS       //     II    XX    XX
        BB   BB  OO    OO  SS        SS            //      II     XX  XX 
       BBBBBB   OO    OO   SSSSSS    SSSSSS      //       II      XXXX  
      BB   BB  OO    OO        SS        SS    //        II     XX  XX 
     BB   BB  OO    OO  SS    SS  SS    SS   //         II    XX    XX
   BBBBBBB    OOOOOO    SSSSSS    SSSSSS   //        IIIIII  XX    XX
                                                                      
                                                                     
                                                                      








        MAI 2000 (Terminal tty1) -- Press 'CTRL'+'C' or 'ESCAPE'...
```




