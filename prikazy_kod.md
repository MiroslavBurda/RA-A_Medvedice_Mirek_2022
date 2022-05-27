#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

    cfg.owner = "mirek"; // Ujistěte se, že v aplikace RBController máte nastavené stejné
    cfg.name = "mojerobotka";

        if (rkButtonUp(true)) {
            rkMotorsDriveAsync(1000, 1000, 100, [](void) {});
            delay(300);
        }

        if (rkButtonLeft(true)) {
            rkMotorsSetSpeed(100, 100);
            delay(300);
        }

        if (rkButtonRight(true)) {
            rkMotorsSetSpeed(0, 0);
            delay(300);
        }

        if (rkButtonDown(true)) { // Tlačítko dolů: otáčej se a hledej kostku
            if (justPressed) {
                justPressed = false; // kdyz by tu tato podminka nebyla, byla by pauza v kazdem cyklu
                delay(300); // prodleva, abyste stihli uhnout rukou z tlačítka
            }
        }

        // Min = (UltraUp < UltraDown) ? UltraUp : UltraDown;
        // if (millis() - last_millis > 50)
            // printf("l: %i  r: %i  UA: %i  UD: %i  Up: %i  Down: %i \n", l, r, rkIrLeft(), rkIrRight(), UltraUp, UltraDown);
        
        // if (Serial1.available() > 0) { 
        //     byte readData[10]= { 1 }; //The character array is used as buffer to read into.
        //     int x = Serial1.readBytes(readData, 10); //It require two things, variable name to read into, number of bytes to read.
        //     printf("bytes: "); 
        //     // Serial.println(x); //display number of character received in readData variable.
        //     printf("h: %i, ", readData[0]);
        //     printf("h: %i, ", readData[1]);
        //     for(int i = 2; i<10; i++) {
        //         printf("%i: %i, ", i-2, readData[i]); // ****************
        //     }
        //     printf("\n ");
        // }  

unsigned long last_millis = 0;
bool finding = false; // našel kostku
bool previousLeft = false;
bool justPressed = true; //je tlačítko stisknuto poprvé
int ledBlink = 10; // blikani zadanou LED, 10 vypne všechny LED
int UltraUp = 5000, UltraDown, Min;
int found = 0; // kolik kostek našel
int k = 0; // pocitadlo pro IR

void blink() { // blikani zadanou LED
    while (true) { 
        // rkSmartLedsRGB(3, 255, 255, 255);
        // delay(500);
        rkSmartLedsRGB(3, 0, 0, 0);
        delay(500); 
    }
}


void Print() {
    printf("L: %d   R: %d  UltraUp: %i  Ultradown: %i \n", l, r, UltraUp, UltraDown);
    SerialBT.print("L: ");
    SerialBT.print(l);
    SerialBT.print("  R: ");
    SerialBT.print(r);
    SerialBT.print("  UltraUP: ");
    SerialBT.print(UltraUp);
    SerialBT.print("  UltraDown: ");
    SerialBT.print(UltraDown);
    SerialBT.print("  Min: ");
    SerialBT.println(Min);
    last_millis = millis();
}


int l = 0; // hodnta leveho ultrazvuku
int r = 0; // hodnota praveho ultrazvuku
int IrL[] = { 0, 0, 0, 0 }; // pole pro levy ultrazvuk
int IrR[] = { 0, 0, 0, 0 }; // pole pro pravy ultrazvuk

void rkIr() { // prumerovani IR
    while (true) {
        k++;
        if (k == 30000)
            k = 0;
        switch (k % 4) {
        case 0:
            IrL[0] = rkIrLeft();
            IrR[0] = rkIrRight();
            break;
        case 1:
            IrL[1] = rkIrLeft();
            IrR[1] = rkIrRight();
            break;
        case 2:
            IrL[2] = rkIrLeft();
            IrR[2] = rkIrRight();
            break;
        case 3:
            IrL[3] = rkIrLeft();
            IrR[3] = rkIrRight();
            break;
        default:
            printf("Chyba v prikazu switch");
            break;
        }
        l = (IrL[0] + IrL[1] + IrL[2] + IrL[3]) / 4;
        r = (IrR[0] + IrR[1] + IrR[2] + IrR[3]) / 4;
        delay(10);
    }
}

void serva() { // testovani maximalnich poloh všech serv 

        for (int i = 1; i < 5; i++) {  // fungují 
        rkServosSetPosition(i, 90);
        printf("servo %i\n", i);
        delay(1000);
    }

    for (int i = 1; i < 5; i++) {
        rkServosSetPosition(i, 0);
        printf("servo, %i\n", i);
        delay(1000);
    }

    for (int i = 1; i < 5; i++) {
        rkServosSetPosition(i, -90);
        printf("servo, %i\n", i);
        delay(1000);
    }
}

if(SERVO)
        serva(); // až za rkSetup(cfg); 

void serva() { //nastavovani polohy serva 2
    int k = 0 ; 
    while(true) {

        if (rkButtonLeft(true)) {
            k -= 5;
            //delay(300);
        }

        if (rkButtonRight(true)) {
            k += 5;
            //delay(300);
        }

        rkServosSetPosition(2, k);
        printf("servo %i\n", k);
        delay(100);
    }

}