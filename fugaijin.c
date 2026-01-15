#include <reg52.h>
#include <absacc.h>
//原070402
sbit warmer       = P1 ^ 4;  //加热控制
sbit DQ           = P1 ^ 5;  //定义通信端口
sbit LED_1        = P1 ^ 0;  //定义通信端口
sbit LED_2        = P1 ^ 1;  //定义通信端口
sbit LED_3        = P1 ^ 2;  //定义通信端口
sbit LED_4        = P1 ^ 3;  //定义通信端口
sbit SET_S        = P2 ^ 7;  //定义通信端口
sbit Flag         = P3 ^ 2;  //定义通信端口
unsigned int qian = 0;
unsigned char jia;    //定义全局计数变量
unsigned int jj = 0;  //定义全局计数变量
char bdata haha;
unsigned char shu     = 2;
unsigned char set_num = 0;
//int wendu[]={ 0x12A,0x170,0x1c0,0x22E};
//unsigned int wendu[]={ 0xfb,0x12d,0x173,0x1c3};
//25.2,30.2, 37.2,   45.2  , 56

//unsigned int wendu[]={ 0x0fe,0x130,0x176,0x1c6};
//25.4,30.4, 37.4,   45.4  , 56
unsigned int wendu_set[] = {0x0fd, 0x12f, 0x175, 0x1c5};
unsigned int wendu[]     = {0x0fd, 0x12f, 0x175, 0x1c5};
//25.3,30.3, 37.3,   45.3  , 56

void delay(unsigned int count);
void Init_DS18B20(void);
bit tmpread(void);
unsigned char ReadOneChar(void);
void WriteOneChar(unsigned char dat);
unsigned int ReadTemperature(void);
void display_tempmain(int wen);
void intersvr0(void);

void main(void)  //主函数
{
    unsigned int temp, aa, out_num;
    P0    = 0xff;  //初始化
    P2    = 0xff;
    P1    = 0xef;
    LED_3 = 0;
    IT0   = 1;  //下降沿触发
    EA    = 1;  // 中断允许
    EX0   = 1;
    switch (P2 & 0x70)  //拔码开关设定校准数字,开关2、3、4位共设定0、1、2、3、4、5、6、7个数值
    {
        case 0x00:
        {
            set_num = 0;
            break;
        }
        case 0x10:
        {
            set_num = 2; // SW7
            break;
        }
        case 0x20:
        {
            set_num = 4;
            break;
        }
        case 0x30:
        {
            set_num = 6;
            break;
        }
        case 0x40:
        {
            set_num = 8;
            break;
        }
        case 0x50:
        {
            set_num = 10;
            break;
        }
        case 0x60:
        {
            set_num = 12;
            break;
        }
        case 0x70:
        {
            set_num = 14;
            break;
        }
    }

    if (SET_S == 1)  //拔码开关设定校准方向，开关1=0（加），开关1=1（减）
    {
        wendu[shu] = wendu_set[shu] - set_num;
    }
    else
    {
        wendu[shu] = wendu_set[shu] + set_num;
    }
    //外中断0允许                       //外中断0允许
    aa = wendu[shu];
    while (1)  //主循环
    {
        switch (P2 & 0x70)  //拔码开关设定校准数字,开关2、3、4位共设定0、1、2、3、4、5、6、7个数值
        {
            case 0x00:
            {
                set_num = 0;
                break;
            }
            case 0x10:
            {
                set_num = 2;
                break;
            }
            case 0x20:
            {
                set_num = 4;
                break;
            }
            case 0x30:
            {
                set_num = 6;
                break;
            }
            case 0x40:
            {
                set_num = 8;
                break;
            }
            case 0x50:
            {
                set_num = 10;
                break;
            }
            case 0x60:
            {
                set_num = 12;
                break;
            }
            case 0x70:
            {
                set_num = 14;
                break;
            }
        }

        if (SET_S == 1)  //拔码开关设定校准方向，开关1=0（加），开关1=1（减）
        {
            wendu[shu] = wendu_set[shu] - set_num;
        }
        else
        {
            wendu[shu] = wendu_set[shu] + set_num;
        }  //外中断0允许
        jj++;
        temp = ReadTemperature();
        if (jj > 300)
        {
            if (qian < temp)
                aa = wendu[shu] - 20;
            else
                aa = wendu[shu];
            jj   = 0;
            qian = temp;
        }
        //	else {if(temp<aa)  ;}
        //	if(temp<wendu[shu])
        if ((temp < aa) & (temp > 0))
            warmer = 1;
        else
            warmer = 0;
        //	temperature=temp;
        //    if((temp<wendu[shu]+2)&(temp>wendu[shu]-2))
        //      aa=wendu[shu];
        //	else aa=temp;
        if (SET_S == 1)
        {
            out_num = temp - 4 + set_num;
        }
        else
        {
            out_num = temp - 4 - set_num;
        }
        //out_num=temp-4;
        display_tempmain(out_num);

        if (jia == 1)
        {
            warmer = 0;  //关掉加热
            if (shu > 2)
                shu = 0;
            else
                shu++;
            haha  = LED_4;
            LED_4 = LED_3;
            LED_3 = LED_2;
            LED_2 = LED_1;
            LED_1 = haha;
            delay(300);
            //EA=1;                       // 中断允许
            //EX0=1;
            jia = 0;
        }
    }
}

void delay(unsigned int count)  //延时函数
{
    unsigned int i;
    while (count)
    {
        i = 200;
        while (i > 0)
            i--;
        count--;
    }
}
//初始化函数
void Init_DS18B20(void)
{
    unsigned char x = 0;
    unsigned int i;
    //DQ = 1;    //DQ复位
    //delay(10);  //稍做延时
    DQ = 0;  //单片机将DQ拉低
    //delay(600); //精确延时 大于 480us~960之间
    i = 103;
    while (i > 0)
        i--;
    DQ = 1;  //拉高总线15~60
    //delay(65);	//60~240us
    i = 4;
    while (i > 0)
        i--;
    x = DQ;  //稍做延时后 如果x=0则初始化成功 x=1则初始化失败
}
//读一个字节
bit tmpread(void)  // 读取数据的一位
{
    unsigned int i;
    bit dat;
    DQ = 0;
    i++;
    DQ = 1;
    i++;
    i++;  //延时
    dat = DQ;
    i   = 8;
    while (i > 0)
        i--;  // 延时
    return (dat);
}

unsigned char ReadOneChar(void)  //读一个字节
{
    unsigned char i, j, dat;
    dat = 0;
    for (i = 1; i <= 8; i++)
    {
        j   = tmpread();
        dat = (j << 7) | (dat >> 1);
    }
    return (dat);
}

//写一个字节                                                                //写一个字节
void WriteOneChar(unsigned char dat)
{
    unsigned int i;
    unsigned char j;
    bit testb;
    for (j = 1; j <= 8; j++)
    {
        testb = dat & 0x01;
        dat >>= 1;
        if (testb)
        {
            DQ = 0;
            i++;
            i++;
            DQ = 1;
            i  = 8;
            while (i > 0)
                i--;
        }
        else
        {
            DQ = 0;
            i  = 8;
            while (i > 0)
                i--;
            DQ = 1;
            i++;
            i++;
        }
    }
}
//读取温度                                                                  //读取温度
unsigned int ReadTemperature(void)
{
    unsigned int tt, i;
    float Temp_mid;
    unsigned int ramvalue[9];
    //unsigned char a=0;
    //unsigned char b=0;
    unsigned int t;
    Init_DS18B20();
    delay(1);
    WriteOneChar(0xCC);  // 跳过读序号列号的操作
    WriteOneChar(0x44);  // 启动温度转换
    Init_DS18B20();
    delay(1);
    WriteOneChar(0xCC);  //跳过读序号列号的操作
    WriteOneChar(0xBE);  //读取温度寄存器等（共可读9个寄存器） 前两个就是温度
    for (i = 0; i < 9; i++)
        ramvalue[i] = ReadOneChar();
    tt = ramvalue[1];
    tt <<= 8;
    tt += ramvalue[0];
    Temp_mid = tt / 2 - (float)0.25 + (float)(ramvalue[7] - ramvalue[6]) / (float)(ramvalue[7]);
    t        = Temp_mid * 10;
    //a=a/2;            //低位右移4位，舍弃小数部分
    //b=b<<4;            //高位左移4位，舍弃符号位
    //t=a;

    return (t);
}
void display_tempmain(int wen)  //主程序温度显示函数
{
    unsigned int shu, shiwei, gewei, pp;
    shu = wen % 10;
    // shu<<=4;
    pp = wen / 10;
    if (pp < 65)
    {
        P2     = shu | 0XF0;
        gewei  = pp % 10;
        shiwei = pp / 10;
        shiwei = shiwei << 4;
        gewei  = gewei & 0x0F;
        pp     = gewei | shiwei;
        P0     = pp;
    }
}

//中断响应程序
void intersvr0(void) interrupt 0 using 1
{   //EX0=0;
    //EA=0;
    delay(80);
    if (Flag == 0)
    {
        jia = 1;
    }
    else
    {
        jia = 0;
    }
}
