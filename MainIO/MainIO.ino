// -----------------------------------------------------------------------------
// MainIO.ino
//
// Program to do the main I/O on the Intel 28F400BX FLASH chip.
// Talks over serial to set addresse bits (see SerialAddress.ino).
//
// IMPORTANT: Set the Teensy to 96 MHz (overclock) to use 6MHz serial.
// If you don't want to overclock, lower the serial baud to 3MHz or lower.
//
// Be sure to wire the 28F400 into x8 mode (BYTE# pin tied to ground), since
// that is how the program is currently setup.
//
// Note: I don't verify the state of the FLASH, so if you do something stupid
// (e.g. put chip in deep power down, then try to read), something stupid will
// probably happen. So good luck!
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Includes
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------

// Data I/O pins
const uint8_t num_DQs(8);
const uint8_t pin_DQ0(2);
const uint8_t pin_DQ1(3);
const uint8_t pin_DQ2(4);
const uint8_t pin_DQ3(5);
const uint8_t pin_DQ4(14);
const uint8_t pin_DQ5(15);
const uint8_t pin_DQ6(16);
const uint8_t pin_DQ7(17);
//const uint8_t pin_DQ8();
//const uint8_t pin_DQ9();
//const uint8_t pin_DQ10();
//const uint8_t pin_DQ11();
//const uint8_t pin_DQ12();
//const uint8_t pin_DQ13();
//const uint8_t pin_DQ14();
//const uint8_t pin_DQ15();
const uint8_t DQ_pins[num_DQs] = {
  pin_DQ0, pin_DQ1, pin_DQ2, pin_DQ3, pin_DQ4, pin_DQ5, pin_DQ6, pin_DQ7,
  //pin_DQ8, pin_DQ9, pin_DQ10, pin_DQ11, pin_DQ12, pin_DQ13, pin_DQ14, pin_DQ15
};

// Control pins
const uint8_t pin_WP_(6);
const uint8_t pin_CE_(7);
const uint8_t pin_OE_(8);
const uint8_t pin_RP_(19);
const uint8_t pin_WE_(18);

// LED
const uint8_t pin_LED(13);

// Commands
const uint8_t cmd_ReadArray(0xFF);
const uint8_t cmd_ProgramSetup(0x40); // 0x10 is also valid
const uint8_t cmd_EraseSetup(0x20);
const uint8_t cmd_EraseConfirm(0xD0);
const uint8_t cmd_EraseSuspend(0xB0);
const uint8_t cmd_EraseResume(cmd_EraseConfirm);
const uint8_t cmd_ReadStatusRegister(0x70);
const uint8_t cmd_ClearStatusRegister(0x50);
const uint8_t cmd_IntelligentID(0x90);


// -----------------------------------------------------------------------------
// Declarations
// -----------------------------------------------------------------------------

// Verify we have an Intel 28F400BX chip (true is success)
bool VerifyIntel28F400();

// Decode Status Register
void DecodeStatusRegister();
void DecodeStatusRegister(const uint8_t& status);

// Dump an address range
void Dump(const uint32_t& startAddress, const uint32_t& stopAddress);

// Print the data at the specified address
void PrintData(const uint32_t& addr);

// Switch mode on DQ pins (are used for both INPUT and OUTPUT)
void ConfigureDQMode(uint8_t mode);

// Tell the other Teensy to set the address pins
void SetAddress(const uint32_t& addr, const bool& printAddress = false);

// Set data on DQ pins
void SetDQs(const uint8_t& data);

// Get data on DQ pins
uint8_t GetDQs();

// Read the array
uint8_t ReadArray(const uint32_t& addr);

// Write the array
bool WriteArray(const uint32_t& addr, const uint8_t& data);

// Erase a block in the array
bool EraseArrayBlock(const uint32_t& addr);

// Begin R/W operations (enables device, which latches address)
void flash_Begin();

// Put chip into standby
void flash_Standby();

// Deep power down for maximum power savings
void flash_DeepPowerDown();

// Latch an address into the part
void flash_LatchAddress(const uint32_t& addr);

// Write data to the chip
void flash_Write(const uint8_t& data); // Assumes address is latched already

// Read data from the chip
uint8_t flash_Read(); // Assumes address is already set and chip is ready

// Put the chip into read array mode
void flash_ReadArrayMode();

// Read the manufacturer (addr=0) or device (addr=2) ID from the chip
uint8_t flash_IntelligentID(const uint32_t& addr);

// Status register functions
uint8_t flash_ReadStatusRegister();
void flash_ClearStatusRegister();

// Erase entire device
void EraseAll();

// Program entire device (or just a region)
void ProgramAll();

// Add a nibble to a value
void AddNibble(uint32_t& value, const char& nibble);
void AddNibble(uint8_t& value, const char& nibble);

// Get an address, in hex, from USB
bool usb_GetAddress(uint32_t& address);

// Get a byte, in hex, from USB
bool usb_GetData(uint8_t& data);

// -----------------------------------------------------------------------------
// Globals
// -----------------------------------------------------------------------------

// Serial IO
usb_serial_class  usb = usb_serial_class();
HardwareSerial    ser = HardwareSerial();


// -----------------------------------------------------------------------------
// Function Definitions
// -----------------------------------------------------------------------------

// Initial setup routine
void setup()
{
  // Configure control pins
  pinMode(pin_WP_, OUTPUT);
  pinMode(pin_CE_, OUTPUT);
  pinMode(pin_OE_, OUTPUT);
  pinMode(pin_RP_, OUTPUT);
  pinMode(pin_WE_, OUTPUT);

  // Leave boot block write protected
  digitalWrite(pin_WP_, LOW);

  // Put chip in standby
  flash_Standby();

  // Configure serial
  usb.begin(9600); // USB is always full speed, this baud does nothing
  ser.begin(6000000);

  // Enable LED
  pinMode(pin_LED, OUTPUT);
}

// Main program loop
void loop()
{
  // USB serial
  if (usb.available() > 0)
  {
    const char incomingByte(static_cast<char>(usb.read()));

    switch (incomingByte)
    {
    case('`'):
      ProgramAll();
      break;

    case('.'):
      VerifyIntel28F400();
      break;

    case('*'):
      flash_ReadArrayMode();
      break;

    case('+'):
      DecodeStatusRegister();
      break;

    case('-'):
      flash_ClearStatusRegister();
      flash_ReadArrayMode();
      break;

    // Erase all
    case('E'):
    case('e'):
      EraseAll();
      break;

    // Dump all
    case('A'):
    case('a'):
      Dump(0x0, 0x7FFFF);
      break;

    // Dump boot block
    case('B'):
    case('b'):
      Dump(0x0, 0x3FFF);
      break;

    case('Q'):
    case('q'):
      WriteArray(0x4000, 0x55);
      break;

    // Print parameter block
    case('p'):
      Dump(0x4000, 0x6000 - 1);
      break;
    // Write parameter block
    case('['):
      usb.print("Writing parameter block with sequential data...");
      for (uint32_t addr(0x4000); addr < 0x6000; ++addr)
      {
        WriteArray(addr, addr % 0x100);
      }
      usb.println("done.");
      break;
    case('{'):
      usb.print("Writing parameter block with 0x00...");
      for (uint32_t addr(0x4000); addr < 0x6000; ++addr)
      {
        WriteArray(addr, 0x00);
      }
      usb.println("done.");
      break;
    case('P'):
      usb.print("Writing parameter block with 0xA5...");
      for (uint32_t addr(0x4000); addr < 0x6000; ++addr)
      {
        WriteArray(addr, 0xA5);
      }
      usb.println("done.");
      break;
    case('}'):
      usb.print("Writing parameter block with 0xFF...");
      for (uint32_t addr(0x4000); addr < 0x6000; ++addr)
      {
        WriteArray(addr, 0xFF);
      }
      usb.println("done.");
      break;
    // Erase parameter block
    case(']'):
      EraseArrayBlock(0x4000);
      break;

    // Print 96k block
    case('L'):
    case('l'):
      Dump(0x8000, 0x20000 - 1);
      break;
    // Write 96k block
    case(';'):
      for (uint32_t addr(0x8000); addr < 0x20000; ++addr)
      {
        WriteArray(addr, addr % 0x100);
      }
      break;
    // Erase 96k block
    case('\''):
      EraseArrayBlock(0x8000);
      break;

    // Hex input
    case('0'):
      PrintData(0x0);
      break;

    case('1'):
      PrintData(0x1);
      break;

    case('2'):
      PrintData(0x2);
      break;

    case('3'):
      PrintData(0x3);
      break;

    case('4'):
      PrintData(0x4000);
      break;

    case('5'):
      PrintData(0x40);
      break;

    case('6'):
      PrintData(0x81);
      break;

    case('7'):
      PrintData(0xF0);
      break;

    case('8'):
      PrintData(0xF1);
      break;

    case('9'):
      PrintData(0xFE);
      break;

    default:
      // Unknown input
      usb.println(incomingByte);
      usb.print("Invalid input! (0x");
      usb.print(incomingByte, HEX);
      usb.println(")");
      break;
    }
  }
}

bool VerifyIntel28F400()
{
  bool fail(false);

  // Manufacturer ID is at A0=0 (addr=0)
  uint8_t id(flash_IntelligentID(0));
  usb.printf("Got Manufacturer ID = 0x%02X (", id);
  if (id == 0x89)
  {
    usb.print("Intel");
  }
  else
  {
    usb.print("Unknown/Bad");
    fail = true;
  }
  usb.println(")");

  // Device ID is at A0=1 (addr=2 in X8 mode)
  id = flash_IntelligentID(2);
  usb.printf("Got Device ID = 0x%02X (", id);
  if (!fail && id == 0x70)
  {
    usb.print("28F400BX-T");
  }
  else if (!fail && id == 0x71)
  {
    usb.print("28F400BX-B");
  }
  else
  {
    usb.print("Unknown/Bad");
    fail = true;
  }
  usb.println(")");

  if (fail)
  {
    usb.println("Please check chip is an Intel 28F400BX, that it is socketed correctly, and that the wiring is correct.");
  }

  // Did we get a 28F400BX?
  return !fail;
}

void DecodeStatusRegister()
{
  DecodeStatusRegister(flash_ReadStatusRegister());
  flash_ReadArrayMode(); // Back to read array mode
}

void DecodeStatusRegister(const uint8_t& status)
{
  usb.printf("Decoding Status Register: 0x%02X (0b", status);
  usb.print(status, BIN);
  usb.println(")");

  // Bit 7
  usb.print("SR[7] Write State Machine Status: ");
  if (status & (1 << 7))
  {
    usb.println("1 Ready");
  }
  else
  {
    usb.println("0 Busy");
  }

  // Bit 6
  usb.print("SR[6] Erase Suspend Status: ");
  if (status & (1 << 6))
  {
    usb.println("1 Erase Suspended");
  }
  else
  {
    usb.println("0 Erase in Progress/Completed");
  }

  // Bit 5
  usb.print("SR[5] Erase Status: ");
  if (status & (1 << 5))
  {
    usb.println("1 Error in Block Erase");
  }
  else
  {
    usb.println("0 Successful Block Erase");
  }

  // Bit 4
  usb.print("SR[4] Program Status: ");
  if (status & (1 << 4))
  {
    usb.println("1 Error in Byte/Word Program");
  }
  else
  {
    usb.println("0 Successful Byte/Word Program");
  }

  // Bit 3
  usb.print("SR[3] Vpp Status: ");
  if (status & (1 << 3))
  {
    usb.println("1 Vpp Low Detect; Operation Abort");
  }
  else
  {
    usb.println("0 Vpp OK");
  }

  // Bit 2
  usb.printf("SR[2:0] Reserved: %d%d%d",
    status & (1 << 2),
    status & (1 << 1),
    status & (1 << 0));
  usb.println();
}

void Dump(const uint32_t& startAddress, const uint32_t& stopAddress)
{
  usb.println();
  usb.printf("Dumping data from 0x%05X to 0x%05X.", startAddress, stopAddress);
  usb.println();
  usb.println();
  const uint32_t start(millis());
  for (uint32_t address(startAddress); address <= stopAddress; ++address)
  {
    usb.printf("%02X", ReadArray(address));
  }
  const uint32_t totalMillis(millis() - start);
  usb.println();
  usb.println();
  usb.print("Dump complete in ");
  usb.print(totalMillis, DEC);
  usb.println(" ms.");
}

void PrintData(const uint32_t& addr)
{
  const uint8_t data(ReadArray(addr));
  usb.printf("0x%05X: 0x%02X", addr, data);
  usb.println();
}

void ConfigureDQMode(uint8_t mode)
{
  for (int_fast8_t i(0); i < num_DQs; ++i)
  {
    pinMode(DQ_pins[i], mode);
  }
}

void SetAddress(const uint32_t& addr, const bool& printAddress)
{
  // Clear serial buffer
  while (ser.available()) { ser.read(); }

  // Set address
  ser.print(addr, HEX);
  if (printAddress) { usb.printf("Set address to 0x%05X\r\n", addr); }

  // Wait for confirmation
  ser.print('\r');
  ser.flush(); // Send now!
  while (ser.read() != '\n') {};
}

void SetDQs(const uint8_t& data)
{
  ConfigureDQMode(OUTPUT);
  for (int_fast8_t i(0); i < num_DQs; ++i)
  {
    digitalWrite(
      DQ_pins[i],
      (data & (1 << i)) ? HIGH : LOW);
  }
}

uint8_t GetDQs()
{
  ConfigureDQMode(INPUT);
  uint8_t data(0);
  for (int_fast8_t i(0); i < num_DQs; ++i)
  {
    data |= digitalRead(DQ_pins[i]) << i;
  }
  return data;
}

uint8_t ReadArray(const uint32_t& addr)
{
  // Assume we are in array read mode
  flash_LatchAddress(addr);
  return flash_Read();
}

// Write the array
bool WriteArray(const uint32_t& addr, const uint8_t& data)
{
  // Setup Program
  flash_LatchAddress(addr);
  flash_Write(cmd_ProgramSetup);

  // Program
  flash_Begin(); // Address hasn't changed, just latch
  flash_Write(data);

  // Wait for write to complete
  //const uint32_t start(micros());
  bool busy;
  uint8_t status;
  do
  {
    status = flash_ReadStatusRegister();
    busy = (status & (1 << 7)) == 0;
  } while (busy);
  //const uint32_t totalMicros(micros() - start);
  //usb.printf("Wrote 0x%02X to 0x%05X in %ld us", data, addr, totalMicros);
  //usb.println();

  // Exit programming mode
  flash_ReadArrayMode();

  // Errors?
  const uint8_t errors(status & 0x78);
  if (errors != 0)
  {
    usb.println("Error writing array!");
    DecodeStatusRegister(status);
  }
  return errors == 0;
}

bool EraseArrayBlock(const uint32_t& addr)
{
  // Setup Erase
  flash_LatchAddress(addr);
  flash_Write(cmd_EraseSetup);

  // Erase
  flash_Begin(); // Address hasn't changed, just latch
  flash_Write(cmd_EraseConfirm);

  // Wait for erase to complete
  usb.printf("Erasing block containing address 0x%05X...", addr);
  const uint32_t start(millis());
  bool busy;
  uint8_t status;
  do
  {
    status = flash_ReadStatusRegister();
    busy = (status & (1 << 7)) == 0;
  } while (busy);
  const uint32_t totalMillis(millis() - start);
  usb.printf("done in %ld ms.", totalMillis);
  usb.println();

  // Exit erase mode
  flash_ReadArrayMode();

  // Errors?
  const uint8_t errors(status & 0x78);
  if (errors != 0)
  {
    usb.print("Error erasing array! ");
    DecodeStatusRegister(status);
  }
  return errors == 0;
}

void flash_Begin()
{
  // Not in Deep Power Down
  digitalWrite(pin_RP_, HIGH);

  // Put DQs in High-Z
  digitalWrite(pin_OE_, HIGH);
  digitalWrite(pin_WE_, HIGH);

  // Latch address, enable chip for R/W
  digitalWrite(pin_CE_, LOW);
  digitalWrite(pin_LED, HIGH);

  // Let chip stabilize
  delayMicroseconds(1); // Can take up to 300 ns, but we have to wait 1000 ns
}

void flash_Standby()
{
  // Not in Deep Power Down
  digitalWrite(pin_RP_, HIGH);

  // Disable the device
  digitalWrite(pin_CE_, HIGH);
  digitalWrite(pin_LED, LOW);

  // These are don't care, but set them up for later anyway
  //digitalWrite(pin_OE_, HIGH); // OE# is a don't care
  //digitalWrite(pin_WE_, HIGH); // WE# is a don't care

  // Address is a don't care
  // DQ pins go High-Z
}

void flash_DeepPowerDown()
{
  digitalWrite(pin_RP_, LOW);
  digitalWrite(pin_CE_, HIGH); // CE# is a don't care
  digitalWrite(pin_LED, LOW);
  digitalWrite(pin_OE_, HIGH); // OE# is a don't care
  digitalWrite(pin_WE_, HIGH); // WE# is a don't care
  // Address is a don't care
  // DQ pins go High-Z
}

void flash_LatchAddress(const uint32_t& addr)
{
  flash_Standby();
  SetAddress(addr);
  flash_Begin();
}

void flash_Write(const uint8_t& data)
{
  // Assume address is latched already

  // Prepare DQs
  SetDQs(data);

  // Write 
  digitalWrite(pin_WE_, LOW); // Begin a write
  delayMicroseconds(1); // Only need 0.02, but this is the best we can do
  digitalWrite(pin_WE_, HIGH); // End write, latch command
  delayMicroseconds(1); // Only need 10ns, but this is the best we can do

  // Return part to standby
  flash_Standby();
}

uint8_t flash_Read()
{
  // Assume address is latched already

  // Read
  digitalWrite(pin_OE_, LOW); // Chip output on
  delayMicroseconds(1); // Only need 80 ns from CE# going low / 40 ns from OE# going low, but this is the best we can do
  // TODO: smaller delay somehow?
  uint8_t data(GetDQs());
  digitalWrite(pin_OE_, HIGH); // Chip output off

  // Return part to standby
  flash_Standby();

  // Return data
  return data;
}

void flash_ReadArrayMode()
{
  // Address is don't care
  flash_Begin(); // Enable chip, latch the don't care address
  flash_Write(cmd_ReadArray);
  flash_Standby(); // Return to standby
}

uint8_t flash_IntelligentID(const uint32_t& addr)
{
  // First Cycle (write cmd_IntelligentID)
  // Address is don't care
  flash_Begin(); // latch don't care address
  flash_Write(cmd_IntelligentID);

  // Second Cycle (read the ID)
  const uint8_t id(ReadArray(addr));

  // Put the part back into array read mode
  flash_Begin(); // latch don't care address
  flash_Write(cmd_ReadArray);

  return id;
}

uint8_t flash_ReadStatusRegister()
{
  // First Cycle (write cmd_ReadStatusRegister)
  // Address is don't care
  flash_Begin(); // latch don't care address
  flash_Write(cmd_ReadStatusRegister);

  // Second Cycle (read the register)
  // Address is don't care
  flash_Begin(); // latch don't care address
  const uint8_t& status(flash_Read());

  return status;
}

void flash_ClearStatusRegister()
{
  flash_Begin();
  flash_Write(cmd_ClearStatusRegister);
}

void EraseAll()
{
  usb.print("Are you sure you want to erase the entire flash array? (Y,[N]) ");
  while (usb.available() == 0)
  {
    delay(1);
  }
  char consent(usb.read());
  usb.println(consent);
  if ((consent != 'Y') && (consent != 'y'))
  {
    usb.println("Canceled erase.");
    return;
  }

  // Verify we have an Intel 28F400
  bool canHasIntel(VerifyIntel28F400());
  if (!canHasIntel)
  {
    usb.println("I don't know how to erase this device. Canceled erase.");
    return;
  }

  // Determine block locations
  usb.print("Is the device a -T or -B? (T,B) ");
  while (usb.available() == 0)
  {
    delay(1);
  }
  char topBottom(usb.read());
  usb.println(topBottom);
  if ((topBottom == 'T') || (topBottom == 't')) // Top boot block
  {
    usb.println("Erasing MAIN BLOCK 1...");
    if (!EraseArrayBlock(0x00000))
    {
      usb.println("Erase failed!");
      return;
    }
    usb.println("Erasing MAIN BLOCK 2...");
    if (!EraseArrayBlock(0x20000))
    {
      usb.println("Erase failed!");
      return;
    }
    usb.println("Erasing MAIN BLOCK 3...");
    if (!EraseArrayBlock(0x40000))
    {
      usb.println("Erase failed!");
      return;
    }
    usb.println("Erasing MAIN BLOCK 4...");
    if (!EraseArrayBlock(0x60000))
    {
      usb.println("Erase failed!");
      return;
    }
    usb.println("Erasing PARAMETER BLOCK 1...");
    if (!EraseArrayBlock(0x78000))
    {
      usb.println("Erase failed!");
      return;
    }
    usb.println("Erasing PARAMETER BLOCK 2...");
    if (!EraseArrayBlock(0x7A000))
    {
      usb.println("Erase failed!");
      return;
    }
    usb.println("Erasing BOOT BLOCK...");
    digitalWrite(pin_WP_, HIGH); // Un-protect boot block
    if (!EraseArrayBlock(0x7C000))
    {
      usb.println("Erase failed!");
      return;
    }
  }
  else if ((topBottom == 'B') || (topBottom == 'b')) // Bottom boot block
  {
    usb.println("Erasing BOOT BLOCK...");
    digitalWrite(pin_WP_, HIGH); // Un-protect boot block
    if (!EraseArrayBlock(0x00000))
    {
      usb.println("Erase failed!");
      return;
    }
    usb.println("Erasing PARAMETER BLOCK 1...");
    if (!EraseArrayBlock(0x04000))
    {
      usb.println("Erase failed!");
      return;
    }
    usb.println("Erasing PARAMETER BLOCK 2...");
    if (!EraseArrayBlock(0x06000))
    {
      usb.println("Erase failed!");
      return;
    }
    usb.println("Erasing MAIN BLOCK 1...");
    if (!EraseArrayBlock(0x08000))
    {
      usb.println("Erase failed!");
      return;
    }
    usb.println("Erasing MAIN BLOCK 2...");
    if (!EraseArrayBlock(0x20000))
    {
      usb.println("Erase failed!");
      return;
    }
    usb.println("Erasing MAIN BLOCK 3...");
    if (!EraseArrayBlock(0x40000))
    {
      usb.println("Erase failed!");
      return;
    }
    usb.println("Erasing MAIN BLOCK 4...");
    if (!EraseArrayBlock(0x60000))
    {
      usb.println("Erase failed!");
      return;
    }
  }
  else
  {
    usb.println("Could not determine if you said T or B. Canceled erase.");
    return;
  }

  usb.println("Erase complete!");
}

void ProgramAll()
{
  // For the record, this it a kludge, and doesn't seem all that stable.
  // It DOES work though...
  // TODO: not this

  // Get start address
  uint32_t address;
  if (!usb_GetAddress(address))
  {
    usb.println("Programming canceled.");
    return;
  }

  // Status
  usb.printf("Ready to program array, starting at address 0x%05X.", address);
  usb.println();
  usb.println("Paste in hex data with spaces and/or enters between each byte.");

  // Program the data
  uint8_t data;
  while (usb_GetData(data))
  {
    WriteArray(address++, data);
  }

  // TODO: done?
  usb.println();
  usb.println("Programming completed.");
}

void AddNibble(uint32_t& value, const char& nibble)
{
  // Got a new nibble (0-F hex)!
  value <<= 4;
  value += (nibble & 0xF);
}
void AddNibble(uint8_t& value, const char& nibble)
{
  // Got a new nibble (0-F hex)!
  value <<= 4;
  value += (nibble & 0xF);
}

bool usb_GetAddress(uint32_t& address)
{
  address = 0;
  usb.print("Enter address to begin writing (in hex): 0x");
  while (true)
  {
    if (usb.available())
    {
      const char input(usb.read());
      switch (input)
      {
      // Hex input
      case('0'):
      case('1'):
      case('2'):
      case('3'):
      case('4'):
      case('5'):
      case('6'):
      case('7'):
      case('8'):
      case('9'):
        usb.print(input);
        AddNibble(address, input - '0');
        break;
      case('A'):
      case('B'):
      case('C'):
      case('D'):
      case('E'):
      case('F'):
        usb.print(input);
        AddNibble(address, input - 'A' + 10);
        break;
      case('a'):
      case('b'):
      case('c'):
      case('d'):
      case('e'):
      case('f'):
        usb.print(input);
        AddNibble(address, input - 'a' + 10);
        break;

      // Backspace / DEL
      case('\b'):
      case('\x7F'):
        usb.print('\x7F');
        address >>= 4; // remove last nibble
        break;

      // Enter
      case('\r'):
        usb.println();
        return true;

      // Escape
      case('\x1B'):
        usb.println();
        return false;

      default:
        usb.println('\b');
        usb.println("Unknown character entered! Enter hex digits only, or press Esc to cancel.");
        usb.printf("You pressed keycode 0x%02X", input);
        usb.println();
        usb.print(address, HEX);
        break;
      }
    }
    else
    {
      delay(1);
    }
  }
}

bool usb_GetData(uint8_t& data)
{
  data = 0;
  while (true)
  {
    if (usb.available())
    {
      const char input(usb.read());
      switch (input)
      {
      // Hex input
      case('0'):
      case('1'):
      case('2'):
      case('3'):
      case('4'):
      case('5'):
      case('6'):
      case('7'):
      case('8'):
      case('9'):
        usb.print(input);
        AddNibble(data, input - '0');
        break;
      case('A'):
      case('B'):
      case('C'):
      case('D'):
      case('E'):
      case('F'):
        usb.print(input);
        AddNibble(data, input - 'A' + 10);
        break;
      case('a'):
      case('b'):
      case('c'):
      case('d'):
      case('e'):
      case('f'):
        usb.print(input);
        AddNibble(data, input - 'a' + 10);
        break;

      // Backspace / DEL
      case('\b'):
      case('\x7F'):
        usb.print('\x7F');
        data >>= 4; // remove last nibble
        break;

      // Enter
      case('\r'):
        usb.println();
        return true;

      // Space
      case(' '):
        return true;

      // Escape
      case('\x1B'):
        usb.println();
        return false;

      default:
        usb.println('\b');
        usb.println("Unknown character entered! Enter hex digits only, or press Esc to cancel.");
        usb.printf("You pressed keycode 0x%02X", input);
        usb.println();
        usb.print(data, HEX);
        break;
      }
    }
    else
    {
      delay(1);
    }
  }
}
