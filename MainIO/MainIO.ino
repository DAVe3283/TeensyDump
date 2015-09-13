// -----------------------------------------------------------------------------
// MainIO.ino
//
// Program to do the main I/O on the Intel 28F400 FLASH chip.
// Talks over serial to set addresse bits (see SerialAddress.ino).
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

// Switch mode on DQ pins (are used for both INPUT and OUTPUT)
void ConfigureDQMode(uint8_t mode);

// Tell the other Teensy to set the address pins
void SetAddress(const uint32_t& addr, const bool& printAddress = false);

// Set data on DQ pins
void SetDQs(const uint8_t& data);

// Get data on DQ pins
uint8_t GetDQs();

// Flash Routines
void flash_Standby();
void flash_DeepPowerDown();
void flash_Write(const uint8_t& data);
uint8_t flash_Read();
uint8_t flash_IntelligentID(const uint32_t& addr);


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
  usb.begin(115200);
  ser.begin(115200);
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
      {
        usb.print("Reading Manufacturer ID: ");
        uint8_t id = flash_IntelligentID(0);
        usb.println(id, HEX);
        usb.print("Reading Device ID: ");
        id = flash_IntelligentID(2);
        usb.println(id, HEX);
      }
      break;

    // Hex input
    case('0'):
      SetAddress(0x0, true);
      break;

    case('1'):
      SetAddress(0x1, true);
      break;

    case('2'):
      SetAddress(0xF, true);
      break;

    case('3'):
      SetAddress(0x10, true);
      break;

    case('4'):
      SetAddress(0x21, true);
      break;

    case('5'):
      SetAddress(0x40, true);
      break;

    case('6'):
      SetAddress(0x81, true);
      break;

    case('7'):
      SetAddress(0xF0, true);
      break;

    case('8'):
      SetAddress(0xF1, true);
      break;

    case('9'):
      SetAddress(0xFE, true);
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

void flash_Standby()
{
  digitalWrite(pin_RP_, HIGH);
  digitalWrite(pin_CE_, HIGH);
  // OE# is a don't care
  // WE# is a don't care
  // Address is a don't care
  // DQ pins go High-Z
}

void flash_DeepPowerDown()
{
  digitalWrite(pin_RP_, LOW);
  // CE# is a don't care
  // OE# is a don't care
  // WE# is a don't care
  // Address is a don't care
  // DQ pins go High-Z
}

void flash_Write(const uint8_t& data)
{
  // Assume address is set and stable
  SetDQs(data);
  digitalWrite(pin_CE_, LOW); // Enable the chip
  digitalWrite(pin_WE_, LOW); // Begin a write
  delayMicroseconds(1); // Only need 0.02, but this is the best we can do
  digitalWrite(pin_WE_, HIGH); // End write, latch command
  delayMicroseconds(1); // Only need 0.01, but this is the best we can do
  digitalWrite(pin_CE_, HIGH); // Disable the chip
}

uint8_t flash_Read()
{
  // Assume address is set and stable
  digitalWrite(pin_CE_, LOW); // Enable the chip
  digitalWrite(pin_OE_, LOW); // Chip output on
  delayMicroseconds(1); // Only need 0.04, but this is the best we can do
  uint8_t data(GetDQs());
  digitalWrite(pin_OE_, HIGH); // Chip output off
  digitalWrite(pin_CE_, HIGH); // Disable the chip
  return data;
}

uint8_t flash_IntelligentID(const uint32_t& addr)
{
  // Read the Intelligent ID
  // Addr only cares about A0, which in x8 mode, might be bit 1.
  // So try addr=1, but it might need 2 to work

  // Initial chip setup
  digitalWrite(pin_RP_, HIGH); // Not in reset
  digitalWrite(pin_CE_, HIGH); // Disable the chip
  digitalWrite(pin_OE_, HIGH); // Chip output off
  digitalWrite(pin_WE_, HIGH); // Not writing
  delayMicroseconds(1); // Only takes 0.3 for chip to stabilize, but this is the best we can do

  // First Cycle (write cmd_IntelligentID)
  // Address is don't care
  flash_Write(cmd_IntelligentID);

  // Second Cycle (read the ID)
  //ConfigureDQMode(INPUT);
  SetAddress(addr);
  return flash_Read();
}
