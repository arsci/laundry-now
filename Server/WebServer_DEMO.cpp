/*------------------------------------------------------------------------------------
---- Title: Laundry Now Web Server

---- File: WebServer

---- Function: Performs HTTP Web Server functions and 
               manages controller nodes.
               
---- Authors: Ryan Russell
              Frank Monforte
              
---- Sonoma State University Department of Engineering Science
---- Senior Design Project - Spring 2015
------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------
--------------------------------------------------------------------------------------
-------- Begin Variable Declarations
--------------------------------------------------------------------------------------
------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------
---- Include Defs 
------------------------------------------------------------------------------------*/
#include    <plib.h>
#include    <p32xxxx.h>
#include    <xc.h>
#include    <WiFiShieldOrPmodWiFi_G.h>
#include    <DNETcK.h>
#include    <DWIFIcK.h>
#include    <SD.h>
#include    <HTTPServer.h>
#include    <stdio.h>

/*------------------------------------------------------------------------------------
---- Defs 
------------------------------------------------------------------------------------*/
#define    RX                 39          //UART RX PIN (Server/Controller)
#define    TX                 40          //UART TX PIN (Server/Controller)
#define    LD3                PORTAbits.RA0 //47          //WF32 On Board LED#3
#define    LD4                PORTAbits.RA1 //48          //WF32 On Board LED#4
#define    LD5                PORTFbits.RF0 //43          //WF32 On Board LED#5
#define    LD6                PORTGbits.RG6 //13          //WF32 On Board LED#6
#define    CS2                LATEbits.LATE6 //32          //Controller Chip Select 3
#define    CS3                LATEbits.LATE7 //33          //Controller Chip Select 2
#define    CS0                LATEbits.LATE4 //30          //Controller Chip Select 0
#define    CS1                LATEbits.LATE5 //31          //Controller Chip Select 1
#define    IC1                62          //I2C SDA Line (RTC)
#define    IC1a               PORTAbits.RA4
#define    IC2                63          //I2C SCL Line (RTC)
#define    IC2a               PORTAbits.RA5
#define    RTC                104         //Real Time Clock
#define    BT2                PORTAbits.RA6 //65          //WF32 On Board Button #1
#define    BT3                PORTAbits.RA7 //66          //WF32 On Board Button #2
#define    RTC_ADD            0b11010000   //RTC DS3231SN I2C Hardware Address
#define    SDCS               51
#define    SDCSa              PORTGbits.RG12 //51          //SD Card SPI Chip Select (CS) Pin
#define    ALRT               PORTDbits.RD11 //35          //RTC Alert Input Pin
#define    TXREG              U4TXREG
#define    RXREADY            U4STAbits.URXDA
#define    RXREG              U4RXREG
#define    SYSCLK             (80000000)
#define    PBCLK              (SYSCLK)
#define    DESIRED_BAUDRATE   (9600)  
#define    cDESIRED_BAUDRATE  (1201)  
#define    BAUD_VALUE         ((PBCLK/16/DESIRED_BAUDRATE)-1)
#define    cBAUD_VALUE        ((PBCLK/16/cDESIRED_BAUDRATE)-1)
#define    I2C_BAUD           98

/*------------------------------------------------------------------------------------
---- Extern Variables (Global - All Files)
------------------------------------------------------------------------------------*/
extern int sensors[4] = {0};
extern unsigned long controller_raw[4] = {0};
extern unsigned int controller_change[4][8] = {0};
extern unsigned int machine_cycles[30][4][8] = {0};
extern int cycles_day[4][8] = {0};
extern int cycles_week[4][8] = {0};
extern int cycles_month[4][8] = {0};
extern int cycles_year[4][8] = {0};
extern char szSsid[30] = "";
extern char szPassPhrase[30] = "";

int curr_date = 0; 
int last_date = 0;
int NUM[3] = {0};
int controller_temp = 0;    
int sentOK = 0;
int sameOK1 = 0;
int sameOK2 = 0;
int sameOK3 = 0;
int break1 = 0;
int break2 = 0;
int break3 = 0;
int twoTouch = 0;
int ctl_temp = 0;
int ctl_sel = 0;
int break_out = 0;
int  T_D = 30;       //20 works - 30 safe
int  L_D = 00;       //100 works
int tmp_ct = 0;
String temp_str2;
char temp_char2[3] = "0";
char temp_cyc2[4] = "0";
File fileSD = File();
int SD_Known = 0;

/*------------------------------------------------------------------------------------
---- Function: setup
---- Purpose: Setup function. First function to run on power
              on. Main loop runs next.
              
---- Inputs: None
---- Returns: None
------------------------------------------------------------------------------------*/
void setup(void) {
  
  TRISDbits.TRISD14 = 1;    //UART RX
  TRISDbits.TRISD15 = 0;    //UART TX
  TRISAbits.TRISA0 = 0;     //LED3
  TRISAbits.TRISA1 = 0;     //LED4
  TRISFbits.TRISF0 = 0;     //LED5
  TRISGbits.TRISG6 = 0;     //LED6
  TRISEbits.TRISE4 = 0;     //CS0
  TRISEbits.TRISE5 = 0;     //CS1
  TRISEbits.TRISE6 = 0;     //CS2
  TRISEbits.TRISE7 = 0;     //CS3
  TRISDbits.TRISD11 = 1;    //ALRT
  TRISAbits.TRISA6 = 1;     //BTN1
  TRISAbits.TRISA7 = 1;     //BTN2
  TRISGbits.TRISG12 = 0;    //SDCS
  
  LATAbits.LATA4 = 1;     //I2C SDA PULLUP
  LATAbits.LATA5 = 1;     //I2C SCL PULLUP
  CS0 = 1;
  CS1 = 1;
  CS2 = 1;
  CS3 = 1;
  
  __XC_UART = 1;
  U1MODEbits.UARTEN = 1;             
  U1BRG = BAUD_VALUE;      
  U4MODEbits.UARTEN = 1;
  U4BRG = cBAUD_VALUE;
  U4STAbits.UTXEN = 1;
  U4STAbits.URXEN = 1;
  U4MODEbits.ON = 1;
  
  printf("----------------------- Laundry Now ------------------------\r\n");
  printf("\r\n");
  printf("          Advanced Laundry Room Monitoring System\r\n");
  printf("\r\n");
  printf("Designed By:\r\n");
  printf("             Ryan Russell\r\n");
  printf("             Frank Monforte\r\n");
  printf("\r\n");
  printf(" Sonoma State University Department of Engineering Science\r\n");
  printf("Electrical Engineering Senior Design Project -- Spring 2015\r\n");
  printf("\r\n");
  printf("------------------------------------------------------------\r\n");
  printf("\r\n");
  printf("Starting Up...\r\n");
  printf("\r\n");
  
  delay(2000);
  
  

  printf("Setting up Controllers...\r\n");
  setupControllers();
  printf("\r\n");
  
  curr_date = getDays();
  
  if(SD.begin(SDCS)){
    SD_Known = 1;
    printf("\r\n");
    importData();
    writeData();
  }
  else{
    SD_Known = 0;
    printf("No SD Card Found. Unable to Load/Save Data\r\n");
    printf("\r\n");
  }
  
  if(BT2){
    printf("Clearing Data...\r\n");
    if(SD_Known){
      writeDefault();
      importData();
    }
    printf("All Data Cleared\r\n");
  }

  printf("\r\n");
  
  if(SD_Known) getNetwork();
  else printf("No SD Card. Cannot Import Network Settings\r\n");
  
  printf("\r\n");
  printf("Setup Completed\r\n");
  printf("\r\n");
  printf("------------------------------------------------------------\r\n");
  printf("\r\n");
  printf("Configuring Server...\r\n"); 
  ServerSetup();
  printf("Server Configured\r\n");
  printf("\r\n");
  printf("Data Monitoring Started\r\n");
  printf("\r\n");
  printf("Setting up Network Connection\r\n");
  printf("\r\n");

  updateData();
  updateCalculations();
  clearRTCAlarm();
  setRTCAlarm(); 
}

/*------------------------------------------------------------------------------------
---- Function: Main (loop)
---- Purpose: Main function loop. Server runs out of this
              loop after setup function runs.
              
---- Inputs: None
---- Returns: None
------------------------------------------------------------------------------------*/
void loop(void) {
  for(unsigned long ibiz = 0; ibiz < 0x05F7A0; ibiz++){ //0xfff4f = 45seconds; 0x6B2E91 = 5minutes
  
    ProcessServer();
    
    if(ALRT == 0){
      //at24();
    } 
  } 
  updateData();
}

/*------------------------------------------------------------------------------------
---- Function: updateData
---- Purpose: Connects with controllers over UART to determine
              latest status of the machines (sensors). This 
              data is stored in RAM and saved to the SD card
              once every 24 hours.
              
---- Inputs: None
---- Returns: None
------------------------------------------------------------------------------------*/
void updateData(void){
  ctl_temp = 0;
  for(int x = 0; x < 4; x++){
    if(x == 0) CS0 = 0;
    if(x == 1) CS1 = 0;
    if(x == 2) CS2 = 0;
    if(x == 3) CS3 = 0;
    ctl_temp = sensors[x];
    if(ctl_temp > 0) {
  
      sameOK1 = 0;
      sameOK2 = 0;
      sameOK3 = 0;
      
      break1 = 50;
      break2 = 50;
      break3 = 50;
      
      while(sameOK1 == 0){
        if(break1 == 10) delay(100);
        if(break1 == 0) {
          sameOK1 = 1;
          printf("Update Failed: Controller: ");
          printf("%d",x+1);
          printf(": Running Register\r\n");
        }
        break1 = break1 - 1;
        controller_temp = 0;
        TXREG = 0b10000001;
        delay(T_D);
        if(RXREADY){
          controller_temp = RXREG; 
          printf("CON1: %i\r\n",controller_temp);     
          for(int j = 0; j < 8; j++) {            
            if(bitRead(controller_raw[x],j) == 1 && bitRead(controller_temp,j) == 0){
              cycles_day[x][j-0]++;
              if(SD_Known) writeDay();
              controller_change[x][j] = 1;
            }
            else controller_change[x][j] = 0;
            bitWrite(controller_raw[x],j,(bitRead(controller_temp,j)));
          }
          sameOK1 = 1;
        }
      }
      while(sameOK2 == 0){
        if(break2 == 10) delay(200);
        if(break2 == 0) {
          sameOK2 = 1;
          printf("Update Failed: Controller: ");
          printf("%d",x+1);
          printf(": Available Register\r\n");
        }
        break2 = break2 - 1;
        controller_temp = 0;
        TXREG = 0b10000011;
        delay(T_D);
        if(RXREADY){
          controller_temp = RXREG;
          printf("CON2: %i\r\n",controller_temp);
          for(int j = 8; j < 16; j++) {
            bitWrite(controller_raw[x],j,bitRead(controller_temp,j-8));
          }
          sameOK2 = 1;
        }
      } 
      while(sameOK3 == 0){
        if(break3 == 10) delay(200);
        if(break3 == 0) {
          sameOK3 = 1;
          printf("Update Failed: Controller: ");
          printf("%d",x+1);
          printf(": Error Register\r\n");
        }
        break3 = break3 - 1;
        controller_temp = 0;
        TXREG = 0b10000111;
        delay(T_D);
        if(RXREADY){
          controller_temp = RXREG;
          printf("%i\r\n",controller_temp);
          for(int j = 16; j < 24; j++) {
            bitWrite(controller_raw[x],j,bitRead(controller_temp,j-16));
          }
          sameOK3 = 1;
        }
      }
      printf("-----------------------\r\n");    
    }
    CS0 = 1;
    CS1 = 1;
    CS2 = 1;
    CS3 = 1;
  }
  LD4 = 0;
  LD5 = 0;
  if(bitRead(controller_raw[0],0)==1) LD4 = 1;
  if(bitRead(controller_raw[1],0)==1) LD5 = 1;
  delay(L_D);
}

/*------------------------------------------------------------------------------------
---- Function: importData 
---- Purpose: Checks and loads cycle data from SD card. Only
              called on startup of server, data from SD card
              is loaded into RAM. 
              
---- Inputs: None
---- Returns: 0 If file is valid, 1 if file is invalid. 
              (Currently not used)
------------------------------------------------------------------------------------*/
int importData(void){
  int temp_in = 0;
  int first = 0;
  int second = 0;
  int third = 0;
  int shift = 0;
  int failed = 0;
  
  if(SD.exists("laun_now.txt") == 1 && SD.exists("laun_day.txt") == 1){
    fileSD = SD.open("laun_now.txt",FILE_READ);
    if(fileSD.read() == '#' && fileSD.read() == '#' && fileSD.read() == '#' && fileSD.read() == '#'){
      printf("Valid Data File Found. Loading From SD...\r\n");
      first = getINT(fileSD.read());
      second = getINT(fileSD.read());
      third = getINT(fileSD.read());
      last_date = getValue(first,second,third);
      fileSD.seek(fileSD.position()+10);
      for(int i = 0; i < 30; i++){
        for(int j = 0; j < 4; j++){
          for(int k = 0; k < 8; k++){
           first = getINT(fileSD.read());
           second = getINT(fileSD.read());
           third = getINT(fileSD.read());
           machine_cycles[i][j][k] = getValue(first,second,third);
           fileSD.seek(fileSD.position()+1);
          }
          fileSD.seek(fileSD.position()+9);
        }
      }
      fileSD.close(); 
    }
    else failed = 1; 
    
    fileSD = SD.open("laun_day.txt",FILE_READ);
    if(fileSD.read() == '#' && fileSD.read() == '#' && fileSD.read() == '#' && fileSD.read() == '#'){
      fileSD.seek(fileSD.position()+6);
      for(int j = 0; j < 4; j++){
        for(int k = 0; k < 8; k++){
          first = getINT(fileSD.read());
          second = getINT(fileSD.read());
          third = getINT(fileSD.read());
          cycles_day[j][k] = getValue(first,second,third);
          fileSD.seek(fileSD.position()+1);
        }
        fileSD.seek(fileSD.position()+5);
      }           
      fileSD.close();
    }
    else failed = 1;    
    
    if(failed){
      printf("Invalid/Corrupted File Found on SD. Creating New File...\r\n");
      fileSD.close();
      SD.remove("laun_now.txt");
      writeDefault();
      printf("New Data File Creaded Sucessfully\r\n");
      return 1;
    }
    else{
      shift = curr_date-last_date;      
      if(shift != 0){
        printf("Shifting Data...\r\n");
        for(int b = shift; b > 0; b--){
          for(int i = 29; i > 0; i--){
            for(int j = 3; j > -1; j--){
              for(int k = 7; k > -1; k--){
                machine_cycles[i][j][k] = machine_cycles[i-1][j][k];
                machine_cycles[i-1][j][k] = 0;
                if(i == 1) machine_cycles[0][j][k] = cycles_day[j][k];
              }
            }
          }
        }
      }
      else printf("No Data Shift Needed\r\n");
      printf("File Loaded Sucessfully\r\n");
      return 0;
    }
  }
  else {
    printf("No Data Files Found on SD. Creating New File...\r\n");
    writeDefault();
    printf("Data File Created Sucessfully\r\n");
    return 1;
  }
}

/*------------------------------------------------------------------------------------
---- Function: setupControllers 
---- Purpose: Attempt to connect to controllers over
              UART to determine if they are active. If
              they are active, determine how many sensors
              are connected and save the value.
              
---- Inputs: None
---- Returns: None
------------------------------------------------------------------------------------*/
void setupControllers(void){

  for(int x2 = 0; x2 < 4; x2++){
    break_out = 0;
    ctl_sel = 0;
    sentOK = 0;
    ctl_temp = 0;
    if(x2 == 0) CS0 = 0;
    if(x2 == 1) CS1 = 0;
    if(x2 == 2) CS2 = 0;
    if(x2 == 3) CS3 = 0;
    
    while(sentOK == 0){
      if(break_out == 25){
        sensors[x2] = 0;
        sentOK = 1;
        printf("Controller #");
        printf("%d",x2+1);
        printf(" Not Found\r\n");
        break;
      }
      break_out++;
      TXREG = 0b11110000;
      delay(T_D);
      if(RXREADY){
        ctl_temp = RXREG;
        if(ctl_temp == 0b00001111){ 
          TXREG = 0b11100000;
          delay(T_D);
          if(RXREADY){
            ctl_sel = RXREG;
            if(ctl_sel < 9 && sentOK == 0){
              sentOK = 1;
              sensors[x2] = ctl_sel;
              printf("Controller #");
              printf("%d",x2+1);
              printf(" Connected: ");
              printf("%d",ctl_sel);
              printf(" Sensors Found\r\n");
            }
          }
        }
      }
    }    
    CS0 = 1;
    CS1 = 1;
    CS2 = 1;
    CS3 = 1;
  }
}

/*------------------------------------------------------------------------------------
---- Function: getINT
---- Purpose: Converts an ASCII value to it's decimal form
              
---- Inputs: ASCII Value
---- Returns: Decimal Value
------------------------------------------------------------------------------------*/
int getINT(int input){
  return input-48;
}

/*------------------------------------------------------------------------------------
---- Function: getASCII
---- Purpose: Gets the ASCII value for a given integer.
              
---- Inputs: INT to be converted to ASCII
---- Returns: ASCII Value
------------------------------------------------------------------------------------*/
int getASCII(int input){
  return input+48;
}

/*------------------------------------------------------------------------------------
---- Function: getValue 
---- Purpose: Converts 3 separate integers into one. int ONE represents the
              hundreds digit, int TWO represents the TENS digit, int THREE 
              represents the singles digit.              
              
---- Inputs: 3 Integers, to be converted to a single number
---- Returns: Integer Value
------------------------------------------------------------------------------------*/
int getValue(int one, int two, int three){
  int temp = 0;
  temp = three;
  temp = temp + two*10;
  temp = temp + one*100;
  return temp;
}

/*------------------------------------------------------------------------------------
---- Function: getDays 
---- Purpose: Calculated the numerical value day of the year (0-365)
              based on the date retrieved from the RTC
              
---- Inputs: None
---- Returns: Day of the year
------------------------------------------------------------------------------------*/
int getDays(void){
  
  int Months[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
  int monthNum = 0;
  int days = 0;
  int temp_days = 0;
  int temp_days_H = 0;

  monthNum = READI2C(RTC_ADD,0x05);
  if(monthNum > 1){
    for(int i=monthNum-2; i>-1; i--){
     days = days + Months[i];
    }
  }

  temp_days = READI2C(RTC_ADD,0x04);
  
  bitWrite(temp_days_H,0,bitRead(temp_days,4));
  bitWrite(temp_days_H,1,bitRead(temp_days,5));
  bitWrite(temp_days,4,0);
  bitWrite(temp_days,5,0);
  temp_days = temp_days + temp_days_H*10;
  days = days + temp_days;
  
  return days;
}

/*------------------------------------------------------------------------------------
---- Function: writeDefault 
---- Purpose: Writes the default data file to the SD card. Called if the
              data file does not exist on the SD card, or the existing
              data file is invalid or corrupted.
              
---- Inputs: None
---- Returns: None
------------------------------------------------------------------------------------*/
void writeDefault(void){
  fileSD = SD.open("laun_now.txt",FILE_WRITE);
  fileSD.seek(0);
  fileSD.write('#');
  fileSD.write('#');
  fileSD.write('#');
  fileSD.write('#');
  fileSD.write('0');
  fileSD.write('0');
  fileSD.write('0');
  fileSD.write(13);
  fileSD.write(10);
  for(int l = 0; l < 3; l++){
    for(int i = 0; i < 10; i++){
      for(int j = 0; j < 4; j++){
        fileSD.write('0');
        fileSD.write(getASCII(l));
        fileSD.write(getASCII(i));
        fileSD.write(32);
        fileSD.write('0');
        fileSD.write('0');
        fileSD.write(getASCII(j));
        for(int k = 0; k < 8; k++){
          fileSD.write(32);
          fileSD.write('0');
          fileSD.write('0');
          fileSD.write('0');
        }
        fileSD.write(13);
        fileSD.write(10);
      }
    }
  }            
  fileSD.close();
  
  fileSD = SD.open("laun_day.txt",FILE_WRITE);
  fileSD.seek(0);
  fileSD.write('#');
  fileSD.write('#');
  fileSD.write('#');
  fileSD.write('#');
  fileSD.write(13);
  fileSD.write(10);
  for(int j = 0; j < 4; j++){
    fileSD.write('0');
    fileSD.write('0');
    fileSD.write(getASCII(j));
    for(int k = 0; k < 8; k++){
      fileSD.write(32);
      fileSD.write('0');
      fileSD.write('0');
      fileSD.write('0');
    }
    fileSD.write(13);
    fileSD.write(10);
  }           
  fileSD.close();
  
}

/*------------------------------------------------------------------------------------
---- Function: writeData
---- Purpose: Writes the latest machine cycle data saved in RAM to the file saved
              on the SD card. 
              
---- Inputs: None
---- Returns: None
------------------------------------------------------------------------------------*/

void writeData(void){
  printf("Saving Data to SD Card...\r\n");
  int m = 0;
  fileSD = SD.open("laun_now.txt",FILE_WRITE);
  fileSD.seek(0);
  fileSD.write('#');
  fileSD.write('#');
  fileSD.write('#');
  fileSD.write('#');
  individualNums(curr_date);
  fileSD.write(getASCII(NUM[0]));
  fileSD.write(getASCII(NUM[1]));
  fileSD.write(getASCII(NUM[2]));
  fileSD.write(13);
  fileSD.write(10);
  for(int l = 0; l < 3; l++){
    for(int i = 0; i < 10; i++){
      for(int j = 0; j < 4; j++){
        fileSD.write('0');
        fileSD.write(getASCII(l));
        fileSD.write(getASCII(i));
        fileSD.write(32);
        fileSD.write('0');
        fileSD.write('0');
        fileSD.write(getASCII(j));
        for(int k = 0; k < 8; k++){
          m = i+(10*l);
          individualNums(machine_cycles[m][j][k]);
          fileSD.write(32);
          fileSD.write(getASCII(NUM[0]));
          fileSD.write(getASCII(NUM[1]));
          fileSD.write(getASCII(NUM[2]));
        }
        fileSD.write(13);
        fileSD.write(10);
      }
    }
  }
  fileSD.close();
  printf("Data Sucessfully Saved\r\n");
}

/*------------------------------------------------------------------------------------
---- Function: individualNums
---- Purpose: Converts a single integer into separate digits. Result is saved into 
              the global variables NUM_1 (Hundreds), NUM_2 (Tens), NUM_3 (Ones).
              
---- Inputs: Integer between 0 and 999.
---- Returns: None
------------------------------------------------------------------------------------*/
void individualNums(int input){
  
  int num1 = 0;
  int num2 = 0;
  int num3 = 0;
  int num4 = 0;
  
  if(input > 99){
    num1 = input/100; 
  }
  num2 = input-num1*100;
  if(input > 9){
    num3 = num2/10;
  }
  
  num4 = input-(num1*100)-(num3*10);
  
  NUM[0] = num1;
  NUM[1] = num3;
  NUM[2] = num4;  
}

/*------------------------------------------------------------------------------------
---- Function: at24
---- Purpose: Triggered when the real time clock chip produces an alarm. Shifts the
              cycle data, backs up to the SD cars, and updates the week/month calculations
              
---- Inputs: None
---- Returns: None
------------------------------------------------------------------------------------*/
void at24(){
  curr_date = getDays();
  for(int m = 29; m > 0; m--){
    for(int i = 0; i < 4; i++){
      for(int j = 0; j < 8; j++){
        machine_cycles[m][i][j] = machine_cycles[m-1][i][j]; 
        machine_cycles[m-1][i][j] = 0;     
      }
    }
  }       
  for(int j = 0; j < 4; j++){
    for(int i = 0; i < 8; i++){
      machine_cycles[0][j][i] = cycles_day[j][i];
      cycles_day[j][i] = 0;
    }
  }
  if(SD_Known){
    writeData();
    writeDay();
  }
  updateCalculations();
  clearRTCAlarm(); 
  delay(500); 
}

/*------------------------------------------------------------------------------------
---- Function: updateCalculations
---- Purpose: Updates the day/week/month calculations for the website
              
---- Inputs: None
---- Returns: None
------------------------------------------------------------------------------------*/
void updateCalculations(){
  
  for(int j = 0; j < 4; j++){  
    for(int i = 0; i < 8; i++){
      cycles_week[j][i] = 0;
      for(int k = 0; k < 7; k++){
        cycles_week[j][i] = machine_cycles[k][j][i] + cycles_week[j][i];
      }
    }
  }
  for(int j = 0; j < 4; j++){  
    for(int i = 0; i < 8; i++){
      cycles_month[j][i] = 0;
      for(int k = 0; k < 30; k++){
        cycles_month[j][i] = machine_cycles[k][j][i] + cycles_month[j][i];
      }
    }
  }
  for(int j = 0; j < 4; j++){  
    for(int i = 0; i < 8; i++){
      cycles_year[j][i] = 0;
      for(int k = 0; k < 30; k++){
        cycles_year[j][i] = machine_cycles[k][j][i] + cycles_year[j][i];
      }
    }
  }
}

/*------------------------------------------------------------------------------------
---- Function: getNetwork
---- Purpose: Imports the WiFi Network information from the SD card.
              
---- Inputs: None
---- Returns: None
------------------------------------------------------------------------------------*/
void getNetwork(void) {
  printf("Getting Network Info\r\n");
  char next[30] = "";
  char next2[30] = "";
  char switch1[30] = "";
  int i = 1;
  
  if(SD.exists("laun_net.txt") == 0){
    printf("Network Settings File Not Valid\r\n");
  }
  else{
    fileSD = SD.open("laun_net.txt",FILE_READ);
    fileSD.seek(0);
    next[0] = fileSD.read();
    while(i < 30){
      next[i] = fileSD.read();
      if(next[i] == '\n') {
        next[i-1] = NULL;
        
        break;
      }        
      i++;
    }
    for(int j=0;next[j]!=NULL;j++){  
      szSsid[j] = next[j];
    }
    i = 1;
    next2[0] = fileSD.read();
    while(i < 30){
      next2[i] = fileSD.read();
      if(next2[i] == '\n') {
        next2[i-1] = NULL;
        break;
      }        
      i++;
    }    
    for(int i=0;next2[i]!=NULL;i++){  
      szPassPhrase[i] = next2[i];
    }
    printf("Network Settings Imported\r\n");
    printf("SSID: ");
    printf(szSsid);
    printf("\r\n");
    printf("Network Passphrase: ");
    printf(szPassPhrase);
    printf("\r\r");
  }
}

/*------------------------------------------------------------------------------------
---- Function: writeDay
---- Purpose: Backs up the daily cycle data to the SD card.
              
---- Inputs: None
---- Returns: None
------------------------------------------------------------------------------------*/
void writeDay(void){
  int m = 0;
  fileSD = SD.open("laun_day.txt",FILE_WRITE);
  fileSD.seek(0);
  fileSD.write('#');
  fileSD.write('#');
  fileSD.write('#');
  fileSD.write('#');
  fileSD.write(13);
  fileSD.write(10);
  for(int j = 0; j < 4; j++){
    fileSD.write('0');
    fileSD.write('0');
    fileSD.write(getASCII(j));
    for(int k = 0; k < 8; k++){
      individualNums(cycles_day[j][k]);
      fileSD.write(32);
      fileSD.write(getASCII(NUM[0]));
      fileSD.write(getASCII(NUM[1]));
      fileSD.write(getASCII(NUM[2]));
    }
    fileSD.write(13);
    fileSD.write(10); 
  }
  fileSD.close();
}

/*------------------------------------------------------------------------------------
---- Function: checkRTCAlarm
---- Purpose: 
              
---- Inputs: 
---- Returns: 
------------------------------------------------------------------------------------*/
int checkRTCAlarm(void){
  int temp = 0;
  temp = READI2C(RTC_ADD,0x0F);
  if(bitRead(temp,0)){
    return 1;
  }
  else return 0;
}
  
/*------------------------------------------------------------------------------------
---- Function: clearRTCAlarm
---- Purpose: Clears the 24HR midnight alarm on the Real Time Clock (RTC) after it
              it goes off.
---- Inputs: None
---- Returns: None
------------------------------------------------------------------------------------*/
void clearRTCAlarm(void){
  WRITEI2C(RTC_ADD,0x0F,0b10000000);
}

/*------------------------------------------------------------------------------------
---- Function: setRTCAlarm
---- Purpose: Resets?Reprograms the alarm on the Real Time Clock chip. Only called
              when the server is first powered on.
---- Inputs: None
---- Returns: None
------------------------------------------------------------------------------------*/
void setRTCAlarm(void){

  WRITEI2C(RTC_ADD,0x0E,0b01011101);

  WRITEI2C(RTC_ADD,0x07,0x00);

  WRITEI2C(RTC_ADD,0x08,0x05);

  WRITEI2C(RTC_ADD,0x09,0x00);

  WRITEI2C(RTC_ADD,0x0A,0x80);
}

/*------------------------------------------------------------------------------------
---- Function: setRTC
---- Purpose: Sets the time on the Real Time Clock (RTC).
              
---- Inputs: None
---- Returns: None
------------------------------------------------------------------------------------*/
void setRTC(void){

  WRITEI2C(RTC_ADD,0x00,0x00);

  WRITEI2C(RTC_ADD,0x01,0x20);
 
  WRITEI2C(RTC_ADD,0x02,0x00);

  WRITEI2C(RTC_ADD,0x03,0x03);

  WRITEI2C(RTC_ADD,0x04,0x10);

  WRITEI2C(RTC_ADD,0x05,0x03);

  WRITEI2C(RTC_ADD,0x06,0x15);
  
}

/*------------------------------------------------------------------------------------
---- Function: getRTC
---- Purpose: Calls the Real Time Clock for the input specified and returns the
              value. This function is currently not used.
              
---- Inputs: RTC Option (Not Used)
             0: Seconds
             1: Minutes
             2: Hours
             3: Day
             4: Date
             5: Month
             6: Year
 
---- Returns: RTC Data (Not Used)
------------------------------------------------------------------------------------*/
int getRTC(int option){
  int hour = 0;
  int minute = 0;
  int second = 0;

  second = READI2C(RTC_ADD,0);

  minute = READI2C(RTC_ADD,1);

  hour = READI2C(RTC_ADD,2);
  
  printf("%i:",hour);
  printf("%i:",minute);
  printf("%i\r\n",second);
  
  return option; 
}

/*------------------------------------------------------------------------------------
---- Function: WRITEI2C
---- Purpose: 
              
---- Inputs: None
---- Returns: None
------------------------------------------------------------------------------------*/
void WRITEI2C(int addr, int reg, int data){
  
  OpenI2C2(I2C_ON,I2C_BAUD);
  IdleI2C2();
  StartI2C2();
  IdleI2C2();
  MasterWriteI2C2(addr);
  IdleI2C2();
  MasterWriteI2C2(reg);
  IdleI2C2();
  MasterWriteI2C2(data);
  IdleI2C2();
  StopI2C2();
  IdleI2C2();
  
}

/*------------------------------------------------------------------------------------
---- Function: READI2C
---- Purpose: 
              
---- Inputs: None
---- Returns: None
------------------------------------------------------------------------------------*/
int READI2C(int addr, int reg){

  int temp = 0;
  
  OpenI2C2(I2C_ON,I2C_BAUD);
  IdleI2C2();
  StartI2C2();
  IdleI2C2();
  MasterWriteI2C2(addr);
  IdleI2C2();
  MasterWriteI2C2(reg);
  IdleI2C2();
  StartI2C2();
  IdleI2C2();
  MasterWriteI2C2(addr+1);
  IdleI2C2();
  temp = MasterReadI2C2();
  IdleI2C2();
  StopI2C2();
  IdleI2C2();

  return temp;
}
