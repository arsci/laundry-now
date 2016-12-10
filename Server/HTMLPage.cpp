/*------------------------------------------------------------------------------------
---- Title: Laundry Now Web Server

---- File: HTMLPage.cpp

---- Function: Code that stores the temporary HTML file and generates the final
               page to be returned to the user.
------------------------------------------------------------------------------------*/
#include    <HTTPServer.h>
#include    <stdlib.h>
#include    <string.h>

extern int sensors[4];
extern unsigned long controller_raw[4];
extern unsigned int controller_change[4][8];
extern unsigned int machine_cycles[30][4][8];
extern int cycles_day[4][8];
extern int cycles_week[4][8];
extern int cycles_month[4][8];
extern int cycles_year[4][8];
extern char time[35];
char YEL[] = "#F3E707";
char RED[] = "#FF0000";
char GRN[] = "#32CD32";
char BLK[] = "#000000";
char BLU[] = "#4169E1";
char COMPL[] = "Just Finished  ";
char AVAIL[] = "Finished       ";
char INUSE[] = "Running        ";
char ERROR[] = "-SEN/CON ERROR-";
String temp_str;
char temp_char[3] = "0";
char temp_cyc[4] = "0";
char szSample[20000] = "";

static const char szHTTP404Error[] = "HTTP/1.1 404 Not Found\r\n\r\n";
static const char szHTTPOK[] = "HTTP/1.1 200 OK\r\n";
static const char szNoCache[] = "Cache-Control: no-cache\r\n";
static const char szContentType[] = "Content-Type: ";
static const char szContentLength[] = "Content-Length: ";
static const char szLineTerminator[] = "\r\n";

static CLIENTINFO * pClientMutex = NULL;

typedef enum {
    WRITECONTENT,
    DONE
} STATE;

int loc = 0;

char szHead[300] = "<!DOCTYPE html><html><head><meta http-equiv=refresh content=500><title>Laundry Now</title></head><body>\r\n\
<h1><p align=center>Laundry Now</h1></p><h3><p align=center>Laundry Room Monitoring System</p></h3>\r\n";

char szControl[300] = "<table width=70% align=center border=1><tr><td>\r\n\
<table width=100% border=0><tr><th width=10%></th><th width=10%>Row #--</th><th width=10%></th>\r\n\
<th align=left>Machine Number</th><th>Availability</th></tr><tr height=20></tr>\r\n";

char szControlTEMP[300] = "";

char szSensorTEMP[200] = "";

char szSensorW[200] = "<tr height=35><th width=10%></th><th width=10% bgcolor=------->--</th><th width=10%></th>\r\n\
<th align=left>Machine #--</th><th>---------------</th></tr><tr height=12></tr>\r\n";

char szEndControl[50] = "</table></td></tr></table>";

char szData[500] = "<table width=70% align=center border=1><tr><td>\r\n\
<table width=90% border=0 align=center><tr><td align=center><p><h3>Machine Usage Data</h3></p><p><h5>Number of Cycles</h5></p></td></tr></table>\r\n\
<table width=90% border=0 align=center><tr><th width=25%>Machine</th><th width=25%>Last 24 Hours</th><th width=25%>Last 7 Days</th>\r\n\
<th width=25%>Last 30 Days</th></td></tr><tr height=10></tr>\r\n";

char szMachineDataTEMP[200] = "";

char szMachineData[200] = "<tr><th width=25%><font color=------->Row -- Machine --</font></th><th width=25%>---</th><th width=25%>---</th><th width=25%>---</th></tr>";

char szEndData[50] = "</table>";

char szEnd[15] = "</body></html>";

GCMD::ACTION ComposeHTMLHome(CLIENTINFO * pClientInfo)
{

    loc = 10;
    memmove((szSample+loc),szHead,300);
    loc = 350;
    memmove((szSample+14980),szEnd,15);
    
    for(int i = 0; i < 4; i++){   
      if(sensors[i] > 0){
        
        memmove((szControlTEMP),szControl,300);
        temp_str = String(i+1);
        temp_str.toCharArray(temp_char,2);
        memmove((szControlTEMP+118),temp_char,2);
        
        memmove((szSample+loc),szControlTEMP,300);
        loc = loc+300;
        memmove(szSensorTEMP,szSensorW,200);
        for(int j = 0; j < sensors[i]; j++){
          
          temp_str = String(j+1);
          temp_str.toCharArray(temp_char,2);
          memmove((szSensorTEMP+115),temp_char,2);

          if(bitRead(controller_raw[i],j+8) == 1){
            memmove((szSensorTEMP+55),GRN,7);
            memmove((szSensorTEMP+126),AVAIL,15);
            memmove((szSensorTEMP+63),"  ",2);
          }
          if(bitRead(controller_raw[i],j) == 1){
            memmove((szSensorTEMP+55),RED,7);
            memmove((szSensorTEMP+126),INUSE,15);
            memmove((szSensorTEMP+63),"  ",2);
          }
          
          if(controller_change[i][j] == 1){
            memmove((szSensorTEMP+55),YEL,7);
            memmove((szSensorTEMP+126),COMPL,15);
            memmove((szSensorTEMP+63),"  ",2);
          }
          if(cycles_week[i][j] == 0){
            memmove((szSensorTEMP+63),"! ",2);
          }
          if(bitRead(controller_raw[i],j+16) == 1 || 
            (bitRead(controller_raw[i],j+8) == 1 && bitRead(controller_raw[i],j) == 1) ||
            (bitRead(controller_raw[i],j+8) == 0 && bitRead(controller_raw[i],j) == 0 && bitRead(controller_raw[i],j+16) == 0)){
            memmove((szSensorTEMP+55),BLU,7);
            memmove((szSensorTEMP+126),ERROR,15);
            memmove((szSensorTEMP+63),"!!",2);
          }
          memmove((szSample+loc),szSensorTEMP,200);
         
          loc = loc+300;
        }
        memmove((szSample+loc),szEndControl,50);
        loc = loc+50;
      }
    }
    memmove((szSample+loc),szData,500);
    loc = loc+500;
    
    for(int i = 0; i < 4; i++){
      if(sensors[i] > 0){
        memmove(szMachineDataTEMP,szMachineData,200);
        for(int j = 0; j < sensors[i]; j++){
          
          temp_str = "";
          temp_char = {};
          temp_str = String(i+1);
          temp_str.toCharArray(temp_char,2);
          memmove((szMachineDataTEMP+42),temp_char,2);
          
          temp_str = "";
          temp_char = {};
          temp_str = String(j+1);
          temp_str.toCharArray(temp_char,2);
          memmove((szMachineDataTEMP+53),temp_char,2);
          
          temp_str = "";
          temp_char = {};
          temp_str = String(cycles_day[i][j]);
          temp_str.toCharArray(temp_cyc,3); 
          memmove(szMachineDataTEMP+81,temp_cyc,3);
          
          temp_str = "";
          temp_char = {};
          temp_str = String(cycles_week[i][j]);
          temp_str.toCharArray(temp_cyc,3); 
          memmove(szMachineDataTEMP+103,temp_cyc,3);

          temp_str = "";
          temp_char = {};
          temp_str = String(cycles_month[i][j]);
          temp_str.toCharArray(temp_cyc,3); 
          memmove(szMachineDataTEMP+125,temp_cyc,3);
          
          if(cycles_week[i][j] == 0){
            memmove(szMachineDataTEMP+30,RED,7);
          }
          else{
            memmove(szMachineDataTEMP+30,BLK,7);
          }
        
          memmove((szSample+loc),szMachineDataTEMP,200);
          loc = loc+200;
   
        }
      }
    }
    
    GCMD::ACTION retCMD = GCMD::DONE;
    switch(pClientInfo->htmlState)
    
    {
        case HTTPSTART:

            pClientInfo->cbWrite = BuildHTTPOKStr(false, sizeof(szSample)-1, ".htm", (char *) pClientInfo->rgbOut, sizeof(pClientInfo->rgbOut));
            pClientInfo->pbOut = pClientInfo->rgbOut;

            retCMD = GCMD::WRITE;
            
            pClientInfo->htmlState = WRITECONTENT;

            break;

         case WRITECONTENT:
            
            pClientInfo->pbOut = (const byte *) szSample;

            pClientInfo->cbWrite = sizeof(szSample)-1;

            retCMD = GCMD::WRITE;

            pClientInfo->htmlState = DONE;             
            break;

        case DONE:
        default:

            retCMD = GCMD::DONE;
            break;
    }

    return(retCMD);
}

uint32_t BuildHTTPOKStr(bool fNoCache, uint32_t cbContentLen, const char * szFile, char * szHTTPOKStr, uint32_t cbHTTPOK)
{  
    uint32_t i = 0;
    char szContentLenStr[36];
    uint32_t cbContentLenStr = strlen(itoa(cbContentLen, szContentLenStr, 10));
    const char * szContentTypeStr = "text/html";
    uint32_t cbContentTypeStr = 0;
    uint32_t cb = sizeof(szHTTPOK) + sizeof(szLineTerminator) - 2;

    cbContentTypeStr = strlen(szContentTypeStr);
    cb += cbContentTypeStr + sizeof(szContentType) + sizeof(szLineTerminator) - 2;

    cb += cbContentLenStr + sizeof(szContentLength) + sizeof(szLineTerminator) - 2;
    
    memcpy(szHTTPOKStr, szHTTPOK, sizeof(szHTTPOK) - 1);
    i = sizeof(szHTTPOK) - 1;

    memcpy(&szHTTPOKStr[i], szContentType, sizeof(szContentType) - 1);
    i += sizeof(szContentType) - 1;
    memcpy(&szHTTPOKStr[i], szContentTypeStr, cbContentTypeStr);
    i += cbContentTypeStr;
    memcpy(&szHTTPOKStr[i], szLineTerminator, sizeof(szLineTerminator) - 1);
    i += sizeof(szLineTerminator) - 1;
    
    memcpy(&szHTTPOKStr[i], szContentLength, sizeof(szContentLength) - 1);
    i += sizeof(szContentLength) - 1;
    memcpy(&szHTTPOKStr[i], szContentLenStr, cbContentLenStr);
    i += cbContentLenStr;
    memcpy(&szHTTPOKStr[i], szLineTerminator, sizeof(szLineTerminator) - 1);
    i += sizeof(szLineTerminator) - 1;

    memcpy(&szHTTPOKStr[i], szLineTerminator, sizeof(szLineTerminator) - 1);
    i += sizeof(szLineTerminator) - 1;

    szHTTPOKStr[i] = '\0';
    
    return(i);
}
