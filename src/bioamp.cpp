#include "bioamp.h"
bool FPGA= false;


BioAmp::BioAmp(std::string config_filename) {

  char dll_date[32], dll_time[32];
  printf("---- BioAmp application v1.1 ----\n");
  if (FALSE == okFrontPanelDLL_LoadLib(NULL)) {
    printf("FrontPanel DLL could not be loaded.\n");
  }
  okFrontPanelDLL_GetVersion(dll_date, dll_time);
  printf("FrontPanel DLL loaded.  Built: %s  %s\n", dll_date, dll_time);

  aer_enabled = false;
  tp_enabled = false;
  is_listening = false;
  is_writing = false;
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
      exit(2);
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

  // fifo in initial states to true
  fifo_state = std::vector<bool>(2,0);

  for(int i = 0 ; i < N_FIFO_IN ; i++) {
    ev_cnt[i] = 0;
    of_cnt[i] = 0;
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
  if(ofs != NULL) {
        std::cout << "Closing file" << std::endl;
        if(ofs->is_open()) ofs->close();
  }
  delete ofs;
  std::cout << "Terminate threads" << std::endl;
  // Detach from thread
  if(is_listening)  listener.detach();
  if(is_writing) writer.detach();

  delete g_devInfo;
  delete dev;

  for(int i = 0 ; i < N_FIFO_IN ; i++) {
    printf("FiFo %d : Sent %ld events, %ld overflow\n",i,ev_cnt[i],of_cnt[i]);
  }
  printf("All done, Bye Bye.\n");
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

void BioAmp::write(std::queue<EventRaw>* ev_buffer, unsigned char addr)
{
  unsigned int N = ev_buffer->size();

  if(N < 256) {
    if(N >= 128) N = 128;
    else if (N >= 64) N = 64;
    else if(N >= 32) N = 32;
    else if (N >= 16) N = 16;
    else if (N >= 8) N = 8;
    else return;
  }
  else {
    N = 256;
  }

  unsigned char g_buf[4*N];

	for (unsigned int i=0; i<N; i++){
    EventRaw ev = ev_buffer->front();
    if(ev.data == 0xFFFF && ev.ts == 0xFFFF) of_cnt[addr-WRITE_ADDR]++;
    else ev_cnt[addr-WRITE_ADDR]++;
    g_buf[4*i+0] = (unsigned char)((unsigned short)ev.data & 0xFF);
    g_buf[4*i+1] = (unsigned char)(((unsigned short)ev.data >> 8) & 0xFF);
    g_buf[4*i+2] = (unsigned char)((unsigned short)ev.ts & 0xFF);
    g_buf[4*i+3] = (unsigned char)(((unsigned short)ev.ts >> 8) & 0xFF);
    ev_buffer->pop();
	}
  boost::lock_guard<boost::mutex> guard(thread_safety_);
	long ret = dev->WriteToBlockPipeIn(addr, 4*N, 4*N, &g_buf[0]);
  //#ifdef DEBUG
  printf("Write done with status %ld (%d)\n",ret,addr-WRITE_ADDR);
  //#endif
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

bool BioAmp::read(std::queue<Event2d>* ev_buffer)
{
  long ret;
  boost::lock_guard<boost::mutex> guard(thread_safety_);
  dev->UpdateWireOuts();
  data_count = dev->GetWireOutValue(0x20);
  #ifdef DEBUG
    std::cout << "Data count in FIFO : " << data_count << "(" << data_count*4 << " bytes)" << std::endl;
  #endif
  while((data_count*4) >= MIN_BLOCK_SIZE) {
    // Find nearest block size to efficiently read from device
    unsigned int block_size = (1 << ((32 - __builtin_clz (data_count*4 ))-1));
    //printf("Data count : %d (%d) , block size %d,\n",data_count, data_count*4,block_size);
    unsigned char g_rbuf[block_size];
    ret = dev->ReadFromBlockPipeOut(READ_ADDR, block_size, block_size , &g_rbuf[0]);
    if (false == exitOnError((okCFrontPanel::ErrorCode)ret)) {
      std::cout << "Error" << std::endl;
    }
    data_count -= ret/4;
    for (unsigned int i=0; i<ret/4; i++){
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
        Event2d ev;
        EventRaw tt;
        event_counter++;
        if(ev_buffer != NULL) {
            if(data == 0x1000  ) {
                
                ev = Event2d(time_ref + timestamp, 35,35,0);
           //     std::cout << "TP down" << std::endl;
         //       std::cout << "Polarity: " << (int)ev.p <<std::endl;

            }
            else if ( data == 0x1001){
                ev = Event2d(time_ref + timestamp, 35,35,1);
  //              std::cout << "TP Up" << std::endl;
//                std::cout << "Polarity: " << (int)ev.p <<std::endl;

            }
            else {
                unsigned int x = data & 0x1F;
                unsigned int y = (data >> 6) & 0x1F;
                unsigned char p1 = (data >> 5) & 0x1;
                unsigned char p2 = (data >> 11) & 0x1;
                std::cout << "AER event" << std::endl;

                if(p1!=p2) {
                    std::cout << "WARNING : Invalid event." << std::endl;
                }
                ev = Event2d(time_ref + timestamp, x,y,p1);
               

            }
            tt = EventRaw(time_ref + timestamp, data);
         //   std::cout << "time" << timestamp <<"data" << data<< std::endl;
            // store to file
            if(record && ofs->is_open()) {
                tt.WriteEvent(ofs);
           //     std::cout << "time tt.ts" << tt.ts <<"data" << tt.data<< std::endl;


            }
            ev_buffer->push(ev);
            
            }
      }
    }
  }
  if (false == dev->IsOpen()){
    exitOnError(okCFrontPanel::DeviceNotOpen);
  }
  return(false);
}

unsigned int BioAmp::get_aer_state() {
  boost::lock_guard<boost::mutex> guard(thread_safety_);
  dev->UpdateWireOuts();
  return dev->GetWireOutValue(0x22);
}

bool BioAmp::get_fifo_state(unsigned char id) {
  boost::lock_guard<boost::mutex> guard(thread_safety_);
  dev->UpdateWireOuts();
  unsigned int val = dev->GetWireOutValue(0x22);
  for(int i = 0 ; i < N_FIFO_IN ; i++) {
    fifo_state.at(i) = (val >> i)&0x1;
  }
  return fifo_state.at(id);
}

void BioAmp::listen(std::queue<Event2d>* ev_buffer) {
  while(is_listening) {
    read(ev_buffer);
    std::this_thread::sleep_for(std::chrono::milliseconds(READ_DELAY));
  }
}

void BioAmp::write_wrapper(std::queue<EventRaw>* ev_buffer) {
  while(is_writing) {
    for(unsigned char i = 0 ; i < N_FIFO_IN ; i++) {
      //bool full = get_fifo_state(i);
      if(true) {
        write(ev_buffer+i,WRITE_ADDR+i);
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}



void BioAmp::join_thread(std::queue<Event2d>* ev_buffer) {
  is_listening = true;
  listener = std::thread([this, ev_buffer]{listen(ev_buffer);});
}

void BioAmp::join_write_thread(std::queue<EventRaw>* ev_buffer) {
  is_writing = true;
  writer = std::thread([this, ev_buffer]{write_wrapper(ev_buffer);});
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
    return dev->IsOpen();
}


// ********************************
// ***** BIASES GENERATION ********
// ********************************

void BioAmp::set_biases(int *biases) {
    printf("== Configuring Biases ==\n");
    boost::lock_guard<boost::mutex> guard(thread_safety_);
    for (unsigned int i=0; i < N_BIASES; i++) {
        unsigned int encoded =(i << 16) +  (static_cast<unsigned int>((biases[i]/5000.)*4095.) << 2) + 0x00C003;
        printf("Set bias #%d to %d mV [%06X]\n",i,biases[i],encoded);
        dac_write(encoded);
    }
    printf("\n");
}

void BioAmp::load_biases(std::string filename) {
  std::ifstream file;
  char buf[256];
  unsigned int i;

  // Open file
  file.open(filename.c_str());
  if(!file.good()) {
      printf("Bias file '%s' not found\n",filename.c_str());
      exit(1);
  }
  // Discard header
  for (;file.peek() == '%'; file.getline(buf, sizeof(buf)));

  // Read biases
  for (i=0; i < N_BIASES; i++) {
    file.getline(buf, sizeof(buf));
    if (file.gcount() == 0)
      break;
    values[i] = atoi(buf);
  }

  // Close file
  file.close();

  // Verify if the file had enough data in it
  if (i < N_BIASES) {
    std::cout << "Error loading biases" << std::endl;
  }
  else {
    set_biases(values);
  }
}


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
