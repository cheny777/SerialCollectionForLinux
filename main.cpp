#include <QCoreApplication>
#include "lymacroremote.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "serial.h"
#include <QString>
#include <QDebug>
#include <unistd.h>
#include <iostream>
#include <string>

#include <sstream>
#include <pthread.h>

using namespace std;

#define UART_AMOUNT 1
#define CHN_PER_UART 8
const char* uart_name={"/dev/ttyS1"};

#define ip_path "/home/Lynuc/Users/Config/Extends/svr_set.txt"
#define ci_path "/home/Lynuc/Users/Config/Extends/ci_set.txt"
#define type_path "/home/Lynuc/Users/Config/Extends/type_set.txt"

#define HEARTBEAT 365
#define SERICALERROR 366



bool OnLoadSvIP(unsigned char * ip)
{
    FILE * fp = fopen(ip_path,"rb");
    if(fp == NULL){
        fp = fopen(ip_path,"wb");
        if(fp == NULL)
        {
            return false;
        }
        fprintf(fp,"192,168,1,158#");
        fclose(fp);
        ip[0] = 192;
        ip[1] = 168;
        ip[2] = 1;
        ip[3] = 158;
        return true;
    }
    long a,b,c,d;
    long cnt =  fscanf(fp,"%ld,%ld,%ld,%ld#",&a,&b,&c,&d);
    fclose(fp);
    if(cnt != 4)
    {
        return false;
    }
    ip[0] = a;
    ip[1] = b;
    ip[2] = c;
    ip[3] = d;
    return true;

}


bool OnLoadCiType( char * type)
{
    FILE * fp = fopen(type_path,"rb");
    if(fp == NULL){
        fp = fopen(type_path,"wb");
        if(fp == NULL)
        {
            return false;
        }
        fprintf(fp,"c#");
        fclose(fp);
        * type = 'c';
        printf("ER1\n");
        return true;
    }
    char a;
    long cnt =  fscanf(fp,"%c#",&a);
    fclose(fp);
    if(cnt != 1)
    {
        printf("ER2\n");
        return false;
    }
    if(a == 'C' || a == 'c'  )
    {
        *type = 'c';
        printf("ER3\n");
        return true;
    }
    if( a == 'a' || a== 'A' )
    {
        *type = 'a';
        printf("ER4\n");
        return true;
    }

    printf("ER5\n");
    return false;

}


bool OnLoadSendTarget(long * addr)
{
    FILE * fp = fopen(ci_path,"rb");
    if(fp == NULL){
        fp = fopen(ci_path,"wb");
        if(fp == NULL)
        {
            return false;
        }
        fprintf(fp,"160,161,162,163,164,165,166,167,0,0,0,0,0,0,0,0,0,0,0,0,#");
        fclose(fp);
        addr[0] = 160;
        addr[1] = 161;
        addr[2] = 162;
        addr[3] = 163;
        addr[4] = 164;
        addr[5] = 165;
        addr[6] = 166;
        addr[7] = 167;
        addr[8] = 0;
        addr[9] = 0;
        addr[10] = 0;
        addr[11] = 0;
        addr[12] = 0;
        addr[13] = 0;
        addr[14] = 0;
        addr[15] = 0;
        addr[16] = 0;
        addr[17] = 0;
        addr[18] = 0;
        addr[19] = 0;
        return true;
    }
    for(int i =0; i<20;i++)
    {
        long cnt =  fscanf(fp,"%ld,",addr+i);
        if(cnt<1)
        {
            fclose(fp);
            return false;
        }
    }
    fclose(fp);
    return true;

}


/**
*  \brief Check all com ports, if the device nodes exist.
*
*  \return 0 OK
*			1~5 means which port do not exist.
*
*  \details
*/
int OnInitDev_UARTs()
{
    int i=0;
    //// check uart1 exist status
    for(; i<UART_AMOUNT; i++)
    {
        if(0 != access(uart_name,F_OK))
        {
            return i+1;
        }
    }
    return 0;
}


int ToLong(unsigned char *clong)
{
    int lo = 0;
    for(int i =0;i<4;i++)
    {
        lo= clong[i]|lo<<8;
    }
    return lo;
}


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    bool is_c = true;
    char type;

    printf("version V1.2\n");
    /*加载CI变量类型*/
    if(OnLoadCiType(&type) == true)
    {
        if(type == 'a')
        {
            is_c = false;
            printf("IS FALSE\n");
        }
        else
        {
            printf("IS true\n");
        }
    }

    /*设置是否打印信息*/
    int show_tag = 0;
    if(argc > 1)
    {
        show_tag = atoi(argv[1]);
    }

    /*初始化串口*/
    int failed_com = OnInitDev_UARTs();

    if(failed_com > 0)
    {
        printf("INIT DEV UART%d ERROR\n", failed_com);
        printf("Use ls /dev/ttyS* to check is the node is OK?\n");
        return -1;
    }


    /*加载IP*/
    unsigned char ipv[CHN_PER_UART];
    char char_ip[20];
    if( OnLoadSvIP(ipv)== false)
    {
        printf("Load Svr Set Err\n");
        return -1;
    }
    else
    {
        sprintf(char_ip,"%d.%d.%d.%d", ipv[0],ipv[1],ipv[2],ipv[3]);
        printf("LINK TO  %s... \n",char_ip);
    }

    long addr[20];//The address of controllor Macro
    long kk; //only for loop
    for(kk = 0; kk < 20; kk ++)
    {
        addr[kk] = 0;
    }
    //read address of macro from file or give a default address
    /*加载CI地址*/
    if( OnLoadSendTarget(addr)== false)
    {
        printf("Load Send Target Err\n");
        return -1;
    }
    unsigned int handle;
    /*连接网络*/
    if(is_c == true)
    {
        while(connectHost(&handle, char_ip) == false)//link to controllor.
        {
            usleep(100000);
        }
    }

    printf("LINK TO %d.%d.%d.%d  OK \n",ipv[0],ipv[1],ipv[2],ipv[3]);

    long t_send_cnt = 0;//How many force data to sent
    int f_send_addr[20];//address of macro

    for(kk = 0; kk < 20; kk ++)
    {
        if(addr[kk] != 0)
        {
            f_send_addr[t_send_cnt] = addr[kk];
            t_send_cnt ++;
        }
    }

    /*初始化串口*/
    CSerial serial;
    if(serial.OpenPort(uart_name,115200,8,1,0) == false)
    {
        printf("Can not open com %ld\n", kk);
        return -1;
    }

    unsigned char read_buf[512];
    long read_size;
    /*010301C2000A65CD*/
    /*010300500002C41A*/
    /*发送数据*/
    char str16a[8];
    str16a[0] = 0x01;
    str16a[1] = 0x03;
    str16a[2] = 0x01;
    str16a[3] = 0xC2;
    str16a[4] = 0x00;
    str16a[5] = 0x0A;
    str16a[6] = 0x65;
    str16a[7] = 0xCD;

    int m_sensor[5];
    unsigned char m_sensorChar[5][4];

    /*心跳*/
    double flagnum[1];
    flagnum[0] = 0;
    int CIH[1] = {HEARTBEAT};
    int heatnum = 0;

    /*清空报错*/
    double err[1] = {0};
    int CIerr[1] = {SERICALERROR};
    writeMacro(handle,CIerr,err,1);

    while (1)
    {
        heatnum++;
        if(heatnum == 20)
        {
            writeMacro(handle,CIH,flagnum,1);
            if(flagnum[0] == 10)
            {
                flagnum[0] =0;
            }
            flagnum[0]++;
            heatnum=0;
        }
        /*写数据*/
        int errnum = serial.WritePort(str16a,8);
        if(errnum !=8 )
        {
            printf("error0\n");
            continue;
        }
        usleep(5000);
        /*读取512个字节的数据*/

        read_size = serial.ReadPort(read_buf,512);

        /*01 03 14 00 00 00 94 FF FF FF 88 FF FF FF F7 FF FF FF F7 FF FF FF F4 BE 4A */
        if(read_size > 0)
        {
            read_buf[read_size] = '\0';
            if(show_tag == 3)
            {
                for(int i =0;i<read_size;i++)
                {
                    printf("%X ",read_buf[i]);
                }
                printf("\n");
            }

            if(read_size == 25)
            {
                if(read_buf[0] != 0x01 || read_buf[1] !=0x03 || read_buf[2] != 0x14)
                {
                    continue;
                }
                for(int i =0 ;i<5;i++)
                {
                    m_sensorChar[i][0] = read_buf[i*4+3];
                    m_sensorChar[i][1] = read_buf[i*4+4];
                    m_sensorChar[i][2] = read_buf[i*4+5];
                    m_sensorChar[i][3] = read_buf[i*4+6];
                    m_sensor[i] = ToLong(m_sensorChar[i]);

                    if(show_tag == 2)
                    {
                        printf("real:%d \n",m_sensor[i]);
                        printf("%d:%X %X %X %X\n",i,read_buf[i*4+3],read_buf[i*4+4],read_buf[i*4+5],read_buf[i*4+6]);
                    }
                }
            }

            if(show_tag == 1)
            {
                for(int i = 0;i<5;i++)
                {
                    cout<<m_sensor[i];
                    cout<<"  ";
                    if(i == 4)
                    {
                        cout<<endl;
                    }
                }
            }
            double sn_v[20];
            for(int kk = 0; kk < 5; kk ++)
            {
                sn_v[kk] = m_sensor[kk];
            }

            if(false==writeMacro(handle, f_send_addr, sn_v, t_send_cnt))
            {
                while(connectHost(&handle, char_ip) == false)//link to controllor.
                {
                    usleep(100000);
                }
                printf("re_connect ok\n");
            }
        }
        else
        {
            err[0] = 1;
            writeMacro(handle,CIerr,err,1);
            cout<<"serical error !"<<endl;
        }
    }

    //等各个线程退出后，进程才结束，否则进程强制结束了，线程可能还没反应过来；
    pthread_exit(NULL);
    return a.exec();
}
