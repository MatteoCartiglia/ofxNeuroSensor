#include "bioamp.h"
#include <iostream>

#define N_BIASES 12

bool FPGA= false;


BioAmp::BioAmp(std::string config_filename) {
  char dll_date[32], dll_time[32];
  printf("---- BioAmp application v1.0 ----\n");
  if (FALSE == okFrontPanelDLL_LoadLib(NULL)) {
    printf("FrontPanel DLL could not be loaded.\n");
  }
  okFrontPanelDLL_GetVersion(dll_date, dll_time);
  printf("FrontPanel DLL loaded.  Built: %s  %s\n", dll_date, dll_time);

  aer_enabled = true;
  tp_enabled = false;
  is_listening = false;
  record = false;

  event_counter = 0;
  of_counter = 0;

  // initialize local timestamp reference to 0
  time_ref = 0;

  // Initialize the FPGA with our configuration bitfile.
  //check if bitfile exists
  struct stat buffer;
  if(stat (config_filename.c_str(), &buffer) != 0) {
      printf("ERROR : Bitfile '%s' not found!\n",config_filename.c_str());
      //exit(2);
  }
    std::cout << "dev: " << dev<< std::endl;

  // Configure FPGA
  initializeFPGA(config_filename);

  // Set all buffers to default state
  enable_aer(false);
  enable_tp(false);
  // Configure DACs
    if(FPGA){
  init_DACs();
       
        reset();
    

  ofs = NULL;
    }
  // Check FPGA configuration
  if (NULL == dev) {
    printf("\nFPGA could not be initialized.\n\n");
  }
  else {
    printf("\nFPGA initialized.\n\n");
  }
}

BioAmp::~BioAmp() {
    if (FPGA){
    if(ofs != NULL) {
        std::cout << "Closing file" << std::endl;
        if(ofs->is_open()) ofs->close();
  }
 delete ofs;
  std::cout << "Terminate poolling thread" << std::endl;
  // Detach from thread
  listener.detach();
  delete g_devInfo;
  delete dev;
  printf("All done, Bye Bye.\n");
    }
}

void BioAmp::initializeFPGA(std::string config_filename) {
    // Open the first XEM
    dev = new okCFrontPanel;
    if (okCFrontPanel::NoError != dev->OpenBySerial()) {
        printf("Device could not be opened. Is one connected?\n");
        dev = NULL;
        FPGA=FALSE;
        return;
    }
    printf("Found a device: %s (%s)\n", dev->GetBoardModelString(dev->GetBoardModel()).c_str(),dev->GetDeviceID().c_str());
    FPGA=true;

    g_devInfo = new okTDeviceInfo;
    dev->GetDeviceInfo(g_devInfo);
    if(dev->GetBoardModel() != okCFrontPanel::brdXEM7310A75) {
        printf("Unsupported device.\n");
        dev = NULL;
        FPGA=false;
        return;
    }
    // Configure the PLL appropriately
    dev->LoadDefaultPLLConfiguration();

    // Get some general information about the XEM.
    printf("Device firmware version: %d.%d\n", dev->GetDeviceMajorVersion(), dev->GetDeviceMinorVersion());
    printf("Device serial number: %s\n", dev->GetSerialNumber().c_str());

    if (okCFrontPanel::NoError != dev->ConfigureFPGA(config_filename)) {
        printf("FPGA configuration failed.\n");
        dev = NULL;
        FPGA =FALSE;
        return;
    }

    // Check for FrontPanel support in the FPGA configuration.
    if (false == dev->IsFrontPanelEnabled()) {
        printf("FrontPanel support is not enabled.\n");
        dev = NULL;
     //   exit(1);
        return;
    }
}

void BioAmp::reset() {
  // Reset FIFOs
  dev->SetWireInValue(0x00, 0x0004);
  dev->UpdateWireIns();
  // Enable write and read memory transfers
    std::cout << "aer_enabled: "<< aer_enabled << "tp: " << tp_enabled <<std::endl;
  dev->SetWireInValue(0x00, 0x0003 + (aer_enabled << 3) + (tp_enabled << 4));
  dev->UpdateWireIns();
  // reset chip
  dev->ActivateTriggerIn(0x40, 6);
  // reset AER Handler
  dev->ActivateTriggerIn(0x40, 7);
}

void BioAmp::enable_aer(bool state) {
  aer_enabled = state;
}

void BioAmp::enable_tp(bool state) {
  tp_enabled = state;
}

// Exits on critical failure
// Returns true if there is no error
// Returns false on non-critical error
bool BioAmp::exitOnError(okCFrontPanel::ErrorCode error)
{
	switch (error) {
	case okCFrontPanel::DeviceNotOpen:
		printf("Device no longer available.\n");
		exit(EXIT_FAILURE);
	case okCFrontPanel::Failed:
		printf("Transfer failed.\n");
		exit(EXIT_FAILURE);
	case okCFrontPanel::Timeout:
		printf("   ERROR: Timeout\n");
		return false;
	case okCFrontPanel::TransferError:
		std::cout << "   ERROR: TransferError" << '\n';
		return false;
	case okCFrontPanel::UnsupportedFeature:
		printf("   ERROR: UnsupportedFeature\n");
		return false;
	case okCFrontPanel::DoneNotHigh:
	case okCFrontPanel::CommunicationError:
	case okCFrontPanel::InvalidBitstream:
	case okCFrontPanel::FileError:
	case okCFrontPanel::InvalidEndpoint:
	case okCFrontPanel::InvalidBlockSize:
	case okCFrontPanel::I2CRestrictedAddress:
	case okCFrontPanel::I2CBitError:
	case okCFrontPanel::I2CUnknownStatus:
	case okCFrontPanel::I2CNack:
	case okCFrontPanel::FIFOUnderflow:
	case okCFrontPanel::FIFOOverflow:
	case okCFrontPanel::DataAlignmentError:
	case okCFrontPanel::InvalidParameter:
		std::cout << "   ERROR: " << error << '\n';
		return false;
	default:
		return true;
	}
}

void BioAmp::write(std::queue<EventRaw>* ev_buffer)
{
  unsigned int N = ev_buffer->size();
  unsigned char g_buf[4*N];

	for (unsigned int i=0; i<N; i++){
    EventRaw ev = ev_buffer->front();
    g_buf[4*i+0] = (unsigned char)((unsigned short)ev.data & 0xFF);
    g_buf[4*i+1] = (unsigned char)(((unsigned short)ev.data >> 8) & 0xFF);
    g_buf[4*i+2] = (unsigned char)((unsigned short)ev.ts & 0xFF);
    g_buf[4*i+3] = (unsigned char)(((unsigned short)ev.ts >> 8) & 0xFF);
    ev_buffer->pop();
	}
  boost::lock_guard<boost::mutex> guard(thread_safety_);
	long ret = dev->WriteToBlockPipeIn(WRITE_ADDR, 4*N, 4*N, &g_buf[0]);
  #ifdef DEBUG
	  std::cout << "Write done with status " << ret << std::endl;
  #endif
	if (ret < 0) {
		exitOnError((okCFrontPanel::ErrorCode)ret);
	}
  if(ret != 4*N) {
    printf("oupsi, asked to send %d but %ld really sent\n",4*N, ret);
    //throw;
  }
}

bool BioAmp::read() {
  read(NULL);
  return(false);
}

bool BioAmp::read(std::queue<EventRaw>* ev_buffer)
{
  long ret;
  boost::lock_guard<boost::mutex> guard(thread_safety_);
  dev->UpdateWireOuts();
  data_count = dev->GetWireOutValue(0x20);
  #ifdef DEBUG
    std::cout << "Data count in FIFO : " << data_count << "(" << data_count*4 << " bytes)" << std::endl;
  #endif
  if (data_count < BLOCK_SIZE/4) {
    #ifdef DEBUG
      std::cout << "Not enough data to be read, skipping..." << std::endl;
    #endif
    return(true);
  }
  for (unsigned int k = 0 ; k < (data_count*4)/BLOCK_SIZE ; k++) {
    unsigned char g_rbuf[BLOCK_SIZE];
    ret = dev->ReadFromBlockPipeOut(READ_ADDR, BLOCK_SIZE, BLOCK_SIZE , &g_rbuf[0]);
    if (false == exitOnError((okCFrontPanel::ErrorCode)ret)) {
      std::cout << "Error" << std::endl;
    }
    for (unsigned int i=0; i<BLOCK_SIZE/4; i++){
      unsigned short timestamp = g_rbuf[4*i+2]+(g_rbuf[4*i+3]<<8);
      unsigned short data = g_rbuf[4*i]+(g_rbuf[4*i+1]<<8);

      if(data == OF_CODE && timestamp == OF_CODE) {
        of_counter++;
        // add a full overflow to time reference (2^16)
        time_ref += 65535;
        #ifdef DEBUG
          printf("(%d.%d) | Timer overflow\n",k,i);
        #endif
      }
      else {
        event_counter++;
        if(ev_buffer != NULL) {
          EventRaw tt = EventRaw(time_ref + timestamp, data);
          // store to file
          if(record && ofs->is_open()) {
            tt.WriteEvent(ofs);
          }
          ev_buffer->push(tt);
        }
        #ifdef DEBUG
          printf("(%d.%d) | TS : %d - data : %04X \n",k,i,timestamp,data);
        #endif
      }
    }
  }
  if (false == dev->IsOpen()){
    exitOnError(okCFrontPanel::DeviceNotOpen);
  }
  return(false);
}

void BioAmp::get_count() {
  unsigned int val = dev->GetWireOutValue(0x22);
  printf("Fifo2AER : %d \t| AER2Fifo : %d\n",val&0xFFFF,(val&0xFFFF)>>16);
}

void BioAmp::listen(std::queue<EventRaw>* ev_buffer) {
  while(is_listening) {
    read(ev_buffer);
    std::this_thread::sleep_for(std::chrono::milliseconds(READ_DELAY));
  }
}



void BioAmp::join_thread(std::queue<EventRaw>* ev_buffer) {
  is_listening = true;
  listener = std::thread([this, ev_buffer]{listen(ev_buffer);});
}

// Store with default filename (record+date)
void BioAmp::enable_record(bool state) {
    if(state == true) {
        std::string file = "recordings/record_today.dat";
        ofs = new std::ofstream(file, std::ios::out | std::ios::binary);
    }
    else {
        ofs->close();
    }
    record = state;
}
// Store in given filename
void BioAmp::enable_record(bool state, std::string record_file) {
  if(state == true) {
    ofs = new std::ofstream(record_file, std::ios::out | std::ios::binary);
  }
  else {
    ofs->close();
  }
  record = state;
}

bool BioAmp::ok() {
    std::cout <<"prima "<< std::endl;
    return dev->IsOpen();
    std::cout <<"dopo "<< std::endl;

}


// ********************************
// ***** BIASES GENERATION ********
// ********************************

void BioAmp::set_biases(int *biases) {
  printf("== Configuring Biases ==\n");
  boost::lock_guard<boost::mutex> guard(thread_safety_);
  for (unsigned int i=0; i < N_BIASES; i++) {
    unsigned int encoded =((unsigned int)i << 16) +  ((static_cast<unsigned int>((biases[i]/5000.)*4095.)) << 2) + 0x00C003;
    unsigned int val = biases[i];
    printf("Set bias #%d to %d mV [%06X]\n",i,val,encoded);
    dac_write(encoded);
  }
  printf("\n");
}
/* original
 void BioAmp::set_biases(BioampBiases *biases) {
    printf("== Configuring Biases ==\n");
    boost::lock_guard<boost::mutex> guard(thread_safety_);
    for (unsigned int i=0; i < N_BIASES; i++) {
        unsigned int encoded = biases->get_encoded((BioampBiases::Bias)i);
        unsigned int val = biases->get((BioampBiases::Bias)i);
        printf("Set bias #%d to %d mV [%06X]\n",i,val,encoded);
        dac_write(encoded);
    }
    printf("\n");
}*/


void BioAmp::init_DACs() {
  boost::lock_guard<boost::mutex> guard(thread_safety_);
  printf("\nDAC INITILIZATION\n");
  // reset DAC handler
  printf("\treset\n");
  dev->ActivateTriggerIn(0x40, 0);
  dac_write(SOFT_RST);
  usleep(1500);
  printf("\tconfig register\n");
  dac_write(CR);
  // POWER up the device
  printf("\tpower up\n");
  dac_write(SOFT_PWRUP);
  usleep(1500);
  for (unsigned int i=0; i < N_BIASES; i++) {
    // Config Offset register
    dac_write( OFFSET_CFG + (i<<ADDR_SHIFT) );
    // Config Gain register
    dac_write( GAIN_CFG + (i<<ADDR_SHIFT) );
  }
}

void BioAmp::dac_write(unsigned int val) {
  // Update WireIn values
  dev->SetWireInValue(0x01, val );
  dev->UpdateWireIns();
  // Activate Trigger to notify FPGA of new values
  dev->ActivateTriggerIn(0x40, 1);
}
