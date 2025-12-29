

// mem read
uint8_t read(uint16_t address)
{
  // Virtual hardware (reads a char from stdin)
  if (address == 0x01) {
    if (!kbhit())
      return 0;
    return getch();
  }

  return mem[address];
}

// mem write
void write(uint16_t address, uint8_t data)
{
  // Virtual hardware (writes char to stdout)
  if (address == 0x00) {
    printf("%c",(char)data);
    fsync(1);
    return;
  }

  mem[address] = data;
}