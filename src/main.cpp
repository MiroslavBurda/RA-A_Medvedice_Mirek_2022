#include "BluetoothSerial.h"
#include "robotka.h"
#include <thread>


BluetoothSerial SerialBT;

//************************************************************************************************

#include <driver/i2c.h>

#define RETURN_IF_ERR(x) do {                                         \
        esp_err_t __err_rc = (x);                                       \
        if (__err_rc != ESP_OK) {                                       \
            return __err_rc;                                            \
        }                                                               \
    } while(0)

gpio_num_t sda_pin = GPIO_NUM_18;
gpio_num_t scl_pin = GPIO_NUM_19; 
uint8_t address = 0x04;
i2c_port_t bus_num = I2C_NUM_0; 
uint8_t DataToReceive[] = {2, 2, 2, 2};
size_t len = sizeof(DataToReceive);
uint8_t DataToSend[] = {11, 12, 13, 14, 21, 22, 23, 24};


//*************************************************************************************************************



unsigned long startTime = 0; // zacatek programu 

unsigned long last_millis = 0;
bool finding = false; // našel kostku
bool previousLeft = false;
bool justPressed = true; //je tlačítko stisknuto poprvé
int ledBlink = 10; // blikani zadanou LED, 10 vypne všechny LED
int UltraUp = 5000, UltraDown, Min;
int found = 0; // kolik kostek našel
int l = 0; // hodnta leveho ultrazvuku
int r = 0; // hodnota praveho ultrazvuku
int IrL[] = { 0, 0, 0, 0 }; // pole pro levy ultrazvuk
int IrR[] = { 0, 0, 0, 0 }; // pole pro pravy ultrazvuk
int k = 0; // pocitadlo pro IR

void blink() { // blikani zadanou LED
    while (true) { 
        // rkSmartLedsRGB(3, 255, 255, 255);
        // delay(500);
        rkSmartLedsRGB(3, 0, 0, 0);
        delay(500); 
    }
}

void stopTime() { // STOP jizde po x milisec 
    while(true) {
        if (( millis() - startTime ) > 4000) { // konci cca o 700ms driv 
            printf("cas vyprsel: ");
            printf("%lu, %lu \n", startTime, millis() );
            rkSmartLedsRGB(0, 255, 0, 0);
            while(true); // tady musi program skoncit po uplynuti limitu  
        }
        delay(10); 
    }
}

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

void serva();

void setup() {
    startTime = millis();
    rkSmartLedsRGB(0, 0, 0, 0);
    // serva();
    Serial1.begin(115200, SERIAL_8N1, 17, 16); // Rx = 17 Tx = 16
    
    
    i2c_config_t conf_slave;
    conf_slave.mode = I2C_MODE_SLAVE;
    conf_slave.sda_io_num = sda_pin;
    conf_slave.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf_slave.scl_io_num = scl_pin;            
    conf_slave.scl_pullup_en = GPIO_PULLUP_ENABLE;
    
    conf_slave.slave = {
        .addr_10bit_en = 0,
        .slave_addr = address,  
    };

    int i = 0; 
    int written = 0;
    ESP_ERROR_CHECK(i2c_param_config(bus_num, &conf_slave));
    ESP_ERROR_CHECK(i2c_driver_install(bus_num, I2C_MODE_SLAVE, 2000, 2000, 0)); 
    written = i2c_slave_write_buffer(bus_num, DataToSend, 8, pdMS_TO_TICKS(25)); 
    ESP_LOGI("WRITTEN:", "%i \n", written);  

    rkConfig cfg;
    cfg.owner = "mirek"; // Ujistěte se, že v aplikace RBController máte nastavené stejné
    cfg.name = "mojerobotka";
    cfg.motor_max_power_pct = 100; // limit výkonu motorů na xx %

    cfg.motor_enable_failsafe = false;
    cfg.rbcontroller_app_enable = false; // nepoužívám mobilní aplikaci (lze ji vypnout - kód se zrychlí, ale nelze ji odstranit z kódu -> kód se nezmenší)
    rkSetup(cfg);

    if (!SerialBT.begin("Burda_ctverec")) //Bluetooth device name; zapnutí BT musí být až za rkSetup(cfg); jinak to nebude fungovat a bude to tvořit reset ESP32
    {
        printf("!!! Bluetooth initialization failed!");
    } else {
        SerialBT.println("!!! Bluetooth work!\n");
        printf("!!! Bluetooth work!\n");
        rkLedBlue(true);
        delay(300);
        rkLedBlue(false);
    }

    rkUltraMeasureAsync(1, [&](uint32_t distance_mm) -> bool { // nesmí být v hlavním cyklu, protože se je napsaná tak, že se cyklí pomocí return true
        return false;
        UltraUp = distance_mm;
        return true;
    });

    rkUltraMeasureAsync(2, [&](uint32_t distance_mm) -> bool {
        UltraDown = distance_mm;
        return true;
    });

    std::thread t1(rkIr); // prumerne hodnoty z IR v samostatném vlákně
    std::thread t2(blink); // prumerne hodnoty z IR v samostatném vlákně
    std::thread t3(stopTime); // prumerne hodnoty z IR v samostatném vlákně

    delay(300);
    fmt::print("{}'s Robotka '{}' with {} mV started!\n", cfg.owner, cfg.name, rkBatteryVoltageMv());
    rkLedYellow(true); // robot je připraven

    serva(); // až za rkSetup(cfg); 

    while (true) {

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
        
        if (Serial1.available() > 0) { 
            byte readData[10]= { 1 }; //The character array is used as buffer to read into.
            int x = Serial1.readBytes(readData, 8); //It require two things, variable name to read into, number of bytes to read.
            Serial.print("bytes: "); 
            Serial.println(x); //display number of character received in readData variable.
            for(int i = 0; i<10; i++) {
              //  printf("i: %i, ", readData[i]); // ****************
            }
        }  
        delay(10);           

    }
}

void serva() {

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