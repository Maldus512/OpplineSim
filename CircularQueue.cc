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
        //Queue overflow
        return -1;
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

void CircularQueue::remove(std::string s) {//TODO consider empty queue
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
        if (cqueue_arr[i].compare(s) == 0) {
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
        rear--;
    } else if (rear < front) {
        if (found <= rear) {
            for (i = found; i < rear; i++) {
                cqueue_arr[i] = cqueue_arr[i+1];
            }
            rear--;
        } else if (found >= front) {
            for (i = found; i > front; i--) {
                cqueue_arr[i] = cqueue_arr[i-1];
            }
            front++;
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
        if (cqueue_arr[i].compare(s) == 0) {
            return true;
        }
    } while(i != rear);

    return false;
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
        EV << '\t' << cqueue_arr[i] << '\n';
    }else if (i < rear) {
        EV<< '\t' <<"Printing Q of size " << size() << '\n';
        while(i <= rear) {
            EV << '\t' << cqueue_arr[i] << " ("<< i<<")" << '\t';
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

