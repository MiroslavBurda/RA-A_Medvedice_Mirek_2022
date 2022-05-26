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

gpio_num_t sda_pin = GPIO_NUM_21;
gpio_num_t scl_pin = GPIO_NUM_22; 
uint8_t address = 0x04;
i2c_port_t bus_num = I2C_NUM_0; 
uint8_t DataToReceive[] = {2, 2, 2, 2};
size_t len = sizeof(DataToReceive);
uint8_t DataToSend[] = {11, 12, 13, 14, 21, 22, 23, 24};

//*************************************************************************************************************

unsigned long startTime = 0; // zacatek programu 
const bool SERVO = false;

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

void stopTime() { // STOP jizde po x milisec 
    while(true) {
        if (( millis() - startTime ) > 3000000) { // konci cca o 700ms driv 
            printf("cas vyprsel: ");
            printf("%lu, %lu \n", startTime, millis() );
            rkSmartLedsRGB(0, 255, 0, 0);
            delay(100); // aby stihla LED z predchoziho radku rozsvitit - z experimentu
            abort(); // tady musi program skoncit
        }
        delay(10); 
    }
}

void serva();

void setup() {
    
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
    printf("WRITTEN: %i \n", written);  


    rkConfig cfg;
    cfg.owner = "mirek"; // Ujistěte se, že v aplikace RBController máte nastavené stejné
    cfg.name = "mojerobotka";
    cfg.motor_max_power_pct = 100; // limit výkonu motorů na xx %

    cfg.motor_enable_failsafe = false;
    cfg.rbcontroller_app_enable = false; // nepoužívám mobilní aplikaci (lze ji vypnout - kód se zrychlí, ale nelze ji odstranit z kódu -> kód se nezmenší)
    rkSetup(cfg);

    rkLedBlue(true); // cekani na stisk 
    while(true) {
        printf("cekani na stisk Up\n");
        if(rkButtonUp(true)) {
            break;
        }
        delay(500);
    }
    rkLedBlue(false);
    rkLedYellow(true);
    startTime = millis();
    rkSmartLedsRGB(0, 0, 0, 0);    
    rkServosSetPosition(1, 65);   // vychozi pozice praveho serva nahore - až za rkSetup(cfg); 
    rkServosSetPosition(2, -60);  // vychozi pozice leveho serva nahore

    // rkMotorsSetSpeed(100, 100); // testovaci 

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

    // std::thread t1(rkIr); // prumerne hodnoty z IR v samostatném vlákně
    std::thread t2(blink); // prumerne hodnoty z IR v samostatném vlákně
    std::thread t3(stopTime); // prumerne hodnoty z IR v samostatném vlákně

    delay(300);
    fmt::print("{}'s Robotka '{}' with {} mV started!\n", cfg.owner, cfg.name, rkBatteryVoltageMv());
    rkLedYellow(true); // robot je připraven

    if(SERVO)
        serva(); // až za rkSetup(cfg); 

    int ii = 0; 
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

        int res = i2c_slave_read_buffer(bus_num, DataToReceive, 4, pdMS_TO_TICKS(25)); 
        printf("TAG1: %i, %i \n", ii++, res);
        for (int k = 0; k < 4; k++) {
            printf("DATA: %i \n", DataToReceive[k] );
        }
          
        delay(1000); 

        //delay(10);           

    }
}

// void serva() { // testovani maximalnich poloh všech serv 

//         for (int i = 1; i < 5; i++) {  // fungují 
//         rkServosSetPosition(i, 90);
//         printf("servo %i\n", i);
//         delay(1000);
//     }

//     for (int i = 1; i < 5; i++) {
//         rkServosSetPosition(i, 0);
//         printf("servo, %i\n", i);
//         delay(1000);
//     }

//     for (int i = 1; i < 5; i++) {
//         rkServosSetPosition(i, -90);
//         printf("servo, %i\n", i);
//         delay(1000);
//     }
// }

void serva() { //nastavovani polohy serva 1
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