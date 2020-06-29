#ifndef SERIAL_H
#define SERIAL_H
#include  <stdio.h>
#include  <stdlib.h>
#include  <unistd.h>
#include  <sys/types.h>
#include  <sys/signal.h>
#include  <sys/stat.h>
#include  <fcntl.h>
#include  <termios.h>
#include  <errno.h>
#include  <limits.h>
#include  <string.h>

class CSerial
{
public:
    CSerial();
    ~CSerial();
    bool OpenPort(const char * name,int speed=115200,int databits=8,int stop_bits=1,int parity=0);
    int WritePort(const char *data,int datalength);
    int write22(const char *data,int datalength);
    int ReadPort(unsigned char *data,int datalength);

    int getf();
    void ClosePort();
protected:
    int m_fd;
    void SetPara(int speed=115200,int databits=8,int stopbits=1,int parity=0);
    int BaudRate( int baudrate);
};
#endif
