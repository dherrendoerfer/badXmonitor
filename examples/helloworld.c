// our message buffer
static char msg[]="\r\nHello World\r\n"
                  "\r\nPress CNTL-X to exit.\r\n";

void main() 
{
   int i;

   for (i=0;;i++) {
    if (msg[i]==0)
        break;
    *((char*)0xFF00)=msg[i];
   }

   //read input forever
   while(1)
    i=*((char*)0xFF01);
}