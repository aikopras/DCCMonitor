Directory doc/ contains the documentation. See the file doc/README for more
information.

Directory src/ contains the programs to be flashed on the microcontroller.

Directory src/dccmon contains the monitor for the DCC protocol and
src/rsmon contains the monitor for the RS-bus protocol. To build the
programs, run make in one of those directories. Directory src/common
contains code used by both programs.

To compile the program, create images for flashing and a disassembly listing,
type
	make
or
	make all

Other targets are:
make init-chip	Set the so-called fuse bits in the µC correctly with avrdude*.
		Should be done once for a new chip.
make prog	Flash the program with avrdude* and start the program on the μC.

		* You need to have an .avrduderc file set with correct defaults
		  for your programmer

make hex	Make an Intel HEX format image for flashing
make bin	Make a binary image for flashing
make srec	Make an srec image for flashing
make text	Make all of the formats above for flashing

make elf	Compiles the complete program into dccmon.elf or rsmon.elf.
		The file dccmon.map / rsmon.map contains a linker map.
		
make lst	Make a disassembly listing named dccmon.elf.lst / rsmon.elf.lst.

make clean	The usual meaning

And for every source file *.c there is the target:

make *.lst	Make a disassembly listing of the specified object file
		(Example: make dcc_receiver.lst)

clean and *.lst are also available in the firmware/common directory.

Other available targets are less interesting.

Customising the firmware
========================

By defining INCLUDE_TESTS in src/Makefile.common, you can include the test
routines in the firmware. The keys are used to start and stop tests. The keys on
the board are numbered 0 to 2, with key 0 being connected to PC0 of the µC, up
to key 2 connected to PC2. The LEDs are numbered 0 to 2, LED 0 being connected
to PC3 and situated next to key 0 on the PCB, up to LED 2 connected to PC5 and
situated next to key 2.

Key 0 starts a "Communication protocol test"
Key 1 starts a "Communication forward test" (only run this on a daisy-chained
board, not the one connected to the PC)
Key 2 toggles between doing a single test run or testing continuously (tests
don't consume all the bandwidth of the serial connection). When testing
continuously, a specific test can be turned off by it's respective key.

LEDs 0 and 1 indicate a running test started by key 0 or 1 respectively.

Tests are intended as a verification tool for developers, not for end-users.

By defining DCCMON_FILTER in src/dccmon/Makefile, you can include code to filter
out DCC packets that are not Accessory Decoder packets. This is useful for a DCC
bus that is only connected to, f.e., turnouts, and you are not interested in DCC
commands to trains and such, since none are connected.

You toggle between filtering and not filtering with key 2. LED 2 will be on when
filtering is active. Key 2 will lose it's above meaning when INCLUDE_TESTS is
defined as well; instead, tests are always run continuously until explicitly
turned off.

Additionally, a Management Protocol frame is sent to the PC indicating whether
the filter was turned on or off.
