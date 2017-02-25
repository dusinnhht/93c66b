//
// this is a simple bit-banging class for a MicroChip 93C66B 4k-bit EEPROM
//
// the "B" version of the part is 256 words of 16 bits, and is coded below.
//
// see datasheet:
//
// http://ww1.microchip.com/downloads/en/DeviceDoc/21795b.pdf
//
// not coded here is the sequential read mode nor support for the "A" part (512 words of 8 bits).
//
// this code has been compiled on Arduino 1.6.11 (Windows), and run on a Pro Mini (16MHz, 5V).
//

//
// 93c66b -> arduino pin mapping.   see part pinout on page 1 of datasheet.
//
#define CS  6
#define CLK 7
#define DI  8
#define DO  9

class ee_93c66b {
public:

  ee_93c66b()
  {
    pinMode(CS, OUTPUT);  digitalWrite(CS, LOW);
    pinMode(CLK, OUTPUT); digitalWrite(CLK, LOW);
    pinMode(DI, OUTPUT);  digitalWrite(DI, LOW);
    pinMode(DO, INPUT);
  }

  //
  // read from address
  //
  uint16_t
  read(uint16_t addr)
  {
    command _(*this, 0x06, addr);
 
    return word(16);
  }

  //
  // write to address
  //
  void
  write(uint16_t addr, uint16_t valu)
  {
    command _(*this, 0x05, addr);

    word(16, valu);
    wait();
  }

  //
  // write to all addresses
  //
  void
  wral(uint16_t valu)
  {
    command _(*this, 0x04, 0x0040);

    word(16, valu);
    wait();
  }

  //
  // enable write
  //
  void
  ewen()
  {
    command _(*this, 0x04, 0x00c0);
  }

  //
  // disable write
  //
  void
  ewds()
  {
    command _(*this, 0x04, 0x0000);
  }

  //
  // erase address
  //
  void
  erase(uint16_t addr)
  {
    command _(*this, 0x07, addr);

    wait();
  }

  //
  // erase all
  //
  void
  eral()
  {
    command _(*this, 0x04, 0x0080);

    wait();
  }

protected:

  void
  select()
  {
    digitalWrite(CS, HIGH);
  }

  void
  deselect()
  {
    digitalWrite(CS, LOW);
    digitalWrite(DI, LOW);
  }

  void
  wait()
  {
    deselect();
    select();
    while(digitalRead(DO) == 0)
      ;
  }

  class command {
  public:
    ~command()
    {
      device.deselect();
    }

    command(ee_93c66b &ee, uint8_t which, uint16_t addr)
      : device(ee)
    {
      device.select();
      device.word(11, ((uint16_t) which << 8) | (0x00ff & addr));
    }

  private:
    ee_93c66b &device;
  };

  //
  // write an n-bit word; max n is 16
  //
  void
  word(int n, uint16_t w)
  {
    w <<= (16 - n);
    for(; n > 0; --n, w <<= 1) {
      digitalWrite(DI, (w & 0x8000) ? HIGH : LOW);
      digitalWrite(CLK, HIGH);
      digitalWrite(CLK, LOW);
    }
  }

  //
  // read n-bits
  //
  uint16_t
  word(int n)
  {
    uint16_t w = 0;
 
    while(n-- > 0) {
      digitalWrite(CLK, HIGH);
      w <<= 1;
      w |= digitalRead(DO);
      digitalWrite(CLK, LOW);
    }
    return w;
  }

private:
};

///
/// test harness follows
///
/// note that this test will overwrite all device content
///

static void
fill(ee_93c66b &ee)
{
  for(uint16_t a = 0; a < 256; ++a)
    ee.write(a, a*a);
}

static void
hex4(uint16_t v)
{
  char  buf[5], *p = buf + 5;

  *--p = '\0';
  for(int n = 0; n < 4; ++n, v >>= 4)
    *--p = "0123456789abcdef"[v & 0x0f];

  Serial.print(p);
}

static void
dump(ee_93c66b &ee, const char *label)
{
  Serial.println("");
  Serial.println(label);
  Serial.println("");

  for(uint16_t a = 0; a < 256; ++a) {

    switch(a & 0x0f) {
    case  0:
      hex4(a);
      Serial.print(":");
      /*FALLTHROUGH*/
    case  4:
    case  8:
    case  12:
      Serial.print(" ");
      /*FALLTHROUGH*/
    default:
      Serial.print(" ");
      break;
    }
    hex4(ee.read(a));

    if((a & 0x0f) == 0x0f)
      Serial.println("");
  }
}

void
setup()
{
  Serial.begin(115200);

  Serial.println("test begin");

  //
  // this is your last chance:  the following test will overwrite all device content
  //
  ee_93c66b ee;

  ee.ewen();

  fill(ee);
  dump(ee, "squares");

  ee.erase(0x0080);
  dump(ee, "addr 0x0080 should be 0xffff");

  ee.ewds();

  ee.erase(0x0090);
  dump(ee, "addr 0x0090 should not be 0xffff");

  ee.ewen();

  ee.wral(0x1248);
  dump(ee, "constant");

  ee.eral();
  dump(ee, "empty");

  ee.ewds();

  Serial.println("");
  Serial.println("test complete");
}

void
loop()
{
  for(;;);
}

