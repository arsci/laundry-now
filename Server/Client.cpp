/*------------------------------------------------------------------------------------
---- Title: Laundry Now Web Server

---- File: Client.cpp

---- Function: Handles the client side functions of the web server
------------------------------------------------------------------------------------*/
#include    <HTTPServer.h>

typedef struct 
{
    const char *    szMatchString;
    uint32_t        cbMatch;
    FNRENDERHTML    ComposeHTMLPage;
} HTTPCMD;

GCMD::ACTION ComposeHTMLHome(CLIENTINFO * pClientInfo);

static HTTPCMD rgHttpCmd[CNTHTTPCMD];
static FNRENDERHTML DefaultHTMLPage = ComposeHTMLHome;

typedef enum
{
    START           = 0,
    READINPUT,
    PARSENEXTLINE,
    WAITCMD,
    DEFAULTCMD,
    DISPLAYTIME,
    PROCESSHTML,
    WRITEBUFFER,
    STOPCLIENT,     // any state after the connection is lost must be placed below here 
    EXIT,           // just finishing up the client processing
    RESTART,        // want to restart the server
    TERMINATE,      // want to terminate the server
    REBOOT          // reboot the processor; MCLR
} CLIENTSTATE;


GCMD::ACTION ProcessClient(CLIENTINFO * pClientInfo)
{   
    GCMD::ACTION    action = GCMD::CONTINUE;
    DNETcK::STATUS  status = DNETcK::None;
    uint32_t        tCur = millis();

    if(pClientInfo->clientState != START && (tCur - pClientInfo->tStartClient)  >= (secClientTO * 1000))
    {
        printf("Timeout on client: 0x");
        printf("%08x\r\n",pClientInfo);
        pClientInfo->nextClientState    =   EXIT;

        if(pClientInfo->cbRead == 0)
        {
            pClientInfo->clientState        =   STOPCLIENT;
        }

        else if(pClientInfo->ComposeHTMLPage == NULL)
        {
            pClientInfo->clientState        =   DEFAULTCMD;
        }
        else
        {
            pClientInfo->clientState        =   PROCESSHTML;
            pClientInfo->htmlState          =   HTTPTIMEOUT;
        }
    }

    if(((CLIENTSTATE) pClientInfo->clientState < STOPCLIENT)  && !pClientInfo->tcpClient.isConnected())
    {
        printf("Connection Lost\r\n");
        pClientInfo->clientState        = STOPCLIENT;
        pClientInfo->tStartClient       = tCur;
        pClientInfo->nextClientState    = EXIT;
    }

    switch((CLIENTSTATE) pClientInfo->clientState)
    {   
        case START:
            printf("New Client detected\r\n");

            pClientInfo->pbOut              =   pClientInfo->rgbOut;
            pClientInfo->htmlState          =   HTTPSTART;                  // forces the initialization state to be executed
            pClientInfo->tStartClient       =   tCur;
            pClientInfo->clientState        =   READINPUT;
            pClientInfo->nextClientState    =   WAITCMD;
            break;

        case READINPUT:

                if(pClientInfo->tcpClient.available() >= 0 && pClientInfo->cbRead < sizeof(pClientInfo->rgbIn))
                {                        
                    pClientInfo->cbRead += pClientInfo->tcpClient.readStream(&pClientInfo->rgbIn[pClientInfo->cbRead], sizeof(pClientInfo->rgbIn) - pClientInfo->cbRead);
                }
                pClientInfo->clientState = pClientInfo->nextClientState;
                break;

        case WAITCMD:

            pClientInfo->clientState        =   READINPUT;
            pClientInfo->nextClientState    =   WAITCMD;

            if(pClientInfo->cbRead > 0)
            {
                bool fPartialMatch = false;

                for(uint32_t i = 0; i < CNTHTTPCMD; i++) 
                {

                    if(rgHttpCmd[i].cbMatch > 0)
                    {
                        uint32_t cbCmp = rgHttpCmd[i].cbMatch > pClientInfo->cbRead ? pClientInfo->cbRead : rgHttpCmd[i].cbMatch;

                        if(memcmp(pClientInfo->rgbIn, rgHttpCmd[i].szMatchString, cbCmp) == 0)
                        {

                            fPartialMatch = true;

                            if(cbCmp == rgHttpCmd[i].cbMatch)
                            {
                                pClientInfo->ComposeHTMLPage = rgHttpCmd[i].ComposeHTMLPage;
                                pClientInfo->clientState = DISPLAYTIME;
                                pClientInfo->tStartClient = tCur;

                                break;
                            }
                        }
                    }
                }

                if(pClientInfo->clientState == READINPUT)
                {

                    if(!fPartialMatch || pClientInfo->cbRead == sizeof(pClientInfo->rgbIn) ||
                        (pClientInfo->cbRead >= 4 && 
                        pClientInfo->rgbIn[pClientInfo->cbRead-4] == '\r' && pClientInfo->rgbIn[pClientInfo->cbRead-3] == '\n' && 
                        pClientInfo->rgbIn[pClientInfo->cbRead-2] == '\r' && pClientInfo->rgbIn[pClientInfo->cbRead-1] == '\n'  ) )
                    {
                        pClientInfo->clientState = DEFAULTCMD;
                        pClientInfo->tStartClient = tCur;
                    }
                }
             }
            break;

        case DEFAULTCMD:
            pClientInfo->ComposeHTMLPage = DefaultHTMLPage;
            pClientInfo->clientState = DISPLAYTIME;
            pClientInfo->tStartClient = tCur;
            break;

        case DISPLAYTIME:
            {
                unsigned int epochTime = 0;
                char szTemp[256];

                epochTime = DNETcK::secondsSinceEpoch();  
                pClientInfo->clientState = PROCESSHTML;   
                pClientInfo->tStartClient = tCur;
            }
            break;

        case PROCESSHTML:

            switch((action = pClientInfo->ComposeHTMLPage(pClientInfo)))
            {

                case GCMD::CONTINUE:
                    pClientInfo->clientState = PROCESSHTML;
                    break;

                case GCMD::READ:
                    pClientInfo->nextClientState    =   PROCESSHTML;
                    pClientInfo->clientState        =   READINPUT;
                    break;

                case GCMD::WRITE:
                    pClientInfo->nextClientState    = PROCESSHTML;
                    pClientInfo->clientState        = WRITEBUFFER;
                    break;

                case GCMD::DONE:
                    pClientInfo->nextClientState    = EXIT;         // we are done!
                    pClientInfo->clientState        = STOPCLIENT;
                    break;

                case GCMD::RESTART:
                    pClientInfo->nextClientState    = RESTART;      // we want to terminate the server
                    pClientInfo->clientState        = STOPCLIENT;
                    break;

                case GCMD::TERMINATE:
                    pClientInfo->nextClientState    = TERMINATE;   // we want to terminate the server
                    pClientInfo->clientState        = STOPCLIENT;
                    break;
     
                case GCMD::REBOOT:
                    pClientInfo->nextClientState    = REBOOT;       // we want to reboot the server
                    pClientInfo->clientState        = STOPCLIENT;
                    break;
     
                case GCMD::GETLINE:
                    pClientInfo->clientState        = PARSENEXTLINE;
                    break;

                default:
                    printf("Unsupported compose command detected: ");
                    printf("%zu",action);
                    pClientInfo->nextClientState    = EXIT;     // we are done!
                    pClientInfo->clientState        = STOPCLIENT;
                    break;
            }

            pClientInfo->cbWritten          = 0;
            pClientInfo->tStartClient       = tCur;
            break;

        case PARSENEXTLINE:
            {
                uint32_t    i                   = 0;
                boolean     fFoundNewLine       = false;
                boolean     fNullTerminator     = false;
 
                for(i = 0; i < pClientInfo->cbRead; i++)
                {
                    if( (fFoundNewLine = pClientInfo->rgbIn[i] == '\n')  ||
                        (fNullTerminator = pClientInfo->rgbIn[i] == '\0')                                   )
                    {
                         break;
                    }
                }

                if(fFoundNewLine)
                {
                    pClientInfo->rgbIn[i] = '\0';
                    if(i > 0 && pClientInfo->rgbIn[i-1] == '\r')
                    {
                        pClientInfo->rgbIn[i-1] = '\0';
                    }
                    pClientInfo->clientState = PROCESSHTML; 
                }

                else if(fNullTerminator)
                {

                    for( ; i < pClientInfo->cbRead && pClientInfo->rgbIn[i] == '\0'; i++);
 
                    if(i == pClientInfo->cbRead)
                    {
                        pClientInfo->cbRead             = 0;
                        pClientInfo->clientState        = READINPUT;
                        pClientInfo->nextClientState    = PARSENEXTLINE;
                        if(pClientInfo->rgbOverflow[0] != '\0');
                        {
                            pClientInfo->rgbIn[0] = pClientInfo->rgbOverflow[0];
                            pClientInfo->rgbOverflow[0] = '\0';
                            pClientInfo->cbRead = 1;
                        }
                    }

                    else
                    {
                        pClientInfo->cbRead -= i;
                        memcpy(pClientInfo->rgbIn, &pClientInfo->rgbIn[i], pClientInfo->cbRead);
                    }
                }

                else if(i == sizeof(pClientInfo->rgbIn))
                {

                    pClientInfo->rgbOverflow[0] = pClientInfo->rgbIn[sizeof(pClientInfo->rgbIn)-1];
                    pClientInfo->rgbIn[sizeof(pClientInfo->rgbIn)-1] = '\0';
                    pClientInfo->clientState = PROCESSHTML; 
                }

                else
                {
                    pClientInfo->clientState        = READINPUT;
                    pClientInfo->nextClientState    = PARSENEXTLINE;
                }
            }

            break;

        case WRITEBUFFER:

            if(pClientInfo->cbWritten == pClientInfo->cbWrite)
            {
                pClientInfo->clientState        = pClientInfo->nextClientState;
                pClientInfo->nextClientState    = STOPCLIENT;
                break;
            }

            pClientInfo->cbWritten += pClientInfo->tcpClient.writeStream(&pClientInfo->pbOut[pClientInfo->cbWritten], pClientInfo->cbWrite - pClientInfo->cbWritten, &status);

            if(DNETcK::isStatusAnError(status))
            {
                pClientInfo->clientState        = STOPCLIENT;
                pClientInfo->nextClientState    = EXIT;
            }

            else if(pClientInfo->cbWritten == pClientInfo->cbWrite)
            {
                pClientInfo->clientState        = pClientInfo->nextClientState;
                pClientInfo->nextClientState    = STOPCLIENT;
            }

            pClientInfo->tStartClient = tCur;
            break;

         case STOPCLIENT:  
            printf("Closing connection for client: 0x");
            printf("%08x\r\n",pClientInfo);
            pClientInfo->tcpClient.close();
            if(pClientInfo->ComposeHTMLPage != NULL)
            {
                pClientInfo->htmlState = HTTPDISCONNECT;
                pClientInfo->ComposeHTMLPage(pClientInfo);
            }
            pClientInfo->clientState = pClientInfo->nextClientState;
            pClientInfo->tStartClient = tCur;
            break;
                     
        case EXIT:
            pClientInfo->fInUse             =   false;
            break;

        case RESTART:
            printf("Restart Commanded\r\n");
            pClientInfo->fInUse             =   false;
            return(GCMD::RESTART);
            break;

        case TERMINATE:
            printf("Termination Commanded\r\n");
            pClientInfo->fInUse             =   false;
            return(GCMD::TERMINATE);
            break;

        case REBOOT:
            printf("Reboot Commanded\r\n");
            pClientInfo->fInUse             =   false;
            return(GCMD::REBOOT);
            break;

        default:
            printf("Unknown client state: ");
            printf("%f",pClientInfo->clientState);
            pClientInfo->clientState        = STOPCLIENT;
            pClientInfo->nextClientState    = EXIT;
            pClientInfo->tStartClient       = tCur;
            break;

    }

    return(GCMD::CONTINUE);
}
