// ***********************************************
// Title: Rewrite
// ***********************************************
// Purpose:
//
// ***********************************************
//
// ***********************************************
// Author:
// ***********************************************

// Include
#include <htc.h>
#include <p18f45k20.h>
#include <stdio.h>
#include <stdlib.h>
//#include <math.h>

// CONFIG
#pragma config FOSC = INTIO67, FCMEN = OFF, IESO = OFF                      // CONFIG1H
#pragma config PWRT = ON, BOREN = OFF, BORV = 30                           // CONFIG2L
#pragma config WDTEN = OFF, WDTPS = 32768                                  // CONFIG2H
#pragma config MCLRE = OFF, LPT1OSC = OFF, PBADEN = OFF, CCP2MX = PORTC     // CONFIG3H
#pragma config STVREN = ON, LVP = OFF, XINST = OFF                          // CONFIG4L
#pragma config CP0 = OFF, CP1 = OFF, CP2 = OFF, CP3 = OFF                   // CONFIG5L
#pragma config CPB = OFF, CPD = OFF                                         // CONFIG5H
#pragma config WRT0 = OFF, WRT1 = OFF, WRT2 = OFF, WRT3 = OFF               // CONFIG6L
#pragma config WRTB = OFF, WRTC = OFF, WRTD = OFF                           // CONFIG6H
#pragma config EBTR0 = OFF, EBTR1 = OFF, EBTR2 = OFF, EBTR3 = OFF           // CONFIG7L
#pragma config EBTRB = OFF                                                  // CONFIG7H768

// Define
#define _XTAL_FREQ 1000000
#define testbit_1(var, bit)  ((var) & (1 <<(bit)))
#define setbit(var, bit)    ((var) |= (1 <<(bit)))
#define clrbit(var, bit)    ((var) &= ~(1 <<(bit)))

#define DEVID   0b10000000
#define X_AXIS  0b10110010
#define Y_AXIS  0b10110100
#define Z_AXIS  0b10110110

// Variables
int alt = 0;
int int_enable = 0;
int int_enable_value = 0;
int thresh_tap = 0;
int thresh_tap_value = 0;
int thresh_act = 0;
int thresh_act_value = 0;
int duration = 0;
int duration_value = 0;
int act_inact_ctl = 0;
int act_inact_ctl_value = 0;
int latency = 0;
int latency_value = 0;
int window = 0;
int window_value = 0;
int fifo_ctl = 0;
int fifo_ctl_value = 0;
int power_ctl = 0;
int power_ctl_value = 0;
int act_inact = 0;
int act_inact_value = 0;
int tap_axes = 0;
int tap_axes_value = 0;
int data_format = 0 ;
int data_format_value = 0;
int int_map = 0;
int int_map_value = 0;

int devid = 0;
int sensor[10];
int num_sen = 0;

int int_source = 0;
int int_source_value = 0;

int SEN_NUM = 0;
int CON1 = 0;
int CON2 = 0;
int CON3 = 0;
int CON4 = 0;

int cycling = 0;
int past_op1 = 0;
int past_op2 = 0;
int past_op3 = 0;
int past_op4 = 0;
int current_op = 0;
int past_op_int = 0;
int current_op_int = 0;
int past_op_final = 0;
int current_op_final = 0;
int small_op = 0;
int operation[10];
int seconds = 0;
int xT = 0;
int yT = 0;
int zT = 0;
int x1 = 0;
int y1 = 0;
int z1 = 0;
int x2 = 0;
int y2 = 0;
int z2 = 0;

int b = 0;

int pass1[10];
int pass2[10];
int pass3[10];
int pass4[10];
int pass5[10];
int lockout[10];

int avg_sensor = 0;

int pass1a[10];
int pass1b[10];
int pass1c[10];

int number_sensor = 0;
int addr = 0;

float wash_val[10];
float wash_count[10];

int lockout[10];
int timeout[10];
int timeout2[10];
int cancel[10];
int end_val[10];

int delaya = 500;
int delayb = 100;

// Functions
int request_id(int chipselect);
int number_of_sensor();
int error_check();
int return_to_measurement(int chipselect);
int collect_interupt_flags(int chipselect);
int time_delay();
int x_axis(int chipselect);
int y_axis(int chipselect);
int z_axis(int chipselect);
int three_axis(int chipselect);
int getVal(int addr, int chipselect);
void sendVal(int addr, int data, int chipselect);

void intialize_pic(void){
// This intializes the PIC
    TRISA = 0b00000000;
    TRISB = 0b00000011;
    TRISC = 0b10010110;
    TRISD = 0b00000000;
    ADCON0 = 0x00;
    TXSTA = 0b00100000;
    BAUDCON = 0b00000000;
    SPBRG = 12;
    RCSTA = 0b11010000;
    IPEN = 0;
    WPUB = 1;
    INTCON = 0b11000000;
    INTCON2 = 0b00000000;
    INTCON3 = 0b00000000;
    PIE1 = 0;
    PIE2 = 0;
    IPR1 = 0;
    RCIE = 1;
    RCIP = 1;
    SSPSTAT = 0b00000000;
    SSPCON1 = 0b00110001;
}
void intialize_sensor(void){


    for (int chipselect = 0; chipselect < 7; chipselect++)
    {
        /*
        sendVal(0b00110001,0b00000000,chipselect);
        sendVal(0b00101101,0b00001000,chipselect);
        */
        RCIE = 0;

        LATD = chipselect;
        SSPBUF = 0b00110001;
        while(!SSPSTATbits.BF);
        SSPBUF = 0b00000000;
        while(!SSPSTATbits.BF);
        LATD = 7;

        __delay_ms(1);

        LATD = chipselect;
        SSPBUF = 0b00101101;
        while(!SSPSTATbits.BF);
        SSPBUF = 0b00001000;
        while(!SSPSTATbits.BF);
        LATD = 7;

        RCIE = 1;

        __delay_ms(1);


    }


}
int testbit(int var_, int bit_){
    if(testbit_1(var_,bit_) > 0) return 1;
    else return 0;
}
int abval(int val){
    return(val < 0 ? (-val) : val);
}
int number_of_sensor(void){

    int num_sen = 0;
    int temp_num = 0;
    int chipselect = 0;
    for(chipselect = 0; chipselect < 7; chipselect++)
    {
        temp_num = 0;
        sensor[chipselect] = 0;

        for(int i = 0; i < 35; i++){
            sensor[chipselect] = getVal(0b10000000,chipselect);//request_id(chipselect);
            if(sensor[chipselect] == 229) temp_num++;
            __delay_ms(1);
        }

        if (temp_num > 2) num_sen++;
    }

    number_sensor = 2;

    return num_sen;
}
int error_check(void){

    int chipselect = 0;
    for (chipselect = 0; chipselect < 7; chipselect++)
    {
        sensor[chipselect] = 0;
        sensor[chipselect] = getVal(0b10000000,chipselect);
        if (sensor[chipselect] == 0xE5)
        {
            setbit(number_sensor, chipselect);
        }
        else clrbit(number_sensor, chipselect);

    }
    return 0;
}
int uartCheck(void){

    int recieve = 0;
    int clear = 0;

    if(RCIF == 1){
        recieve = RCREG;
        RCIF = 0;
        clear = RCREG;
        clear = RCREG;

    }
     if(FERR == 1 || OERR == 1){
            RCSTAbits.CREN = 0;
            RCSTAbits.CREN = 1;
            TXEN = 0;
            TXEN = 1;
            TXIF = 0;
            RCIF = 0;
            RCREG = 0;
            TXREG = 0;
    }
    return recieve;
}
void monitorUART(void){

    int tempUART = uartCheck();

    //Alive Status Check
    if(tempUART == 0b11110000){
            while(TXIF == 0);
            if(TXIF == 1){
                TXREG = 0b00001111;
                TXIF = 0;

        }
    }
    //Request Number of sensors
    if(tempUART == 0b11100000){
        while(TXIF == 0);
        if(TXIF == 1){
            TXREG = SEN_NUM;
            TXIF = 0;
        }
    }
    //Request Data 1
    if(tempUART == 0b10000001){
        while(TXIF == 0);
        if(TXIF == 1){
            TXREG = CON1;
            TXIF = 0;

        }
    }
    //Request Data 2
    if(tempUART == 0b10000011){
        while(TXIF == 0);
        if(TXIF == 1){
            TXREG = CON2;
            TXIF = 0;
        }
    }
    //Request Data 2
    if(tempUART == 0b10000111){
        while(TXIF == 0);
        if(TXIF == 1){
            TXREG = CON3;
            TXIF = 0;
        }
    }

    if(tempUART == 0b10001111){
        while(TXIF == 0);
        if(TXIF == 1){
            TXREG = CON4;
            TXIF = 0;
        }
    }
}
void interrupt com1(void){
    int clear = 0;

    if(PORTBbits.RB1 == 1){
        if(RCIF == 1){
            clear = RCREG;
            clear = RCREG;
            clear = RCREG;
            RCIF = 0;
        }
    }
    else{
        while(PORTBbits.RB1 == 0){
            monitorUART();
        }
    }
    RCIF = 0;
}
void wash_alg(void){

    for(int chipselect = 0; chipselect < 8; chipselect++){
        if(1){

            //if(chipselect == 0) operation[chipselect] = three_axis(chipselect);
            operation[chipselect] = three_axis(chipselect);
            //if(chipselect == 0) CON3 = three_axis(chipselect);

            if(lockout[chipselect] > 0) {
                __delay_ms(30);
                lockout[chipselect]--;
            }

            if(lockout[chipselect] == 0 && timeout[chipselect] > 0 && pass1[chipselect] == 1 && pass2[chipselect] == 0 && pass3[chipselect] == 0 && pass4[chipselect] == 0){
                timeout[chipselect]--;
                if(timeout[chipselect] == 0){
                    pass1[chipselect] = 0;
                    pass2[chipselect] = 0;
                    pass3[chipselect] = 0;
                    pass4[chipselect] = 0;
                    if(chipselect == 0) PORTA = 0;
                    wash_val[chipselect] = 0;
                    wash_count[chipselect] = 0;
                    //if(chipselect == 0) CON3 = 0;

                    //CON3 = 0;
                }
            }
            if(operation[chipselect] > 10 && pass1[chipselect] == 0 && pass2[chipselect] == 0 && pass3[chipselect] == 0 && pass4[chipselect] == 0 && lockout[chipselect] == 0) {
                //pass1[chipselect] = 1;

                //if(chipselect == 0) CON3 = operation[chipselect];
                wash_val[chipselect] = wash_val[chipselect] + operation[chipselect];

                wash_count[chipselect]++;

                timeout[chipselect] = 10;
                //if(chipselect == 0) CON3 = operation[chipselect];
                if(pass1a[chipselect] == 0 && pass1b[chipselect] == 0 && pass1c[chipselect] == 0 && lockout[chipselect] == 0){
                    pass1a[chipselect] = 1;
                    lockout[chipselect] = 2;
                }
                if(pass1a[chipselect] == 1 && pass1b[chipselect] == 0 && pass1c[chipselect] == 0 && lockout[chipselect] == 0){
                    pass1b[chipselect] = 1;
                    lockout[chipselect] = 2;
                }
                if(pass1a[chipselect] == 1 && pass1b[chipselect] == 1 && pass1c[chipselect] == 0 && lockout[chipselect] == 0){
                    pass1c[chipselect] = 1;
                    lockout[chipselect] = 2;
                }
                if(pass1a[chipselect] == 1 && pass1b[chipselect] == 1 && pass1c[chipselect] == 1 && lockout[chipselect] == 0){
                    pass1[chipselect] = 1;
                    lockout[chipselect] = 2;
                    if(chipselect == 0) PORTAbits.RA0 = 1;
                }
            }

            if(operation[chipselect] > 30 && pass1[chipselect] == 1 && pass2[chipselect] == 0 && pass3[chipselect] == 0 && pass4[chipselect] == 0 && lockout[chipselect] == 0) {
                pass2[chipselect] = 1;
                wash_val[chipselect] = wash_val[chipselect]/wash_count[chipselect];
                wash_val[chipselect] = wash_val[chipselect] * 0.5;
                end_val[chipselect] = wash_val[chipselect] + 65;
                //if(chipselect == 0) CON3 = end_val[0];

                if(chipselect == 0)CON1 = 1;
                if(chipselect == 0)CON2 = 0;


                //setbit(CON1,chipselect);
                //clrbit(CON2,chipselect);
                //clrbit(CON3,chipselect);
                if(chipselect == 0) PORTAbits.RA1 = 1;
                cancel[chipselect] = 100;
                lockout[chipselect] = 5;//50
                timeout[chipselect] = 0;
            }

            if(operation[chipselect] < 10 && pass1[chipselect] == 1 && pass2[chipselect] == 1 && pass4[chipselect] == 0 && pass3[chipselect] == 0 && lockout[chipselect] == 0) {
                pass3[chipselect] = 1;
                if(chipselect == 0) PORTAbits.RA2 = 1;
                cancel[chipselect]--;
                lockout[chipselect] = 5;//50

                //timeout[chipselect] = 255;

            }
            if(operation[chipselect] < 10 && pass1[chipselect] == 1 && pass2[chipselect] == 1 && pass4[chipselect] == 0 && pass3[chipselect] == 1 && lockout[chipselect] == 0) {
                cancel[chipselect]--;
            }
            if(operation[chipselect] > 20 && pass1[chipselect] == 1 && pass2[chipselect] == 1 && pass4[chipselect] == 0 && pass3[chipselect] == 1 && lockout[chipselect] == 0) {
                cancel[chipselect] = 100;
            }

            if(operation[chipselect] > end_val[chipselect] && pass1[chipselect] == 1 && pass2[chipselect] == 1 && pass3[chipselect] == 1 && pass4[chipselect] == 0 && lockout[chipselect] == 0 && cancel[chipselect] == 0) {
                //set actual to 65
                pass4[chipselect] = 1;
                if(chipselect == 0) PORTAbits.RA3 = 1;
                lockout[chipselect] = 5;//50
                timeout2[chipselect] = 50;

            }

            if(operation[chipselect] < 8 && pass1[chipselect] == 1 && pass2[chipselect] == 1 && pass3[chipselect] == 1 && pass4[chipselect] == 1 && lockout[chipselect] == 0){
                if(chipselect == 0)CON1 = 0;
                if(chipselect == 0)CON2 = 1;


                //clrbit(CON1,chipselect);
                //setbit(CON2,chipselect);
                //clrbit(CON3,chipselect);

                pass1[chipselect] = 0;
                pass2[chipselect] = 0;
                pass3[chipselect] = 0;
                pass4[chipselect] = 0;
                pass5[chipselect] = 0;
                if(chipselect == 0) PORTA = 0xFF;
                wash_val[chipselect] = 0;
                wash_count[chipselect] = 0;
                lockout[chipselect] = 50;
                timeout2[chipselect] = 0;


            }

             if(timeout2[chipselect] > 0 && pass1[chipselect] == 1 && pass2[chipselect] == 1 && pass3[chipselect] == 1 && pass4[chipselect] == 1){
                timeout2[chipselect]--;
                if(timeout2 == 0){
                    if(chipselect == 0) PORTA = 0;
                    if(chipselect == 0) PORTAbits.RA2 = 1;
                    //lockout[chipselect] = 20;
                    pass4[chipselect] = 0;

                }
             }
            if(cancel[chipselect] == 0){
                pass4[chipselect] = 1;
                pass3[chipselect] = 1;
                cancel[chipselect] = 255;

            }
            //if(chipselect == 0) CON3 = cancel[0];

            __delay_ms(50);
        }
    }
}
void dryer_algorthum(void){
    //This determins if the sensor is in active mode or in
    //Determine the mode that the sensor is in
    int chipselect = 0;
    past_op4 = 0x00; //This clears the data stores so that it does not get a false read
    past_op4 = past_op3; // This sets the the current operations tp the past operations
    past_op3 = past_op2;
    past_op2 = past_op1;
    past_op1 = current_op;
    current_op = 0x00; //This clears the current operation so that it can be changed

    //This detects if each sensor is currently set into active mode or none active.
    for(chipselect = 0; chipselect < 8; chipselect++)
    {
        operation[chipselect] = 0x00;//This clears the data stores so that it does not get a false read
        operation[chipselect] = three_axis(chipselect);
        //if(chipselect == 0) CON3 = operation[1];
        if(operation[chipselect] > 10) //This looks to see if each sensor it set in active mode
        {
            setbit(current_op,chipselect);//Sets a 1 if the sensor is in active mode
        }
    }

    error_check(); //This is error detection and also determine which sensor are active currently
    //This is the main algoritum
    for(chipselect = 0; chipselect < 8; chipselect++)//out1
    {
        if(testbit(number_sensor,chipselect))//Determines if sensor mode 0-7 nonerror
        {
            if(testbit(current_op,chipselect))// looks for the active flag is set
            {
                if(testbit(past_op1,chipselect))//looks fot the active flag is set in the past_op
                {
                    if(testbit(past_op2,chipselect))
                    {
                        if(testbit(past_op3,chipselect))
                        {
                            if(testbit(past_op4,chipselect))
                            {
                                setbit(CON1,chipselect);
                                clrbit(CON2,chipselect);
                            }
                            else
                            {
                                setbit(CON2,chipselect);
                                clrbit(CON1,chipselect);
                            }
                        }
                        else
                        {
                            setbit(CON2,chipselect);
                            clrbit(CON1,chipselect);
                        }
                    }
                    else
                    {
                        setbit(CON2,chipselect);
                        clrbit(CON1,chipselect);
                    }
                }
                else
                {
                    setbit(CON2,chipselect);
                    clrbit(CON1,chipselect);
                }
            }
            else
            {
                if(testbit(past_op1,chipselect))// looks to see if perivouslty the sensor was in active mode
                {
                    setbit(CON1,chipselect);
                    clrbit(CON2,chipselect);
                }
                else
                {
                    setbit(CON2,chipselect);
                    clrbit(CON1,chipselect);
                }
            }
        }
        else// Sets values in number_sensor = 0 to 8 in CON1
        {
            if(sensor[chipselect] > 0){
            //setbit(CON3,chipselect);
            //clrbit(CON1,chipselect);
            //clrbit(CON2,chipselect);
            }
        }
    }
    for(chipselect = 0; chipselect < 8; chipselect++)
    {
        if((testbit(CON1,chipselect) == 1) && (testbit(CON2,chipselect) == 1))
        {
            //setbit(CON3,chipselect);
            //clrbit(CON1,chipselect);
            //clrbit(CON2,chipselect);
        }
    }
}
int three_axis(int chipselect){

    double tempT = 0;

    for(int i = 0; i < avg_sensor; i++)
    {
        __delay_ms(10);
        x1 = getVal(X_AXIS,chipselect);
        __delay_us(500);
        x2 = getVal(X_AXIS,chipselect);

        y1 = getVal(Y_AXIS,chipselect);
        __delay_us(500);
        y2 = getVal(Y_AXIS,chipselect);

        z1 = getVal(Z_AXIS,chipselect);
        __delay_us(500);
        z2 = getVal(Z_AXIS,chipselect);

        xT = abval(x2 - x1);
        yT = abval(y2 - y1);
        zT = abval(z2 - z1);



        tempT = tempT + ((xT+yT+zT)/3);
    }
    if(chipselect == 0) return (tempT/avg_sensor)/3;
    else return tempT/avg_sensor;
    //return tempT;
}
int getVal(int addr, int chipselect){

    int repeat = 5;
    int temp_val = 0;

    while(repeat > 0){

        RCIE = 0;

        LATD = chipselect;
        SSPBUF = addr;
        while(!SSPSTATbits.BF);
        SSPBUF = 0;
        while(!SSPSTATbits.BF);
        LATD = 7;
        temp_val = SSPBUF;

        if(temp_val > 0) {
            RCIE = 1;
            return temp_val;
        }
        else repeat--;

        RCIE = 1;

        __delay_ms(1);
    }
    //if(repeat == 1 && addr != 0b10000000) return operation[chipselect];
    return temp_val;
}
void main(void){
    int temp_con2 = 0;

    intialize_pic();
    intialize_sensor();

    SEN_NUM = number_of_sensor();

    CON1 = 0;
    CON2 = 0;//0xFF;
    CON3 = 0;

    avg_sensor = 20;

    if(PORTBbits.RB3 == 0)
    {
        while(1)
        {
            dryer_algorthum();
            //time_delay();//Delay set for 1 minute
        }
    }
    if(PORTBbits.RB3 == 1)
    {
        PORTA = 0;
        while(1)
        {
            wash_alg();

            //washer_algorthum();
            //time_delay();
        }
    }
}
