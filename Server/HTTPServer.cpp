/*------------------------------------------------------------------------------------
---- Title: Laundry Now Web Server

---- File: HTTPServer.h

---- Function: Helper functions/definitions for Server.cpp and Client.cpp
------------------------------------------------------------------------------------*/
#if !defined(_WEBSERVER_H)
#define	_WEBSERVER_H

#include	<WProgram.h>
#include	<inttypes.h>
#include        <NetworkProfile.x>
#include        <DNETcK.h>
#include        <DWIFIcK.h>

#define OFFSETOF(t,m)         ((uint32_t) (&(((t *) 0)->m)))
#define CNTHTTPCMD            10      // The Max number of unique HTML pages we vector off of. This must be at least 1. 
#define secClientTO           3       // time in seconds to wait for a response before aborting a client connection.
#define CBCLILENTINPUTBUFF    256     // The max size of the TCP read buffer, this typically only has to be as large as the URL up to the HTTP tag in the GET line
#define CBCLILENTOUTPUTBUFF   256     // This is scratch output buffer space to generate out going HTML strings it, it is optional if not needed can be set as small as 4;
#define HTTPSTART             10000   // This is a predefine state for the rendering HTML page to initialize the state machine 
#define HTTPDISCONNECT        20001   // This state is called after the TCP connection is closed, for whatever reason even timeouts, so the HTML state machine to clean up
#define HTTPTIMEOUT           20002   // If a read timeout occured, your state machine will be called with this timeout value
#define SDREADTIMEOUT         1000    // We have waited too long to read from the SD card in ms

namespace GCMD {
    typedef enum
    {
        CONTINUE = 0,   // just call the HTML code again on the next loop
        READ,           // Read more input from the TCP connection
        GETLINE,        // Read the next line of input.
        WRITE,          // iteratively write data to the TCP connection until the buffer has been completely written
        DONE,           // The HTML page is finished and the TCP connection can be closed. 
        ERROR,          // An error occured in the underlying code
        RESTART,        // Restart the network stack
        TERMINATE,      // Terminate the server; spin
        REBOOT,         // Reboot the processor; do a soft MCLR
    } ACTION;
}

typedef GCMD::ACTION (* FNRENDERHTML) (struct CLIENTINFO_T * pClientInfo);
typedef struct CLIENTINFO_T
{
    bool            fInUse;                         // used only by the HTTP Server code to indicate that this is an active TCP client connection
    TcpClient       tcpClient;                      // the socket
    uint32_t        clientState;                    // a state machine variable for process client to use
    uint32_t        nextClientState;                // a delayed state variable for process client to use (state specific)
    uint32_t        tStartClient;                   // a timer value used for timeout
    uint32_t        cbRead;                         // number of valid bytes in rgbIn, 
    byte            rgbIn[CBCLILENTINPUTBUFF];      // The input buffer, this is where the /GET and URL will be
    byte            rgbOverflow[4];                 // some overflow space to put characters in while parsing; typically not used.
    uint32_t        htmlState;                      // a state variable for the HTML web page state machine to use; each page is different
    uint32_t        cbWrite;                        // number of bytes to write out when GCMD::WRITE is returned
    uint32_t        cbWritten;                      // a variable for process client to use to know how many bytes have been written
    byte            rgbOut[CBCLILENTOUTPUTBUFF];    // a per client working scratch output buffer space that can be used if the HTML page/data is being created dynamically. 
    const byte *    pbOut;                          // The actual pointer to the output buffer for ProcessClient to write to the TCP connection. 
                                                    // Typically this is either set to a static string in flash, or to a dynmically created string stored in rgbOut[] or another static buffer in RAM
                                                    // This is usally assigned in the HTML rendering code. If GCMD::WRITE is returned, this pointer must not be NULL.

    FNRENDERHTML    ComposeHTMLPage;
} CLIENTINFO;

void ServerSetup(void);
void ProcessServer(void);

GCMD::ACTION ProcessClient(CLIENTINFO * pClientInfo);

uint32_t BuildHTTPOKStr(bool fNoCache, uint32_t cbContentLen, const char * szFile, char * szHTTPOKStr, uint32_t cbHTTPOK);

int GetNumb(byte * rgb, int cb, char chDelim, char * sz);
int GetIP(IPv4& ip, char * sz);

#endif
