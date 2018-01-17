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

#define XILINX_CONFIGURATION_FILE "data/bioAmp_1_0_XEM7310.bit"

#include "events.h"
#include "bioamp.h"

#define BIAS_FILE "bias.xml"

struct polarity {
    ofPoint pos; //has .x, .y .z
    int timestamp;
    bool pol; // 0 or 1
    bool valid; // true ok
};


class ofxNeuroSensor {
    
public:
    ofxNeuroSensor();
    void setup();
    void update();
    void draw();
    
    
    
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
    void onTextInputEvent(ofxDatGuiTextInputEvent e);
    void onColorPickerEvent(ofxDatGuiColorPickerEvent e);
    
    void update_bioamp();
    
    //Event handling
    polarity packetPolarity;
    vector<polarity> packetsPolarity;
    vector<int> index_time_bin;
    void populate_index_time_bin();
    polarity PopulatePolarity(int data, int timestamp );
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
    
    
    //Germain
    std::queue<EventRaw> ev_buffer;
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
