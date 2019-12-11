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
/*
    Websocket Test
*/

void WSHandler::onMessage(char* text)
{
    //printf("TEXT: [%s]\r\n", text);

    mbed_stats_heap_t heap_info;
    mbed_stats_heap_get( &heap_info );
    int wsCount = _clientConnection->getServer()->getWebsocketCount();    

    const char msg[] = "{\"allocated\": %lu, \"allocations\": %lu, \"ws_count\": %i, \"client_connection\": %x }";
    int n = snprintf(_buffer, sizeof(_buffer), msg, heap_info.current_size, heap_info.alloc_cnt, wsCount, (uint)_clientConnection);

    
    if (_clientConnection)
        _clientConnection->sendFrame(WSop_text, (uint8_t*)_buffer, n);
}

void WSHandler::onMessage(char* data, size_t size)
{
    //int8_t lv = data[0];
    //int8_t rv = data[1];

 
}

void WSHandler::onOpen(ClientConnection *clientConnection)
{
    WebSocketHandler::onOpen(clientConnection);

    printf("websocket opened\r\n");
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
