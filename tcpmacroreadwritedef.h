/********************************************************************
File Name   :	TcpMacroReadWriteDeg.h
Description :	宏变量批量读取数据定义文件
Programmer  :	liu.z
Make Date   :	2017.04.15
History     :
*********************************************************************/
#ifndef TCPMACROREADWRITEDEF_H
#define TCPMACROREADWRITEDEF_H

#define PORT 5002

#define SOH 0x01
#define STX 0x02
#define RDWR -1
#define READ 0
#define WRITE 1
#define MAX_NUMBER 20
#define DATA_SIZE 256

#define CLIENT_MAX 16

#pragma pack (1)

/** 数据头 **/
struct _dataHead
{
    char szFlag[2];
    unsigned int unIndex;
    char chFlag;
    int nNumber;
};

/** 写宏变量项目结构 **/
struct _dataWriteItem
{
    int macro;
    double value;
};

/** 写宏变结构 **/
struct _dataWrite
{
    _dataWriteItem item[MAX_NUMBER];
};

/** 读宏变量结构 **/
struct _dataRead
{
    int macro[MAX_NUMBER];
};

/** 读宏变量返回数据结构 **/
struct _dataReadRet
{
    double value[MAX_NUMBER];
};

/** 写宏变量返回数据结构 **/
typedef struct _dataRead _dataWriteRet;

#pragma pack ()


#endif    // TCPMACROREADWRITEDEF_H
