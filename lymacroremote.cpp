#include "lymacroremote.h"
#include "tcpmacroreadwritedef.h"
#include <stdio.h>

#ifdef _WINDLL
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")
#else
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>


#define SOCKET int
#define __stdcall
#define WORD unsigned short
#define closesocket close
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif
enum
{
    STATE_UNCONNECT,
    STATE_CONNECT,
    STATE_BUSY
};

class macro_remote
{
private:
    SOCKET s_client;
    int s_state;
    int s_nOffset;
    _dataHead s_dataHead;
    char s_szData[DATA_SIZE];
    char s_szIP[20];

    int selectSocket();
    int recieveSocketData(char *);
    bool recieveData(char *, double *);
    int dataHandler(char *, int nLen, double *);
    bool isValidHead(_dataHead *);
    bool isEqualHead(_dataHead *);
public:
    macro_remote();
    bool  _connectHost(char * ip);
    void  _closeConnect();

    bool  _readMacro(int * macro, double * value, int count);
    bool  _writeMacro(int * macro, double * value, int count);
};
/**
*  \brief 建立到目标机器的socket链接
 *
 *  \param [out] socket链接的句柄
 *  \param [in] ip 目标机器的IP地址
 *  \return true succuss
 *  		false failed
 *
 *  \details 对一个IP地址重复connect可能会有问题
 *			创建一个类的实例，并调用该实例建立连接
*/
bool __stdcall connectHost(unsigned int *handle, char * ip)
{
    macro_remote *p = new macro_remote;
    bool ret = false;
    if(p)
    {
        *handle = (unsigned int)p;
        ret = p->_connectHost(ip);
        if(ret)
        {
            return true;
        }
        else
        {
            delete p;
            *handle = 0;
            return false;
        }
    }
    else
    {
        return false;
    }
}

bool __stdcall readNCName(unsigned int handle, char * filename, int count) //count should not more than 64.
{
    //buff is too long
    if(count > 64)
    {
        count = 64;
    }
    //buff is too short
    else if(count < 1)
    {
        return false;
    }
    int macro[8]={};
    double value[8]={};
    char* pFilename = (char*)&value;
    bool ret=false;
    macro[0] = -1;
    macro_remote *p = static_cast<macro_remote*>((void*)handle);
    if(p)
    {
        ret =  p->_readMacro(macro, value, 8);
        if(ret)
        {
//			 FILE *fp = fopen("d:\\tmp.txt","w");
            memcpy(filename, pFilename, count);
/*			fprintf(fp, filename);
            fprintf(fp, "\n");
            fprintf(fp, pFilename);
            fclose(fp);*/
        }
        return ret;
    }
    else
    {
        return false;
    }
}

/**
*  \brief 断开连接
 *
 *  \param [in] socket链接的句柄
 *  \return void
 *
 *  \details 断开连接
*/
void __stdcall closeConnect(unsigned int handle)
{
    macro_remote *p = static_cast<macro_remote*>((void*)handle);
    if(p)
    {
        p->_closeConnect();
    }
    delete p;
    return;
}

/**
*  \brief 读宏变量
 *
 *  \param [in] socket链接的句柄
 *  \param [in] 读Macro的地址数组
 *  \param [out] 读Macro的结果数组
 *  \param [in] 读Macro的数量
 *  \return true succuss
 *  		false failed
 *
 *  \details 当Macro地址是-1的时候读文件名（需要控制器端配合）
*/
bool __stdcall readMacro(unsigned int handle, int * macro, double * value, int count)
{
    macro_remote *p = static_cast<macro_remote*>((void*)handle);
    if(p)
    {
        return p->_readMacro(macro, value, count);
    }
    else
    {
        return false;
    }
}

/**
*  \brief 读宏变量
 *
 *  \param [in] socket链接的句柄
 *  \param [in] 写Macro的地址数组
 *  \param [out] 写Macro的值数组
 *  \param [in] 写Macro的数量
 *  \return true succuss
 *  		false failed
 *
 *  \details 很多macro是只读的，需要参考手册
*/
bool __stdcall writeMacro(unsigned int handle, int * macro, double * value, int count)
{
    macro_remote *p = static_cast<macro_remote*>((void*)handle);
    if(p)
    {
        return p->_writeMacro(macro, value, count);
    }
    else
    {
        return false;
    }
}
#define TIMEOUT 350

/*
*
*/
macro_remote::macro_remote()
{
    s_state = STATE_UNCONNECT;
    s_nOffset = 0;
}

/**
*  \brief 建立到目标机器的socket链接
 *
 *  \param [in] ip 目标机器的IP地址
 *  \return true succuss
 *  		false failed
 *
 *  \details 对一个IP地址重复connect可能会有问题
 *			创建一个类的实例，并调用该实例建立连接
*/
bool  macro_remote::_connectHost(char * ip)
{
    if (s_state != STATE_UNCONNECT)    return false;
#ifdef _WINDLL
    WORD sockVersion = MAKEWORD(2,2);
    WSADATA data;
    if(WSAStartup(sockVersion, &data) != 0)
    {
        return false;
    }
#endif
    memset(s_szIP, 0, sizeof(s_szIP));
    memcpy(s_szIP, ip, strlen(ip));

    s_client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(s_client == INVALID_SOCKET)
    {
        printf("1\n");
        return false;
    }

    sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(PORT);
#ifdef _WINDLL
    serAddr.sin_addr.S_un.S_addr = inet_addr(ip);
#else
    serAddr.sin_addr.s_addr = inet_addr(ip);
#endif
    if (connect(s_client, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
    {
        closesocket(s_client);
        printf("2 %s %d\n",ip, errno);
        return false;
    }

    //int optval = 50;
    //setsockopt(s_client, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, sizeof(int));//使用KEEPALIVE
    //setsockopt(s_client, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, sizeof(int));//禁用NAGLE算法
    int nNetTimeout = TIMEOUT;
    setsockopt(s_client, SOL_SOCKET, SO_SNDTIMEO, (char *)&nNetTimeout, sizeof(int));
    setsockopt(s_client, SOL_SOCKET, SO_RCVTIMEO, (char *)&nNetTimeout, sizeof(int));

    s_state = STATE_CONNECT;
    s_dataHead.szFlag[0] = SOH;
    s_dataHead.szFlag[1] = STX;
    s_dataHead.unIndex = 0;
    return true;
}

/**
*  \brief 断开连接
 *
 *  \return void
 *
 *  \details 断开连接
*/
void macro_remote::_closeConnect()
{
    if (s_state != STATE_UNCONNECT)
    {
        closesocket(s_client);
        s_state = STATE_UNCONNECT;
    }
}

/**
*  \brief 读宏变量
 *
 *  \param [in] 读Macro的地址数组
 *  \param [out] 读Macro的结果数组
 *  \param [in] 读Macro的数量
 *  \return true succuss
 *  		false failed
 *
 *  \details 当Macro地址是-1的时候读文件名（需要控制器端配合）
*/
bool macro_remote::_readMacro(int * macro, double * value, int count)
{
    /** 数据长度判断 **/
    if (count > MAX_NUMBER)    return false;
    /** 连接状态判断 **/
    if (s_state == STATE_UNCONNECT && !_connectHost(s_szIP))
    {
        return false;
    }
    if (s_state != STATE_CONNECT)   return false;
    s_state = STATE_BUSY;

    /** 数据赋值 **/
    char szData[DATA_SIZE];
    _dataRead dataRead;
    s_dataHead.unIndex += 1;
    s_dataHead.chFlag = READ;
    s_dataHead.nNumber = count;
    for (int i = 0; i < count; ++i)
    {
        dataRead.macro[i] = macro[i];
    }
    int nByteHead = sizeof(_dataHead);
    int nByteData = count * sizeof(int);
    memcpy(szData, &s_dataHead, nByteHead);
    memcpy(szData + nByteHead, &dataRead, nByteData);

    /** 发送数据 **/
    int nSendLen = send(s_client, szData, nByteHead + nByteData, 0);
    if (nSendLen == -1)
    {
        closesocket(s_client);
        s_state = STATE_UNCONNECT;
        return false;
    }
    else if (nSendLen != (nByteHead + nByteData))
    {
        s_state = STATE_CONNECT;
        return false;
    }

    /** 接收数据 **/
    bool ret = recieveData(szData, value);
    s_state = STATE_CONNECT;
    return ret;
}

/**
*  \brief 读宏变量
 *
 *  \param [in] 写Macro的地址数组
 *  \param [out] 写Macro的值数组
 *  \param [in] 写Macro的数量
 *  \return true succuss
 *  		false failed
 *
 *  \details 很多macro是只读的，需要参考手册
*/
bool macro_remote::_writeMacro(int * macro, double * value, int count)
{
    /** 数据长度判断 **/
    if (count > MAX_NUMBER)    return false;
    /** 连接状态判断 **/
    if (s_state == STATE_UNCONNECT && !_connectHost(s_szIP))
    {
        return false;
    }
    if (s_state != STATE_CONNECT)   return false;
    s_state = STATE_BUSY;

    /** 数据赋值 **/
    char szData[DATA_SIZE];
    _dataWrite dataWrite;
    s_dataHead.unIndex += 1;
    s_dataHead.chFlag = WRITE;
    s_dataHead.nNumber = count;
    for (int i = 0; i < count; ++i)
    {
        dataWrite.item[i].macro = macro[i];
        dataWrite.item[i].value = value[i];
    }
    int nByteHead = sizeof(_dataHead);
    int nByteData = count * sizeof(_dataWriteItem);
    memcpy(szData, &s_dataHead, nByteHead);
    memcpy(szData + nByteHead, &dataWrite, nByteData);

    /** 发送数据 **/
    int nSendLen = send(s_client, szData, nByteHead + nByteData, 0);
    if (nSendLen == -1)
    {
        closesocket(s_client);
        s_state = STATE_UNCONNECT;
        return false;
    }
    else if (nSendLen != (nByteHead + nByteData))
    {
        s_state = STATE_CONNECT;
        return false;
    }

    /** 接收数据 **/
    bool ret = recieveData(szData, NULL);
    s_state = STATE_CONNECT;
    return ret;
}

/**
*  \brief select
*  \return int
*
*  \details 内部成员函数
*/
int macro_remote::selectSocket()
{
    fd_set fd;
    FD_ZERO(&fd);
    FD_SET(s_client, &fd);
    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = TIMEOUT * 1000;
    int nResult = select(s_client+1, &fd, NULL, NULL, &timeout);
    return nResult;
}

/**
*  \brief 接收数据
*  \param [out] 接收数据的buffer
*  \return int 数据长度
*
*  \details 内部成员函数
*/
int macro_remote::recieveSocketData(char * szData)
{
    /** 接收数据 **/
    memcpy(szData, s_szData, s_nOffset);
    int nRecvSize = ::recv(s_client, szData + s_nOffset, DATA_SIZE - s_nOffset, 0);
    return nRecvSize + s_nOffset;
}

/**
*  \brief 接收数据
*  \param [out] 接收原始数据的buffer
*  \param [out] 接收数据的double型 buffer
*  \return bool 是否成功
*
*  \details 内部成员函数
*/
bool macro_remote::recieveData(char * szData, double * dValue)
{
    if (selectSocket() == -1)
    {
        return false;
    }
    /** 接收数据 **/
    int nRet = 0;
    int nRecvSize = DATA_SIZE;
    while (nRecvSize == DATA_SIZE)
    {
        nRecvSize = recieveSocketData(szData);
        if (nRecvSize > 0)
        {
            nRet = dataHandler(szData, nRecvSize, dValue);
            memcpy(szData, s_szData, s_nOffset);
        }
    }
    /** 数据不匹配，在收一遍 **/
    if (nRet == -2)
    {
        if (selectSocket() == -1)
        {
            return false;
        }
        nRecvSize = recieveSocketData(szData);
        if (nRecvSize > 0)
        {
            nRet = dataHandler(szData, s_nOffset + nRecvSize, dValue);
            memcpy(szData, s_szData, s_nOffset);
        }
    }

    return nRet == 1;
}

/********************************************************************
Functions     :	dataHandler
Description   :	数据处理函数
Programmer    :	liu.z
Make Date     :	2017.04.15
Parameters    : [IN] char *: 数据, int: 数据长度, double *: 读到的宏变量
Return Value  :
*********************************************************************/
int macro_remote::dataHandler(char * szData, int nLen, double * dValue)
{
    int nByteLeft = nLen;

    /** 数据长度小于头部长度，返回 **/
    const int nByteHead = sizeof(_dataHead);
    if (nByteLeft <= nByteHead)
    {
        s_nOffset = nLen;
        memcpy(s_szData, szData, nLen);
        return 0;
    }
    nByteLeft -= nByteHead;

    /** 头部数据非法，返回 **/
    _dataHead dataHeadRet;
    memcpy(&dataHeadRet, szData, nByteHead);
    if (!isValidHead(&dataHeadRet))
    {
        s_nOffset = nByteLeft;
        memcpy(s_szData, szData, nByteLeft);
        return -1;
    }

    /** 数据长度判断 **/
    const int nByteData = dataHeadRet.chFlag == READ ?
        (dataHeadRet.nNumber * sizeof (double)) : (dataHeadRet.nNumber * sizeof (int));
    if (nByteLeft < nByteData)
    {
        s_nOffset = nLen;
        memcpy(s_szData, szData, nLen);
        return 0;
    }
    nByteLeft -= nByteData;

    /** 数据匹配判断 **/
    if (!isEqualHead(&dataHeadRet))
    {
        s_nOffset = nByteLeft;
        memcpy(s_szData, szData, nByteLeft);
        return -2;
    }

    /** 数据处理 **/
    if (dataHeadRet.chFlag == READ)
    {
        _dataReadRet dataReadRet;
        memcpy(&dataReadRet, szData + nByteHead, nByteData);
        for (int i = 0; i < dataHeadRet.nNumber; ++i)
        {
            dValue[i] = dataReadRet.value[i];
        }
    }
    else
    {
        ;//_dataWriteRet dataWriteRet;
    }

    /** 余留数据 **/
    s_nOffset = nByteLeft;
    memcpy(s_szData, szData, nByteLeft);
    return 1;
}

bool macro_remote::isValidHead(_dataHead * pDataHead)
{
    return ( (pDataHead->szFlag[0] == SOH && pDataHead->szFlag[1] == STX) &&
             (pDataHead->chFlag == READ || pDataHead->chFlag == WRITE) &&
             (pDataHead->nNumber > 0 && pDataHead->nNumber <= MAX_NUMBER) );
}

bool macro_remote::isEqualHead(_dataHead * pDataHead)
{
    return ( (pDataHead->chFlag == s_dataHead.chFlag) &&
             (pDataHead->unIndex == s_dataHead.unIndex) &&
             (pDataHead->nNumber == s_dataHead.nNumber) );
}
