## The VIC20-Simulation
it's a valid question to ask if a computer is more than the sum of its parts..  
#### skope:
the VIC-20 files provide the parts found in a Commodore VIC-20 as separate linux programs.  
all parts are loaded by monitor and called from their registered addresses or through the harrware clock tick.  
also there are dynamic traps for load and save to get software into the simulation.  
provided are:  
- via1 (timers, joystick)
- via2 (timers, keyboard, joystick)
- vic (graphics)
- the load trap
- the save trap
- roms (kernal, basic, charracters)
#### usage
./vic20.mon  
