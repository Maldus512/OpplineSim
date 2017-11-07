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
#include "inet/linklayer/ieee80211/mgmt/Ieee80211AgentSTA.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Mac.h"
#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/linklayer/common/MACAddress.h"
#include "inet/common/NotifierConsts.h"
#include "CircularQueue.h"
#include "SSIDMessage.h"
using namespace omnetpp;

#define OBSTATE "Opportunistic Beacon"
#define BOSTATE "Beacon Observer"
#define TRANSOB "transitioning to OB"
#define TRANSBO "transitioning to BO"


const uint64_t minMac = 11725260718081;
//const uint64_t maxMac = 11725260718081;

class myListener;



class OpplineApp : public cSimpleModule {
  public:
    OpplineApp();
    cModule *parentHost;
    cModule *ap;
    cModule *wlan;
    inet::MACAddress address;
    long numSent, numSentAck;
    long numRecvd, numRecvdAck;
    long numAckd;
    long numGen;
    myListener *listener;
    virtual void scan();

  protected:
    // The following redefined virtual function holds the algorithm.
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

    CircularQueue *messageQ;
    std::string state;
    std::string curssid;
    string curdst;

  private:
    bool f_wait4msg;
    void test();
    simtime_t interval;
    simtime_t tmi, txbo, txob;
    int numDevices;
    bool initial_msg;
    virtual void OBtoBO();
    virtual void BOtoOB();
    virtual void delayOBtoBO();
    virtual void delayBOtoOB();
    virtual void newMessage();
    virtual void newMessage(int dst);

    inet::MACAddress randomMAC();
    virtual void finish() override;
};
// The module class needs to be registered with OMNeT++
Define_Module(OpplineApp);


class myListener : public cListener {
    public:
        OpplineApp *appReference;
        myListener(OpplineApp *app);
        virtual void receiveSignal(cComponent *src, simsignal_t id,
                                      long value, cObject *details) override;
};

myListener::myListener(OpplineApp * app) {
    appReference = app;
}

void myListener::receiveSignal(cComponent *src, simsignal_t id, long value, cObject *details) {

    EV << " I am " << appReference->getFullPath() << " ; Received scan signal from " << src->getFullPath() << endl;
    appReference->scan();
}

OpplineApp::OpplineApp() {
    messageQ = new CircularQueue(10);
    numSent = 0;
    numSentAck = 0;
    numRecvd = 0;
    numRecvdAck = 0;
    numAckd = 0;
    numGen = 0;
}




void OpplineApp::initialize() {
    EV << "initialized\n";
    listener = new myListener(this);
    //subscribe(inet::ieee80211::Ieee80211AgentSTA::opplineSignal, this);
    getParentModule()->subscribe(inet::ieee80211::Ieee80211AgentSTA::opplineSignal, listener);
    //getParentModule()->getParentModule()->subscribe(inet::ieee80211::Ieee80211AgentSTA::opplineSignal, listener);

    //flag to indicate if I am waiting for a message to transition to Opportunistic Beacon
    f_wait4msg = false;
    //My MAC address
    inet::ieee80211::Ieee80211Mac *add;
    //get time for opportunistic beacon and beacon observer states
    interval = par("tob");
    //get time interval for generating new messages
    tmi = par("tmi");
    //duration of BO and OB states (default is same)
    txbo = par("txbo");
    txob = par("txob");
    //get number of devices (to generate MAC destination addresses)
    numDevices = getParentModule()->getParentModule()->par("hostNum");
    //start generating new messages
    cMessage *genOpplineMessage = new cMessage("MSG");
    state = "";
    parentHost = getParentModule();
    ap = parentHost->getSubmodule("wlan", 0);
    wlan = parentHost->getSubmodule("wlan", 1);
    //Get my MAC address
    add = (inet::ieee80211::Ieee80211Mac *)ap->getSubmodule("mac");
    address =  add->getAddress();
    EV << address << '\t';
    EV << address.getInt() <<  '\n';
    scheduleAt(simTime()+tmi, genOpplineMessage);
    WATCH(state);
    WATCH(curssid);
    WATCH(curdst);
    //Generate new message at start (for debug purposes)
    initial_msg = par("initial_msg");

    if (initial_msg) {
        newMessage(initial_msg); //debug
    }
    //Begin as AP or STA
    if (par("initial_state")) {

        BOtoOB();
    } else {
        OBtoBO();
    }
}


//Manage self messages
void OpplineApp::handleMessage(cMessage *msg) {
    if (msg->isName("STA")) {
        OBtoBO();
    } else if (msg->isName("AP")){
        BOtoOB();
    } else if (msg->isName("MSG")) {
        newMessage();
    } else if (msg->isName("SCAN")) {
        //scan();
    } else if (msg->isName("OB")) {
        delayBOtoOB();
    } else if (msg->isName("BO")) {
        delayOBtoBO();
    }
    delete(msg);
}

//Wait for random interval before transitioning to BO
void OpplineApp::delayOBtoBO() {
    //intermediate message to wait txbo seconds
    if (messageQ->size() > 0) {
        cMessage *event = new cMessage("STA");
        state = TRANSBO;
        EV << "waiting " << txbo << "to transition to Beacon Observer\n";

        scheduleAt(simTime() + txbo, event);
        txbo = normal(3.407, 0.327);
    } else {
        f_wait4msg = true;
    }
}

//Wait for random interval before transitioning to OB
void OpplineApp::delayBOtoOB() {
    //intermediate message to wait txob seconds
    if (messageQ->size() > 0) {
        cMessage *event = new cMessage("AP");
        state = TRANSOB;
        EV << "waiting " << txob << "to transition to Opportunistic Beacon\n";
        scheduleAt(simTime() + txob, event);
        txob = normal(4.302, 0.524);
    } else {
        f_wait4msg = true;
    }
}

//Transition to BO
void OpplineApp::OBtoBO() {
    cMessage *event;
    state = BOSTATE;
    inet::ieee80211::Ieee80211MgmtAP *mgmtAP =
            (inet::ieee80211::Ieee80211MgmtAP *) ap->getSubmodule("mgmt");
    EV << "Switching from Opportunistic Beacon to Beacon Observer\n";
    mgmtAP->setSSID("disabled");
    curssid = "disabled";
    curdst = "";
    event = new cMessage("OB");
    //scanEv = new cMessage("SCAN");
    scheduleAt(simTime() + interval, event);
    //scheduleAt(simTime() + tsi, scanEv);
}

//Transition to OB
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
        f_wait4msg = true;
        OBtoBO();
    }

}

//Generate a random MAC address among possible ones
inet::MACAddress OpplineApp::randomMAC() {
    uint64_t mac = 0;
    int tmp;
    do {
        tmp = (int)intuniform(0, numDevices-1);
        mac = minMac + tmp*2;
    } while (mac == address.getInt());
    return inet::MACAddress(mac);
}

//Generate new message (random destination)
void OpplineApp::newMessage() {
    inet::MACAddress mac = randomMAC();
    OpplineMsg *msg = new OpplineMsg(simTime(), REQ, address, mac);
    //One more request message is sent
    numSent++;
    //One more message is generated
    numGen++;
    messageQ->enQueue(msg->getSSID());
    EV << "Sending message from " << address << " to " << mac <<endl;
    EV << "Message: " << msg->getSSID() << endl;
    cMessage *genOpplineMessage = new cMessage("MSG");
    scheduleAt(simTime() + tmi, genOpplineMessage);

    //If I was waiting to have a message in my queue now I can transition to OB state and relay it
    if (messageQ->size() > 0 && f_wait4msg) {
        f_wait4msg = false;
        delayBOtoOB();
    }
}

//Generate message with specific destination (for debug purposes)
void OpplineApp::newMessage(int dst) {
    OpplineMsg *msg = new OpplineMsg(simTime(), REQ, address, inet::MACAddress(minMac + dst));
    numSent++;
    numGen++;
    messageQ->enQueue(msg->getSSID());

}

//Scan
void OpplineApp::scan() {
    Enter_Method_Silent();
    if (state.compare(BOSTATE) != 0) {
        EV << "tried to scan while not active as BO"<<endl;
        return;
    }

    inet::ieee80211::Ieee80211AgentSTA *agentSTA =
                    (inet::ieee80211::Ieee80211AgentSTA *) wlan->getSubmodule("agent");
    agentSTA->setProbeDelay(normal(3.0, 0.247));

    OpplineMsg *m;
    inet::ieee80211::Ieee80211MgmtSTA *mgmtSTA =
                (inet::ieee80211::Ieee80211MgmtSTA *) wlan->getSubmodule("mgmt");
    inet::ieee80211::Ieee80211MgmtSTA::AccessPointList::const_iterator iterator, end;
    EV << parentHost->getFullName() << " ssids: " << std::to_string(mgmtSTA->apList.size()) << "\n";
    //Iterate throught the AP list
    for (iterator = mgmtSTA->apList.begin(), end = mgmtSTA->apList.end(); iterator != end; ++iterator) {
        //continue on invalid address
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
        //If it's a request message sent to me receive it and send the ack
        if (!m->isAck() && m->dstAdd.compareTo(address) == 0) {
            EV << "Message received\n";
            //record its latency
            recordScalar("#latency", m->latency(simTime()));
            //one more received request message
            numRecvd++;
            //one more ack sent
            numSentAck++;
            //one more message generated
            numGen++;
            messageQ->enQueue(m->response(simTime()));

          //If it's an ack message sent to me receive it
        } else if (m->isAck() && m->srcAdd.compareTo(address) == 0 && messageQ->isQueued(m->original())) {
            EV << "************* RECEIVED ACK *************\n";
            messageQ->remove(m->original());
            //record the ack latency
            recordScalar("#acklatency", m->latency(simTime()));
            //one more ack received
            numAckd++;
            //one more ack received
            numRecvdAck++;

        //If it's an ack but not directed to me delete the original message (if I have it in my queue) and store it
        } else if (m->isAck() && messageQ->isQueued(m->original())) {
            EV << "PASSING ACK\n";
            messageQ->remove(m->original());
            //one more generated message
            numGen++;
            messageQ->enQueue(m->getSSID());

        //If it's none of the above and I don't have it store it in my queue
        } else if (!messageQ->isQueued(m->getSSID())) {
            if (!messageQ->isQueued(m->response())) {
                //One more generated message
                numGen++;
                messageQ->enQueue(m->getSSID());
            }
        }
    }
    messageQ->print();

    if (messageQ->size() > 0 && f_wait4msg) {
        f_wait4msg = false;
        delayBOtoOB();
    } else {
        //scheduleAt(simTime() + tsi, scanEv);
    }
}


void OpplineApp::test() {
    int size = 10, i;
    CircularQueue msgQ(size);
    OpplineMsg *msg;
    string *cqueue_arr = new std::string[size+5];
    for (i = 0; i < size+5; i++) {
        msg = new OpplineMsg(simTime(), i%2==0?REQ:ACK, address, inet::MACAddress(minMac+i));
        cqueue_arr[i] = std::to_string(i);
        msgQ.enQueue(cqueue_arr[i]);
    }


    msgQ.debugPrint();
}


void OpplineApp::finish()
{
    // This function is called by OMNeT++ at the end of the simulation.
    EV << "Sent:     " << numSent << '\t' << "Sent ack:    " << numSentAck <<endl;
    EV << "Received: " << numRecvd << endl;
    EV << "Acked:    " << numAckd << endl;
    recordScalar("#sent", numSent);
    recordScalar("#acksent", numSentAck);
    recordScalar("#received", numRecvd);
    recordScalar("#ackreceived", numRecvdAck);
    recordScalar("#acked", numAckd);
    recordScalar("#generated", numGen);
}
