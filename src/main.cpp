#include "robotka.h"
#include <thread>
#include <driver/i2c.h>
#include <driver/uart.h>

/* Spouštění robota Medvědice 
1) Start programu na kostce - TCA-AMedv1e.c
2) Reset RBCX (např. reset tlačítko přímo na ESP32)
3) Zkontrolovat zastrčení startovacího lanka 
4) Zmáčknout tlačítko Up na kostce, potom vybrat barvu (tlačítko Down)
5) Zmáčknout Up na RBCX - rozsvítí se žlutá dioda na RBCX a červená dole na LED pásku

*/


// todo jak se vymaže buffer? nebo: jak vím, že jsem na konci/ na aktuálních hodnotách 

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
uint8_t header = 10; // hlavicka pro vstup i vystup  
uint8_t DataToReceive[] = {2, 2, 2, 2};
size_t len = sizeof(DataToReceive);
uint8_t DataToSend[3] = {0}; // hlavicka; nejblizsi vzdalenost; cislo ultrazvuku, ktery ji nameril 


// Ultrasonic 
const byte readSize = 8;
const byte headerU = 250; //hlavicka zpravy ma tvar: {250, 250+k}, k = 1 ... 3    
constexpr byte msgHeader[3] = {251, 252, 253};
const byte minVzdal = 50; // minimalni vzdalenost, na ktere se sousedni robot muze priblizit, if se priblizi vic, tak abort();

byte readData0[readSize]= {0}; //The character array is used as buffer to read into.
byte readData1[readSize]= {0};
byte readData2[readSize]= {0};

unsigned long startTime = 0; // zacatek programu 
const bool SERVO = false;

/**
 * @brief Funkce na vyhledání nejmenší hodnoty z byte pole. !!! Nulu to nebere jako nejmenší !!!
 * 
 * @param arr   Ukazatel na pole hodnot
 * @param index Adresa kam má funkce vrátit pozici nejmenší hodnoty
 * @return byte Vrací nejmenší hodnotu pole 
 */
byte min_arr(byte *arr, int &index){
    byte tmp = 255;
    index = -1;
    for (size_t i = 0; i < 8; i++) {
        if (arr[i] < tmp && arr[i] != 0) {
            tmp = arr[i];
            index = i;
        }
    }
    return tmp;
}

void stopTime() { // STOP jizde po x milisec 
    while(true) {
        if (( millis() - startTime ) > 127000) { // konci cca o 700ms driv real: 127000
            printf("cas vyprsel: ");
            printf("%lu, %lu \n", startTime, millis() );
            rkSmartLedsRGB(0, 255, 0, 0);
            delay(100); // aby stihla LED z predchoziho radku rozsvitit - z experimentu
            abort(); // program skonci -> dojde k resetu a zustane cekat pred stiskem tlacitka Up
        }
        delay(10); 
    }
}

void ultrasonic_color(byte dis, size_t i) {
    if (dis > 200 || dis == 0) {
        rkSmartLedsRGB(i, 0, 50, 0);
    } else if (dis > 180) {
        rkSmartLedsRGB(i, 20, 50, 0);
    } else if (dis > 150) {
        rkSmartLedsRGB(i, 35, 50, 0);
    } else if (dis > 120) {
        rkSmartLedsRGB(i, 50, 50, 0);
    } else if (dis > 100) {
        rkSmartLedsRGB(i, 50, 35, 0);
    } else if (dis > 80) {
        rkSmartLedsRGB(i, 50, 20, 0);
    } else if (dis > 60) {
        rkSmartLedsRGB(i, 204, 102, 0);
    } else if (dis > 50) {
        rkSmartLedsRGB(i, 250, 60, 0);
    } else {
        rkSmartLedsRGB(i, 50, 0, 0);
    } 
}

void ultrasonic() {
    printf("Ultrasonic \n");
    rkSmartLedsRGB(0, 0, 50, 0);  // kontrola led pásku --> zapljsem koukátko
    int pozice0, pozice1, pozice2;
    while (true) {
            if (Serial1.available() > 0) { 
                int temp = Serial1.read();
                if(temp == headerU) {
                    // printf("bytes: %i \n", header); 
                    if (Serial1.available() > 0) {
                        int head = Serial1.read();
                        //printf("head: %i ", head); 
                        switch (head) {
                        case msgHeader[0]: 
                            Serial1.readBytes(readData0, readSize); //It require two things, variable name to read into, number of bytes to read.
                            for(int i = 0; i<8; i++) { printf("%i: %i, ", i, readData0[i]); } printf("\n ");
                            break;        
                        case msgHeader[1]:
                            Serial1.readBytes(readData1, readSize); 
                            //for(int i = 0; i<8; i++) { printf("%i: %i, ", i, readData1[i]); } printf("\n ");
                            break;        
                        case msgHeader[2]:
                            Serial1.readBytes(readData2, readSize); 
                            //for(int i = 0; i<8; i++) { printf("%i: %i, ", i, readData2[i]); } printf("\n ");
                            break;
                        default:
                            printf("Nenasel druhy byte hlavicky !! "); 
                        }
                    }
                    int min0 = min_arr(readData0, pozice0); 
                    int min1 = min_arr(readData1, pozice1);
                    int min2 = min_arr(readData2, pozice2); 
                    if ( (min0 == min1) && (min0 == min2) && (min0 < minVzdal) ) {
                        //printf("Souper blizi...");
                        //if(startState) {
                            printf("Souper se prilis priblizil...");
                            //TODO zde je potreba zavolat funkci, která zastaví robota 
                            DataToSend[0] = 2;
                            int written = i2c_slave_write_buffer(bus_num, DataToSend, 8, pdMS_TO_TICKS(25));

                        //}
                    }
                    for (size_t i = 0; i < 8; i++) {
                        ultrasonic_color(readData0[i], i);
                    }
                    
                }
             
        }
        delay(50);            
    }
}

void setup() {
    
    
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

    int written = 0;
    ESP_ERROR_CHECK(i2c_param_config(bus_num, &conf_slave));
    ESP_ERROR_CHECK(i2c_driver_install(bus_num, I2C_MODE_SLAVE, 2000, 2000, 0)); 
    written = i2c_slave_write_buffer(bus_num, DataToSend, 8, pdMS_TO_TICKS(25)); 
    printf("WRITTEN: %i \n", written);  

    rkConfig cfg;
    cfg.motor_max_power_pct = 100; // limit výkonu motorů na xx %

    cfg.motor_enable_failsafe = false;
    cfg.rbcontroller_app_enable = false; // nepoužívám mobilní aplikaci (lze ji vypnout - kód se zrychlí, ale nelze ji odstranit z kódu -> kód se nezmenší)
    rkSetup(cfg);

    for (size_t i = 0; i < 8; i++) {     // vymyzání LED pásku
        rkSmartLedsRGB(i, 0, 0, 0);
    }
    rkLedBlue(true); // cekani na stisk 
    printf("cekani na stisk Up\n");
    while(true) {   
        if(rkButtonUp(true)) {
            break;
        }
        delay(10);
    }
    rkLedBlue(false);
    rkLedYellow(true);
    startTime = millis();

    rkServosSetPosition(1, -10);   // vychozi pozice praveho serva nahore - až za rkSetup(cfg); 
    rkServosSetPosition(2, 10);  // vychozi pozice leveho serva nahore

    // čekání na vytažení startovacího lanka
    printf("čekaní na vytažení startovacího lanka\n");
    rkSmartLedsRGB(7, 50, 0, 0);  
    while (rkButtonLeft(true)) {
        delay(10);
    }
    rkSmartLedsRGB(7, 0, 0, 0);  
    printf("startovací lanko vytaženo\n");
    delay(5000);
    DataToSend[0] = 1;  // signal, ze se robot ma rozjet
    int writ = i2c_slave_write_buffer(bus_num, DataToSend, 8, pdMS_TO_TICKS(25));
    //TODO otestovat poradne 
    
    Serial1.begin(115200, SERIAL_8N1, 17, 16); // Rx = 17 Tx = 16 
    
    std::thread t2(ultrasonic);  // vlakno pro prijimani a posilani dat z ultrazvuku
    //uart_flush(UART_NUM_1);
    // std::thread t3(stopTime);    // vlakno pro zastaveni po uplynuti casu 

    delay(300);
    fmt::print("{}'s Robotka '{}' with {} mV started!\n", cfg.owner, cfg.name, rkBatteryVoltageMv());
    rkLedYellow(true); // robot je připraven

    int ii = 0; 
    while (true) {
        int res = i2c_slave_read_buffer(bus_num, DataToReceive, 4, pdMS_TO_TICKS(25)); 
       // printf("TAG1: %i, %i \n", ii++, res);
        for (int k = 0; k < 4; k++) {
            //printf("DATA: %i ", DataToReceive[k] );
        }    
        //printf("\n " );

        if((DataToReceive[0] == 10) && (DataToReceive[1] == DataToReceive[2] )) {  // test hlavicky a shodnosti obou bytů 
                int servo = DataToReceive[1]-100;  
                rkServosSetPosition(1, servo);
                rkServosSetPosition(2, -servo);
                printf("Servo1: %i, servo2: %i\n", servo, -servo);
        }

        delay(100); // cas, aby se serva nastavila do spravne polohy         

    }
}