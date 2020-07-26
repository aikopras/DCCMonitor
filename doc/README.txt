Files
=====

wiki/				
	Backup of http://dcc.simpleweb.org/ at the time of release.

*.T3001
	Target! 3001 files, the source of all of the following files.

* schematic.png
	Schematic for the boards in bitmap format.

* schematic.ps
	PostScript version of the schematic.

* bom.csv
	Bill Of Materials: all components with their number, values and
	packages.
	The fields of the CSV are tab-separated. CSV files can also be viewed
	as textfiles.

PCB/
	Files for creating the PCB's.

PCB/* copper bottom.ps
	The copper tracks on the bottom of the PCB, in PostScript format.
	Top view! So if you look at the bottom of the finished PCB, it's
	mirrored. 

PCB/* placement print top.ps
	The top of the PCB showing the layout of the components (also called
	silkscreen), in PostScript format.

PCB/Gerber/
	Gerber files for production of the PCB.
