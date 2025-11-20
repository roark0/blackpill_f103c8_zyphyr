#include "REGX51.H"

#define SCANCODESET_1 1
#define SCANCODESET_2 2
#define SCANCODESET_3 3

#define SCANCODESET SCANCODESET_2

#define MACHINE_1904C 0x10
#define MACHINE_9000 0x13

#define TRUE 1
#define FALSE 0

#define UINT8 unsigned char
#define UINT16 unsigned short

#define TRANSDELAY 20

void Serial_Init(void);
void SendChar(UINT8 ch);
UINT8 ReceiveChar(void);
UINT8 Scankey1904C(void);  //Scan 1904c keyboard function
UINT8 ScanOthekey9000(void);

volatile unsigned int gKey_Buffer  = 0;      //common keycode buffer
volatile unsigned int gFunc_Buffer = 0;      //function keycode buffer
volatile bit Flag_Updown           = FALSE;  //the flag of function key keeping down or not
volatile bit Flag_Key              = FALSE;  //the flag of any key keeping down or not

volatile unsigned char Key1904c = 0x00;
volatile unsigned char Key9000c = 0x00;

volatile char G_cComRecData = 0x00;

#define MAX9000SCAN 3
volatile char G_uc9000ScanInterval = 0;

#define MAX1904SCAN 3
volatile char G_uc1904ScanInterval = 0;

//晶振11.0592M

void Delay(unsigned int t)
{
    unsigned int i, j;
    for (i = 0; i < t; i++)  //0.11ms
        for (j = 0; j < 7; j++)
            ;
}

UINT8 Key_Scan(void)
{
    unsigned char Key_Code = 0;
    P1                     = 0x00;  //line scan
    P2                     = 0xFF;
    Key_Code               = P2;
    if (Key_Code != 0xFF)
    {               //judge whether a key is down or not
        Delay(50);  //delay 10ms to remove key's dithering

        P2       = 0xFF;
        Key_Code = P2;
        if (Key_Code != 0xFF)
        {
            Flag_Key    = TRUE;  //symbol that a key is down
            gKey_Buffer = Key_Code;
            P2          = 0x00;
            P1          = 0xFF;
            Key_Code    = P1;
            gKey_Buffer = (gKey_Buffer << 8) + Key_Code;  //store the key scancode in the common key buffer

            return TRUE;
        }
    }
    else
    {
        Flag_Key = FALSE;  //no key is down

        return FALSE;
    }
}

unsigned int Key_Decode(unsigned int key)
{
    volatile unsigned int iKeyID = 0;
    switch (key)
    {
        //the breakcode of scancode is loaded in high byte
        //the makecode of scancode is loaded in low byte
        case 0xfdef:
            iKeyID = 0xF045;
            break;  //0
        case 0xfef7:
            iKeyID = 0xF016;
            break;  //1
        case 0xfdf7:
            iKeyID = 0xF01E;
            break;  //2
        case 0xfbf7:
            iKeyID = 0xF026;
            break;  //3
        case 0xf7f7:
            iKeyID = 0xF025;
            break;  //4
        case 0xeff7:
            iKeyID = 0xF02E;
            break;  //5
        case 0xdff7:
            iKeyID = 0xF036;
            break;  //6
        case 0xbff7:
            iKeyID = 0xF03D;
            break;  //7
        case 0x7ff7:
            iKeyID = 0xF03E;
            break;  //8
        case 0x7fef:
            iKeyID = 0xF046;
            break;  //9

        // 8
        case 0xfdfd:
            iKeyID = 0xF01C;
            break;  //A
        case 0xdffe:
            iKeyID = 0xF032;
            break;  //B
        case 0xf7fe:
            iKeyID = 0xF021;
            break;  //C
        case 0xf7fd:
            iKeyID = 0xF023;
            break;  //D
        case 0xf7fb:
            iKeyID = 0xF024;
            break;  //E
        case 0xeffd:
            iKeyID = 0xF02B;
            break;  //F
        case 0xdffd:
            iKeyID = 0xF034;
            break;  //G
        case 0xbffd:
            iKeyID = 0xF033;
            break;  //H
        case 0xbfef:
            iKeyID = 0xF043;
            break;  //I
        case 0x7ffd:
            iKeyID = 0xF03B;
            break;  //J
        case 0xfbef:
            iKeyID = 0xF042;
            break;  //K
        case 0xf7df:
            iKeyID = 0xF04B;
            break;  //L
        case 0x7ffe:
            iKeyID = 0xF03A;
            break;  //M
        case 0xbffe:
            iKeyID = 0xF031;
            break;  //N
        case 0xbfdf:
            iKeyID = 0xF044;
            break;  //O
        case 0xfbdf:
            iKeyID = 0xF04D;
            break;  //P
        case 0xfdfb:
            iKeyID = 0xF015;
            break;  //Q
        case 0xeffb:
            iKeyID = 0xF02D;
            break;  //R
        case 0xfbfd:
            iKeyID = 0xF01B;
            break;  //S
        case 0xdffb:
            iKeyID = 0xF02C;
            break;  //T
        case 0x7ffb:
            iKeyID = 0xF03C;
            break;  //U
        case 0xeffe:
            iKeyID = 0xF02A;
            break;  //V
        case 0xfbfb:
            iKeyID = 0xF01D;
            break;  //W
        case 0xfbfe:
            iKeyID = 0xF022;
            break;  //X
        case 0xbffb:
            iKeyID = 0xF035;
            break;  //Y
        case 0xfdfe:
            iKeyID = 0xF01A;
            break;  //Z

        case 0xdfef:
            iKeyID = 0xF05A;
            break;  //enter
        case 0xefef:
            iKeyID = 0xF029;
            break;  //space
        case 0xf7ef:
            iKeyID = 0xF049;
            break;  //dot	 		// 7
        case 0x7fdf:
            iKeyID = 0xF066;
            break;  //backspace
        case 0xfefb:
            iKeyID = 0xF00D;
            break;  //TAB
        case 0xfefd:
            iKeyID = 0xF058;
            P3_4   = P3_4 ^ 1;
            break;  //capslock LED

        default:
            iKeyID = 0;
            break;
    }
    Delay(100);

    return iKeyID;
}

void Function_Key_Decode(unsigned int Func_Key)  //Func_Key	= gKey_Buffer
{
    switch (Func_Key)
    {
        case 0xFEFE:  //shift
            while (1)
            {
                Key_Scan();
                if (Flag_Key == FALSE)
                {  //if no key is down the shift key has released and break
                    break;
                }

                if (gKey_Buffer != 0xFEFE)
                {
                    if ((gKey_Buffer & 0xFF) == 0xFE)
                    {
                        gKey_Buffer = gKey_Buffer | 0x01FE;  //the common key is the same line as the shift
                    }
                    else if (((gKey_Buffer >> 8) & 0xFE) == 0xFE)
                    {
                        gKey_Buffer = gKey_Buffer | 0xFE01;  //the common key is the same column as the shift
                    }
                    else
                    {
                        gKey_Buffer = gKey_Buffer | 0x0101;  //the common key isn't the same
                    }
                    //line and column as the shift
                    gFunc_Buffer = 0xF012;  // the bread and make code of shift key

                    Flag_Updown = 1;  //the symbol of the function key down
                    break;
                }
            }
            break;  //LShift is down
        case 0xFEEF:
            while (1)
            {
                Key_Scan();
                if (Flag_Key == FALSE)
                {
                    break;
                }

                if (gKey_Buffer != 0xFEEF)
                {
                    if ((gKey_Buffer & 0xFF) == 0xEF)
                    {
                        gKey_Buffer = gKey_Buffer | 0x01EF;
                    }
                    else if (((gKey_Buffer >> 8) & 0xff) == 0xFE)
                    {
                        gKey_Buffer = gKey_Buffer | 0xFE10;
                    }
                    else
                    {
                        gKey_Buffer = gKey_Buffer | 0x0110;
                    }

                    gFunc_Buffer = 0xF014;

                    Flag_Updown = 1;
                    break;
                }
            }
            break;  //LCtrl
        default:
            Flag_Updown = 0;
            break;  //no function key is down
    }
}

void Serial_trans(unsigned char Scancode)
{
    //the XT keyboard protocol: the start bit is always 0,then transmit the 8 data bits from low to high bit
    //and the every bit is read by host PC on the falling edge of the clock signal.
    //notice:the frequency of the clock is about 10~15KHz,and the interval of makecode and breakcode must be 50 microseconds at least
    Delay(TRANSDELAY);

    SendChar(Scancode);
}

void Transmit_Key(void)
{
    volatile unsigned int Key;

    if (TRUE == Key_Scan())
    {
        Function_Key_Decode(gKey_Buffer);  //judge whether the function key is down or not
        Key = Key_Decode(gKey_Buffer);     //decode the common keycode
        if (Flag_Key == TRUE)
        {  //if a function key is down the makecode is transmitted first then the common or second key makecode is transmitted
            //at last the second breakcode is transmitted followed by the first function key's breakcode
            if (Flag_Updown == 1)
            {
                Serial_trans((gFunc_Buffer >> 8) & 0xff);
                Serial_trans(gFunc_Buffer & 0xff);
                Flag_Updown = 0;  //must clear the flag of the function keeping down
            }
            else
            {
                Serial_trans((Key >> 8) & 0xff);
                Serial_trans(Key & 0xff);
            }

            Serial_trans((Key >> 8) & 0xff);
            Serial_trans(Key & 0xff);
        }
    }
    else if (TRUE == ScanOthekey9000())
    {
        Serial_trans(0xF0);
        Serial_trans(Key9000c);

        Serial_trans(0xF0);
        Serial_trans(Key9000c);
    }
}

UINT8 ScanOthekey9000(void)
{
    volatile unsigned char tmp = 0x00;

    Key9000c = 0x00;

    P0  = 0x06;
    tmp = P0;
    if ((tmp & 0x06) == 0x06)
    {
        return FALSE;
    }

    Delay(100);  //delay 10ms to remove key's dithering

    P0  = 0x06;
    tmp = P0;
    if ((tmp & 0x06) == 0x06)
        return FALSE;
    Key9000c = tmp & 0x06;

    P0  = 0xB0;
    tmp = P0;
    if ((tmp & 0xB0) == 0xB0)
        return FALSE;
    Key9000c |= (tmp & 0xB0);

    P0 = 0x0F;

    //Convert key value
    switch (Key9000c)
    {
        case 0xA2:
            tmp = 0x4E;
            break;
        case 0x34:
            tmp = 0x5D;
            break;
        case 0x94:
            tmp = 0x55;
            break;
        default:
            return FALSE;
            break;
    }

    Key9000c = tmp;

    return TRUE;
}

void Initial_Timer0(void)
{
    TMOD = 0x01;  //timer0 mode:16bits for timer/count
    TR0  = 0;     //stop the timer
    TH0  = 0x28;  //set the initial value in register
    TL0  = 0x01;
    ET0  = 1;  //timer interrup is enable

    G_uc9000ScanInterval = 0;

    EA  = 1;
    TR0 = 1;  //TCON: start timer0
}

//timer0 interrupt
void Timer0(void) interrupt 1
{
    TR0 = 0;

    G_uc9000ScanInterval += 1;
    if (G_uc9000ScanInterval >= MAX9000SCAN)
    {
        Transmit_Key();  //transmitting function
        G_uc9000ScanInterval = 0;
    }

    TH0 = 0x28;
    TL0 = 0x01;
    TR0 = 1;
}

void main(void)
{
    Initial_Timer0();
    Serial_Init();
    P3_4 = 1;  //LED on
    P3_5 = 0;  //LED on

    while (1)
    {
        ;
    }
}

//////////////////////////////////////////////////////////////////////////////
void Serial_Init(void)
{
    SCON |= 0xD0; /* mode 3: 9-bit UART, enable receiver   */
    TMOD |= 0x20; /* timer 1 mode 2: 13-Bit                */
    PCON |= 0x80; /* PCON: SMOD=0, Baud generator          */
    TH1 = 0xFD;   /* reload value 19200 baud @ 11.059MHz  */
    TL1 = 0xFD;   // 0xFD(19200)

    ET1 = 0;  //禁止中断
    ES  = 1;
    TR1 = 1; /* timer 1 run                           */
}

void SendChar(UINT8 ch)
{
    P    = 0;  //奇偶校验位
    ACC  = (UINT8)ch;
    TB8  = P;
    SBUF = ch;

    while (0 == TI)
        ;
    TI = 0;

    P3_4 = ~P3_4;
}

UINT8 ReceiveChar(void)
{
    UINT8 ch = (UINT8)SBUF;

    P3_5 = ~P3_5;

    return ch;
}

/*---------------------------------------------------------------------------------------
 * Function   : Serial_ISR
 * Description: This function is the serial receiver / transmitter interrupt. 
 * Arguments  : None
 * Returns    : The receiving data is stord at global variable DP_Serial_REC. 
 *--------------------------------------------------------------------------------------*/
void Serial_ISR(void) interrupt 4
{ /* use registerbank 2 for interrupt */
    if (RI)
    {                                  /* if receiver interrupt            */
        G_cComRecData = ReceiveChar(); /* Receive character                */

        RI = 0; /* clear interrupt request flag     */
    }
}