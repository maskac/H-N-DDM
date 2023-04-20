/*H || !H @A[0], A[1]*/

/*
Attiny pouze loguje stisky tlačítka a přidává k nim timestamp
- čas od posledního odeslání v millis

data se ukládají ve formátu [cyklus][parita][H||!H][timestamp o 13 bitech]

pro náladu platí: Happy => 1, Not happy => 0

Data odesílá buď jednou za 24 hodin, nebo před zaplněním všech 256 bytů
- nejlépe po n-minutové pauze, kdy je attiny v klidu a žádné tlačítko nebylo stisknuto
*/

#include <EEPROM.h>
#include <SoftwareSerial.h>

// Piny
#define switchHappy 1
#define switchNotHappy 2
#define RX 3
#define TX 4
#define ESP 0

// Interní hodnoty
#define attinyMemory 512
#define sendAfterTime 24 * 3600 * 1000 // 24 hodin
#define communicationErrorsAllowed 15
#define sendDelay 1000

// Signály pro komunikaci
#define ready 'R'
#define okay 'O'

int lastWrittenAddress;
int lastSendTime;

int dataBreakpoint;            // Pojistka proti současnému zápisu do paměti a odesílání dat
#define dataBreakpointConst 20 // Velikost předstihu paměti pro pojistku

SoftwareSerial softSerial(RX, TX);

void setup()
{
  // nastavení pinů
  pinMode(switchHappy, INPUT);
  pinMode(switchNotHappy, INPUT);
  pinMode(ESP, OUTPUT);

  digitalWrite(ESP, LOW); // Vypnutí ESP
  softSerial.begin(9600);
  lastWrittenAddress = FindLastBlock(); // Zjištění adresy k zápisu
  SendData();
}

void loop()
{
  HandleInput(switchHappy, 1);
  HandleInput(switchNotHappy, 0);

  // Odesílání dat před naplněním paměti - zabránění overflow při současném zápisu a odesílání
  if (lastWrittenAddress == dataBreakpoint)
    PrepareTransmission();

  // Odesílání dat po 24 hodinách
  if (millis() - lastSendTime > sendAfterTime)
    PrepareTransmission();
}

//
int FindLastBlock()
{
  int currentlySearchingOnMin = 0;
  int currentlySearchingOnMax = attinyMemory / 2 - 1;
  int memoryMiddle;
  int currentValue;
  bool foundLastByte;

  int searchBit = EEPROM.read(0) & (1 << 7);

  while (!foundLastByte)
  {
    memoryMiddle = (currentlySearchingOnMin + currentlySearchingOnMax) / 2;

    currentValue = EEPROM.read(memoryMiddle * 2);

    if ((currentValue & (1 << 7)) == searchBit)
    {
      currentlySearchingOnMin = memoryMiddle;
    }
    else
    {
      currentlySearchingOnMax = memoryMiddle;
    }

    if (currentlySearchingOnMin == currentlySearchingOnMax)
      foundLastByte = true;
  }
  return currentlySearchingOnMax;
}

void HandleInput(int switchPin, int mood)
{
  if (digitalRead(switchPin) == LOW)
  {
    WriteData(mood);
  }
}

void WriteData(int mood)
{
  int lastWrittenData = EEPROM.read(lastWrittenAddress); // Načtení posledních dat kvůli znaménku cyklu
  int thisLogTimestamp = millis() - lastSendTime;
  int parity = 0;

  //[cyklus][parita][H||!H][timestamp o 13 bitech]
  int dataLog = thisLogTimestamp | (mood << 13) | (lastWrittenData & (1 << 15));
  parity = CheckParity(dataLog);
  dataLog |= (parity << 14);

  int nextAddress = (lastWrittenAddress + 2) % attinyMemory;

  if (nextAddress == 0)
    dataLog ^= (1 << 15);

  EEPROM.write(nextAddress, dataLog >> 8);
  EEPROM.write(nextAddress + 1, dataLog);

  lastWrittenAddress = nextAddress;
}

int CheckParity(int log)
{
  log ^= log >> 8;
  log ^= log >> 4;
  log ^= log >> 2;
  log ^= log >> 1;
  return (~log) & 1;
}

void SendData()
{
  for (int address = 0; address < attinyMemory; address += 2)
  {
    int data = (EEPROM.read(address) << 8) | (EEPROM.read(address + 1) << 0);
    softSerial.print(data);
    softSerial.println();
  }
}

void PrepareTransmission()
{
  /*
  Signály pro komunikaci s ESP:
  R => ready
  O => data přijata v pořádku
  N => problém na lince, poslat data znovu
  */

  bool dataSent = false;
  int communicationErrors = 0;

  while (!dataSent) // Odesílání dat, dokud ESP nepotvrdí přijetí
  {
    digitalWrite(ESP, HIGH); // Zapnutí ESP
    delay(100);
    if (softSerial.available()) // Je dostupná komunikace s ESP
    {
      if (softSerial.read() == ready) // ESP je připraveno přijímat
      {
        SendData();

        if (softSerial.read() == okay) // Ověření úspěšného odeslání a přijetí
        {
          dataSent = true;
          dataBreakpoint = (lastWrittenAddress + dataBreakpointConst) % attinyMemory;
        }
        digitalWrite(ESP, LOW); // Vypnutí ESP
        delay(sendDelay);
      }
    }
    else
      communicationErrors++;

    if (communicationErrors >= communicationErrorsAllowed)
      delay(10 * 60 * 1000);
  }

  lastSendTime = millis();
}
