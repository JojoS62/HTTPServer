#include "HTTPhandlers.h"
#include "globalVars.h"

PlatformMutex stdio_mutex;

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

Mutex mutexReqHandlerRoot;

// Requests come in here
void request_handler(HttpParsedRequest* request, ClientConnection* clientConnection) {
    mutexReqHandlerRoot.lock();
    if (request->get_method() == HTTP_GET && request->get_url() == "/") {
        HttpResponseBuilder builder(200, clientConnection);
        builder.set_header("Content-Type", "text/html; charset=utf-8");
        builder.sendHeader();

        string body = 
            "<html><head><title>Hello from mbed</title></head>"
            "<body>"
                "<h1>mbed webserver</h1>"
                "<button id=\"toggle\">Format Flash with LittleFS</button>"
                "<script>document.querySelector('#toggle').onclick = function() {"
                    "var x = new XMLHttpRequest(); x.open('POST', '/toggle'); x.send();"
                "}</script>"
            "</body></html>";

        builder.sendBodyString(body);
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
            builder.sendHeader();

            string body;
            body.reserve(512);

            body += "{\"current_size\": ";
            body += to_string(heap_info.current_size);
            body += ", \"max_size\": ";
            body += to_string(heap_info.max_size);
            body += ", \"alloc_cnt\": ";
            body += to_string(heap_info.alloc_cnt);
            body += ", \"reserved_size\": ";
            body += to_string(heap_info.reserved_size);
            body += "}";

            builder.sendBodyString(body);
        } else
        if (request->get_filename() == "cpu") {
            mbed_stats_cpu_t stats;
            mbed_stats_cpu_get(&stats);

            HttpResponseBuilder builder(200, clientConnection);
            builder.set_header("Content-Type", "application/json");
            builder.sendHeader();

            string body;
            body.reserve(512);

            body += "{\"uptime\": ";
            body += to_string(stats.uptime / 1000000);
            body += ", \"idle_time\": ";
            body += to_string(stats.idle_time / 1000000);
            body += ", \"sleep_time\": ";
            body += to_string(stats.sleep_time / 1000000);
            body += ", \"deep_sleep_time\": ";
            body += to_string(stats.deep_sleep_time / 1000000);
            body += "}";

            builder.sendBodyString(body);
        } else
        if (request->get_filename() == "test") {
            HttpResponseBuilder builder(200, clientConnection);
            builder.set_header("Content-Type", "application/json");
            builder.sendHeader();

            string body;
            body.reserve(512);

            body += "{\"test\": 42}";

            builder.sendBodyString(body);
        } else {
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
