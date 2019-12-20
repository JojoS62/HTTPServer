#include "mbed.h"

//#include "network-helper.h"
#include "HttpServer.h"
#include "HttpResponseBuilder.h"
#include "WebsocketHandlers.h"

#include "threadIO.h"
#include "MQTTThreadedClient.h"

#include "SDIOBlockDevice.h"
#include "SPIFBlockDevice.h"
#include "FATFileSystem.h"
#include "LittleFileSystem.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define USE_HTTPSERVER
#define USE_TFTPSERVER
//#define USE_MQTT

#define DEFAULT_STACK_SIZE (4096)

SDIOBlockDevice bd;
FATFileSystem fs("sda", &bd);

SPIFBlockDevice spif(PB_5, PB_4, PB_3, PB_0);
LittleFileSystem lfs("sdb", &spif);

#ifdef USE_TFTPSERVER
#include "threadTFTPServer.h"
ThreadTFTPServer  threadTFTPpServer;
#endif

#define SAMPLE_TIME     1000 // milli-sec
#define COMPLETED_FLAG (1UL << 0)
PlatformMutex stdio_mutex;
EventFlags threadFlag;

void print_stats()
{
    mbed_stats_socket_t stats[MBED_CONF_NSAPI_SOCKET_STATS_MAX_COUNT];
    static int num = 0;
    int count;

    memset(stats, 0, sizeof(mbed_stats_socket_t) * MBED_CONF_NSAPI_SOCKET_STATS_MAX_COUNT);
    printf("%-15s%-15s%-15s%-15s%-15s%-15s%-15s\n", "Num", "ID", "State", "Proto", "Sent", "Recv", "Time");
    //while (COMPLETED_FLAG != threadFlag.get()) {
        count = SocketStats::mbed_stats_socket_get_each(&stats[0], MBED_CONF_NSAPI_SOCKET_STATS_MAX_COUNT);
        for (int i = 0; i < count; i++) {
            stdio_mutex.lock();
            printf("\n%-15d", num);
            printf("%-15p", stats[i].reference_id);

            switch (stats[i].state) {
                case SOCK_CLOSED:
                    printf("%-15s", "Closed");
                    break;
                case SOCK_OPEN:
                    printf("%-15s", "Open");
                    break;
                case SOCK_CONNECTED:
                    printf("%-15s", "Connected");
                    break;
                case SOCK_LISTEN:
                    printf("%-15s", "Listen");
                    break;
                default:
                    printf("%-15s", "Error");
                    break;
            }

            if (NSAPI_TCP == stats[i].proto) {
                printf("%-15s", "TCP");
            } else {
                printf("%-15s", "UDP");
            }
            printf("%-15d", stats[i].sent_bytes);
            printf("%-15d", stats[i].recv_bytes);
            printf("%-15lld\n", stats[i].last_change_tick);
            stdio_mutex.unlock();
        }
        num++;
        //ThisThread::sleep_for(SAMPLE_TIME);
    //}
    // Now allow the stats thread to simply exit by itself gracefully.
}


//DigitalOut led(LED1);

ThreadIO threadIO(100);
Thread msgSender(osPriorityNormal, DEFAULT_STACK_SIZE * 3);

Mutex mutexReqHandlerRoot;

// Requests come in here
void request_handler(HttpParsedRequest* request, ClientConnection* clientConnection) {
    mutexReqHandlerRoot.lock();
#if 1
    printf("[Http]Request came in: %s %s\n", http_method_str(request->get_method()), request->get_url().c_str());
    
    MapHeaderIterator it;
    int i = 0;

    for (it = request->headers.begin(); it != request->headers.end(); it++) {
        printf("[%d] ", i);
        printf(it->first.c_str());
        printf(" : ");
        printf(it->second.c_str());
        printf("\n");
        i++;
    }
    fflush(stdout);
#endif

    if (request->get_method() == HTTP_GET && request->get_url() == "/") {
        HttpResponseBuilder builder(200, clientConnection);
        builder.set_header("Content-Type", "text/html; charset=utf-8");

        char response[] = "<html><head><title>Hello from mbed</title></head>"
            "<body>"
                "<h1>mbed webserver</h1>"
                "<button id=\"toggle\">Format Flash with LittleFS</button>"
                "<script>document.querySelector('#toggle').onclick = function() {"
                    "var x = new XMLHttpRequest(); x.open('POST', '/toggle'); x.send();"
                "}</script>"
            "</body></html>";

        builder.send(response, sizeof(response) - 1);
    }
    else if(request->get_method() == HTTP_GET) {
        HttpResponseBuilder builder(200, clientConnection);

        builder.sendHeaderAndFile(&fs, request->get_url());
    }
    else if (request->get_method() == HTTP_POST && request->get_url() == "/toggle") {
        printf("toggle LED called\n\n");
        //led = !led;

        HttpResponseBuilder builder(200, clientConnection);
        builder.send(NULL, 0);
    }
    else {
        HttpResponseBuilder builder(404, clientConnection);
        builder.send(NULL, 0);
    }
    mutexReqHandlerRoot.unlock();
}

Mutex mutexReqHandlerStatus;

void request_handler_getStatus(HttpParsedRequest* request, ClientConnection* clientConnection) {
    mutexReqHandlerStatus.lock();
    if (request->get_method() == HTTP_GET) {
        if (request->get_filename() == "mem") {
            mbed_stats_heap_t heap_info;
            mbed_stats_heap_get( &heap_info );

            HttpResponseBuilder builder(200, clientConnection);
            builder.set_header("Content-Type", "application/json");

            string response;
            response.reserve(512);

            response += "{\"current_size\": ";
            response += to_string(heap_info.current_size);
            response += ", \"max_size\": ";
            response += to_string(heap_info.max_size);
            response += ", \"alloc_cnt\": ";
            response += to_string(heap_info.alloc_cnt);
            response += ", \"reserved_size\": ";
            response += to_string(heap_info.reserved_size);
            response += "}";

            builder.send(response.c_str(), response.length());
        }
        if (request->get_filename() == "test") {
            HttpResponseBuilder builder(200, clientConnection);
            builder.set_header("Content-Type", "application/json");

            string response;
            response.reserve(512);

            response += "{\"test\": 42}";

            builder.send(response.c_str(), response.length());
        }
        else {
            HttpResponseBuilder builder(404, clientConnection);
            builder.send(NULL, 0);
        }
    }
    else {
        HttpResponseBuilder builder(404, clientConnection);
        builder.send(NULL, 0);
    }
    mutexReqHandlerStatus.unlock();
}

#ifdef USE_MQTT
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

void formatSPIFlash()
{
    spif.init();
    spif.erase(0, spif.get_erase_size());
    spif.deinit();
    
    lfs.format(&spif);
}


void print_dir(FileSystem *fs, const char* dirname) {
    Dir dir;
    struct dirent ent;

    dir.open(fs, dirname);
    printf("contents of dir: %s\n", dirname);
    printf("----------------------------------------------------\n");

    while (1) {
        size_t res = dir.read(&ent);
        if (0 == res) {
            break;
        }
        printf(ent.d_name);
        printf("\n");
    }
    dir.close();
}

void print_SPIF_info() 
{
        spif.init();
        printf("spif size: %llu\n",         spif.size());
        printf("spif read size: %llu\n",    spif.get_read_size());
        printf("spif program size: %llu\n", spif.get_program_size());
        printf("spif erase size: %llu\n",   spif.get_erase_size());
        spif.deinit();
}

int main() {
    printf("Hello from "  TOSTRING(TARGET_NAME) "\n");
    printf("Mbed OS version: %d.%d.%d\n\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);

    print_SPIF_info();
    printf("\n");

    print_dir(&lfs, "/");
    printf("\n");

    print_dir(&fs, "/htmlRoot");
    printf("\n");

    // IO Thread
    threadIO.start();
    
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

    // show threadstatistics
    //Thread *thread = new Thread(osPriorityNormal1, 2048);
    //thread->start(print_stats);

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

    while(true) {
        ThisThread::sleep_for(10000);
    }
}
