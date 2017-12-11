/*
 * CircularQueue.cpp
 *
 *  Created on: Sep 29, 2017
 *      Author: Maldus
 */
#include <string.h>
#include <iostream>
#include <omnetpp.h>
#include "CircularQueue.h"
#include "SSIDMessage.h"

using namespace omnetpp;


CircularQueue::CircularQueue(int size) {
    cqueue_arr = new std::string[size];
    rear = front = -1;
    qSize = size;
}

CircularQueue::~CircularQueue() {
    delete(cqueue_arr);
}

/*
 * Insert into Circular Queue
 */
int CircularQueue::enQueue(std::string item) {
    if ((front == 0 && rear == qSize-1) || (front == rear+1)) {
        //Queue overflow --> dequeue to make room
        deQueue();
        //removeOldest();
    }

    if (front == -1) {
        front = 0;
        rear = 0;
    } else {
        if (rear == qSize - 1)
            rear = 0;
        else
            rear = rear + 1;
    }
    cqueue_arr[rear] = item ;
    return 0;
}

int CircularQueue::findOldest() {
    int64_t maxlat, tmp;
    OpplineMsg *msg;
    maxlat = 0;
    int res = -1;
    int i = front;
    if (front == -1) {
        return -1;
    } else if (front == rear) {
        return front;
    }else if (i < rear) {
        while(i <= rear) {
            msg = new OpplineMsg(cqueue_arr[i]);
            tmp = msg->latency(omnetpp::simTime());
            if (tmp > maxlat) {
                maxlat = tmp;
                res = i;
            }
            i++;
        }
    } else {
        while (i < qSize) {
            msg = new OpplineMsg(cqueue_arr[i]);
            tmp = msg->latency(omnetpp::simTime());
            if (tmp > maxlat) {
                maxlat = tmp;
                res = i;
            }
            i++;
        }
        i = 0;
        while (i <= rear) {
            msg = new OpplineMsg(cqueue_arr[i]);
            tmp = msg->latency(omnetpp::simTime());
            if (tmp > maxlat) {
                maxlat = tmp;
                res = i;
            }
            i++;
        }
    }
    return res;
}

void CircularQueue::removeOldest() {
    int i = findOldest();
    if (i > 0) {
        remove(cqueue_arr[i]);
    }
}

void CircularQueue::remove(std::string s) {
    int i = front - 1, found = -1;
    if (front == -1) {
        return;
    }
    do {
        if (i == qSize-1) {
            i = 0;
        } else {
            i++;
        }
        EV << "Comparing: " << cqueue_arr[i] << " --- " << s << '\n';
        if (equal(cqueue_arr[i], s)) {
            found = i;
            break;
        }
    } while (i != rear);
    if (found == -1) {
        return;
    }

    if (front == rear && front == found) {
        front = rear = -1;
    } else if (front < rear) {
        for (i = found; i < rear; i++) {
            cqueue_arr[i] = cqueue_arr[i+1];
        }
        if (rear == 0) {
            rear = qSize -1;
        } else {
            rear--;
        }
    } else if (rear < front) {
        if (found <= rear) {
            for (i = found; i < rear; i++) {
                cqueue_arr[i] = cqueue_arr[i+1];
            }
            if (rear == 0) {
                rear = qSize-1;
            } else {
                rear--;
            }
        } else if (found >= front) {
            for (i = found; i > front; i--) {
                cqueue_arr[i] = cqueue_arr[i-1];
            }
            if (front == qSize-1){
                front = 0;
            } else {
                front++;
            }
        }
    }
}

bool CircularQueue::isQueued(std::string s) {
    int i = front-1;
    if (front == -1) {
        return false;
    }
    do {
        if (i == qSize-1) {
            i = 0;
        } else {
            i++;
        }
        if (equal(cqueue_arr[i], s)) {
            return true;
        }
    } while(i != rear);

    return false;
}

bool CircularQueue::equal(string m1, string m2) {
    int i;
    string s;
    if (m1.length() != 32 || m2.length() != 32) {
        if (m1.length() != 32) {
            s = m1;
        } else {
            s = m2;
        }
        std::cout << s;
        throw std::invalid_argument( "message of invalid length in queue: <" + s + ">" );
    }

    for (i = 0; i < 33; i++) {
        if (i >= 2 && i <= 6) {
            continue; //do not consider timestamp in comparing messages
        }
        if (m1[i] != m2[i]) {
            return false;
        }
    }

    return true;
}

/*
 * Delete from Circular Queue
 */
std::string CircularQueue::deQueue() {
    std::string res;
    if (front == -1) {
        //Queue Underflow\n";
        return NULL;
    }
    res = cqueue_arr[front];
    if (front == rear) {
        front = -1;
        rear = -1;
    } else {
        if (front == qSize - 1)
            front = 0;
        else
            front = front + 1;
    }
    return res;
}

int CircularQueue::size() {
    if (front == -1) {
        return 0;
    } else {
        if (front == rear) {
            return 1;
        } else if (front < rear) {
            return rear - front + 1;
        } else {
            return (qSize - front) + rear + 1;
        }
    }
    return qSize;
}


void CircularQueue::print() {
    int i = front;
    if (front == -1) {
        EV << "emtpy queue\n";
    } else if (front == rear) {
        EV<< '\t' <<"Printing Q of size " << size() << '\n';
        EV << '\t' << cqueue_arr[i] << '\n';
    }else if (i < rear) {
        EV<< '\t' <<"Printing Q of size " << size() << '\n';
        while(i <= rear) {
            EV << '\t' << cqueue_arr[i] << " ---size " <<cqueue_arr[i].length() << " ("<< i<<")" << '\t';
            i++;
        }
    } else {
        EV<< '\t' <<"Printing Q of size " << size() << '\n';
        while (i < qSize) {
            EV << '\t' << cqueue_arr[i]<< " ("<< i<<")" << '\n';
            i++;
        }
        i = 0;
        while (i <= rear) {
            EV << '\t' << cqueue_arr[i]<< " ("<< i<<")" << '\n';
            i++;
        }
    }
}

void CircularQueue::debugPrint() {
    int i;
    std::cout << "front: " << front << " --- rear: " << rear << " --- size: " << size() <<'\n';
    for (i = 0; i < qSize; i++) {
        std::cout << cqueue_arr[i] << " (" << i << ")" << '\t';
    }
    std::cout << "\n\n";
}

