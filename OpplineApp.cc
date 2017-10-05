/*
 * OpplineApp.cc
 *
 *  Created on: Mar 26, 2017
 *      Author: maldus
 */

#include <string.h>
#include <omnetpp.h>
#include <queue>
#include "inet/linklayer/ieee80211/mgmt/Ieee80211MgmtSTA.h"
#include "inet/linklayer/ieee80211/mgmt/Ieee80211MgmtAP.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Mac.h"
#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/linklayer/common/MACAddress.h"
#include "CircularQueue.h"
#include "SSIDMessage.h"
using namespace omnetpp;

#define OBSTATE "Opportunistic Beacon"
#define BOSTATE "Beacon Observer"
#define TRANSOB "transitioning to OB"
#define TRANSBO "transitioning to BO"


const uint64_t minMac = 11725260718081;
//const uint64_t maxMac = 11725260718081;

class OpplineApp : public cSimpleModule {
  public:
    OpplineApp();
    cModule *parentHost;
    cModule *ap;
    cModule *wlan;
    inet::MACAddress address;
  protected:
    // The following redefined virtual function holds the algorithm.
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

    CircularQueue *messageQ;
    std::string state;
    std::string curssid;
    string curdst;

  private:
    simtime_t interval;
    simtime_t tmi, txbo, txob;
    int numDevices;
    int initial_msg;
    virtual void OBtoBO();
    virtual void BOtoOB();
    virtual void delayOBtoBO();
    virtual void delayBOtoOB();
    virtual void newMessage();
    virtual void newMessage(int dst);
    virtual void scan();
    inet::MACAddress randomMAC();
};
// The module class needs to be registered with OMNeT++
Define_Module(OpplineApp);

OpplineApp::OpplineApp() {
    messageQ = new CircularQueue(5);
}


void OpplineApp::initialize() {
    EV << "initialized\n";
    inet::ieee80211::Ieee80211Mac *add;
    //get time for opportunistic beacon and beacon observer states
    interval = par("tob");
    //get time interval for generating new messages
    tmi = par("tmi");
    txbo = par("txbo");
    txob = par("txob");
    //get number of devices (to generate MAC destination addresses)
    numDevices = getParentModule()->getParentModule()->par("hostNum");
    cMessage *genOpplineMessage = new cMessage("MSG");

    parentHost = getParentModule();
    ap = parentHost->getSubmodule("wlan", 0);
    wlan = parentHost->getSubmodule("wlan", 1);
    //Get my MAC address
    add = (inet::ieee80211::Ieee80211Mac *)ap->getSubmodule("mac");
    address =  add->getAddress();
    EV << address << '\t';
    EV << address.getInt() <<  '\n';
    scheduleAt(tmi, genOpplineMessage);
    WATCH(state);
    WATCH(curssid);
    WATCH(curdst);
    initial_msg = par("initial_msg");
    //Begin as AP or STA
    if (par("initial_state")) {
        if (initial_msg > 0) {
            newMessage(initial_msg); //remove TODO
        } else {
            newMessage();
        }
        BOtoOB();
    } else {
        OBtoBO();
    }
}



void OpplineApp::handleMessage(cMessage *msg) {
    if (msg->isName("STA")) {
        OBtoBO();
    } else if (msg->isName("AP")){
        BOtoOB();
    } else if (msg->isName("MSG")) {
        newMessage();
    } else if (msg->isName("SCAN")) {
        scan();
    } else if (msg->isName("OB")) {
        delayBOtoOB();
    } else if (msg->isName("BO")) {
        delayOBtoBO();
    }

    delete(msg);
}

void OpplineApp::delayOBtoBO() {
    //intermediate message to wait txbo seconds
    cMessage *event = new cMessage("STA");
    state = TRANSBO;
    EV << "waiting " << txbo << "to transition to Beacon Observer\n";
    scheduleAt(simTime() + txbo, event);
}

void OpplineApp::delayBOtoOB() {
    //intermediate message to wait txob seconds
    cMessage *event = new cMessage("AP");
    if (messageQ->size() <= 0) {
        event = new cMessage("OB");
        scheduleAt(simTime() + interval, event);
        return;
    }
    state = TRANSOB;
    EV << "waiting " << txob << "to transition to Opportunistic Beacon\n";
    scheduleAt(simTime() + txob, event);
}

void OpplineApp::OBtoBO() {
    cMessage *event, *scanEv;
    //In BO state scan the wifi list every tsi seconds (scan interval)
    simtime_t tsi = normal(3.0, 0.247);
    state = BOSTATE;
    inet::ieee80211::Ieee80211MgmtAP *mgmtAP =
            (inet::ieee80211::Ieee80211MgmtAP *) ap->getSubmodule("mgmt");
    EV << "Switching from Opportunistic Beacon to Beacon Observer\n";
    mgmtAP->setSSID("disabled");
    curssid = "disabled";
    curdst = "";
    event = new cMessage("OB");
    scanEv = new cMessage("SCAN");
    scheduleAt(simTime() + interval, event);
    scheduleAt(simTime() + tsi, scanEv);
}

void OpplineApp::BOtoOB() {
    cMessage *event;
    //if I have some messages try to switch to Opportunistic Beacon
    //To transmit them
    if (messageQ->size() > 0) {
        state = OBSTATE;
        inet::ieee80211::Ieee80211MgmtAP *mgmtAP =
                (inet::ieee80211::Ieee80211MgmtAP *) ap->getSubmodule("mgmt");

        EV << "Switching from Beacon Observer to Opportunistic Beacon\n";
        std::string ssid = messageQ->deQueue();
        messageQ->enQueue(ssid);
        mgmtAP->setSSID(ssid);
        curssid = ssid;
        curdst = OpplineMsg(ssid).dstAdd.str();
        event = new cMessage("BO");
        scheduleAt(simTime() + interval, event);
    //If I have no messages in the queue try again later
    } else {
        //TODO move to Opportunistic Beacon when eventually a message is found
        event = new cMessage("AP");
        scheduleAt(simTime() + interval, event);
    }

}


inet::MACAddress OpplineApp::randomMAC() {
    uint64_t mac = 0;
    int tmp;
    do {
        tmp = (int)intuniform(0, numDevices-1);
        mac = minMac + tmp*2;
    } while (mac == address.getInt());
    return inet::MACAddress(mac);
}


void OpplineApp::newMessage() {
    OpplineMsg *msg = new OpplineMsg(simTime(), REQ, address, randomMAC());
    messageQ->enQueue(msg->getSSID());
}

void OpplineApp::newMessage(int dst) {
    OpplineMsg *msg = new OpplineMsg(simTime(), REQ, address, inet::MACAddress(minMac + dst));
    messageQ->enQueue(msg->getSSID());
}

void OpplineApp::scan() {
    if (state.compare(BOSTATE) != 0) {
        EV << "tried to scan while not active as BO";
        return;
    }
    OpplineMsg *m;
    cMessage *event = new cMessage("SCAN");
    simtime_t tsi = normal(3.0, 0.247);
    inet::ieee80211::Ieee80211MgmtSTA *mgmtSTA =
                (inet::ieee80211::Ieee80211MgmtSTA *) wlan->getSubmodule("mgmt");
    inet::ieee80211::Ieee80211MgmtSTA::AccessPointList::const_iterator iterator, end;
    EV << parentHost->getFullName() << " ssids: " << std::to_string(mgmtSTA->apList.size()) << "\n";
    for (iterator = mgmtSTA->apList.begin(), end = mgmtSTA->apList.end(); iterator != end; ++iterator) {
        if (address.compareTo(iterator->address) == 0 ) {
            continue;
        }
        if (iterator->ssid.compare("disabled") == 0) {
            continue;
        }
        if (iterator->ssid[0] != '[' || iterator->ssid[1] != 'c') {
            continue;
        }
        m = new OpplineMsg(iterator->ssid);
        if (!m->isAck() && m->dstAdd.compareTo(address) == 0) {
            getParentModule()->bubble("received message");
            EV << "Message received\n";
            messageQ->enQueue(m->response(iterator->ssid));
        } else if (m->isAck() && m->srcAdd.compareTo(address) == 0) {
            EV << "************* RECEIVED ACK *************\n";
            messageQ->remove(m->original(iterator->ssid));
        } else if (m->isAck() && messageQ->isQueued(m->original(iterator->ssid))) {
            EV << "PASSING ACK\n";
            messageQ->remove(m->original(iterator->ssid));
            messageQ->enQueue(iterator->ssid);
        } else if (!messageQ->isQueued(iterator->ssid)) {
            messageQ->enQueue(iterator->ssid);
        }
    }
    messageQ->print();
    scheduleAt(simTime() + tsi, event);
}
