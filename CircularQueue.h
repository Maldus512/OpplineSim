/*
 * CircularQueue.hpp
 *
 *  Created on: Sep 29, 2017
 *      Author: Maldus
 */

#ifndef CIRCULARQUEUE_H_
#define CIRCULARQUEUE_H_

#include <string.h>

using namespace std;

class CircularQueue {
  public:
    CircularQueue(int size);
    virtual ~CircularQueue();
    virtual int enQueue(std::string s);
    virtual std::string deQueue();
    int size();
    bool isQueued(string s);
    void print();
    void remove(string s);
  private:
    int qSize;
    std::string *cqueue_arr;
    int front, rear;
};



#endif /* CIRCULARQUEUE_H_ */
