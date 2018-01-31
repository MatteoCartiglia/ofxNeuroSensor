#include "events.h"

Event2d::Event2d() {
  ts = 0;
  x = 0;
  y = 0;
  p = 0;
}
Event2d::Event2d(unsigned long long _ts, unsigned int _x, unsigned int _y, unsigned char _p) {
    ts = _ts;
    x = _x;
    y = _y;
    p = _p;
}

void Event2d::WriteEvent(std::ofstream* of) {
    unsigned long long tmp = (unsigned long)ts + (unsigned long)((((x & 0x1F) + ((y & 0x1F) << 5) + (p << 10))));

  of->write( reinterpret_cast<const char*>(&tmp), 8 ) ;

}
/*
Event2d::WriteEvent(std::ofstream *of) {
  char buffer[8];
  //ofstream myFile ("data.bin", ios::out | ios::binary);
  buffer =
  of->.write (buffer, 100);
}*/

EventRaw::EventRaw() {
  ts = 0;
  data = 0;
}
EventRaw::EventRaw(unsigned long long _ts, unsigned short _data) {
  ts = _ts;
  data = _data;
}

void EventRaw::WriteEvent(std::ofstream* of) {
  unsigned long long  tmp = (unsigned long)ts + ((unsigned long)data << 32);
    std::cout << "time" << ts <<"data" << data<< "tmp: "<< tmp  << "size of tmp: " << sizeof(tmp)<<std::endl;

  of->write( reinterpret_cast<const char*>(&tmp), 8 ) ;

}
