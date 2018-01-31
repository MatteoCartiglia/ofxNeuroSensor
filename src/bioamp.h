/*
 * bioamp.h
 * --------
 * Copyright : (c) 2017, Germain Haessig <Germain.Haessig@inserm.fr>
 * Licence   : BSD3
 */

#ifndef __BIOAMP_H
#define __BIOAMP_H

#include "okFrontPanelDLL.h"
#include <iostream>
#include <fstream>
#include <boost/thread.hpp>
#include <vector>
#include <thread>
#include <queue>
#include <sys/stat.h>
#include <stdlib.h>

#include "events.h"

// uncomment to get some verbose debug
//#define DEBUG

#define MIN_BLOCK_SIZE      16              // bytes
#define OF_CODE         0xFFFF          // Overflow code

#define READ_ADDR       0xA0  // Address for reading from device
#define WRITE_ADDR      0x80  // Adress for writting to device

#define READ_DELAY 1 // period of device's pooling

// DAC Specific commands
#define NOP 0x000000          // NOP
#define SOFT_PWRUP 0x090000   // Soft power up
#define SOFT_PWRDN 0x080000   // soft power down
#define CR 0x0C3C00           // Control register mask
#define SOFT_RST 0x0F0000     // Soft reset
#define OFFSET_CFG 0x00A000   // Offset register mask
#define GAIN_CFG 0x007FF8     // Gain register mask
#define DATA_MASK 0x00C000    // Data mask
#define ADDR_SHIFT 16

#define N_FIFO_IN 2

// Biases definitions
#define N_BIASES 12
#define MAXIMUM_BIAS_VOLTAGE 3300 // 3300mV max

extern bool FPGA ;


class BioAmp
{
public:
  /**
   * [BioAmp Constructor]
   * @param config_filename [bitfile to load]
   */
  BioAmp(std::string config_filename);
  ~BioAmp();
  /**
   * [initializeFPGA : Load bitfile in device]
   * @param config_filename [Bitfile to load]
   */
  bool exitOnError(okCFrontPanel::ErrorCode error);
  /**
   * [read : Emptying FPGA buffer -- Doing nothing else]
   * @return [false if ok]
   */
  bool read();
  /**
   * [read : Read data from device and store them in stack]
   * @param  ev_buffer [std::queue for stacking]
   * @return           [false if ok]
   */
  bool read(std::queue<Event2d>* ev_buffer);
  /**
   * [write : Write to device]
   * @param ev_buffer [std::queue containing events to write]
   */
  void write(std::queue<EventRaw>* ev_buffer, unsigned char addr);
  void write_wrapper(std::queue<EventRaw>* ev_buffer);
  void join_write_thread(std::queue<EventRaw>* ev_buffer);
   /**
    * [set_aer : Enable matrix pixels]
    * @param state [true or false]
    */
  void enable_aer(bool state);
  /**
   * [enable_tp : Enable test pixel]
   * @param state [true or false]
   */
  void enable_tp(bool state);
  /**
   * [reset : Reset the device]
   */
  void reset();
  /**
   * [listen : Create a thread listening to the device and pooling it]
   * @param ev_buffer [Pointer to a std::queue where to store events]
   */
  void listen(std::queue<Event2d>* ev_buffer);
  /**
   * [join_thread : Wrapper for threading]
   * @param ev_buffer [Pointer to a std::queue where to store events]
   */
  void join_thread(std::queue<Event2d>* ev_buffer);
  /**
   * [set_biases : Set all the Biases]
   * @param biases [int*]
   */
  void set_biases(int *biases);
  /**
   * [enable_record : Enable record. If filename already given, appends to same file]
   * @param state [true or false]
   */
  void enable_record(bool state);
  /**
   * [enable_record : Enable record, giving filemane]
   * @param state       [true or false]
   * @param record_file [File where to store]
   */
  void enable_record(bool state, std::string record_file);

  bool get_fifo_state(unsigned char id);

  /**
   * [ok : Check device connection]
   * @return [true if device still alive]
   */
  bool ok();


  void load_biases(std::string filename);

  unsigned int get_aer_state();

  bool is_listening, is_writing;
  unsigned long long event_counter;
  unsigned long long of_counter;

  unsigned long ev_cnt[N_FIFO_IN], of_cnt[N_FIFO_IN];

  int values[N_BIASES];

private:
  okTDeviceInfo *g_devInfo;
  okCFrontPanel *dev;
  bool aer_enabled, tp_enabled;
  boost::mutex thread_safety_;
  uint32_t data_count;
  std::thread listener, writer;
  unsigned long long time_ref;
  bool record;
  std::string record_filename;
  std::ofstream* ofs;
  std::vector<bool> fifo_state;
  /**
   * [initializeFPGA : Initialize FPGA with given bitfile]
   * @param config_filename [bitfile to load]
   */
  void initializeFPGA(std::string config_filename);
  /**
   * [exitOnError : Error handler for verbose output]
   * @param  error [Error code]
   * @return       [status]
   */
  /**
   * [init_DACs : Initialize DACs]
   */
  void init_DACs();
  /**
   * [dac_write Write 32 bits to DAC register ]
   * @param val [32 bit word]
   */
  void dac_write(unsigned int val);
};

#endif /* __BIOAMP_H */
