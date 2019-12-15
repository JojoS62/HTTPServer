/* 
 * Copyright (c) 2019 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "WebsocketHandlers.h"
#include "ClientConnection.h"
/*
    Websocket Test
*/

void WSHandler::onMessage(const char* text)
{
    //printf("TEXT: [%s]\r\n", text);

    mbed_stats_heap_t heap_info;
    mbed_stats_heap_get( &heap_info );
    //int wsCount = _clientConnection->getServer()->getWebsocketCount();    

    //const char msg[] = "{\"allocated\": %lu, \"allocations\": %lu, \"ws_count\": %i, \"client_connection\": %x }";
    //int n = snprintf(_buffer, sizeof(_buffer), msg, heap_info.current_size, heap_info.alloc_cnt, wsCount, (uint)_clientConnection);

    const char msg[] = "%f,%f,%f,%f\n";
    float f1, f2, f3, f4;
    f1 = rand() * 100.0f / RAND_MAX;
    f2 = rand() * 100.0f / RAND_MAX;
    f3 = rand() * 100.0f / RAND_MAX;
    f4 = rand() * 100.0f / RAND_MAX;
    int n = snprintf(_buffer, sizeof(_buffer), msg, f1, f2, f3, f4);

    
    if (_clientConnection)
        _clientConnection->sendFrame(WSop_text, (uint8_t*)_buffer, n);
}

void WSHandler::onMessage(const char* data, size_t size)
{
    //int8_t lv = data[0];
    //int8_t rv = data[1];

 
}

void WSHandler::onOpen(ClientConnection *clientConnection)
{
    WebSocketHandler::onOpen(clientConnection);
    clientConnection->setWSTimer(10);
    printf("websocket opened\r\n");
}
 
void WSHandler::onTimer()
{
    onMessage("");
}

void WSHandler::onClose()
{
    printf("websocket closed\r\n");
}


// WSHandler Factory
WebSocketHandler* WSHandler::createHandler()
{
    WebSocketHandler* handler = new WSHandler();
    return handler;
}
