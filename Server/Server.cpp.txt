/*------------------------------------------------------------------------------------
---- Title: Laundry Now Web Server

---- File: Server.cpp

---- Function: Handles servevr side functions of the web server
------------------------------------------------------------------------------------*/
#include    <HTTPServer.h>

#define CBDATETIME          32
#define TooMuchTime()       (millis() > cMSecEnd)
#define RestartTimer()      cMSecEnd = millis() + cSecTimeout * 1000
#define SetTimeout(cSec)    {cSecTimeout = cSec;}

#define cSecDefault         60
#define cSecInitRest        30
#define cSecIncRest         60
#define cSecMaxRest         600
#define cMaxSocketsToListen 6
#define    LD6      PORTGbits.RG6

static IPv4 ipMyStatic = {0,0,0,0};
static byte localStaticIP = 195;
static unsigned short listeningPort = 80; 
static IPv4 ipGateway   = {192,168,1,1};
static IPv4 subnetMask  = {255,255,255,0};
static IPv4 ipDns1      = {8,8,8,8};
static IPv4 ipDns2      = {8,8,4,4}; 
static int cSecTimeout = cSecDefault;
static int cMSecEnd = 0;
static int cSecRest = cSecInitRest;
static IPv4         ipMy;
static IPEndPoint   ipEP;
static IPv4         ipRemote;
static char *       szRemoteURL;

extern char szSsid[30];
extern char szPassPhrase[30];

typedef enum 
{

    NONE = 0,
    STARTSTATE,
    STARTWIFISCAN,
    WAITFORSCAN,
    WIFICONNECT,
    WIFICONNECTED,
    WIFIKEYGEN,
    DYNAMICIPBEGIN,
    STATICIPBEGIN,
    ENDPASS1,
    INITIALIZE,
    WAITFORTIME,
    GETNETADDRESSES,
    PRINTADDRESSES,
    MAKESTATICIP,
    STARTLISTENING,
    LISTENING,
    NOTLISTENING,
    CHECKING,
    RESTARTNOW,
    RESTARTREST,
    TERMINATE,
    SHUTDOWN,
    RESTFORAWHILE,
    TRYAGAIN,
    DONOTHING,
    REBOOT,
    SOFTMCLR,
    MCLRWAIT,
    WAITFORTIMEOUT,
    ERROR    
} STATECMD;

#if defined(USING_WIFI)

DWIFIcK::SECINFO secInfo;
extern int conID = DWIFIcK::INVALID_CONNECTION_ID;
#endif


STATECMD state = STARTWIFISCAN;
STATECMD stateNext = RESTARTREST;
STATECMD stateTimeOut = RESTARTREST;

DNETcK::STATUS status = DNETcK::None;
unsigned int epochTime = 0;

int  cNetworks = 0;
int iNetwork = 0;

static uint32_t iNextClientToProcess = 0;
static uint32_t cWorkingClients = 0;
TcpServer tcpServer(cMaxSocketsToListen);

static CLIENTINFO rgClient[cMaxSocketsToListen];
static char szTemp[256];

void ServerSetup(void) 
{
    int i = 0;     

    DNETcK::setDefaultBlockTime(DNETcK::msImmediate); 
    
#if defined(USING_WIFI)
    state = STARTWIFISCAN;
#else
    state = DYNAMICIPBEGIN;
#endif

    SetTimeout(cSecDefault);
    stateNext = RESTARTREST;
    stateTimeOut = RESTARTREST;
    RestartTimer();

    cSecRest = cSecInitRest;

    for(i=0; i<cMaxSocketsToListen; i++)
    {
        rgClient[i].fInUse = false;
        rgClient[i].tcpClient.close();
    }
    cWorkingClients = 0;
}

void ProcessServer(void)
{   
    int i               = 0;
    int cbAvailable     = 0;

  if(stateTimeOut != NONE && TooMuchTime())
  {
    printf("Timeout occured\r\n");
    state = stateTimeOut;
    stateTimeOut = NONE;
    stateNext = RESTARTREST;
  }

  switch(state)
  {    
    
#if defined(USING_WIFI)
        case STARTWIFISCAN:
            printf("Start WiFi Scan\r\n");
            DWIFIcK::beginScan();
            state = WAITFORSCAN;
            RestartTimer();
            LD6 = 0;
            break;

        case WAITFORSCAN:
            iNetwork = 0;
            if(DWIFIcK::isScanDone(&cNetworks, &status))
            {
                state = WIFICONNECT;
                RestartTimer();
            }
            else if(DNETcK::isStatusAnError(status))
            {
                printf("Scan Failed\r\n");
               
                state = WIFICONNECT;
                RestartTimer();
            }
            break;

        case WIFICONNECT:
            printf("Preparing To Connect\r\n");
     
            if((conID = DWIFIcK::connect(szSsid, szPassPhrase, &status)) != DWIFIcK::INVALID_CONNECTION_ID)
            {
                LD6 = 0;
                printf("Connection Created\r\n");
                printf("Establishing Connection...\r\n");
                state = WIFICONNECTED;
                RestartTimer();
            }
            else
            {
                printf("Incorrect Network Information\r\n");
                state = ERROR;
                RestartTimer();
            }
            break;

        case WIFICONNECTED:
            if(DWIFIcK::isConnected(conID, &status))
            {
                printf("Connection Successful\r\n");
                

                state = WIFIKEYGEN;
                cSecRest = cSecInitRest;
                RestartTimer();
            }
            else if(DNETcK::isStatusAnError(status))
            {
                printf("WiFi not connected\r\n");
                
                state = RESTARTREST;
                RestartTimer();
            }
            break;

    case WIFIKEYGEN:
      
        DWIFIcK::getSecurityInfo(conID, &secInfo);
      
        state = DYNAMICIPBEGIN;
        RestartTimer();
        break;
#endif
    case DYNAMICIPBEGIN:

        if(ipMyStatic.u32IP == 0)
        {
            printf("Initializing Network...\r\n");

            DNETcK::begin();
            state = INITIALIZE;
        }

        else
        {
            state = STATICIPBEGIN;
        }

        RestartTimer();
        break;

    case STATICIPBEGIN: 
        
        DNETcK::begin(ipMyStatic, ipGateway, subnetMask, ipDns1, ipDns2);    
        state = INITIALIZE;
        RestartTimer();
        break;

    case INITIALIZE:

        if(DNETcK::isInitialized(&status))
            {
                
                state = GETNETADDRESSES;
                
                RestartTimer();
            }
        else if(DNETcK::isStatusAnError(status))
            {
                printf("Not Initialized\r\n");
                state = ERROR;
                RestartTimer();
            }
        break;

    case GETNETADDRESSES:

        DNETcK::getMyIP(&ipMy);
        DNETcK::getGateway(&ipGateway);
        DNETcK::getSubnetMask(&subnetMask);
        DNETcK::getDns1(&ipDns1);
        DNETcK::getDns2(&ipDns2);

        if(ipMyStatic.u32IP == 0)
        {
            state = MAKESTATICIP;
        }
        else
        {
            stateTimeOut = PRINTADDRESSES;
            state = WAITFORTIME;
        }

        RestartTimer();
        break;
    
    case MAKESTATICIP:

        ipMyStatic = ipGateway;
        ipMyStatic.rgbIP[3] = localStaticIP;

        if(localStaticIP != 0 && (ipMyStatic.u32IP & subnetMask.u32IP) == (ipGateway.u32IP & subnetMask.u32IP))
        {
            state = ENDPASS1;
        }

        else
        {
            ipMyStatic = ipMy;
            stateTimeOut = PRINTADDRESSES;
            state = WAITFORTIME;
        }
        RestartTimer();
        break;

    case ENDPASS1:

        

        for(i=0; i<cMaxSocketsToListen; i++)
        {
            rgClient[i].fInUse = false;
            rgClient[i].tcpClient.close();
        }
        tcpServer.close();

        DNETcK::end();

#if defined(USING_WIFI) && defined(RECONNECTWIFI)

        DWIFIcK::disconnect(conID);
        state = WIFICONNECTWITHKEY;
#else
        state = STATICIPBEGIN;
#endif 

        stateTimeOut = RESTARTREST;
        RestartTimer();
        break;

    case WAITFORTIME:
        epochTime = DNETcK::secondsSinceEpoch(&status); 
        if(status == DNETcK::TimeSinceEpoch)
        {
            //GetDayAndTime(epochTime, szTemp);
            //Serial.println(szTemp);
            state = PRINTADDRESSES;
            RestartTimer();
        }

        break;

    case PRINTADDRESSES:

        Serial.println("");

        {
            IPv4    ip;
            MAC     mac;

            DNETcK::getMyIP(&ip);
            Serial.print("My ");
            GetIP(ip, szTemp);
            Serial.println(szTemp);

            DNETcK::getGateway(&ip);
            Serial.print("Gateway ");
            GetIP(ip, szTemp);
            Serial.println(szTemp);

            DNETcK::getSubnetMask(&ip);
            Serial.print("Subnet mask: ");
            GetNumb(ip.rgbIP, 4, '.', szTemp);
            Serial.println(szTemp);

            DNETcK::getDns1(&ip);
            Serial.print("Dns1 ");
            GetIP(ip, szTemp);
            Serial.println(szTemp);

            DNETcK::getDns2(&ip);
            Serial.print("Dns2 ");
            GetIP(ip, szTemp);
            Serial.println(szTemp);

            DNETcK::getMyMac(&mac);
            //Serial.print("My ");
            //GetMAC(mac, szTemp);
            //Serial.println(szTemp);

            Serial.println("");
        }

        stateTimeOut = RESTARTREST;
        RestartTimer();
        state = STARTLISTENING;
        break;

    case STARTLISTENING:   

        tcpServer.startListening(listeningPort);
        state = LISTENING;
        RestartTimer();
        break;

    case LISTENING:
        if(tcpServer.isListening(&status))
        {          
            IPv4    ip;

            Serial.print("Listening on ");
            LD6 = 1;
            DNETcK::getMyIP(&ip);
            GetIP(ip, szTemp);
            Serial.print(szTemp);
            Serial.print(":");
            Serial.println(listeningPort, DEC);
            Serial.println();

            state = CHECKING;
            
            stateTimeOut = NONE;
        }
        else if(DNETcK::isStatusAnError(status))
        {
            state = NOTLISTENING;
            RestartTimer();
        }
        break;
 
    case NOTLISTENING:

        
        Serial.println("Not Listening");

        SetTimeout(cSecDefault);
        stateTimeOut = RESTARTREST;

        switch(status)
        {
        case DNETcK::MoreCurrentlyPendingThanAllowed:
            Serial.println("Exceeded the maximum number of connections");
            state = LISTENING;
            break;
        case DNETcK::WaitingConnect:
        case DNETcK::WaitingReConnect:
            Serial.println("Waiting connection");
            state = LISTENING;
            break;
        case DNETcK::NotConnected:
            Serial.println("No longer connected");
            state = RESTARTREST;
            break;
        default:
            Serial.print("Other not-listening status: ");
            Serial.println(status, DEC);
            state = RESTARTREST;
            break;
        }
        Serial.println("");
        RestartTimer();  
        break;

    case CHECKING:   
        if(tcpServer.availableClients() > 0)
        {       

            i = iNextClientToProcess;
            do {
                if(!rgClient[i].fInUse)
                {
                    memset(&rgClient[i].clientState, 0, sizeof(CLIENTINFO) - OFFSETOF(CLIENTINFO, clientState));
                    if(tcpServer.acceptClient(&rgClient[i].tcpClient))
                    {
                        Serial.print("Got a client: 0x");
                        Serial.println((uint32_t) &rgClient[i], HEX);
                        rgClient[i].fInUse = true;

                        RestartTimer();   
                    }
                    else
                    {
                        Serial.println("Failed to get client");
                    }
                    break;
                }
                ++i %=  cMaxSocketsToListen;
            } while(i != iNextClientToProcess);
        }
        else if(!tcpServer.isListening(&status))
        {
            state = NOTLISTENING;
        }
        break;
      
    case RESTARTNOW:
        stateTimeOut = NONE;
        stateNext = TRYAGAIN;
        state = SHUTDOWN;
        break;

    case TERMINATE:
        stateTimeOut = NONE;
        stateNext = DONOTHING;
        state = SHUTDOWN;
        break;

    case REBOOT:
        stateTimeOut = NONE;
        stateNext = MCLRWAIT;
        state = SHUTDOWN;
        break;

    case RESTARTREST:
        stateTimeOut = NONE;
        stateNext = RESTFORAWHILE;
        state = SHUTDOWN;
        break;

    case SHUTDOWN:
 
        

        Serial.println("Shutting down");

        for(i=0; i<cMaxSocketsToListen; i++)
        {
            rgClient[i].fInUse = false;
            rgClient[i].tcpClient.close();
        }
        tcpServer.close();

        DNETcK::end();


#if defined(USING_WIFI)

        DWIFIcK::disconnect(conID);
#endif

        stateTimeOut = NONE;
        state = stateNext;
        stateNext = RESTARTREST;
        break;

    case RESTFORAWHILE:
        {
            static bool fFirstEntry = true;
            static bool fPrintCountDown = true;
            static unsigned int tRestingStart = 0;
            uint32_t tCur = millis();

            if(fFirstEntry)
            {
                fFirstEntry = false;
                fPrintCountDown = true;
                printf("Resting for \r\n");
                printf("%f\r\n",cSecRest);
                printf(" seconds\r\n");
                tRestingStart = tCur;
                stateTimeOut = NONE;
            }

            else if((tCur - tRestingStart) >= (cSecRest * 1000))
            {
                fFirstEntry = true;
                fPrintCountDown = true;

                printf("Done resting\r\n");

                cSecRest += cSecIncRest;
                if(cSecRest > cSecMaxRest) cSecRest = cSecMaxRest;

                SetTimeout(cSecDefault);
                state = TRYAGAIN;
             }

            else if(((tCur - tRestingStart) % 10000) == 0)
            {
                if(fPrintCountDown)
                {
                    printf("%f\r\n",cSecRest - ((tCur - tRestingStart)/1000));
                    printf(" seconds until restart.\r\n");
                    fPrintCountDown = false;
                }
            }

            else
            {
                fPrintCountDown = true;
            }
        }
        break;

    case TRYAGAIN:
        stateNext = RESTARTREST;
        stateTimeOut = RESTARTREST;
#if defined(USING_WIFI)
        state = WIFICONNECT;
#else
        state = STATICIPBEGIN;
#endif 
        RestartTimer();
        break;

    case DONOTHING:
        stateTimeOut = NONE;
        stateNext = DONOTHING;
        break;

    case WAITFORTIMEOUT:
        break;

    case MCLRWAIT:
        stateTimeOut = SOFTMCLR;
        state = WAITFORTIMEOUT;
        SetTimeout(1);
        RestartTimer();
        break;

    case SOFTMCLR:
        executeSoftReset(RUN_SKETCH_ON_BOOT);
        stateTimeOut = NONE;
        stateNext = DONOTHING;
        break;

    case ERROR:
    default:
        
        printf("Hard Error status #");
        printf("%s",status);
        printf(" occurred.\r\n");
        stateTimeOut = NONE;
        state = RESTARTREST;
        break;
    }

    cWorkingClients = 0;
    i = iNextClientToProcess;
    do {
        if(rgClient[i].fInUse)
        {
            cWorkingClients++;
            
            switch(ProcessClient(&rgClient[i]))
            {
                case GCMD::RESTART:
                    state = RESTARTNOW;
                    break;

                case GCMD::TERMINATE:
                    state = TERMINATE;
                    break;

                case GCMD::REBOOT:
                    state = REBOOT;
                    break;

                case GCMD::CONTINUE:
                case GCMD::READ:
                case GCMD::WRITE:  
                case GCMD::DONE:  
                default:
                    break;
            }
            iNextClientToProcess = (i + 1) % cMaxSocketsToListen;
            break;
        }
        ++i %=  cMaxSocketsToListen;
    } while(i != iNextClientToProcess);
    
    DNETcK::periodicTasks();
  
}

int GetIP(IPv4& ip, char * sz)
{
    int cch = 0; 

    strcpy(sz, "IP: ");
    cch = strlen(sz);

    return(cch + GetNumb(ip.rgbIP, 4, '.', &sz[cch]));
}

int GetNumb(byte * rgb, int cb, char chDelim, char * sz)
{
    int cch = 0;

    for(int i=0; i<cb; i++)
    {
        if(chDelim == ':' && rgb[i] < 16)
        {
            sz[cch++] = '0';
        }
    
        if(chDelim == ':')
        {
            itoa(rgb[i], &sz[cch], 16);
        }
        else
        {
            itoa(rgb[i], &sz[cch], 10);
        }  
        cch += strlen(&sz[cch]);
   
        if(i < cb-1)
        {
            sz[cch++] = chDelim;  
        }
    }
    sz[cch] = 0;

    return(cch);
}

