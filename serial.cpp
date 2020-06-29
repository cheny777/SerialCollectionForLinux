#include "serial.h"

CSerial::CSerial()
{
    m_fd = -1;
}
CSerial::~CSerial()
{
    ClosePort();
}


bool CSerial::OpenPort(const char * name,int speed,int databits,int stop_bits,int parity)
{
    if(name == NULL)
    {
        return false;
    }
    long name_len = strlen(name);
    if(name_len<0 || name_len > 100)
    {
        return false;
    }
    ClosePort();
    //申请内存
    char * device = (char *)malloc(1024);
    //初始化内存
    memset(device,0,1024);
    //写入打开的名字
    sprintf(device,"%s",name);

    struct termios termios_old;
    //打开文件
    m_fd = open( device, O_RDWR | O_NOCTTY |O_NONBLOCK);//O_RDWR | O_NOCTTY | O_NDELAY   //O_NONBLOCK
    //释放内存
    free(device);
    //如果文件描述符小于零就返回，打开失败
    if (m_fd < 0)
    {
        m_fd = -1;
        return false;
    }
    //获得标准输入的终端参数，将获得的信息保存在termios_old变量中
    tcgetattr(m_fd , &termios_old);

    SetPara(speed,databits,stop_bits,parity);
    return true;
}

void CSerial::ClosePort()
{
    struct termios termios_old;
    if(m_fd > 0)
    {
        tcsetattr (m_fd, TCSADRAIN, &termios_old);
        ::close (m_fd);
        m_fd = -1;
        usleep(100000);
    }
}


int CSerial::BaudRate( int baudrate)
{
    switch(baudrate)
    {
    case 2400:
        return (B2400);
    case 4800:
        return (B4800);
    case 9600:
        return (B9600);
    case 19200:
        return (B19200);
    case 38400:
        return (B38400);
    case 57600:
        return (B57600);
    case 115200:
        return (B115200);
    default:
        return (B115200);
    }
}
void CSerial::SetPara(int speed,int databits , int stopbits ,int parity )
{
    struct termios termios_new;
    bzero( &termios_new, sizeof(termios_new));//等价于memset(&termios_new,sizeof(termios_new));
    cfmakeraw(&termios_new);//就是将终端设置为原始模式
    termios_new.c_cflag=BaudRate(speed);
    termios_new.c_cflag |= CLOCAL | CREAD;
    //  termios_new.c_iflag = IGNPAR | IGNBRK;

    termios_new.c_cflag &= ~CSIZE;
    switch (databits)
    {
    case 0:
        termios_new.c_cflag |= CS5;
        break;
    case 1:
        termios_new.c_cflag |= CS6;
        break;
    case 2:
        termios_new.c_cflag |= CS7;
        break;
    case 3:
        termios_new.c_cflag |= CS8;
        break;
    default:
        termios_new.c_cflag |= CS8;
        break;
    }

    switch (parity)
    {
    case 0:  				//as no parity
        termios_new.c_cflag &= ~PARENB;    //Clear parity enable
        //  termios_new.c_iflag &= ~INPCK; /* Enable parity checking */  //add by fu
        break;
    case 1:
        termios_new.c_cflag |= PARENB;     // Enable parity
        termios_new.c_cflag &= ~PARODD;
        break;
    case 2:
        termios_new.c_cflag |= PARENB;
        termios_new.c_cflag |= ~PARODD;
        break;
    default:
        termios_new.c_cflag &= ~PARENB;   // Clear parity enable
        break;
    }
    switch (stopbits)// set Stop Bit
    {
    case 1:
        termios_new.c_cflag &= ~CSTOPB;
        break;
    case 2:
        termios_new.c_cflag |= CSTOPB;
        break;
    default:
        termios_new.c_cflag &= ~CSTOPB;
        break;
    }
    tcflush(m_fd,TCIFLUSH); // 清除输入缓存
    tcflush(m_fd,TCOFLUSH); // 清除输出缓存
    termios_new.c_cc[VTIME] = 1;   // MIN与 TIME组合有以下四种：1.MIN = 0 , TIME =0  有READ立即回传 否则传回 0 ,不读取任何字元
    termios_new.c_cc[VMIN] = 1;  //    2、 MIN = 0 , TIME >0  READ 传回读到的字元,或在十分之一秒后传回TIME 若来不及读到任何字元,则传回0
    tcflush (m_fd, TCIFLUSH);  //    3、 MIN > 0 , TIME =0  READ 会等待,直到MIN字元可读
    tcsetattr(m_fd,TCSANOW,&termios_new);  //    4、 MIN > 0 , TIME > 0 每一格字元之间计时器即会被启动 READ 会在读到MIN字元,传回值或
}

int CSerial::WritePort(const char *data, int datalength )//index 代表串口号 0 串口/dev/ttyAMA1 ......
{
    if(m_fd <0){ return -1;}
    int len = 0, total_len = 0;//modify8.
    for (total_len = 0 ; total_len < datalength;)
    {
        len = 0;
        len = write(m_fd, &data[total_len], datalength - total_len);
        if (len > 0)
        {
            total_len += len;
        }
        else if(len <= 0)
        {
            len = -1;
            break;
        }
    }
    return len;
}

int CSerial::write22(const char *data, int datalength)
{
    if(m_fd <0){ return -2;}
    return write(m_fd, data, datalength);
}

int CSerial::ReadPort(unsigned char *data, int datalength)
{

    if(m_fd <0){ return -1;}
    int len = 0;
    //初始化指针
    memset(data,0,datalength);

    int max_fd = 0;
    /*
    fd_set的数据结构，实际上是一long类型的数组，
    每一个数组元素都能与一打开的文件句柄
    (不管是socket句柄，还是其他文件或命名管道或设备句柄)建立联系，
    建立联系的工作由程序员完成，当调用select()时，
    由内核根据IO状态修改fe_set的内容，
    由此来通知执行了select()的进程哪一socket或文件可读。
    */
    fd_set readset;
    struct timeval tv ={0};

    /*将set的所有位置0，如set在内存中占8位则将set置为00000000*/
    FD_ZERO(&readset);
    /*将fd加入set集合*/
    FD_SET((unsigned int)m_fd, &readset);
    max_fd = m_fd +1;
    tv.tv_sec=0;
    tv.tv_usec=1000;

    /*
    select函数的接口比较简单：
    int select(int nfds,  fd_set* readset,  fd_set* writeset,  fe_set* exceptset,  struct timeval* timeout);
    */

    /*功能：测试指定的fd可读？可写？有异常条件待处理？*/
    if (select(max_fd, &readset, NULL, NULL,&tv ) < 0)
    {
        return 0;
    }
    /*测试fd是否在set集合中*/
    int nRet = FD_ISSET(m_fd, &readset);

    if (nRet)
    {
        //读取fd数据
        //len = read(m_fd, data, datalength);
        unsigned char lindata[2];
        int lenn = 0;
        while (len <= 24)
        {
            if(read(m_fd, lindata, 1)>0)
            {
                //printf("%d: %X ",nnn,lindata[0]);
                data[len] = lindata[0];
                len++;
                lenn = 0;
            }
            else
            {

                lenn++;
                if(lenn >= 10000)
                {
                    printf("error rr\n");
                    break;
                }
            }
        }
        //printf("\n");
    }
    return len;
}

int CSerial::getf()
{
    return m_fd;
}



