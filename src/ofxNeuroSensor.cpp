#include "ofxNeuroSensor.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <chrono>
#include <signal.h>
#include <stdlib.h>
#include <ctime>
#include <thread>
#include <iostream>
#include <fstream>
#include <sstream>
#include <bitset>
#include <algorithm>
#include <vector>

// BIOAMP and OPAL KELLY
#include "events.h"
#include "okFrontPanelDLL.h"
#include "bioamp.h"

//#define XILINX_CONFIGURATION_FILE  "data/bioAmp_1_0_XEM7310.bit"
//#define XILINX_CONFIGURATION_FILE  "data/BioAmp_40usdelay.bit"
#define Saving_base_directory "/Users/Matteo1/Desktop/BioAmp_recordings/"

#define BIAS_FILE "bias.xml"

//Used for matrix2D
int evt_time = 0;
int evt_time_old = 0;

//Used for Plot 2D and Plot 3D
int counter=0;
int dt =10000; //100 is 1 us
// Max resolution is 60 fps -> 16ms -> 16000 us --> dt = 1600000 for RT
int tmp1=0;
int tmp2 =0;
bool play = false;
int over_dt = 1;


// Instantiate BioAmp device
BioAmp dev1(XILINX_CONFIGURATION_FILE);

ofxNeuroSensor::ofxNeuroSensor(){}

void ofxNeuroSensor::setup_biases(string load_file){

    settings.loadFile(load_file);

    Values[0]  =  settings.getValue("bias:VBiasCharge",0);
    Values[1]  = settings.getValue("bias:VpiasP2",0);
    Values[2]  = settings.getValue("bias:VPullDn", 0);
    Values[3]  = settings.getValue("bias:VRef", 0);
    Values[4] =  settings.getValue("bias:ChargeAl", 0);
    Values[5]  =  settings.getValue("bias:Ref_B", 0);
    Values[6]  =  settings.getValue("bias:ReqPDn", 0);
    Values[7]  =  settings.getValue("bias:TailN",0 );
    Values[8]  =  settings.getValue("bias:TailP", 0);
    Values[9]  =  settings.getValue("bias:ThrDn", 0);
    Values[10]  = settings.getValue("bias:ThrUp", 0);
    Values[11] =  settings.getValue("bias:VBn", 0);

}

polarity ofxNeuroSensor::PopulatePolarity( Event2d ev ) {
    
  polarity packetsPolarity;
  //  if (data == 0x1000 || data == 0x1001){

  packetsPolarity.pos.x= ev.x;
  packetsPolarity.pos.y= ev.y;
  packetsPolarity.pol = ev.p;
  packetsPolarity.valid=true; //OK
  packetsPolarity.timestamp = ev.ts;

  return packetsPolarity;
}

polarity ofxNeuroSensor::PopulatePolarity( int data, int timestamp ){
    //populate the polarity vector with the data
    //12 bits --> from left to right: pol-5bit X address-pol-5bitY address

    polarity packetsPolarity;
    if (data == 0x1000 || data == 0x1001){
        // this is the test pixel
        packetsPolarity.pos.x= 35;
        packetsPolarity.pos.y= 35;
        if (data == 0x1001 ){
            packetsPolarity.pol = 1;
        }
        else{
            packetsPolarity.pol = 0;
        }
        packetsPolarity.valid=true; //OK
    }
    else {
        if (( 0x0001 & data ) == ((0x0040 & data)>>6)){
            packetsPolarity.valid=true; //OK
        }
        else{
            packetsPolarity.valid=false; //NOT OK

        }
        if ( 0x0001 & data ){
            packetsPolarity.pol = 1;
        }else{
            packetsPolarity.pol = 0;
        }
        packetsPolarity.pos.x= (0x003E & data) >> 1;
        packetsPolarity.pos.y= (0x0F80 & data) >> 7;
    }
    packetsPolarity.timestamp = (long)timestamp;
    return packetsPolarity;
}




int ofxNeuroSensor::pos(int x){
    int distance_point= 10;
    return x*distance_point;
}

int ofxNeuroSensor::isBigEndian()
{
   unsigned int i = 2;
   char *c = (char*)&i;
   if (*c)
   {
       //cout<<"little endian";
       return 0;
   }
   else
   {
       //cout<<"big endian";
       return 1;
   }
}

long ofxNeuroSensor::convertByteToLong(char *take,int startIndex)
{
    //if machines is liitle endian convert and return as this
    if(isBigEndian())
    {
        //std::cout<<"The processor compute as Big Endian"<<std::endl;
        return (((take[startIndex + 0] & 0xff) << 24) | ((take[startIndex + 1] & 0xff) << 16)
                | ((take[startIndex + 2] & 0xff) << 8) | ((take[startIndex + 3] & 0xff)));
    }
    else
    {
        //cout<<"The processor compute as Little Endian"<<endl;
        return ( ((take[startIndex + 3] & 0xff) << 24) | ((take[startIndex + 2] & 0xff) << 16)
                | ((take[startIndex + 1] & 0xff) << 8) | ((take[startIndex + 0] & 0xff) ));
    }
}

void ofxNeuroSensor::create_vector(){
    packetsPolarity.clear();
    int timestamp, data, filesize, i=0, ppp=0;
    int32_t rawdata;
    std::ifstream listStream1;
    listStream1.open (path, std::ios::in | std::ios::binary);
    if (listStream1.is_open())  {
        //Path chooses from GUI, Filename has to be set on top
        std::cout << "Opened: " << path <<  "\n"<< std::endl;

        listStream1.seekg(0,listStream1.end);
        filesize = double(listStream1.tellg());
        listStream1.seekg(0,listStream1.beg);

        while (listStream1.good()) {
            listStream1.read((char*)&rawdata,4);
           // printf("rawdata 0X%04x ", rawdata);
            timestamp = (0xffffffff & rawdata) ;
            std::cout << "time: " << timestamp << std::endl;
            listStream1.read((char*)&rawdata,4);
          //  printf("rawdata 0X%04x ", rawdata);
            data = (0xffff & rawdata);
            std::cout << "data: " << data << std::endl;


            packetPolarity = PopulatePolarity(data, timestamp);
            packetsPolarity.push_back (packetPolarity);
        //    std::cout <<packetPolarity.timestamp <<std::endl;;
            
            tmp1 =packetsPolarity[0].timestamp;
        }
    }
}


void ofxNeuroSensor::correct_timestamps(){
    //Timestamps go in overflow
    if (packetsPolarity.size()==0){
        return;
    }
    for (int ff =0; ff<packetsPolarity.size()-1; ++ff){
        if (packetsPolarity[ff+1].timestamp  < packetsPolarity[ff].timestamp ){
        //    std::cout << "timestamp ff: " <<std::hex  <<packetsPolarity[ff].timestamp <<std::endl;
         //   std::cout << " ff: " <<std::dec  <<ff <<std::endl;

        //    std::cout << "timestamp ff+1: " << std::hex << packetsPolarity[ff+1].timestamp <<std::endl;
            for ( int x = ff+1; x<packetsPolarity.size(); ++x){
                packetsPolarity[x].timestamp += 0xFFFFFFFF;

            }
       //     std::cout << "timestamp ff+1 after correction: " << packetsPolarity[ff+1].timestamp <<std::endl;


    }
}

}
void ofxNeuroSensor::populate_index_time_bin(){
    //Select which events to plot at this iteration

    if (packetsPolarity.size()==0){
        return;
    }

    if (tmp2 < packetsPolarity[packetsPolarity.size()-1].timestamp){
        // tmp1 =packetsPolarity[0].timestamp-1;
        tmp1 += dt/over_dt;
        tmp2 = tmp1 + dt;
        std::cout << "tmp1: " << tmp1 << "tmp2: " << tmp2 << "size packet"  << packetsPolarity.size()<< std::endl;
        if (index_time_bin.size()){
       //     cout <<"packet size: " << packetsPolarity.size() <<endl;
       //     cout <<"index_time_bin[0]: " << index_time_bin[0] <<endl;
       //     cout <<"index_time_bin size: " << index_time_bin.size() <<endl;
            if (index_time_bin[0]>0){
                packetsPolarity.erase (packetsPolarity.begin(), packetsPolarity.begin()+(index_time_bin[0]));
                packetsPolarity.shrink_to_fit();

            }
        }

        index_time_bin.clear();

        for (int ff =0; ff < (packetsPolarity.size()); ++ff){
            if ( packetsPolarity[ff].timestamp > tmp1 &&  packetsPolarity[ff].timestamp < tmp2 ) {
                index_time_bin.push_back(ff);
            //    cout << "size index_time_bin : " << index_time_bin.size() << endl;
            }
        }
    }
    if (tmp2 > packetsPolarity[packetsPolarity.size()-1].timestamp){
        index_time_bin.clear();

        return;

    }
}

void ofxNeuroSensor::Plot_3D_live(){

    mesh.setMode(OF_PRIMITIVE_POINTS);
    glPointSize(5);

    for(int i=0; i < (packetsPolarity.size());i++) {
        mesh.addVertex(ofPoint(pos(packetsPolarity[i].pos.x),pos(packetsPolarity[i].pos.y),counter));
        if(packetsPolarity[i].pol){
            mesh.addColor(ofColor(0,255,0,50)); //Green means on
        }else{
            mesh.addColor(ofColor(255,0,0,50)); // Red means off
        }

    }
    packetsPolarity.clear();
    packetsPolarity.shrink_to_fit();
    counter++;
}

void ofxNeuroSensor::Plot_3D(){
    //plot in 3D


    populate_index_time_bin();

    mesh.setMode(OF_PRIMITIVE_POINTS);
    glPointSize(5);

    if (index_time_bin.size()==0){
        std::cout <<"Null index_time_bin" << std::endl;
        std::cout <<"size pp"<< packetsPolarity.size() << std::endl;

        counter++;

        return;
    }
    else {
        for(int i=index_time_bin[0]; i < (index_time_bin[0]+index_time_bin.size());i++) {
            mesh.addVertex(ofPoint(pos(packetsPolarity[i].pos.x),pos(packetsPolarity[i].pos.y),counter));
            if(packetsPolarity[i].pol){
                mesh.addColor(ofColor(0,255,0,50)); //Green means on
            }else{
                mesh.addColor(ofColor(255,0,0,50)); // Red means off
            }

        }
        counter++;

    }
}
void ofxNeuroSensor::Plot_2D_live(){
    //plot in 2D
    mesh.clear(); // clear every time
    mesh.setMode(OF_PRIMITIVE_POINTS);
    glPointSize(5);

    for(int i=0; i < (packetsPolarity.size());i++) {
        mesh.addVertex(ofPoint(pos(packetsPolarity[i].pos.x),pos(packetsPolarity[i].pos.y)));
        if(packetsPolarity[i].pol){
            mesh.addColor(ofColor(0,255,0,50)); //Green means on
        }else{
            mesh.addColor(ofColor(255,0,0,50)); // Red means off
        }

    }
    packetsPolarity.clear();
    packetsPolarity.shrink_to_fit();
}

void ofxNeuroSensor::Plot_2D(){
    //plot in 2D
    mesh.clear(); // clear every time
    populate_index_time_bin();
    mesh.setMode(OF_PRIMITIVE_POINTS);
    glPointSize(5);

    if (index_time_bin.size()==0){
        return;

    }
    else {

        for(int i=index_time_bin[0]; i < (index_time_bin[0] + index_time_bin.size()); i++) {

            mesh.addVertex(ofPoint(pos(packetsPolarity[i].pos.x),pos(packetsPolarity[i].pos.y)));

            if(packetsPolarity[i].pol){
                mesh.addColor(ofColor(0,255,0,50));
            }else{
                mesh.addColor(ofColor(255,0,0,50));
            }

        }
    }
}

void my_handler(int a ){
    int done = 1;
}

void ofxNeuroSensor::setup_gui(){
    setup_biases(BIAS_FILE);
    f1 = new ofxDatGuiFolder("Control Panel");
    f1->addSlider("Set Framerate", 0.1, 60, ofGetFrameRate());
    ofSetFrameRate(25); //Just for testing
    f1->addFRM();

    f1->addBreak();

    Slider_VbCharge= f1->addSlider("VbiasCharge", 0.0, 3300,Values[0]);
    Slider_VpiasP2= f1->addSlider("VpiasP2", 0.0, 3300,Values[1]);
    Slider_VPullDn = f1->addSlider("VPullDn", 0.0, 3300,Values[2]);
    Slider_Vref=f1->addSlider("VRef", 0.0, 3300,Values[3]);
    Slider_ChargeAl=f1->addSlider("ChargeAl", 0.0, 3300,Values[4]);
    Slider_RefB= f1->addSlider("Ref_B", 0.0, 3300,Values[5]);
    Slider_ReqPDn=f1->addSlider("ReqPDn", 0.0, 3300,Values[6]);
    Slider_TailN=f1->addSlider("TailN", 0.0, 3300,Values[7]);
    Slider_TailP=f1->addSlider("TailP", 0.0, 3300,Values[8]);
    Slider_ThrDn=   f1->addSlider("ThrDn", 0.0, 3300,Values[9]);
    Slider_ThrUp=   f1->addSlider("ThrUp", 0.0, 3300,Values[10]);
    Slider_VBn=  f1->addSlider("VBn", 0.0, 3300,Values[11]);
    Slider_VBn->setPrecision(0);
    Slider_ThrUp->setPrecision(0);
    Slider_ThrDn->setPrecision(0);
    Slider_TailP->setPrecision(0);
    Slider_TailN->setPrecision(0);
    Slider_RefB->setPrecision(0);
    Slider_ReqPDn->setPrecision(0);
    Slider_ChargeAl->setPrecision(0);
    Slider_Vref->setPrecision(0);
    Slider_VPullDn->setPrecision(0);
    Slider_VpiasP2->setPrecision(0);
    Slider_VbCharge->setPrecision(0);

    f1->addBreak();

    twoD_visualisation = f1->addToggle("2D visualisation", false);
    threeD_visualisation = f1->addToggle("3D visualisation", true);
    f1->addSlider("dt", 100, 400000, dt);
    f1->addSlider("over dt", 1, 25, over_dt);
    f1->addButton("Load Recording");
    f1->addBreak();
    play = f1->addToggle("Play/Stop", false);
    f1->addBreak();
    string saving, saving_events;
    Save_Biases = f1->addTextInput("Save Biases:",saving);
    f1->addButton("Load Biases");
    Save_Events = f1->addTextInput("Save events:",saving_events);
    Save_events = f1->addToggle("Events_saving");
    f1->addBreak();
    AER = f1->addToggle("AER");
    f1->addBreak();
    TP = f1->addToggle("TP");
    f1->addBreak();
    LIVE = f1->addToggle("LIVE", false);
    f1->addBreak();

    f1->onButtonEvent(this, &ofxNeuroSensor::onButtonEvent);
    f1->onToggleEvent(this, &ofxNeuroSensor::onToggleEvent);
    f1->onSliderEvent(this, &ofxNeuroSensor::onSliderEvent);
    f1->onTextInputEvent(this, &ofxNeuroSensor::onTextInputEvent);

}

void ofxNeuroSensor::update_gui(){
    if (play->getChecked()){

        if  (LIVE->getChecked() && FPGA){

            if (twoD_visualisation->getChecked()){
                Plot_2D_live ();
            }
            else if (threeD_visualisation->getChecked()){
                Plot_3D_live();
            }
        }
        else
            if (twoD_visualisation->getChecked()){
                Plot_2D ();
            }
            else if (threeD_visualisation->getChecked()){
                Plot_3D();
            }
    }

    f1->update();
}

void ofxNeuroSensor::setup_bioamp(){


    //catch ctrl-C
    signal (SIGINT,my_handler);

    // load biases from file
    setup_biases(BIAS_FILE);

    dev1.enable_aer(false);
    dev1.enable_tp(false);

    dev1.reset();

    // set Biases to device

    dev1.set_biases(Values);

    // join pooling thread
    dev1.join_thread(&ev_buffer);


}

void ofxNeuroSensor::update_bioamp(){

    start = std::chrono::high_resolution_clock::now();
    // Check input data, emptying buffer
    while(ev_buffer.size() > 0) {
        Event2d ev = ev_buffer.front();
        ev_buffer.pop();
        std::cout << std::dec <<ev.ts   << std::endl;
        packetPolarity = PopulatePolarity(ev);
        packetsPolarity.push_back(packetPolarity);
      }

        end = std::chrono::high_resolution_clock::now();
        int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
                             (end-start).count();
        int runtime = std::chrono::duration_cast<std::chrono::milliseconds>
                             (end-origin).count();
    printf("[%.01fs] Events: %llu \t Overflow: %llu (%d/%f ms) (%.1fkev/s)\n",runtime/1000.,dev1.event_counter-old_ev_cnt,dev1.of_counter-old_of_cnt,elapsed, (1/ofGetFrameRate()), (dev1.event_counter-old_ev_cnt)/(1.*ofGetFrameRate()));
    old_ev_cnt = dev1.event_counter;
    old_of_cnt = dev1.of_counter;
    //(1/ofGetFrameRate()) or dt?
    
    //printf("[%.01fs] Events: %llu \t Overflow: %llu (%d/%d ms)\n",runtime/1000.,dev1.event_counter-old_ev_cnt,dev1.of_counter-old_of_cnt,elapsed, (1/ofGetFrameRate()));



}

void ofxNeuroSensor::setup(){
    origin = std::chrono::high_resolution_clock::now();
    setup_gui();
    if(FPGA){
        setup_bioamp();
    }
}

void ofxNeuroSensor::update(){
    if (FPGA){

        update_bioamp();

    }
    update_gui();
}


void ofxNeuroSensor::draw(){

    myCam.begin();
    ofPushMatrix();

    mesh.draw();
    ofPopMatrix();

    myCam.end();

    f1->draw();

}

void ofxNeuroSensor::onToggleEvent(ofxDatGuiToggleEvent e)
{
    if(e.target->getLabel() == ("2D visualisation")){
        twoD_visualisation->getChecked();
        threeD_visualisation->setChecked(false);
        mesh.clear();
        counter =0;
        tmp1=0;
        tmp2=0;
        create_vector();
        correct_timestamps();
    }
    if(e.target->getLabel() == ("3D visualisation")){
        threeD_visualisation->getChecked();
        twoD_visualisation->setChecked(false);
        mesh.clear();
        counter =0;
        tmp1=0;
        tmp2=0;
        create_vector();
       correct_timestamps();
    }

    /* if(e.target->getLabel() == "Play/Stop"){
     play->getChecked();

     }*/
    if(e.target->getLabel() == "AER"){
        dev1.enable_aer(AER->getChecked());
        dev1.reset();

    }
    if(e.target->getLabel() == "TP"){
        dev1.enable_tp(TP->getChecked());
        dev1.reset();

    }
    if(e.target->getLabel() == "Events_saving"){
        if ( Save_events->getChecked() == 0){
            
            dev1.enable_record(false);
        }
        if ( Save_events->getChecked() == 1){
            
            
            std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
            time_t  tt;
            tt = std::chrono::system_clock::to_time_t(now);
            std::string s = Saving_base_directory +std::to_string(tt)+".dat";
            
            dev1.enable_record(true, s);
            
            cout <<"Saving in: "  <<s << endl;
            
            
        }


    }
}
void ofxNeuroSensor::onButtonEvent(ofxDatGuiButtonEvent e)

{
    if(e.target->getLabel() == "Load Recording"){
        ofFileDialogResult result = ofSystemLoadDialog("Load aedat file");
        if(result.bSuccess) {
            path = result.getPath();

            create_vector();
          //  correct_timestamps();
        }
    }
    if(e.target->getLabel() == "Load Biases"){
        ofFileDialogResult result = ofSystemLoadDialog("Load Bias file");
        if(result.bSuccess) {
            bias_path = result.getPath();
            cout << " " << bias_path <<endl;
            //settings.loadFile(bias_path);

            setup_biases(bias_path);

            Slider_VbCharge->setValue(Values[0]);
            Slider_VpiasP2->setValue(Values[1]);
            Slider_VPullDn->setValue(Values[2]);
            Slider_Vref->setValue(Values[3]);
            Slider_ChargeAl->setValue(Values[4]);
            Slider_RefB->setValue(Values[5]);
            Slider_ReqPDn->setValue(Values[6]);
            Slider_TailN->setValue(Values[7]);
            Slider_TailP->setValue(Values[8]);
            Slider_ThrDn->setValue(Values[9]);
            Slider_ThrUp->setValue(Values[10]);
            Slider_VBn->setValue(Values[11]);

            dev1.set_biases(Values);
        }


    }
}

void ofxNeuroSensor::onSliderEvent(ofxDatGuiSliderEvent e)
{
    if(e.target->getLabel() == "Set Framerate")
    {
        cout << "onSliderEvent speed is : " << e.value << endl;
        ofSetFrameRate(e.value);
    }
    if(e.target->getLabel() == "VbiasCharge" || e.target->getLabel() == "VpiasP2" ||e.target->getLabel() == "VRef" || e.target->getLabel() == "VPullDn" || e.target->getLabel() == "ChargeAl" || e.target->getLabel() == "Ref_B" ||e.target->getLabel() == "ReqPDn" || e.target->getLabel() == "TailN" ||  e.target->getLabel() == "TailP" ||e.target->getLabel() == "ThrDn" || e.target->getLabel() == "ThrUp" ||  e.target->getLabel() == "VBn" ){
        cout << "The new bias for "<< e.target->getLabel() << "  is : " << e.value << endl;

        if(e.target->getLabel() == "VbiasCharge"){
            Values[0] =e.value;
            settings.setValue("bias:VBiasCharge", (int)Values[0]);

        }
        else if(e.target->getLabel() == "VpiasP2"){
            Values[1] =e.value;
            settings.setValue("bias:VpiasP2", (int)Values[1]);

        }
        else if(e.target->getLabel() == "VPullDn"){
            Values[2] =e.value;
            settings.setValue("bias:VPullDn", (int)Values[2]);

        }
        else if(e.target->getLabel() == "VRef"){
            Values[3] =e.value;
            settings.setValue("bias:VRef", (int)Values[3]);

        }
        else if(e.target->getLabel() == "ChargeAl"){
            Values[4] =e.value;
            settings.setValue("bias:ChargeAl", (int)Values[4]);

        }
        else if(e.target->getLabel() == "Ref_B"){
            Values[5] =e.value;
            settings.setValue("bias:Ref_B", (int)Values[5]);

        }
        else if(e.target->getLabel() == "ReqPDn"){
            Values[6] =e.value;
            settings.setValue("bias:ReqPDn",(int) Values[6]);

        }
        else if(e.target->getLabel() == "TailN"){
            Values[7] =e.value;
            settings.setValue("bias:TailN",(int)Values[7]);

        }
        else if(e.target->getLabel() == "TailP"){
            Values[8] =e.value;
            settings.setValue("bias:TailP", (int)Values[8]);

        }
        else if(e.target->getLabel() == "ThrDn"){
            Values[9] =e.value;
            settings.setValue("bias:ThrDn",(int) Values[9]);

        }
        else if(e.target->getLabel() == "ThrUp"){
            Values[10] =e.value;
            settings.setValue("bias:ThrUp",(int) Values[10]);

        }
        else if(e.target->getLabel() == "VBn"){
            Values[11] =e.value;
            settings.setValue("bias:VBn", (int)Values[11]);

        }

        settings.saveFile(BIAS_FILE);
        dev1.set_biases(Values);



    }
    if(e.target->getLabel() == "over dt"){
        over_dt = e.value;
    }
    if(e.target->getLabel() == "dt"){
        dt = e.value;
    }


}
void ofxNeuroSensor::onTextInputEvent(ofxDatGuiTextInputEvent e)
{
    if(e.target->getLabel() == "Save Biases:"){
        cout << e.text << endl;
        settings.setValue("bias:VBiasCharge",(int) Values[0]);
        settings.setValue("bias:VpiasP2",(int) Values[1]);
        settings.setValue("bias:VPullDn",(int) Values[2]);
        settings.setValue("bias:VRef",(int) Values[3]);
        settings.setValue("bias:ChargeAl", (int)Values[4]);
        settings.setValue("bias:Ref_B",(int) Values[5]);
        settings.setValue("bias:ReqPDn",(int) Values[6]);
        settings.setValue("bias:TailN",(int)Values[7]);
        settings.setValue("bias:TailP",(int) Values[8]);
        settings.setValue("bias:ThrDn",(int) Values[9]);
        settings.setValue("bias:ThrUp",(int) Values[10]);
        settings.setValue("bias:VBn",(int) Values[11]);

        settings.saveFile(e.text);
    }
    
    if(e.target->getLabel() == "Save events:"){
        cout << e.text << endl;
        std::string s = Saving_base_directory + e.text + ".dat";
        cout <<"Saving in: "  <<s << endl;
        
        dev1.enable_record(true,s);
        
        Save_events->setChecked(true);
    }

}
