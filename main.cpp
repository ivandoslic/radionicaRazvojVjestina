/***************************************************
  Rezultat radionice "Razvijanje vještina kroz rad s
  mikrokontrolerima". Kod je prilagođen procesoru
  ESP32 i određenim komponentama: ILI9341 (TFT) i
  XPT2046 (Touch screen).

  Ovaj kod zahtjeva instalaciju dodatnih libraryja
  koje možete pronaći i u Arduino IDE-u i u
  PlatformIO razvojnom orkuženju.

  Za library TFT_eSPI potrebno je podesiti datoteku
  User_Setup.h. Pravilno podešena datoteka za
  VidiX mikroračunalo nalazi se u ovom repozitoriju.

  Ivan Došlić 16. 12. 2022.
 ****************************************************/

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "SD.h"
#include <FS.h>
#include <XPT2046_Touchscreen.h>

TFT_eSPI tft = TFT_eSPI(); // Stvaranje objekta tft, preko kojega radimo sa zalosnom
XPT2046_Touchscreen ts(4); // Stvaranje objekta ts, preko kojega raidmo sa touch screenom

// Varijable potrebne za mapiranje inputa preko touch screena
int x_piksel, y_piksel;
int min_x = 370;
int min_y = 210;
int max_x = 3900;
int max_y = 3800;

int color = ILI9341_WHITE;  // Trenutno odabrana boja olovke

bool isWriting = false; // Zastavica stanja (ako je false znači da se podaci ne zapisuju na SD memorijsku karticu)

void drawSaveIcon() { // Funkcija za crtanje sličice diskete
  tft.drawLine(10, 5, 10, 45, ILI9341_BLACK);
  tft.drawLine(10, 45, 50, 45, ILI9341_BLACK);
  tft.drawLine(50, 45, 50, 15, ILI9341_BLACK);
  tft.drawLine(50, 15, 45, 5, ILI9341_BLACK);
  tft.drawLine(45, 5, 10, 5, ILI9341_BLACK);
  tft.drawLine(15, 45, 15, 35, ILI9341_BLACK);
  tft.drawLine(15, 35, 25, 35, ILI9341_BLACK);
  tft.drawLine(25, 35, 25, 45, ILI9341_BLACK);
}

void tftInfo(String message, int color) { // Funkcija koja na displayu na 2 sekunde ispiše neku informaciju u odabranoj boji
    tft.setCursor(80, 25);
    tft.setTextColor(color);
    tft.print(message);
    delay(2000);
    tft.setCursor(80, 25);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(message);
}

void writeToImage() { // Funkcija za spremanje slike na memorijsku karticu
  isWriting = true; // Postavljamo zastavicu pisanja, kako bi spriječili daljnji unos korisnika

  if(!SD.begin(22)) { // Započinjemo rad SD sučelja tj. serijsku komunikaciju
    tft.setCursor(10, 15);
    tft.println("SD NIJE DOSTUPAN!");
    return;
  }

  String name = "slika"; // varijabla koja sadrži ime slike, slobodno promjenite u naziv koji želite
  fs::File file = SD.open("/"+name+".pbm", "w");  // Na SD kartici stvaramo datoteku ime.pbm u koju spremamo sliku 

  file.println("P1\n320 240\n");  // postavljamo nužne podatke u zaglavlje datoteke

  file.close(); // Zatvorimo datoteku

  tftInfo("Preparing output...", ILI9341_BLACK);

  tft.setCursor(80, 25);
  tft.setTextColor(ILI9341_BLACK);
  tft.print("Uploading to SD...");

  // Varijable za spremanje vrijednosti piksela koje ćemo upisati u datoteku
  String word = "";
  String res1 = "";
  String res2 = "";
  String res3 = "";

  /*
  Rezultat se dijeli na tri različite varijable zbog ograničenosti memorije u jednoj varijabli
  */

 // 1. dio:
  for(int j = 0; j < 101; j++) {
    word = "";  // Kad ispišemo redak u datoteku, vraćamo ga u praznu riječ
    for(int i = 0; i < 320; i++) {
      uint16_t pix = tft.readPixel(i, j);
      word.concat(pix > 0 ? "0 " : "1 "); // Zapisujemo u piksele u j-tom retku slike
    }
    res1.concat(word);
    res1.concat("\n");  // dodajemo svaki redak slike u rezultat koji zapisujemo u datoteku
  }

  // 2. dio:
  for(int j = 101; j < 202; j++) {
    word = "";
    for(int i = 0; i < 320; i++) {
      uint16_t pix = tft.readPixel(i, j);
      word.concat(pix > 0 ? "0 " : "1 ");
    }
    res2.concat(word);
    res2.concat("\n");
  }

  // 3. dio:
  for(int j = 202; j <= 240; j++) {
    word = "";
    for(int i = 0; i < 320; i++) {
      uint16_t pix = tft.readPixel(i, j);
      word.concat(pix > 0 ? "0 " : "1 ");
    }
    res3.concat(word);
    res3.concat("\n");
  }

  delay(200); // Nije nužno potreban delay


  /*
   Potrebno je obustaviti svu serijsku komunikaciju jer inače
   dolazi do kolizije podataka na sabirnici i SD neće uspjeti
   spremiti sliku. Donji blok koda radi "reset" serijske
   komunikacije.
  */
  Serial.end();
  SPI.end();

  SPI.begin();

  // Opet otvaramo dokument u koji zapisujemo sliku
  fs::File image = SD.open("/"+ime+".pbm", "a", true);

  if(!image) {
    isWriting = false;
    tft.setCursor(125, 15);
    tft.setTextColor(ILI9341_RED);
    tft.print("Ne mogu spremiti!");
    delay(2000);
    tft.setCursor(125, 15);
    tft.setTextColor(ILI9341_BLACK);
    tft.print("Ne mogu spremiti!");
    return;
  } // Ako slika nije uspješno otvorena izbacimo poruku o greški i izađemo iz funkcije

  image.print(res1);  // U sliku spremamo 1. dio slike

  image.print(res2);  // U sliku spremamo 2. dio slike

  image.print(res3);  // U sliku spremamo 3. dio slike

  image.close();

  SD.end(); // Gasimo serijsku komunikaciju sa SD sučeljem

  Serial.begin(9600); // Potrebno je opet pokrenuti Serial sučelje za ispis na računalu

  tft.setCursor(80, 25);
  tft.setTextColor(ILI9341_WHITE);
  tft.print("Uploading to SD...");

  tftInfo("DONE!", ILI9341_BLACK);

  isWriting = false;  // Omogućujemo nastavak rada
}

void checkToolbar(int x, int y) {
  if(x > 230 && x < 270) color = ILI9341_WHITE; // Ako smo pritsnuli bjeli gumb, postavimo boju u bjelu
  else if(x > 275) color = ILI9341_BLACK; // Ako smo pritisnuli crni gumb, postavimo boju u crnu
  else if(x < 60) writeToImage(); // Ako smo pritisnuli ikonu za spremanje, započinjemo spremanje slike na SD memorijsku karticu
}

void setup() {
  Serial.begin(9600);
  tft.begin();  // Započinjemo rad displaya, tj. započinjemo komunikaciju displaya sa procesorom
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(3);
  tft.setCursor(10, 10);
  tft.println((String) "Inicijaliziram touch screen...");
  
  if(!ts.begin()){    // Započinjemo rad touch screena, tj. serijsku komunikaciju [ekvivalent za ovaj blok koda napisan je na dnu dokumenta]
    tft.fillScreen(ILI9341_BLACK);
    tft.println((String) "ERROR! Touch screen se nije uspio inicijalizirati!");
    for(;;); // ako ne uspijemo inicijalizirati touch screen, pokrenemo beskonačnu petlju kako bi spriječili nastavak izvršavanja
  }
  ts.setRotation(1);
  tft.fillScreen(ILI9341_WHITE);

  // Crtamo "gumbe" za odabir boje
  tft.fillCircle(250, 25, 20, ILI9341_BLACK);
  tft.fillCircle(250, 25, 16, ILI9341_WHITE);
  tft.fillCircle(295, 25, 16, ILI9341_BLACK);

  tft.drawLine(0, 50, 320, 50, ILI9341_BLACK);
  drawSaveIcon();
}

void loop() {
  TS_Point p = ts.getPoint(); // Dohvaćamo mjesto dodira zaslona
  if(p.z != 0 && !isWriting) {  // Ako je ekran dotaknut i ne spremamo podatke na SD karticu mapiramo dodir s rezulucijom zaslona
    x_piksel = map(p.x, min_x, max_x, 0, 319);
    y_piksel = map(p.y, min_y, max_y, 0, 239);
    if(y_piksel > 50) tft.fillCircle(x_piksel, y_piksel, 5, color); // Ako smo dodirnuli u području za crtanje stvaramo "točku" na mjestu dodira
    else checkToolbar(x_piksel, y_piksel);  // Ako smo dodirnuli alatnu traku počinjemo provjeru pritiska nekog od gumba
  }
  delay(5);
}

/* ekvivalent za pokretanje touch screena
bool tsSuccess = ts.begin();
if(!sSUccess) {
  tft.fillScreen(ILI9341_BLACK);
  tft.println((String) "ERROR! Touch screen se nije uspio inicijalizirati!");
  for(;;);
}