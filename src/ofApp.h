#pragma once
#include "ofxOpenNI.h"
#include "ofMain.h"
#include "fft.h"
#include "ofxOpenCv.h"

class ofApp : public ofBaseApp{
	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

        float getVal(int x, int y);
        void ripple(float x, float y, float depth);
        void rippleFigure(float x, float y, float depth);
    
        int s_num;//指標
    
        int w,h,w2,h2;//Image Size
        int pixels, pixels2;//Number of pixel
    
        ofImage image;//Original image
        ofImage updatedImage;//Updated image
    
        vector<float> odata;//Old pixel
        vector<float> ndata;//New pixel
        vector<float> tempV;//For temp data
        vector<ofColor> cdata;//For color data

        float eps = 5.0;
        float z = 0.2;
        ofVec3f light = ofVec3f(1, 1, 0);
    
        ofShader shader;	//Shader
        ofFbo fbo;			//Buffer for intermediate drawing
    
        //Audio In
        ofSoundStream soundStream;
        void audioIn(float* input, int bufferSize, int nChannels);
        
        float *left, *right;
        float *magnitudeL, *phaseL, *powerL, *tempL;
        float *magnitudeR, *phaseR, *powerR, *tempR;
        float avg_powerL, avg_powerR;
    
        float time0 = 0;
        float Rad = 500;
        float Vel = 0.1;
        int bandRad = 2;
        int bandVel = 100;
    
        int buffer_size;
        int fft_size;
        class fft myfft;
    
    private:
        ofxOpenNI openNIDevice;
    
        void handEvent(ofxOpenNIHandEvent & event);
        void userEvent(ofxOpenNIUserEvent & event);
        
        int numDevices;
    
        float kinectImageRatio;
        ofImage humanImage;
};
