# badXmonitor
A handy monitor for the bad6502

### usage:
./monitor  
or #!./monitor in an executable script file  

### known commands
e : examine  
d : deposit  
f : flash (deposit and mark page as rom)  
p : protect mark a page as readonly  
l : load a file at address  
r : load a file at address and those pages mark readonly  
h : load a hardware plugin and map it  
t : load a trap code plugin and map it  
g : go (reset the cpu and start from reset vector)  

### adresses and data
adressses : typed as 0 to ffff, end with a space  
data: typed as 0 to ff, end with a space  

### use in scripts
Lines may be extended by ending a line with \  
Data may be separated with ',' and prepended with 0x  
Comments are allowed.  

## Plugins
Plugins (shared libraries) can be used to provide  
virtual hardware.  
Each library can reserve an IO area and the monitori 
will redirect access to the plugin.  

## Traps
Trapcode (shared libraries) can be used to inject  
code at certain breakpoints to offload functions  
from the 6502 to the host.  
