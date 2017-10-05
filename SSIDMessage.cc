/*
 * SSIDMessage.cc
 *
 *  Created on: Sep 29, 2017
 *      Author: Maldus
 */


#include <string.h>
#include <omnetpp.h>
#include "SSIDMessage.h"

using namespace std;

OpplineMsg::OpplineMsg(string m) {
    char time[5];
    char add[8];
    int i;
    uint64_t tmp;
    if (m[0] != '[' || m[1] != 'c') {
        throw std::invalid_argument( "not a valid message" );
    }
    msgType = m[7];
    for (i = 0; i < 5; i++) {
        time[i] = m[2+i];
    }
    timeStamp = base94ToDec(time, 5);

    for (i = 0; i < 8; i++) {
        add[i] = m[16+i];
    }
    tmp = base94ToDec(add, 8);
    srcAdd = inet::MACAddress(tmp);
    for (i = 0; i < 8; i++) {
        add[i] = m[24+i];
    }
    tmp = base94ToDec(add, 8);
    dstAdd = inet::MACAddress(tmp);
}

void OpplineMsg::test(inet::MACAddress add) {
    uint64_t x, y;
    char arr[8];
    y = add.getInt();
    decToBase94(y, 8, arr);
    x = base94ToDec(arr, 8);
}

uint64_t OpplineMsg::base94ToDec(char s[], int len) {
    char tmp;
    uint64_t mult, middle;
    int64_t i;
    uint64_t res = 0;
    mult = 1;
    for (i = len-1; i >=0 ; i--) {
        tmp = s[i]-32;
        middle = tmp*mult;
        res += middle;
        mult *= 94;
    }
    return res;
}

OpplineMsg::OpplineMsg(omnetpp::simtime_t t, char type,inet::MACAddress src, inet::MACAddress dst) {
    char res[32],mac1[8], mac2[8], timestamp[5];
    int i, j;
    uint64_t tmp;
    for (j = 0; j < 32; j++) {
        res[j] = (char)48;
    }
    res[0] = '[';
    res[1] = 'c';
    int time =t.inUnit(SIMTIME_MS);
    timeStamp = time;
    tmp = src.getInt();
    decToBase94(tmp, 8, mac1);
    tmp = dst.getInt();
    decToBase94(tmp, 8, mac2);
    decToBase94(time,5, timestamp);
    for (i = 0; i < 5; i++) {
        res[i+2] = timestamp[i];
    }
    res[7] = type;
    msgType = type;
    for (i = 0; i < 8; i++) {
        res[i+8] = 'x';
    }
    for (i = 0; i < 8; i++) {
        res[16+i] = mac1[i];
    }
    for (i = 0; i < 8; i++) {
        res[16+8+i] = mac2[i];
    }
    msg = string(res);
}

void OpplineMsg::decToBase94(uint64_t dec, int size, char res[size]) {
    char tmp;
    int rem, i = size-1, j;
    for (j = 0; j < size; j++) {
        res[j] = (char)32;
    }
    while (dec >= 94 && i >= 0) {
        rem = dec%94;
        tmp = (char)(rem+32);
        res[i--] = tmp;
        dec = dec/94;
    }
    if (i < 0) {
        return;
    }
    //Need this because apparently with only one instruction it doesn't happen
    // I'm too done with c++ to actually find out what's the problem
    res[i] = (char)(dec+32);
    res[i] = (char)(dec+32);
    res[i] = (char)(dec+32);
    res[i] = (char)(dec+32);
    res[i] = (char)(dec+32);
}

void OpplineMsg::hexToBase94(string hex, int size, char *res) {
    int dec = stoul(hex, 0, 16);
    decToBase94(dec, size, res);
}

string OpplineMsg::getSSID() {
    return msg;
}


string OpplineMsg::response(string orig) {
    orig[7] = (char) ACK;
    return orig;
}

string OpplineMsg::original(string res) {
    res[7] = (char) REQ;
    return res;
}

bool OpplineMsg::isAck() {
    return msgType == ACK;
}
