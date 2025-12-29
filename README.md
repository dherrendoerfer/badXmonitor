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
g : go (reset the cpu and start from reset vector)  

### adresses and data
adressses : typed as 0 to ffff, end with a space  
data: typed as 0 to ff, end with a space  

### use in scripts
Lines may be extended by ending a line with \  
Data may be separated with ',' and prepended with 0x  
Comments are allowed  
