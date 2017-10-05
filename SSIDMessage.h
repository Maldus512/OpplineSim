/*
 * SSIDMessage.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Maldus
 */

#ifndef SSIDMESSAGE_H_
#define SSIDMESSAGE_H_

#include <string.h>
#include <omnetpp.h>
#include "inet/linklayer/common/MACAddress.h"

using namespace std;

#define REQ     48
#define ACK     49

class OpplineMsg {
    public:
        OpplineMsg(string m);
        OpplineMsg(omnetpp::simtime_t time, char type,inet::MACAddress src, inet::MACAddress dst);
        string getSSID();
        inet::MACAddress srcAdd;
        inet::MACAddress dstAdd;
        void test(inet::MACAddress add);
        string response(string orig);
        string original(string res);
        bool isAck();
    private:
        string msg;
        uint64_t timeStamp;
        char msgType;

        void decToBase94(uint64_t dec, int size, char *res);
        void hexToBase94(string hex, int size, char *res);
        uint64_t base94ToDec(char s[], int len);
};



#endif /* SSIDMESSAGE_H_ */
