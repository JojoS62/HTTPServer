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

#include "globalVars.h"

void WSHandler::onMessage(const char* text)
{
    //printf("TEXT: [%s]\r\n", text);

    const char msg[] = "[%f,%f,%f,%f]";
#if 0
    float f1, f2, f3, f4;
    f1 = rand() * 100.0f / RAND_MAX;
    f2 = rand() * 100.0f / RAND_MAX;
    f3 = rand() * 100.0f / RAND_MAX;
    f4 = rand() * 100.0f / RAND_MAX
#endif
    int n = snprintf(_buffer, sizeof(_buffer), msg, globalVars.adcValues[0], 
                                                    globalVars.adcValues[1], 
                                                    globalVars.adcValues[2], 
                                                    globalVars.adcValues[3]);

    
    if (_clientConnection)
        _clientConnection->sendFrame(WSop_text, (uint8_t*)_buffer, n);
}

void WSHandler::onMessage(const char* data, size_t size)
{
}

void WSHandler::onOpen(ClientConnection *clientConnection)
{
    WebSocketHandler::onOpen(clientConnection);
    clientConnection->setWSTimer(20);
    printf("%s: websocket opened\n", _clientConnection->getThreadname());
}
 
void WSHandler::onTimer()
{
    onMessage("");
}

void WSHandler::onClose()
{
    printf("%s: websocket closed\n", _clientConnection->getThreadname());
}


// WSHandler Factory
WebSocketHandler* WSHandler::createHandler()
{
    WebSocketHandler* handler = new WSHandler();
    return handler;
}
