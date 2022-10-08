/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include "RenderServer.h"
#include "TcpStream.h"
#ifdef _WIN32
#include "Win32PipeStream.h"
#else
#include "UnixStream.h"
#endif
#include "RenderThread.h"
#include "FrameBuffer.h"
#include <set>

typedef std::set<RenderThread *> RenderThreadsSet;

RenderServer::RenderServer() :
    m_listenSock(NULL),
    m_exiting(false)
{
}

RenderServer::~RenderServer()
{
    delete m_listenSock;
}


extern "C" int gRendererStreamMode;

RenderServer *RenderServer::create(char* addr, size_t addrLen)
{
    RenderServer *server = new RenderServer();
    if (!server) {
        return NULL;
    }

    if (gRendererStreamMode == STREAM_MODE_TCP) {
        server->m_listenSock = new TcpStream();
    } else {
#ifdef _WIN32
        server->m_listenSock = new Win32PipeStream();
#else
        server->m_listenSock = new UnixStream();
#endif
    }

    char addrstr[SocketStream::MAX_ADDRSTR_LEN];
    if (server->m_listenSock->listen(addrstr) < 0) {
        ERR("RenderServer::create failed to listen\n");
        delete server;
        return NULL;
    }

    size_t len = strlen(addrstr) + 1;
    if (len > addrLen) {
        ERR("RenderServer address name too big for provided buffer: %zu > %zu\n",
                len, addrLen);
        delete server;
        return NULL;
    }
    memcpy(addr, addrstr, len);

    return server;
}

int RenderServer::Main()
{
    RenderThreadsSet threads;

    while(1) {
        SocketStream *stream = m_listenSock->accept();
        if (!stream) {
            fprintf(stderr,"Error accepting connection, aborting\n");
            break;
        }

        unsigned int clientFlags;
        if (!stream->readFully(&clientFlags, sizeof(unsigned int))) {
            fprintf(stderr,"Error reading clientFlags\n");
            delete stream;
            continue;
        }

        DBG("RenderServer: Got new stream!\n");

        // check if we have been requested to exit while waiting on accept
        if ((clientFlags & IOSTREAM_CLIENT_EXIT_SERVER) != 0) {
            m_exiting = true;
            break;
        }

        RenderThread *rt = RenderThread::create(stream);
        if (!rt) {
            fprintf(stderr,"Failed to create RenderThread\n");
            delete stream;
            stream = NULL;
        } else if (!rt->start()) {
            fprintf(stderr,"Failed to start RenderThread\n");
            delete rt;
            rt = NULL;
        }

        //
        // remove from the threads list threads which are
        // no longer running
        //
        for (RenderThreadsSet::iterator n,t = threads.begin();
             t != threads.end();
             t = n) {
            // first find next iterator
            n = t;
            n++;

            // delete and erase the current iterator
            // if thread is no longer running
            if ((*t)->isFinished()) {
                delete (*t);
                threads.erase(t);
            }
        }

        // if the thread has been created and started, insert it to the list
        if (rt) {
            threads.insert(rt);
            DBG("Started new RenderThread\n");
        }
    }

    //
    // Wait for all threads to finish
    //
    for (RenderThreadsSet::iterator t = threads.begin();
         t != threads.end();
         t++) {
        int exitStatus;
        (*t)->wait(&exitStatus);
        delete (*t);
    }
    threads.clear();

    //
    // de-initialize the FrameBuffer object
    //
    FrameBuffer::finalize();
    return 0;
}
