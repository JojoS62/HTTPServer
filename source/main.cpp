#include "mbed.h"

#include "globalVars.h"

//#include "network-helper.h"
#include "HttpServer.h"
#include "HttpResponseBuilder.h"
#include "WebsocketHandlers.h"
#include "HTTPHandlers.h"

#include "threadIO.h"

#include "FSMC_SRAM.h"

#define USE_HTTPSERVER
//#define USE_TFTPSERVER
//#define USE_MQTT

#define DEFAULT_STACK_SIZE (4096)

#define LED_ON  (0)
#define LED_OFF (1)

constexpr int I2C_TCN75_ADDR = 0x90;

#define AD_BASE   (0x60000000UL | 0x00000a0UL)
#define AD_DAT16  (*((volatile unsigned short *)(AD_BASE+2)))

#ifdef USE_TFTPSERVER
#include "threadTFTPServer.h"
ThreadTFTPServer  threadTFTPpServer;
#endif

FSMC_SRAM fsmc;
DigitalOut adcReset(PD_3, 1);
DigitalOut cnvsta(PD_6, 0);
DigitalOut cnvstb(PD_5, 0);
DigitalOut rgbLED_red(LED_LD16_RED, LED_OFF);
DigitalOut rgbLED_green(LED_LD16_GREEN, LED_OFF);
DigitalOut rgbLED_blue(LED_LD16_BLUE, LED_OFF);
DigitalOut sync(PF_0, 1);
SPI spi3(PC_12, PC_11, PC_10, PA_4);

ThreadIO threadIO(100);

#ifdef USE_MQTT
#include "MQTTThreadedClient.h"

Thread msgSender(osPriorityNormal, DEFAULT_STACK_SIZE * 3);
using namespace MQTT;
/*  
    MQTT
*/
static const char * clientID = "mbed-sample";
//static const char * userID = "";
//static const char * password = "";
static const char * topic_1 = "mbed-sample";
static const char * topic_2 = "test";

int arrivedcount = 0;

void messageArrived(MessageData& md)
{
    Message &message = md.message;
    printf("Arrived Callback 1 : qos %d, retained %d, dup %d, packetid %d\n", message.qos, message.retained, message.dup, message.id);
    printf("Payload [%.*s]\n", message.payloadlen, (char*)message.payload);
    ++arrivedcount;
}

class CallbackTest
{
    public:
    
    CallbackTest()
        : arrivedcount(0)
    {}
    
    void messageArrived(MessageData& md)
    {
        Message &message = md.message;
        printf("Arrived Callback 2 : qos %d, retained %d, dup %d, packetid %d\n", message.qos, message.retained, message.dup, message.id);
        printf("Payload [%.*s]\n", message.payloadlen, (char*)message.payload);
        ++arrivedcount;
    }
    
    private:
    
    int arrivedcount;
};
#endif

int main() {
    PortIn portMac(PortG, 0x0FFF);
    PortIn portOption(PortG, 0xF000);
    DigitalOut ledReady(LED_READY);
    I2C i2c1(PB_7, PB_6);
    spi3.format(16, 0);

    ledReady = LED_ON;
    rgbLED_red = LED_ON;
    adcReset = 0;

    printf("Hello from "  MBED_STRINGIFY(TARGET_NAME) "\n");
    printf("Mbed OS version: %d.%d.%d\n\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);

    uint32_t macSetting = portMac;
    uint32_t optionSetting = portOption >> 12;
    printf("mac: %lX  option: %lX\n", macSetting, optionSetting);

    // IO Thread
    //threadIO.start();
    
    // Connect to the network with the default networking interface
    // if you use WiFi: see mbed_app.json for the credentials
    NetworkInterface* network = NetworkInterface::get_default_instance();
    if (!network) {
        printf("Cannot connect to the network, see serial output\n");
        return 1;
    } 
    nsapi_error_t connect_status = network->connect();

    if (connect_status != NSAPI_ERROR_OK) {
        printf("Failed to connect to network (%d)\n", connect_status);
        return 2;
    } 	

#ifdef USE_HTTPSERVER	
    HttpServer server(network, 5, 4);               // max 5 threads, 4 websockets
    server.setHTTPHandler("/", &request_handler);
    server.setHTTPHandler("/stats/", &request_handler_getStatus);
    
    server.setWSHandler("/ws/", WSHandler::createHandler);

    nsapi_error_t res = server.start(8080);

    if (res == NSAPI_ERROR_OK) {
        SocketAddress socketAddress;
        network->get_ip_address(&socketAddress);
        printf("Server is listening at http://%s:8080\n", socketAddress.get_ip_address());
    }
    else {
        printf("Server could not be started... %d\n", res);
    }
#endif

#ifdef USE_TFTPSERVER
    threadTFTPpServer.start(network);
#endif

#ifdef USE_MQTT
    float version = 0.6;
    CallbackTest testcb;

    printf("HelloMQTT: version is %.2f\n", version);
    MQTTThreadedClient mqtt(network);


    //const char* hostname = "jojosRPi3-1";
    const char* hostname = "192.168.100.28";
    int port = 1883;

    MQTTPacket_connectData logindata = MQTTPacket_connectData_initializer;
    logindata.MQTTVersion = 3;
    logindata.clientID.cstring = (char *) clientID;
    //logindata.username.cstring = (char *) userID;
    //logindata.password.cstring = (char *) password;
    
    mqtt.setConnectionParameters(hostname, port, logindata);
    mqtt.addTopicHandler(topic_1, messageArrived);
    mqtt.addTopicHandler(topic_2, &testcb, &CallbackTest::messageArrived);

    // Start the data producer
    msgSender.start(mbed::callback(&mqtt, &MQTTThreadedClient::startListener));
    
    int i = 0;
    while(true)
    {
        PubMessage message;
        message.qos = QOS0;
        message.id = 123;
        
        strcpy(&message.topic[0], topic_1);
        sprintf(&message.payload[0], "Testing %d", i);
        message.payloadlen = strlen((const char *) &message.payload[0]);
        mqtt.publish(message);
        
        i++;
        //TODO: Nothing here yet ...
        ThisThread::sleep_for(10000);
    }
#endif

    rgbLED_red = LED_OFF;
    rgbLED_green = LED_ON;

    {
        int err;
        const char initData[] = { 1, 0x60 };
    
        fflush(stdout);
        err = i2c1.write(I2C_TCN75_ADDR | (3 << 1), initData, sizeof(initData));
        printf("init TCN75A err: %d\n", err);
        fflush(stdout);

        const char selectAmbTempReg[] = { 0 };
        err = i2c1.write(I2C_TCN75_ADDR | (3 << 1), selectAmbTempReg, sizeof(selectAmbTempReg));
    }

    // set gain
    {
        sync = 0;
        spi3.write(0x1802);
        spi3.write(0x1802);
        spi3.write(0x1802);
        spi3.write(0x1802);
        sync = 1;

        sync = 0;
        spi3.write(0x400 | 1023);
        spi3.write(0x400 | 1023);
        spi3.write(0x400 | 1023);
        spi3.write(0x400 | 1023);
        sync = 1;
    }

    while(true) {
        // toggle rgb LED
        rgbLED_green = !rgbLED_green;
        rgbLED_blue = !rgbLED_blue;

        // read air temperature sensor
        {
            int err;
            char temperatureAirRaw[2] = {0, 0};

            i2c1.start();
            err = (i2c1.write(I2C_TCN75_ADDR | (3 << 1) | 0x1) != 1);
            if (!err) {
                temperatureAirRaw[0] = i2c1.read(1);
                temperatureAirRaw[1] = i2c1.read(0);
                i2c1.stop();
            }

            float temperatureAir = temperatureAirRaw[0] + (temperatureAirRaw[1] >> 4) / 16.0f;
            printf("temperatureAir: %f  err: %d\n", temperatureAir, err);
        }

        // read ADC 4 channels
        {
            constexpr float scalingFactor = (5000.0f / 32768);
            // read raw values
            int16_t temp_ad0 = AD_DAT16;
            int16_t temp_ad1 = AD_DAT16;
            int16_t temp_ad2 = AD_DAT16;
            int16_t temp_ad3 = AD_DAT16;

            float ad0 = temp_ad0 * scalingFactor;
            float ad1 = temp_ad1 * scalingFactor;
            float ad2 = temp_ad2 * scalingFactor;
            float ad3 = temp_ad3 * scalingFactor;
            
            // start next conversion
            cnvsta = 1;
            cnvstb = 1;

            cnvsta = 0;
            cnvstb = 0;
            
            printf("ch0: %9.3f ch1: %9.3f ch2: %9.3f ch3: %9.3f\n", ad0, ad1, ad2, ad3);
        }

        ThisThread::sleep_for(1000);
    }
}
