#pragma once

#ifndef ofxNeuroSensor_h
#define ofxNeuroSensor_h

#include "ofMain.h"
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <string>
#include <math.h>
#include <stdlib.h>
#include <iostream>
#include "ofxXmlSettings.h"
#include "ofxDatGui.h"

// XEM6310
//#define XILINX_CONFIGURATION_FILE "../src/bioamp/bioAmp_1_05.bit"
// XEM7310
//#define XILINX_CONFIGURATION_FILE "data/bioAmp_1_05_XEM7310.bit"
//#define XILINX_CONFIGURATION_FILE "data/Bioamp_1_12_fixed_fsm_timing.bit"
//Bioamp_1_13_200ns_delay_R_A.bit
//#define XILINX_CONFIGURATION_FILE "data/Bioamp_1_13_200ns_delay_R_A.bit"
//#define XILINX_CONFIGURATION_FILE "data/Bioamp_1_14_2us_delay_R_A.bit"
//#define XILINX_CONFIGURATION_FILE "data/Bioamp_1_15_20us_delay_R_A.bit"
//#define XILINX_CONFIGURATION_FILE "data/Bioamp_1_15_added_delay_nRes.bit"
//#define XILINX_CONFIGURATION_FILE "data/Bioamp_1_12_fixed_fsm_timing.bit"
//#define XILINX_CONFIGURATION_FILE "data/Bioamp_1_15_no_nRes.bit"

//#define XILINX_CONFIGURATION_FILE "data/Bioamp_1_16_no_delay_nRes.bit"
#define XILINX_CONFIGURATION_FILE "data/Bioamp_1_17_delay_nRes.bit"

//#define XILINX_CONFIGURATION_FILE "data/Bioamp_1_18_nRes_only_aer_and50ns .bit"
//#define XILINX_CONFIGURATION_FILE "data/Bioamp_1_19_nRes_fixed.bit"
//#define XILINX_CONFIGURATION_FILE "data/Bioamp1_20_nRes_aeronly.bit"

//#define XILINX_CONFIGURATION_FILE "data/Bioamo_1_21_simple.bit"
//#define XILINX_CONFIGURATION_FILE "data/Bioamp_1_22_nRes_aerof.bit"
//#define XILINX_CONFIGURATION_FILE "data/Bioamp_1_22_test.bit"
//#define XILINX_CONFIGURATION_FILE "data/Bioamp_1_23_4state_machine_tp_priority.bit"


#include "events.h"
#include "bioamp.h"


#define BIAS_FILE "bias.xml"

struct polarity {
    ofPoint pos; //has .x, .y .z
    long timestamp;
    bool pol; // 0 or 1
    bool valid; // true ok
};


class ofxNeuroSensor {

public:
    ofxNeuroSensor();
    void setup();
    void update();
    void draw();


    int isBigEndian();
    long convertByteToLong(char *take,int startIndex);
    void onButtonEvent(ofxDatGuiButtonEvent e);
    void onToggleEvent(ofxDatGuiToggleEvent e);
    void onSliderEvent(ofxDatGuiSliderEvent e);

    //event plotting
    int pos(int x);
    void Plot_2D ();
    void Plot_3D();
    void Plot_3D_live();
    void Plot_2D_live();


    void check_device();

    ofxDatGuiTextInput * Save_Biases;
    ofxDatGuiTextInput * Save_Events;
    void onTextInputEvent(ofxDatGuiTextInputEvent e);
    void onColorPickerEvent(ofxDatGuiColorPickerEvent e);

    void update_bioamp();

    //Event handling
    polarity packetPolarity;
    vector<polarity> packetsPolarity;
    vector<int> index_time_bin;
    void populate_index_time_bin();
    polarity PopulatePolarity( int data, int timestamp );
    polarity PopulatePolarity( Event2d ev );
    void correct_timestamps();
    void create_vector ();

    //Graphics
    ofEasyCam myCam;
    ofMesh mesh;
    //ofxPanel gui;
    string path;
    ofxDatGuiFolder* f1;
    bool drawGui;

    void setup_gui();
    void update_gui();
    void setup_bioamp();
    void lock();
    void unlock();

    //Germain
    boost::mutex mtx_;
    std::queue<Event2d> ev_buffer;
    unsigned long long old_ev_cnt = 0;
    unsigned long long old_of_cnt = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> start, end, origin;

    // Bias
    void setup_biases(string bias);
    void biases_save();
    string bias_path;

    ofxDatGuiSlider* Slider_VbCharge;
    ofxDatGuiSlider* Slider_VpiasP2;
    ofxDatGuiSlider* Slider_VPullDn;
    ofxDatGuiSlider* Slider_Vref;
    ofxDatGuiSlider* Slider_ChargeAl;
    ofxDatGuiSlider* Slider_RefB;
    ofxDatGuiSlider* Slider_ReqPDn;
    ofxDatGuiSlider* Slider_TailN;
    ofxDatGuiSlider* Slider_TailP;
    ofxDatGuiSlider* Slider_ThrDn;
    ofxDatGuiSlider* Slider_ThrUp;
    ofxDatGuiSlider* Slider_VBn;

    ofxDatGuiToggle* twoD_visualisation;
    ofxDatGuiToggle* threeD_visualisation;
    ofxDatGuiToggle* play;
    ofxDatGuiToggle* AER;
    ofxDatGuiToggle* TP;
    ofxDatGuiToggle* LIVE;
     ofxDatGuiToggle* Save_events;


    ofxXmlSettings settings;
    ofxXmlSettings bias;

    ofParameter<int> VbiasCharge ;
    ofParameter<int> VpiasP2 ;
    ofParameter<int> VPullDn ;
    ofParameter<int> Vref  ;
    ofParameter<int> ChargeAl ;
    ofParameter<int> RefB;
    ofParameter<int> ReqPDn;
    ofParameter<int> TailN ;
    ofParameter<int> TailP ;
    ofParameter<int> ThrDn ;
    ofParameter<int> ThrUp ;
    ofParameter<int> VBn;

    int Values[12];

};

#endif /* ofxNeuroSensor_h */
