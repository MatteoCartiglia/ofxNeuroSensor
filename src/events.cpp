#include "events.h"

EventRaw::EventRaw() {
  ts = 0;
  data = 0;
}
EventRaw::EventRaw(unsigned long long _ts, unsigned short _data) {
  ts = _ts;
  data = _data;
}

void EventRaw::WriteEvent(std::ofstream* of) {
  unsigned long tmp = (unsigned long)ts + ((unsigned long)data << 32);
  of->write( reinterpret_cast<const char*>(&tmp), 8 ) ;
}
