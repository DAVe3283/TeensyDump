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

// Time how long the dump takes, ignoring print statements
void DumpTime(const uint32_t& startAddress, const uint32_t& stopAddress);

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

// Prepare chip for R/W operations
void flash_Ready();

// Put chip into standby
void flash_Standby();

// Deep power down for maximum power savings
void flash_DeepPowerDown();

// Write data to the chip (assumes address is already set)
void flash_Write(const uint8_t& data);

// Read data from the chip
uint8_t flash_Read(const uint32_t& addr); // Assumes chip is ready
uint8_t flash_Read(); // Assumes address is already set and chip is ready

// Put the chip into read array mode
void flash_ReadArrayMode();

// Read the manufacturer (addr=0) or device (addr=2) ID from the chip
uint8_t flash_IntelligentID(const uint32_t& addr);

// Status register functions
uint8_t flash_ReadStatusRegister();
void flash_ClearStatusRegister();


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
    case('.'):
      VerifyIntel28F400();
      flash_ReadArrayMode();
      break;

    case('-'):
      flash_ReadArrayMode();
      break;

    case('+'):
      DecodeStatusRegister();
      flash_ReadArrayMode();
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

    // Time whole dump, ignoring print times
    case('T'):
    case('t'):
      DumpTime(0x0,0x7FFFF);
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
      PrintData(0x21);
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
  for (uint32_t address(startAddress); address <= stopAddress; ++address)
  {
    usb.printf("%02X", flash_Read(address));
  }
  usb.println();
  usb.println();
  usb.println("Dump complete.");
}

void DumpTime(const uint32_t& startAddress, const uint32_t& stopAddress)
{
  usb.println();
  usb.printf("Timing dump of data from 0x%05X to 0x%05X.", startAddress, stopAddress);
  usb.println();
  const uint32_t start(millis());
  for (uint32_t address(startAddress); address <= stopAddress; ++address)
  {
    flash_Read(address);
  }
  const uint32_t totalMillis(millis() - start);
  usb.print("Dump took ");
  usb.print(totalMillis, DEC);
  usb.println(" ms.");
}

void PrintData(const uint32_t& addr)
{
  const uint8_t data(flash_Read(addr));
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
  ser.flush();
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
  // ConfigureDQMode(INPUT_PULLUP);
  // delayMicroseconds(10); // Per https://www.pjrc.com/teensy/td_digital.html
  ConfigureDQMode(INPUT);
  uint8_t data(0);
  for (int_fast8_t i(0); i < num_DQs; ++i)
  {
    data |= digitalRead(DQ_pins[i]) << i;
  }
  return data;
}

void flash_Ready()
{
  digitalWrite(pin_RP_, HIGH);
  digitalWrite(pin_OE_, HIGH);
  digitalWrite(pin_WE_, HIGH);
  digitalWrite(pin_CE_, LOW);
  // Address is a don't care
  // DQ pins go High-Z
  delayMicroseconds(1); // Only takes 0.3 for chip to stabilize, but this is the best we can do
}

void flash_Standby()
{
  digitalWrite(pin_RP_, HIGH);
  digitalWrite(pin_CE_, HIGH);
  digitalWrite(pin_OE_, HIGH); // OE# is a don't care
  digitalWrite(pin_WE_, HIGH); // WE# is a don't care
  // Address is a don't care
  // DQ pins go High-Z
}

void flash_DeepPowerDown()
{
  digitalWrite(pin_RP_, LOW);
  digitalWrite(pin_CE_, HIGH); // CE# is a don't care
  digitalWrite(pin_OE_, HIGH); // OE# is a don't care
  digitalWrite(pin_WE_, HIGH); // WE# is a don't care
  // Address is a don't care
  // DQ pins go High-Z
}

void flash_Write(const uint8_t& data)
{
  // Assume address is set and stable
  SetDQs(data);
  digitalWrite(pin_WE_, LOW); // Begin a write
  delayMicroseconds(1); // Only need 0.02, but this is the best we can do
  digitalWrite(pin_WE_, HIGH); // End write, latch command
  delayMicroseconds(1); // Only need 0.01, but this is the best we can do
}

uint8_t flash_Read(const uint32_t& addr)
{
  // CE# must be high while setting the address (apparently)
  digitalWrite(pin_CE_, HIGH);
  SetAddress(addr);

  // Read the data
  return flash_Read();
}

uint8_t flash_Read()
{
  // Assume address is set and stable, chip is disabled
  digitalWrite(pin_CE_, LOW); // Enable chip, latch addresses
  digitalWrite(pin_OE_, LOW); // Chip output on
  delayMicroseconds(1); // Only need 80 ns from CE# going low / 40 ns from OE# going low, but this is the best we can do
  uint8_t data(GetDQs());
  digitalWrite(pin_OE_, HIGH); // Chip output off
  digitalWrite(pin_CE_, HIGH); // Disable chip
  return data;
}

void flash_ReadArrayMode()
{
  flash_Ready();
  flash_Write(cmd_ReadArray);
  flash_Standby();
}

uint8_t flash_IntelligentID(const uint32_t& addr)
{
  // Prepare to use the chip
  flash_Ready();

  // First Cycle (write cmd_IntelligentID)
  // Address is don't care
  flash_Write(cmd_IntelligentID);

  // Second Cycle (read the ID)
  SetAddress(addr);
  const uint8_t id(flash_Read());

  // Put chip back into standby
  flash_Standby();

  return id;
}

uint8_t flash_ReadStatusRegister()
{
  flash_Ready();

  // First Cycle (write cmd_ReadStatusRegister)
  // Address is don't care
  flash_Write(cmd_ReadStatusRegister);

  // Second Cycle (read the register)
  // Address is don't care
  const uint8_t status(flash_Read());

  // Put chip back into standby
  flash_Standby();

  return status;
}

void flash_ClearStatusRegister()
{
  flash_Ready();
  flash_Write(cmd_ClearStatusRegister);
  flash_Standby();
}
