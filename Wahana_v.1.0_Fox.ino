
// TIMBANGAN BADAN COIN
// Versi 1.0 Foxy
// Armada Kadot Teknologi - Kalibaru Banyuwangi (c) Jan 2023
// armadakadot@gmail.com

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// -- About OpsLogic
// LOGIC
#define ON 1
#define OFF 0
#define SIM 0
#define OPS 1
#define STDBYMODE 0
#define CSLOTMODE 1
#define LCELLMODE 2
#define TIMBANG_PERMIT 1
#define TIMBANG_NO_PERMIT 0
#define lamaNimbang 15000 // waktu layanan timbangan loop 1sec 10x

// HARDWARES
#define actBtn 2 // ada tombol ke pin 2 buat interrupt
#define signalPin 3 // signal input coin slot as intterupt
#define stdbyPWR 5 // pin 3 arduino as power line (analog)
#define cslotPWR 6 // pin 5 arduino as power line (analog)
#define lcellPWR 10 // pin 6 arduino as power line (analog)
#define multbPWR 8 // pin 6 arduino as power line (analog)
#define counterPin 12 // coin slot COUNTER

unsigned int ongkosPakai = 1000;
volatile bool hentikan = false;

volatile byte whanaMODE = STDBYMODE; // STDBYMODE - CSLOTMODE - LCELLMODE
byte opsMODE = OPS;   // SIM - OPS
unsigned int koin = 0;      // mencatat koin yang baru dimasukkan ke coin slot
unsigned int totalKoin = 0; // koin masuk sudah berapa

// -- About LCD
#define STAY 0
#define SCROLL 1
#define BLINK 2
#define SETDELAY 500
#define HDR 0
#define DTL 1

//----- Scrolling Text Global Vars ------
String HDRStr = "",DTLStr = "";
String HDRStr2Disp = "", DTLStr2Disp = "";
unsigned int HDRStrSUM,DTLStrSUM;
byte HDRMde,DTLMde;
//---------------------------------------

volatile uint8_t coinSign = 0;

//---------- Preset MP3 Sound Lib on SDCard--------------
#define masuk1000 5
#define masuk200 12
#define masuk500s 14
#define masuk500t 13
#define timbanganSiap 7
#define koinSiap 6


//--------------- MODULES ---------------
// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial mySoftwareSerial(4, 7); // RX, TX of MP3 Player
DFRobotDFPlayerMini mp3;

void printDetail(uint8_t type, int value);
void suarakan(uint8_t n,uint32_t lama); 

void ISR_Coin_Signal() {
     Serial.println(" ----------> COIN INTERRUPT");
     
     if( digitalRead(signalPin) == LOW) {
      coinSign = coinSign + 1;
     }

     Serial.println("coinSign: "+String(coinSign));
}


void ISR_actBtn(){
  Serial.println("--INTERRUPT actBtn -- !!!");
  Serial.println("Semula hentikan = " + String(hentikan));
  
  hentikan = true;
  //mp3.stop();
      
  switch (whanaMODE) {
    case STDBYMODE:      
      whanaMODE = CSLOTMODE; 
      break;
    case CSLOTMODE:
      if (totalKoin > 0) whanaMODE = LCELLMODE; 
      break;
  }
  Serial.println("JADI whanaMODE = " + String(whanaMODE));
  Serial.println("Jadi hentikan = " + String(hentikan));
  
}


String RPAD16(String msg, int pmsg) {
  for (int i=0;i < 16 - pmsg;i++) {msg += " ";}
  return msg;
}

void anim_LCD_HeaderFooter () {
  String m1,m2;

  // ---- HEADER
  if (HDRStr != "") {
      lcd.setCursor(0,HDR);
      switch (HDRMde) {
        case STAY:
          lcd.print(HDRStr2Disp);
          break;
        case SCROLL:
          lcd.print(HDRStr2Disp.substring(0,15));
          m1 = HDRStr2Disp.substring(1, HDRStr2Disp.length());
          m2 = HDRStr2Disp.substring(0,1);
          HDRStr2Disp = m1 + m2;
          break;
        case BLINK:
          lcd.print(HDRStr2Disp.substring(0,15));
          delay(SETDELAY*2);
          lcd.print("                ");
          delay(SETDELAY);
          lcd.print(HDRStr2Disp.substring(0,15));
          break; 
      }
  }
  // ---- DETAIL
  if (DTLStr != "") {
     lcd.setCursor(0,DTL);
     switch (DTLMde) {
       case STAY:
         lcd.print(DTLStr2Disp);
         break;
       case SCROLL:
         lcd.print(DTLStr2Disp.substring(0,16));
         m1 = DTLStr2Disp.substring(1, DTLStr2Disp.length());
         m2 = DTLStr2Disp.substring(0,1);
         DTLStr2Disp = m1 + m2;          
         break;
       case BLINK:
         lcd.print(DTLStr2Disp.substring(0,16));
         delay(SETDELAY*2);
         lcd.setCursor(0,DTL);
         lcd.print("                 ");
         delay(SETDELAY);
         lcd.print(DTLStr2Disp.substring(0,16));
         break; 
     }    
  }
}

bool isNewMsg(String msg,byte whichOne) {
  unsigned int sum = 0;
  for(int i=0;i < msg.length();i++){
    sum += msg[i];
  }
  switch (whichOne) {
    case HDR:
       if (HDRStrSUM != sum) {
         HDRStrSUM = sum;
         return true;
       } else return false;
       break;
    case DTL:
       if (DTLStrSUM != sum) {
         DTLStrSUM = sum;
         return true;
       } else return false;
       break;
  }  
}

void readCoinSlot() {
  unsigned long waktuMasuk_RCS = millis();
  bool diisengin = false;
  unsigned int totalKoinSkr = 0;
  koin = 0;
  totalKoin = 0;
  coinSign = 0;
  
    if (opsMODE == SIM) { // Ubah mode operasional di blok definisi diatas
    
    while (totalKoin < ongkosPakai && whanaMODE!=LCELLMODE) { // whanaMODE CSLOTMODE bisa menjadi LCELLMODE karena diinterrupt tombol
      Serial.println("totalKoin = "+String(totalKoin)+" ongkosPakai = "+String(ongkosPakai)+" koin masuk = "+String(koin));
      Serial.println("tota;Koin > ongkpsPakai" + String(totalKoin)+" > " + String(ongkosPakai) + " --> " +String(totalKoin>ongkosPakai));
      delay(random(2000,5000)); // simulasi jeda pengunjung memasukkan koin
      hentikan = false;
      suarakan(23,1000); // klotak
      switch (random(1,4)) { // simulate coin random catcth
        case 1: // uang 200 masuk
          koin = 200;
          totalKoin += koin;
          suarakan(33,500); // kredit diterima
          lcdSet(HDR,"Koin masuk: "+ String(koin),STAY);
          lcdSet(DTL,"Total uang: "+ String(totalKoin),STAY);
          anim_LCD_HeaderFooter();
          suarakan(12,1000); //masuk200
          break;
        case 2: // koin 500 silver masuk
          koin = 500;
          totalKoin += koin;
          suarakan(33,500); // kredit diterima
          lcdSet(HDR,"Koin masuk: "+ String(koin),STAY);
          lcdSet(DTL,"Total uang: "+ String(totalKoin),STAY);
          anim_LCD_HeaderFooter();
          suarakan(masuk500s,1000);
          break;
        case 3: // koin 500 tembaga masuk
          koin = 500;
          totalKoin += koin;
          suarakan(33,500); // kredit diterima
          lcdSet(HDR,"Koin masuk: "+ String(koin),STAY);
          lcdSet(DTL,"Total uang: "+ String(totalKoin),STAY);
          anim_LCD_HeaderFooter();
          suarakan(masuk500t,1000);
          break;
        case 4: // koin 1000 masuk
          koin = 1000;
          totalKoin += koin;
          suarakan(33,500); // kredit diterima
          lcdSet(HDR,"Koin masuk: "+ String(koin),STAY);
          lcdSet(DTL,"Total uang: "+ String(totalKoin),STAY);
          anim_LCD_HeaderFooter();
          suarakan(masuk1000,1000);
          break;
      }
    }
  } else {
  // sensing fisical caoin slot
  koin = 0;
  totalKoin = 0;
  Serial.println("Phissical Coin Slot");
  do {
    
    if (digitalRead(counterPin) == LOW) {
      Serial.println("COUNTER digital: "+String(digitalRead(counterPin))+" >>>> analog: "+String(analogRead(counterPin)));
    }
    
    if(coinSign > 0) {
       hentikan = false;
       suarakan(23,2000); // klotak
       //delay(1000); // let other pulse arrive
       Serial.println("coin detected ! coinSign: " + String(coinSign));
       switch(coinSign) {
         case 1:
            Serial.println("koin 200");
            koin = 200;
            totalKoin += koin;
            suarakan(33,500); // kredit diterima
            lcdSet(HDR,"Koin masuk: "+ String(koin),STAY);
            lcdSet(DTL,"Total uang: "+ String(totalKoin),STAY);
            anim_LCD_HeaderFooter();
            suarakan(12,1000); //masuk200
            break;
         case 2:
            Serial.println("koin 200 200");
            koin = 400;
            totalKoin += koin;
            suarakan(33,500); // kredit diterima
            lcdSet(HDR,"Koin masuk: "+ String(koin),STAY);
            lcdSet(DTL,"Total uang: "+ String(totalKoin),STAY);
            anim_LCD_HeaderFooter();
            break;
         case 3:
            Serial.println("koin 500 tembaga");
            koin = 500;
            totalKoin += koin;
            suarakan(33,500); // kredit diterima
            lcdSet(HDR,"Koin masuk: "+ String(koin),STAY);
            lcdSet(DTL,"Total uang: "+ String(totalKoin),STAY);
            anim_LCD_HeaderFooter();
            suarakan(masuk500t,1000);          
            break;
         case 4:
            Serial.println("koin 200 disusul 500 tembaga");
            koin = 700;
            totalKoin += koin;
            suarakan(33,500); // kredit diterima
            lcdSet(HDR,"Koin masuk: "+ String(koin),STAY);
            lcdSet(DTL,"Total uang: "+ String(totalKoin),STAY);
            anim_LCD_HeaderFooter();
            break;
         case 6:
            Serial.println("koin 500 tembaga 2x");
            koin = 1000;
            totalKoin += koin;
            suarakan(33,500); // kredit diterima
            lcdSet(HDR,"Koin masuk: "+ String(koin),STAY);
            lcdSet(DTL,"Total uang: "+ String(totalKoin),STAY);
            anim_LCD_HeaderFooter();
            suarakan(masuk1000,1000);
            break;
         case 7:
            Serial.println("koin 500 putih");
            koin = 500;
            totalKoin += koin;
            suarakan(33,500); // kredit diterima
            lcdSet(HDR,"Koin masuk: "+ String(koin),STAY);
            lcdSet(DTL,"Total uang: "+ String(totalKoin),STAY);
            anim_LCD_HeaderFooter();
            suarakan(masuk500s,1000);
            break;
         case 8:
            Serial.println("koin 200 lalu 500p");
            koin = 700;
            totalKoin += koin;
            suarakan(33,500); // kredit diterima
            lcdSet(HDR,"Koin masuk: "+ String(koin),STAY);
            lcdSet(DTL,"Total uang: "+ String(totalKoin),STAY);
            anim_LCD_HeaderFooter();
            break;
         case 10:
            Serial.println("koin 500t lalu 500p");
            koin = 1000;
            totalKoin += koin;
            suarakan(33,500); // kredit diterima
            lcdSet(HDR,"Koin masuk: "+ String(koin),STAY);
            lcdSet(DTL,"Total uang: "+ String(totalKoin),STAY);
            anim_LCD_HeaderFooter();
            suarakan(masuk1000,1000);
            break;
         case 14:
            Serial.println("koin 500p 2x");
            koin = 1000;
            totalKoin += koin;
            suarakan(33,500); // kredit diterima
            lcdSet(HDR,"Koin masuk: "+ String(koin),STAY);
            lcdSet(DTL,"Total uang: "+ String(totalKoin),STAY);
            anim_LCD_HeaderFooter();
            suarakan(masuk1000,1000);
            break;
         case 12:
            Serial.println("koin 1000");
            koin = 1000;
            totalKoin += koin;
            suarakan(33,500); // kredit diterima
            lcdSet(HDR,"Koin masuk: "+ String(koin),STAY);
            lcdSet(DTL,"Total uang: "+ String(totalKoin),STAY);
            anim_LCD_HeaderFooter();
            suarakan(masuk1000,1000);
            break;
         case 13:
            Serial.println("koin 200 lalu 1000");
            koin = 1200;
            totalKoin += koin;
            suarakan(33,500); // kredit diterima
            lcdSet(HDR,"Koin masuk: "+ String(koin),STAY);
            lcdSet(DTL,"Total uang: "+ String(totalKoin),STAY);
            anim_LCD_HeaderFooter();
            break;
         case 15:
            Serial.println("koin 500t lalu 1000");
            koin = 1500;
            totalKoin += koin;
            suarakan(33,500); // kredit diterima
            lcdSet(HDR,"Koin masuk: "+ String(koin),STAY);
            lcdSet(DTL,"Total uang: "+ String(totalKoin),STAY);
            anim_LCD_HeaderFooter();
            break;
         case 19:
            Serial.println("koin 500s lalu 1000");
            koin = 1500;
            totalKoin += koin;
            suarakan(33,500); // kredit diterima
            lcdSet(HDR,"Koin masuk: "+ String(koin),STAY);
            lcdSet(DTL,"Total uang: "+ String(totalKoin),STAY);
            anim_LCD_HeaderFooter();
            break;
         case 24:
            Serial.println("koin 1000 2x");
            koin = 2000;
            totalKoin += koin;
            suarakan(33,500); // kredit diterima
            lcdSet(HDR,"Koin masuk: "+ String(koin),STAY);
            lcdSet(DTL,"Total uang: "+ String(totalKoin),STAY);
            anim_LCD_HeaderFooter();
            break;
       }
    }
    coinSign = 0;
    if (totalKoin != totalKoinSkr ) { //kalo sudah ada koin masuk tidak sedang iseng
       if (waktuMasuk_RCS - millis() > 30000 ) { // waktu tunggu sudah lewat 30detik !
         diisengin = true;    
       } else {
        // tulis waktu tunggu
        Serial.println("nunggu koin dimasukkan... 30s - "+String(waktuMasuk_RCS - millis()));
        delay(20);
              
       }
    } else { // koin sekarang sudah berubah lho
      totalKoinSkr = totalKoin;
    }
    
  } while (totalKoin < ongkosPakai && whanaMODE!=LCELLMODE && !diisengin);
   
}
}

void operateLcell() { 
  delay(lamaNimbang);
}

String jfy(String a, String b) { //justify 2 words in 16 spaces
  String spc = "";
  if (a.length()+b.length() < 16) {
    for (int i=0;i < 15 - (a.length() + b.length());i++) spc += " ";
  }
  return ( a + " " + spc + b );
}

void lcdSet(byte whichOne,String msg,byte MDE) {
  switch (whichOne) {
    case HDR:
      HDRMde = MDE;
      if (isNewMsg(msg,HDR)) {
        HDRStr = msg;
        switch (MDE) {
           case STAY:
             if (HDRStr.length() < 16) {
               HDRStr2Disp = RPAD16(HDRStr,HDRStr.length());
             } else HDRStr2Disp = HDRStr;
             break;
           case SCROLL:
             HDRStr2Disp = HDRStr + "                ";
             break;
           case BLINK:
             if (HDRStr.length() < 16) {
               HDRStr2Disp = RPAD16(HDRStr,HDRStr.length());
             } else HDRStr2Disp = HDRStr;
             break;
        } //switch
      }// if isNewMsg
      break;
    case DTL:
      DTLMde = MDE;
      if (isNewMsg(msg,DTL)) {
        DTLStr = msg;
        switch (MDE) {
           case STAY:
             if (DTLStr.length() < 16) {
               DTLStr2Disp = RPAD16(DTLStr,DTLStr.length());  
             } else DTLStr2Disp = DTLStr;
             break;
           case SCROLL:
             DTLStr2Disp = DTLStr + "                ";
             break;
           case BLINK:
             if (DTLStr.length() < 16) {
               DTLStr2Disp = RPAD16(DTLStr,DTLStr.length());  
             } else DTLStr2Disp = DTLStr;
             break;
        } // switch    
      } // if isNewMsg
      break;
  } // switch
} // lcdSET


void setMODE(byte MODE) {
  
  whanaMODE = MODE;

  // off all POWER
  analogWrite(stdbyPWR,OFF);
  analogWrite(cslotPWR,OFF);
  analogWrite(lcellPWR,OFF);
  digitalWrite(multbPWR,LOW);
  
  switch (MODE) {
    case STDBYMODE:
      //
      Serial.println(">>>>>>>>>>>>>>>>>>>>>>>>>>> STAND BY MODE");
      koin = 0;
      totalKoin = 0;
      
      lcdSet(HDR,"TIMBANGAN BADAN",STAY);
      lcdSet(DTL,"Tarif Rp " + String(ongkosPakai),STAY);
      anim_LCD_HeaderFooter();
      delay(200);
      if(!hentikan) delay(1000);

      for (byte i=0;i<5 && !hentikan;i++) { // say identitas if not hentikan by user
         switch (i) {
           case 0 : // detik ke 0
              Serial.println("sayWAHANA");
              lcdSet(DTL,"Armada          ",STAY);
              anim_LCD_HeaderFooter();
              hentikan = false;
              suarakan(8, 2000); // 0 : ngga usah delay ms
              if(!hentikan) delay(SETDELAY*2);
              break;
           case 2:
              Serial.println("sayRancangan");
              hentikan = false;
              suarakan(17, 1000);
                lcdSet(DTL,"  -- KADOT --   ",STAY);
                anim_LCD_HeaderFooter();
                if(!hentikan) delay(SETDELAY*2);
              break;
           case 3:
              Serial.println("sayArmada");
              hentikan = false;
              suarakan(31, 10); // armada ilpan
                lcdSet(DTL,"Technology      ",STAY);
                anim_LCD_HeaderFooter();
              if(!hentikan)  delay(SETDELAY*4);
              break;
           case 4:
                lcdSet(DTL,"Kalibaru Kulon  ",STAY);       
                anim_LCD_HeaderFooter();
                if(!hentikan) delay(SETDELAY*4);
              break;
         }               
         if(!hentikan) delay(SETDELAY);
      }

      if(!hentikan) { // say ongkos bila tidak dihentikan oleh user
        lcdSet(DTL,"Tarif Rp " + String(ongkosPakai),STAY);
        anim_LCD_HeaderFooter();
        hentikan = false;
        suarakan(9,1500); // say Ongkos
      }

      if(!hentikan) {
        Serial.println("Mau katakan rupiah ongkos nehh..");
        switch (ongkosPakai) {
          case 1000:
             Serial.println("  ooh serebu");
             hentikan = false;
             suarakan(4, 1000);
             delay(200);
             break;
          case 1500:
             Serial.println("  ooh bumaratos");
             hentikan = false;
             suarakan(1, 2000);
             break;
          case 2000:
             Serial.println("  ooh rong ewu");
             hentikan = false;
             suarakan(15, 2000);
             break;
          case 2500:
             Serial.println("  ooh 2500");
             hentikan = false;
             suarakan(11, 2000);
             break;
          case 3000:
             Serial.println("  ooh lung ewu");
             hentikan = false;
             suarakan(2, 2000);
             break;
        }
      }

      if(!hentikan) { // menunggu, tombol coklat, bila diperkenankan user
      
        while (whanaMODE!=CSLOTMODE) {

           // wait a man response pressing actBtn
           Serial.println("..loop !CSLOTMODE.");
           hentikan = false;
           suarakan(32,2000);// r2d2
           hentikan = false;
           suarakan(26, 2000); // say: uang pakai koin 200an 500an atau 1000an

           if(!hentikan) {
             lcdSet(DTL,"Koin 200 bisa   ",STAY);
             anim_LCD_HeaderFooter();
             if(!hentikan) delay(SETDELAY*4);
             lcdSet(DTL,"                ",STAY);
             anim_LCD_HeaderFooter();
             if(!hentikan) delay(SETDELAY*2);
           
             lcdSet(DTL,"Koin 500 bisa   ",STAY);
             anim_LCD_HeaderFooter();
             if(!hentikan) delay(SETDELAY*4);
             lcdSet(DTL,"                ",STAY);
             anim_LCD_HeaderFooter();
             if(!hentikan) delay(SETDELAY*2);

             lcdSet(DTL,"Koin 1000 bisa   ",STAY);
             anim_LCD_HeaderFooter();
             if(!hentikan) delay(SETDELAY*4);
             lcdSet(DTL,"                ",STAY);
             anim_LCD_HeaderFooter();
             if(!hentikan) delay(SETDELAY*2);
   
             hentikan = false;
             suarakan(27,100);
             coinSign = 0;
           }

           while(whanaMODE==STDBYMODE && !hentikan) { 
           
             for(int i=0;i<8 && whanaMODE==STDBYMODE && !hentikan;i++) {
                if(random(1,30)==2) {
                  hentikan = false;
                  suarakan(27,0);
                }
                switch (i) {
                 case 0:
                    lcdSet(DTL,"Tekan", STAY);
                    break;
                 case 1:
                    lcdSet(DTL,"-tombol coklat-", STAY);
                    break;
                 case 2:
                    lcdSet(DTL,"masuk koin", STAY);
                    break;
                 case 3:
                    lcdSet(DTL,"Tarif Rp " + String(ongkosPakai), STAY);
                    break;
                 case 4:
                    lcdSet(DTL,"Koin 200 bisa", STAY);
                    break;
                 case 5:
                    lcdSet(DTL,"Koin 500 bisa", STAY);
                    break;
                 case 6:
                    lcdSet(DTL,"Koin 1000 bisa", STAY);
                    break;
                 case 7:
                    lcdSet(DTL,"(c) Kadot 2023", STAY);
                    break;
                }
                
                anim_LCD_HeaderFooter();
                delay(SETDELAY);
             
                digitalWrite(multbPWR,HIGH); // 
                delay(SETDELAY);
                digitalWrite(multbPWR,LOW);
             }
           }
        }
      }
      
      break;
    case CSLOTMODE:
      //
      Serial.println(">>>>>>>>>>>>>>>>>>>>>>>>>>> COIN SLOT MODE");
      // SAY: coin slot aktif
      koin = 0;
      totalKoin = 0; 
      lcdSet(HDR,"Masukkan koinnya", STAY);
      lcdSet(DTL,"Sampai Rp " + String(ongkosPakai), STAY);
      anim_LCD_HeaderFooter();
      delay(SETDELAY);
      analogWrite(cslotPWR,255); // 5DCV
      delay(1000);
      hentikan = false;
      suarakan(22,2000);
      readCoinSlot(); // stay loop until done
      //delay(2000); // biar signal panjang bisa masuk dulu
      analogWrite(cslotPWR,OFF);
      if (totalKoin > 0) {
         whanaMODE = LCELLMODE;  
      } else {
         whanaMODE = STDBYMODE;
      }
      
      break;
    case LCELLMODE:
      //
      Serial.println(">>>>>>>>>>>>>>>>>>>>>>>>>>> LCELL MODE");
      analogWrite(lcellPWR,300); // 3.3DCV
      lcdSet(HDR,"TIMBANGAN AKTIF", STAY);
      lcdSet(DTL,"Silahkan naik...", STAY);
      anim_LCD_HeaderFooter();
      delay(SETDELAY);
      hentikan = false;
      suarakan(7,2000);//timbangan siap
      
      for (int i=0;i<20;i++) {
        lcdSet(DTL,"                ", STAY);
        anim_LCD_HeaderFooter();
        delay(SETDELAY);
        lcdSet(DTL,"Silahkan naik...", STAY);
        anim_LCD_HeaderFooter();
        delay(SETDELAY);
      }
      
      lcdSet(DTL,"Terima kasih", STAY);
      anim_LCD_HeaderFooter();
      hentikan = false;
      suarakan(10,4000);
      lcdSet(DTL,"                ", STAY);
      anim_LCD_HeaderFooter();
      delay(SETDELAY);  
      lcdSet(DTL,"Sehat Selalu", STAY);
      anim_LCD_HeaderFooter();
      hentikan = false;
      suarakan(29,2000);// r2d2
      //delay(2000);
      
      whanaMODE = STDBYMODE;
      analogWrite(lcellPWR,0); // OFF
      break;    
  }
}

void suarakan(uint8_t n,uint32_t lama) {
  unsigned long timer = millis();

    mp3.play(n);

 while (millis() - timer < lama && !hentikan) {
   // diem sini
   delay(0);
 }
}

void setup()
{
  
  // auto-run for shake
  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);
  
  // Setup PINs
  // ACTION BUTTON
  pinMode(actBtn, INPUT_PULLUP);
  attachInterrupt(0, ISR_actBtn, FALLING);

  // signal coin slot
  pinMode(counterPin, INPUT_PULLUP);
  pinMode(signalPin, INPUT_PULLUP);
  attachInterrupt(1, ISR_Coin_Signal, FALLING);

  coinSign = 0;
  // POWER LINE
  pinMode(stdbyPWR,OUTPUT);
  pinMode(cslotPWR,OUTPUT);
  pinMode(lcellPWR,OUTPUT);
  pinMode(multbPWR,OUTPUT);
  
  analogWrite(stdbyPWR,0);
  analogWrite(cslotPWR,0);
  analogWrite(lcellPWR,0);
  digitalWrite(multbPWR,LOW);

  randomSeed(analogRead(A5));

  whanaMODE = STDBYMODE;

  //initialize serial of mp3 player 
  mySoftwareSerial.begin(9600);
  Serial.begin(115200);
  
  Serial.println();
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  
  if (!mp3.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true) {delay(0);}
  }

  // initialize the LCD
  lcd.begin();
  
  lcdSet(HDR,"TIMBANGAN BADAN",STAY);
  lcdSet(DTL,"    >> Koin    ",STAY);
  anim_LCD_HeaderFooter();
  
  mp3.volume(30);  //Set volume value. From 0 to 30
  mp3.EQ(DFPLAYER_EQ_BASS);
  
  hentikan = false;
  suarakan(20,20000);

  HDRStrSUM = 0;
  DTLStrSUM = 0;

}

void loop() {
  Serial.println("Got loop");
  setMODE(STDBYMODE); // stay loop sampai pengunjung tekan tombol mulai
  setMODE(CSLOTMODE); // stay loop sampai koin lengkap masuk atau MONGGO_LAH
  setMODE(whanaMODE); // operated for given time
}
