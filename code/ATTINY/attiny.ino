#include <EEPROM.h>
#include <SoftwareSerial.h>

#define tl_1 1
#define tl_2 2
#define RX 3
#define TX 4
#define ESP 0
#define potreba_pro_odeslani 32
int adresa = 0;
bool zapis_mod = true;
int h = 0;
int h2 = 0;
int i = 0;
bool tlstav_1 = false;
bool tlstav_2 = false;
char data[3];
SoftwareSerial sSerial(RX,TX);

void setup() {
  pinMode(tl_1, INPUT);
  pinMode(tl_2, INPUT);
  pinMode(ESP, OUTPUT);
  digitalWrite(ESP, LOW);
  sSerial.begin(9600);
  zjisteni_adresy();
}
void zjisteni_adresy() { //efektivně zjistí adresu na kterou má zapisovat
  sSerial.print("\nstart");
  h = EEPROM.read(511);
  zapis_mod = ((~h) & 128);
  sSerial.print(h, BIN);
  sSerial.print(zapis_mod, BIN);
  sSerial.print("\n");
  i = 8;
  adresa = 0;
  while(i > 0){
      if(zapis_mod == (128 & EEPROM.read(adresa | (1 << i)))) {
        adresa = adresa | (1 << i);
      }
      sSerial.print(adresa, BIN);
      sSerial.print(i, DEC);
    i--;
  }
  adresa = adresa + 2;
  if(zapis_mod != (128 & EEPROM.read(0))) {
    adresa = 0;
  }
  sSerial.print("\n");
  sSerial.print(adresa, DEC);
  sSerial.print(EEPROM.read(adresa), BIN);
  
}

void loop() {
  if ((digitalRead(tl_1) == LOW) != tlstav_1) {
    tlstav_1 = !tlstav_1;
    if (tlstav_1 == true) {//volá se jednou při zmáčknutí tlačítka
      h = (EEPROM.read(adresa) & 127);
      h = h + 1;
      if(h < potreba_pro_odeslani){ //podmínka zda se má počet zmáčknutí odeslat
      EEPROM.update(adresa, (h & 127) | (zapis_mod ^ 128));//uložení, ještě se neodesílá
      } else {
        odeslani(h - (potreba_pro_odeslani-1), 0);//odeslání
      }
    }
  }
  if ((digitalRead(tl_2) == LOW) != tlstav_2) {
    tlstav_2 = !tlstav_2;
    if (tlstav_2 == true) {//volá se jednou při zmáčknutí tlačítka
      h2 = (EEPROM.read(adresa | 1) & 127);
      h2 = h2 + 1;
      if(h2 < potreba_pro_odeslani){ //podmínka zda se má počet zmáčknutí odeslat
      EEPROM.update(adresa | 1, (h2 & 127) | (zapis_mod ^ 128));//uložení, ještě se neodesílá
      } else {
        odeslani(0, h2 - (potreba_pro_odeslani-1));//odeslání
      }
      
    }
  }


}

void odeslani(byte plus1, byte plus2) {
  h = (EEPROM.read(adresa) & 127) + plus1;
  h2 = (EEPROM.read(adresa | 1) & 127) + plus2;
  //odeslani
  digitalWrite(ESP, HIGH);
  while (!(data[0]=="R" && data[1]=="T" && data[2]=="S")){
    if(sSerial.available()){
      data[2] = data[1];
      data[1] = data[0];
      data[0] = sSerial.read();
    }
  }
odeslani_dat:
  sSerial.print('H');
  sSerial.print(h);
  sSerial.print('N');
  sSerial.print(h2);
  sSerial.print('*');
  sSerial.print(h + h2);
  sSerial.print('#');
  sSerial.print((h + h2)%11);
  sSerial.println();
  while (true){
    if(sSerial.available()){
      data[1] = data[0];
      data[0] = sSerial.read();
    }
    if(data[0]=="O" && data[1]=="K"){
      break;
    }
    if(data[0]=="R" && data[1]=="N"){
      goto odeslani_dat;
    }
  }
  
  EEPROM.update(adresa,(h & 127) | zapis_mod);
  EEPROM.update(adresa | 1,(h2 & 127) | zapis_mod);
  zjisteni_adresy();
}
