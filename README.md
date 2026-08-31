This an emulator for a MAI basic four 2000 (code name Eagle)

History
=
I have worked with that machine in the 80s and got a working one in 2011 out of the US. It is an 110 Volt model so i have to run it with a transformer from 230 to 100 V. 
As i had a lot of documentation (scanned and made available at bitsavers/pdf/mai) i decided to start writing an emulator for it. Not a very good idea because i has no experience with 68000 assembler at all.

I started with the basics, got the excellent mushshi 68k emulator, implemented memory in supervisor mode and
made the tape drive working to be able to load diagnoistics from tape.
That was a challenge as there was no documentation for the tape controller available. The rescue was a utility that came with the diags, mcsfs, that one had a lot of information in that made it possible to write an emulation
for it, of cause, it was not perfect but it was able load the diagnostics from tape.

As i had already done some basic support for the 2 serials on the CMB, i tried ti implement the harddisk controller wd0. I tried it used in diags fixing test for test but i struggled fixing the required status register changes. Mainly due to my lack of 68000 assembler knowledge.

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

He did an amazing work on adding all the missing peaces like the mmu, the fourway controller, implemented wd and fixed a lot of badly errors in my code, only to mention parts of his work, see NOTES.md for details.

The emulator is now at a stage where it can not only boot the diagnostics from tape but also a full system from harddisk including entering multi user mode and having multiple terminals connected via telnet.

How to run it
=
The design is fully based on debugging. It is aligned to the internal debugger that is included in the 2000 boot roms for stepping and breakpoints.
? will show the available commands
```

Help for the 68010 emulator debugger
 ====================================
 an         aX - change register A0 to A7
 break      BrkNum address [count] - set breakpoint 0 to 3
 watch      WatchNum [address] [len] - log writes to an address
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
 exec       [load address] - load exec
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
 help       show this help
 quit       terminate emulator
args can be hex values, decimal values if started with # or register values
if started with -. + at end makes value a pointer.
A0+: pointer to addess 0xA0, -A0+: pointer to contents of A0
-A0: contents of A0
You can break into the simulator debugger with control x or by by sending
SIGINT to eagleemu.
```
When not debugging, you only need
go	to start
^x	to break
quit	to exit the emulator

The more useful command for non developers is the dev command. It provides device based command as setting the image file for the harddisk or the directory for tape files. There are also commands for redirecting ports
to the local console or tcp to connect via telnet to.
```
dev ? lists valid devices
dev device ? lists valid command for a device
```
samples
=
```
set directory for tape files
dev cs ?

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

dev cs dir cs/diag

 cs help commands
 ================
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
Redirecting devices
=
By default, the port A of the main board are console, the other ports are listening for telnet connections starting from port 4000. You can change that with the dev scc command.
The 2 supported fourway controllers are always on tcp starting with port 4002.
There is some translation of basic four escape sequences to vt100 sequences. This is settable via dev sock command, try dev sock ?
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

