#include "robotka.h"
#include <thread>
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

unsigned long startTime = 0; // zacatek programu 
const bool SERVO = false;

void stopTime() { // STOP jizde po x milisec 
    while(true) {
        if (( millis() - startTime ) > 3000000) { // konci cca o 700ms driv 
            printf("cas vyprsel: ");
            printf("%lu, %lu \n", startTime, millis() );
            rkSmartLedsRGB(0, 255, 0, 0);
            delay(100); // aby stihla LED z predchoziho radku rozsvitit - z experimentu
            abort(); // program skonci -> dojde k resetu a zustane cekat pred stiskem tlacitka Up
        }
        delay(10); 
    }
}

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

    std::thread t3(stopTime); // vlakno pro zastaveni po uplynuti casu 

    delay(300);
    fmt::print("{}'s Robotka '{}' with {} mV started!\n", cfg.owner, cfg.name, rkBatteryVoltageMv());
    rkLedYellow(true); // robot je připraven

    int ii = 0; 
    while (true) {
        int res = i2c_slave_read_buffer(bus_num, DataToReceive, 4, pdMS_TO_TICKS(25)); 
        printf("TAG1: %i, %i \n", ii++, res);
        for (int k = 0; k < 4; k++) {
            printf("DATA: %i \n", DataToReceive[k] );
        }        
        delay(1000);          

    }
}