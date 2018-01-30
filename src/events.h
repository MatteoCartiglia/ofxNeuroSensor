/*
 * events.h
 * --------
 * Copyright : (c) 2017, Germain Haessig <Germain.Haessig@inserm.fr>
 * Licence   : BSD3
 */

#ifndef __EVENTS_H
#define __EVENTS_H

#include <fstream>
#include <iostream>

class Event2d {
public:
    Event2d();
    Event2d(unsigned long long _ts, unsigned int _x, unsigned int _y, unsigned char _p);
    ~Event2d(){};
    void WriteEvent(std::ofstream* of);
    unsigned long long ts;
    unsigned int x,y;
    unsigned char p;
};

class EventRaw {
public:
    EventRaw();
    EventRaw(unsigned long long _ts, unsigned short _data);
    ~EventRaw(){};
    void WriteEvent(std::ofstream* of);
    unsigned long long ts;
    unsigned short data;
};

#endif /* __EVENTS_H */
