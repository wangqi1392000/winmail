#include <direct.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string>
//#include <winsock.h>
#include <WinSock2.h>
using namespace std;
#pragma comment(lib, "ws2_32.lib") /*链接ws2_32.lib动态链接库*/
#pragma warning(disable: 4996)

unsigned char* acl_base64_encode(const char* in);
unsigned char* acl_base64_decode(const char* in);

int writeFile(char* str)
{
    const int size = 1000;
    char path[size];    //获取当前工作目录函数
    system("cls");      //清屏函数(清空控制台)
    _getcwd(path, size);
    printf("path=%s\n", path);

    const char* FILENAME = "test.txt";
    char* pt = strcat(strcat(path, "/"), FILENAME);
    printf("path=%s\n", pt);

    FILE* pFile = fopen(FILENAME, "wb");
    if (pFile == NULL) {
        printf("pfile is null");
    } else {
        fwrite(str, 1, strlen(str), pFile);
        fflush(pFile);
        fclose(pFile);
    }
    return 0;
}

/*
Client: send name
Server: +OK
Client: send pass
Server: +OK 6 messages (33753 octets)
Client: send stat
Server: +OK 6 33753
Client: send list
Server: +OK 6 messages (33753 octets)
你想查看哪一封邮件？请输入序号
*/
//实现编码由utf-8到ANSI的转换
char* UTF8TOANSI(const char* srcCode)
{
    int srcCodeLen = 0;
    srcCodeLen = MultiByteToWideChar(CP_UTF8, NULL, srcCode, strlen(srcCode), NULL, 0);
    wchar_t* result_t = new wchar_t[srcCodeLen + 1];
    MultiByteToWideChar(CP_UTF8, NULL, srcCode, strlen(srcCode), result_t, srcCodeLen);

    //utf-8转换为Unicode
    result_t[srcCodeLen] = '\0';
    srcCodeLen = WideCharToMultiByte(CP_ACP, NULL, result_t, wcslen(result_t), NULL, 0, NULL, NULL);
    char* result = new char[srcCodeLen + 1];
    WideCharToMultiByte(CP_ACP, NULL, result_t, wcslen(result_t), result, srcCodeLen, NULL, NULL);

    //Unicode转换为ANSI
    result[srcCodeLen] = '\0';
    delete result_t;
    return result;
}

int getMailCount(const char* str)
{
    // str=+OK 6 messages (33753 octets)
    int length = strlen(str);
    if (str[0] == '+'
    && str[1] == 'O'
    && str[2] == 'K'
    && str[3] == ' ' )
    {
        char temp[100] = "";
        int temp_i = 0;
        for (int i = 4; i < length && str[i] != ' ' && str[i] != '\0'; i++)
        {
            temp[temp_i] = str[i];
            temp_i++;
        }
        temp[temp_i] = '\0';

        int temp_length = strlen(temp);
        if (temp_length > 0)
        {
            int num = 0;
            int num_x = 1;//倍数
            for (int i = temp_length - 1; i >= 0; i--)
            {
                num += (temp[i] - 48) * num_x;
                num_x = num_x * 10;
            }
            return num;
        }
    }
    return 0;
}

/** 接收邮件 */
int pullmail(int argc, char* argv[])
{
/*
    cout << "argc=" << argc << "\n";
    for (int i = 0; i < argc; i++) {
        cout << "argv[" << i << "]=" << argv[i] << "\n";
    }
*/
    const int minParamCount = 4;
    // winmail.exe是第一个参数
    if (argc < minParamCount + 1) {
        cout << "程序调用格式：winmail.exe 邮箱地址 邮箱密码 pop3服务器地址 smtp服务器地址" << endl;
        cout << "程序调用示例：winmail.exe username@sina.com 123456 pop3.sina.com smtp.sina.com" << endl;
        return 0;
    }
    const string USERNAME = argv[1];
    const string PASSWORD = argv[2];
    const char* POP3ADDR = argv[3];
    const char* SMTPADDR = argv[4];
    int pop3Port = 110;
    int smtpPort = 25;
    const string NEWLINE = "\r\n";
    const int BUFFSIZE = 1024 * 500; //500k
    int SLEEPSECOND = 300;

//    strcpy(emailAddr, argv[1]);

    char buff[BUFFSIZE]; //收到recv函数返回的结果
    string message;
    string subject;
    string info;
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 1);
    //WSAStarup，即WSA(Windows SocKNDs Asynchronous，Windows套接字异步)的启动命令
    int err = WSAStartup(wVersionRequested, &wsaData);
    SOCKADDR_IN addrServer; //服务端地址
    HOSTENT* pHostent;//hostent是host entry的缩写，该结构记录主机的信息，包括主机名、别名、地址类型、地址长度和地址列表
    SOCKET sockClient; //客户端的套接字
    /*
    使用 MAIL 命令指定发送者
    使用 RCPT 命令指定接收者，可以重复使用RCPT指定多个接收者
    */
    int call = 1;// 1.查看邮箱 2.发送邮件
    if (call == 1) {
        sockClient = socket(AF_INET, SOCK_STREAM, 0); //建立socket对象
//        const char* host_id = POP3ADDR;
        pHostent = gethostbyname(POP3ADDR);
        addrServer.sin_addr.S_un.S_addr = *((DWORD*)pHostent->h_addr_list[0]); //得到smtp服务器的网络字节序的ip地址
        addrServer.sin_family = AF_INET;
        addrServer.sin_port = htons(pop3Port); //连接端口110
        err = connect(sockClient, (SOCKADDR*)&addrServer, sizeof(SOCKADDR)); //向服务器发送请求
        buff[recv(sockClient, buff, BUFFSIZE, 0)] = '\0';

        message = "user " + USERNAME + NEWLINE;
        send(sockClient, message.c_str(), message.length(), 0); //发送账号
        buff[recv(sockClient, buff, BUFFSIZE, 0)] = '\0';   //接收返回值
        Sleep(1);
        std::cout << "debug Client: send name \n";
        std::cout << "debug Server: " << buff << std::endl;

        message = "pass " + PASSWORD + NEWLINE;
        send(sockClient, message.c_str(), (int)message.length(), 0); //发送Mima
        buff[recv(sockClient, buff, BUFFSIZE, 0)] = '\0';   //接收返回值
        Sleep(1);
        std::cout << "debug Client: send pass \n";
        std::cout << "debug Server: " << buff << std::endl;

        message = "stat" + NEWLINE;
        send(sockClient, message.c_str(), (int)message.length(), 0); //发送状态
        buff[recv(sockClient, buff, BUFFSIZE, 0)] = '\0';   //接收返回值
        Sleep(1);
        std::cout << "debug Client: send stat \n";
        std::cout << "debug Server: " << buff << std::endl;
        message = "list" + NEWLINE;
        send(sockClient, message.c_str(), (int)message.length(), 0); //发送状态
        buff[recv(sockClient, buff, BUFFSIZE, 0)] = '\0';   //接收返回值
        Sleep(1);
        std::cout << "debug Client: send list \n";
        std::cout << "debug Server: " << buff << std::endl;

        int num = getMailCount(buff);
        cout << "debug mail count: " << num << endl;

        message = "retr " + to_string(num) + NEWLINE;
        send(sockClient, message.c_str(), (int)message.length(), 0); //发送状态
        Sleep(SLEEPSECOND);
        //std::cout << "debug Client: send retr (...) \n";
        buff[recv(sockClient, buff, BUFFSIZE, 0)] = '\0';   //接收返回值
        char* content = UTF8TOANSI(buff);
//        std::cout << "debug Server：" << content << std::endl;


        writeFile(content);
        delete content;
    }

/*
        if (call == 2) {
            sockClient = socket(AF_INET, SOCK_STREAM, 0); //建立socket对象
            pHostent = gethostbyname(SMTPADDR); //得到有关于域名的信息,链接到qq邮箱服务器
            addrServer.sin_addr.S_un.S_addr = *((DWORD*)pHostent->h_addr_list[0]); //得到smtp服务器的网络字节序的ip地址
            addrServer.sin_family = AF_INET;
            addrServer.sin_port = htons(smtpPort); //连接端口25
            //int connect (SOCKET s , const struct sockaddr FAR *name , int namelen ); //函数原型
            err = connect(sockClient, (SOCKADDR*)&addrServer, sizeof(SOCKADDR)); //向服务器发送请求
            buff[recv(sockClient, buff, BUFFSIZE, 0)] = '\0';
            // 登录邮件服务器
            message = "ehlo sina.com" + NEWLINE;
            send(sockClient, message.c_str(), message.length(), 0); //发送ehlo命令
            buff[recv(sockClient, buff, BUFFSIZE, 0)] = '\0';   //接收返回值
            cout <<"1" <<  buff << endl;

            message = "auth login" + NEWLINE;
            send(sockClient, message.c_str(), message.length(), 0);
            buff[recv(sockClient, buff, BUFFSIZE, 0)] = '\0';
            cout << "2" << buff << endl;

            // 发送base64加密的用户名、密码
            // message = "X==" + NEWLINE;  // X换成自己邮箱的公钥
            char* en_username = (char*)acl_base64_encode(USERNAME.c_str());
            message = en_username + NEWLINE;  // X换成自己邮箱的公钥
            send(sockClient, message.c_str(), message.length(), 0);
            buff[recv(sockClient, buff, BUFFSIZE, 0)] = '\0';
            cout << "3" << buff << endl;
            free(en_username);

            // message = "Y==" + NEWLINE;///Y换成自己邮箱的密钥
            char* en_password = (char*)acl_base64_encode(PASSWORD.c_str());
            message = en_password + NEWLINE;     // Y换成自己邮箱的密钥
            send(sockClient, message.c_str(), message.length(), 0);
            buff[recv(sockClient, buff, BUFFSIZE, 0)] = '\0';
            cout << "4" << buff << endl;
            free(en_password);

            string mail;
            cout << "请输入收件人邮箱：";
            cin >> mail;

            message = "MAIL FROM:<" + USERNAME + "> " + NEWLINE;
            message.append("RCPT TO:<" + mail + "> " + NEWLINE);
            send(sockClient, message.c_str(), message.length(), 0);
            buff[recv(sockClient, buff, BUFFSIZE, 0)] = '\0';
            cout <<"5" <<  buff << endl;
             // buff[recv(sockClient, buff, BUFFSIZE, 0)] = '\0';

            // 使用 DATA 命令告诉服务器要发送邮件内容
            message = "DATA" + NEWLINE;
            send(sockClient, message.c_str(), message.length(), 0);
            buff[recv(sockClient, buff, BUFFSIZE, 0)] = '\0';
            cout <<"6" <<  buff << endl;

            cout << "主题：";
            cin >> subject;
            cout << "内容：";
            cin >> info;
            message = "From: " + USERNAME + NEWLINE;
            message.append("To: " + mail + NEWLINE);
            message.append("subject:" + subject + NEWLINE + NEWLINE);
            message.append(info + NEWLINE + "." + NEWLINE);
            send(sockClient, message.c_str(), message.length(), 0);
            cout <<"7" <<  buff << endl;

            message = "QUIT" + NEWLINE;
            send(sockClient, message.c_str(), message.length(), 0);
            buff[recv(sockClient, buff, BUFFSIZE, 0)] = '\0';
            cout <<"8" <<  buff << endl;

            cout << "发送成功！" << endl;
            system("pause");
        }
*/
    return 0;
}

static const unsigned char to_b64[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const unsigned char un_b64[] = {
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62,  255, 255, 255, 63,
        52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  255, 255, 255, 255, 255, 255,
        255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,
        15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  255, 255, 255, 255, 255,
        255, 26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
        41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

#define UNSIG_CHAR_PTR(x) ((const unsigned char *)(x))
unsigned char* acl_base64_encode(const char* in) {
    const unsigned char* cp;
    int len = strlen(in);
    int count, size = len * 4 / 3;

    unsigned char* out = (unsigned char*)malloc(size + 1);
    int out_index = 0;
    for (cp = UNSIG_CHAR_PTR(in), count = len; count > 0; count -= 3, cp += 3) {
        out[out_index++] = to_b64[cp[0] >> 2];
        if (count > 1) {
            out[out_index++] = to_b64[(cp[0] & 0x3) << 4 | cp[1] >> 4];
            if (count > 2) {
                out[out_index++] = to_b64[(cp[1] & 0xf) << 2 | cp[2] >> 6];
                out[out_index++] = to_b64[cp[2] & 0x3f];
            }
            else {
                out[out_index++] = to_b64[(cp[1] & 0xf) << 2];
                out[out_index++] = '=';
                break;
            }
        }
        else {
            out[out_index++] = to_b64[(cp[0] & 0x3) << 4];
            out[out_index++] = '=';
            out[out_index++] = '=';
            break;
        }
    }
    out[out_index] = 0;
    return out;
}

unsigned char* acl_base64_decode(const char* in) {
    const unsigned char* cp;
    int     count;
    int     ch0;
    int     ch1;
    int     ch2;
    int     ch3;
    int     len = strlen(in);

    if (len % 4) {
        return (NULL);
    }
    #define INVALID		0xff
    unsigned char* out = (unsigned char*)malloc(len + 1);
    int out_index = 0;
    for (cp = UNSIG_CHAR_PTR(in), count = 0; count < len; count += 4) {
        if ((ch0 = un_b64[*cp++]) == INVALID
            || (ch1 = un_b64[*cp++]) == INVALID)
            return (0);
        out[out_index++] = ch0 << 2 | ch1 >> 4;
        if ((ch2 = *cp++) == '=')
            break;
        if ((ch2 = un_b64[ch2]) == INVALID)
            return (0);
        out[out_index++] = ch1 << 4 | ch2 >> 2;
        if ((ch3 = *cp++) == '=')
            break;
        if ((ch3 = un_b64[ch3]) == INVALID)
            return (0);
        out[out_index++] = ch2 << 6 | ch3;
    }

    out[out_index] = 0;
    return out;
}


/** 主程序 */
int main(const int argc, char* argv[])
{
    pullmail(argc, argv);

    return 0;
}