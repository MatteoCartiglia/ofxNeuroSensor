ofxNeuroSensor
================


Introduction
------------

This addon is for openFrameworks and it enable the use of the Dynamic Neural Tissue Sensor. This addon is a basic logger and player for the sensors www.ini.uzh.ch 

OpenFrameworks addon for Neural Bioamplifier chip.

![alt text](https://github.com/federicohyo/ofxNeuroSensor/blob/master/docs/neuroSensor.gif "ofxNeuroSensor")


License
-------
ofxNeuroSensor is distributed under the [MIT License](https://en.wikipedia.org/wiki/MIT_License), and you might consider using this for your repository. 

Installation
------------
Any steps necessary to install your addon. Optimally, this means just dropping the folder into the `openFrameworks/addons/` folder.

Under linux, please go inside the folder addons/ofxNeuroSensor/example_ofxNeuroSensorGui and type 'make'. To launch the application, cd bin/ , ./example_ofxNeuroSensorGui, - or - make RunRelease.

Dependencies
------------
This addons depends on two additions addons; 1) ofxDatGui (https://github.com/braitsch/ofxDatGui.git), 2) ofxXmlSetting (https://github.com/openframeworks/openFrameworks/tree/master/addons/ofxXmlSettings).

It requires the following libraries -lpthread, -ldl, -lokFrontPanel, -lboost_system
The FrontPanel library is distributed by opalKelly and it is available from the libs folder (please install it in your system under /usr/local/lib)

Download
--------
This module uses submodules from git. Please ensure you have all the required files in src/bioamp
Download this addon by using the Download button on the right side of the github page. Unzip, rename and copy it to your addons folder.
OR
Use the git clone command :
git clone -b develop --recursive https://github.com/federicohyo/ofxNeuroSensor.git
**PLEASE DON'T FORK** the addon template repo if you plan on creating your own addon, this will lead to confusion on the Github inheritance/forking graph, and you will unnecessarily have this repository's history in your own git repo.


Compatibility
------------
Works under Linux and Mac os X. It requires openframeworks >= 0.9.6

Known issues
------------

Requires parameters tuning, and patience. Still an alpha version.

Version history
------------

### Version 0.1 (17 January 2017):
First addon version with live recording, bias control, 2D and 3D visualization. Still has some problem with Linux.


