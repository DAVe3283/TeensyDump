// -----------------------------------------------------------------------------
// SerialAddress.ino
//
// A program to take a serial input of the address, and put it out on hardware
// pins.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Includes
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------

// Address output pins
const uint8_t address_bits(19);
const uint8_t pin_Aminus1(14);
const uint8_t pin_A0(10);
const uint8_t pin_A1(9);
const uint8_t pin_A2(8);
const uint8_t pin_A3(7);
const uint8_t pin_A4(6);
const uint8_t pin_A5(5);
const uint8_t pin_A6(4);
const uint8_t pin_A7(3);
const uint8_t pin_A8(23);
const uint8_t pin_A9(22);
const uint8_t pin_A10(21);
const uint8_t pin_A11(20);
const uint8_t pin_A12(19);
const uint8_t pin_A13(18);
const uint8_t pin_A14(17);
const uint8_t pin_A15(16);
const uint8_t pin_A16(15);
const uint8_t pin_A17(2);
const uint8_t address_pins[address_bits] = { pin_Aminus1, pin_A0, pin_A1, pin_A2, pin_A3, pin_A4, pin_A5, pin_A6, pin_A7, pin_A8, pin_A9, pin_A10, pin_A11, pin_A12, pin_A13, pin_A14, pin_A15, pin_A16, pin_A17 };


// -----------------------------------------------------------------------------
// Declarations
// -----------------------------------------------------------------------------
uint32_t WriteAddress();
void AddressAddNibble(const char& amount);

// -----------------------------------------------------------------------------
// Globals
// -----------------------------------------------------------------------------
uint32_t address(0);

// Serial IO
usb_serial_class  usb = usb_serial_class();
HardwareSerial    ser = HardwareSerial();


// -----------------------------------------------------------------------------
// Function Definitions
// -----------------------------------------------------------------------------

// Initial setup routine
void setup()
{
  // Configure address pins as output
  for (int_fast8_t i(0); i < address_bits; ++i)
  {
    pinMode(address_pins[i], OUTPUT);
    digitalWrite(address_pins[i], LOW);
  }

  // Configure serial
  usb.begin(9600);
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
      AddressAddNibble(incomingByte - '0');
      usb.print(incomingByte);
      break;
    case('A'):
    case('B'):
    case('C'):
    case('D'):
    case('E'):
    case('F'):
      AddressAddNibble(incomingByte - 'A' + 10);
      usb.print(incomingByte);
      break;
    case('a'):
    case('b'):
    case('c'):
    case('d'):
    case('e'):
    case('f'):
      AddressAddNibble(incomingByte - 'a' + 10);
      usb.print(incomingByte);
      break;

    // Backspace / DEL
    case('\b'):
    case('\x7F'):
      usb.print('\x7F');
      address >>= 4; // remove last nibble
      break;

    // Enter
    case('\r'):
    {
      // Activate address
      const uint32_t setAddress(WriteAddress());
      usb.println();
      usb.printf("Setting pins to address 0x%05X (0b", setAddress);
      // usb.print("Setting pins to address 0x");
      // usb.print(setAddress, HEX);
      // usb.print(" (0b");
      usb.print(setAddress, BIN);
      usb.print(", ");
      usb.print(setAddress, DEC);
      usb.println(").");
      break;
    }

    default:
      // Unknown input
      usb.println(incomingByte);
      usb.print("Invalid input! (0x");
      usb.print(incomingByte, HEX);
      usb.println(") Address reset to 0, please try entering it again.");
      address = 0;
      break;
    }
  }

  // Hardware serial
  if (ser.available() > 0)
  {
    const char incomingByte(static_cast<char>(ser.read()));

    switch (incomingByte)
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
      AddressAddNibble(incomingByte - '0');
      break;
    case('A'):
    case('B'):
    case('C'):
    case('D'):
    case('E'):
    case('F'):
      AddressAddNibble(incomingByte - 'A' + 10);
      break;
    case('a'):
    case('b'):
    case('c'):
    case('d'):
    case('e'):
    case('f'):
      AddressAddNibble(incomingByte - 'a' + 10);
      break;

    // Activate
    case('\r'):
      WriteAddress();
      ser.print('\n');
      break;

    default:
      break;
    }
  }
}

uint32_t WriteAddress()
{
  // Write address
  for (int_fast8_t i(0); i < address_bits; ++i)
  {
    digitalWrite(
      address_pins[i],
      (address & (1 << i)) ? HIGH : LOW);
  }

  // Reset current address, return what we wrote
  const uint32_t setAddress(address & ((1 << address_bits) - 1));
  address = 0;
  return setAddress;
}

void AddressAddNibble(const char& amount)
{
  // Got a new nibble (0-F hex)!
  address <<= 4;
  address += (amount & 0xF);
}
