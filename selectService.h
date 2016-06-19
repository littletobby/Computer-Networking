#ifndef SELECT_SERVICE_H_  
#define SELECT_SERVICE_H_  
#include <list>  
using namespace std;  
  
class SelectService{  
public:  
    explicit SelectService(unsigned short port)  
    : port_(port), listenSocket(0){}  
    bool beginRun();  
private:  
    void runImpl();  
    void acceptOpe();  
  
private:  
    list<int> socketList;  
    unsigned short port_;  
    int listenSocket;  
};  
  
#endif  
