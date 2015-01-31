#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofLog(OF_LOG_VERBOSE);
    
    shader.load("shaderVert.c", "shaderFrag.c");
    fbo.allocate(ofGetWidth(), ofGetHeight());
    
    //Kinect
    openNIDevice.setup();
    openNIDevice.addImageGenerator();
    openNIDevice.addDepthGenerator();
    openNIDevice.setRegister(true);
    openNIDevice.setMirror(true);
    openNIDevice.addUserGenerator();
    openNIDevice.setMaxNumUsers(4);
    openNIDevice.addHandsGenerator();
    openNIDevice.addAllHandFocusGestures();
    openNIDevice.setMaxNumHands(8);
    openNIDevice.setUsePointCloudsAllUsers(true);
    openNIDevice.setPointCloudDrawSizeAllUsers(20);
    openNIDevice.setPointCloudResolutionAllUsers(20);
    openNIDevice.start();
    openNIDevice.getDepthGenerator().GetAlternativeViewPointCap().SetViewPoint(openNIDevice.getImageGenerator());
    
    // I don't know why this is NOT working
    //w = openNIDevice.getWidth();
    //h = openNIDevice.getHeight();
    w = 640;
    h = 480;
    pixels = w*h;
    w2 = w/2;
    h2 = h/2;
    pixels2 = w2*h2;
    updatedImage.allocate(w, h, OF_IMAGE_COLOR);

    light.normalize();
    
    tempV.resize(pixels2);
    odata.resize(pixels2);
    ndata.resize(pixels2);
    cdata.resize(pixels2);
    
    for (int i=0; i<pixels2; i++) {
        tempV[i] = 0.0;
        odata[i] = 0.0;
        ndata[i] = 0.0;
    }
    
    //Images
    humanImage.allocate(w, h, OF_IMAGE_COLOR_ALPHA);
    
    //FFT new
    fft_size = 2048;
    buffer_size = fft_size * 2;
    
    left = new float[buffer_size];
    magnitudeL = new float[fft_size];
    phaseL = new float[fft_size];
    powerL = new float[fft_size];
    
    right = new float[buffer_size];
    magnitudeR = new float[fft_size];
    phaseR = new float[fft_size];
    powerR = new float[fft_size];
    
    tempL = new float[fft_size];
    tempR = new float[fft_size];
    
    ofSoundStreamSetup(0, 2, this, 44100, buffer_size, 4);
    
    for (int i = 0; i < fft_size; i++) {
        tempL[i] = 0.0f;
        tempR[i] = 0.0f;
    }
}

//--------------------------------------------------------------
void ofApp::update(){
    int ms = ofGetElapsedTimeMillis() % 1000;

    //Audio In
    avg_powerL = 0.0;
    avg_powerR = 0.0;
    myfft.powerSpectrum(0, fft_size, left, buffer_size,	magnitudeL, phaseL, powerL, &avg_powerL);
    myfft.powerSpectrum(0, fft_size, right, buffer_size, magnitudeR, phaseR, powerR, &avg_powerR);
    
    for (int i = 0; i < fft_size; i++) {
        tempL[i] *= 0.97f;
        tempR[i] *= 0.97f;
        tempL[i] = max(magnitudeL[i], tempL[i]);
        tempR[i] = max(magnitudeR[i], tempR[i]);
    }
    
    float time = ofGetElapsedTimef();
    float dt = time - time0;
    dt = ofClamp( dt, 0.0, 0.1 );
    time0 = time;
    
    Rad = ofMap(tempL[ bandRad ], 1, 3, 400, 800, true);// Like getting "Kick"
    Vel = ofMap(tempR[ bandVel ], 0, 0.1, 0.05, 0.5);// Like getting "Snare"

    openNIDevice.update();
    
    float minDis = 500.0f;
    float maxDis = 1000.0f;
    
    if (openNIDevice.isNewFrame() == true) {
        // Sound --------------------------------------------------------------
        float rippleH = (float)h / (float)fft_size;
        int numFlag = true;
        if(ms%10==0){
            for (int i = 0; i < fft_size; i+=10) {
                if(numFlag==true){
                    numFlag=false;
                    ripple(11+tempL[i]*2.0f, h2-rippleH*(float)i, tempL[i]/10.0f);
                    ripple(w-10-tempR[i]*2.0f, h2-rippleH*(float)i, tempR[i]/10.0f);
                }else{
                    numFlag=true;
                    ripple(11+tempL[i]*2.0f, h2+rippleH*(float)i, tempL[i]/10.0f);
                    ripple(w-10-tempR[i]*2.0f, h2+rippleH*(float)i, tempR[i]/10.0f);
                }
                //printf("rippleH*(float)i: %f, tempR: %f \n", rippleH*(float)i, tempR[i]);
            }
        }
        
        // Figure --------------------------------------------------------------
        ofPixels openNIDevicePixel = openNIDevice.getImagePixels();
        unsigned short *depthData = openNIDevice.getDepthRawPixels().getPixels();
        
        bool detectFigure = false;
        int currentRow = 0;
        
        //Mask
        for (int k = 0; k < pixels; k++) {
            //printf("k: %i, humanPixel[k * 4 + 0]: %i, openNIDevicePixel[k * 3 + 0]: %i \n", k, humanPixel[k * 4 + 0], openNIDevicePixel[k * 3 + 0]);
            int x = k % w;
            int y = k / w;
            
            ofColor c = openNIDevicePixel.getColor(x, y);
            
            //If there in NEW row, try to get edge again.
            if (currentRow != y) {
                detectFigure = true;
            }
            
            if (minDis <= depthData[k] && depthData[k] < maxDis){
                c.set(c.r, c.g, c.b, 255);
                humanImage.setColor(x, y, c);
                
                //Get Edge
                if (detectFigure == true) {
                    detectFigure = false;
                    int targetPix = ((int)x/2) + (w2 * ((int)y/2));
                    float depthVal = ofMap(depthData[k], minDis, maxDis, 10.0f, 50.0f);
                    depthVal=50.0f-depthVal;
                    odata[targetPix] = depthVal;
                }
            }else{
                c.set(0, 0, 0, 120);
                humanImage.setColor(x, y, c);
                                
                //Get Edge
                if (detectFigure == false) {
                    detectFigure = true;
                    int targetPix = ((int)x/2-1) + (w2 * ((int)y/2));
                    float depthVal = ofMap(depthData[k-1], minDis, maxDis, 10.0f, 50.0f);
                    depthVal=50.0f-depthVal;
                    odata[targetPix] = depthVal;
                }
            }
            currentRow = y;
        }
        
        humanImage.update();
        
        //--------------------------------------------------------------
        for (int i=0; i<pixels2; i++) {
            tempV[i] = odata[i];
        }
        for (int i=0; i<pixels2; i++) {
            odata[i] = ndata[i];
        }
        for (int i=0; i<pixels2; i++) {
            ndata[i] = tempV[i];
        }
        
        for (int i=0; i<pixels2; i++) {
            int x = i % w2;
            int y = i / w2;
            
            int threathold = 1;
            if (x > threathold && y > threathold && x < w2 - threathold && y < h2 - threathold){
                int aPos = (x-1)+y*w2;
                int bPos = (x+1)+y*w2;
                int cPos = x+(y-1)*w2;
                int dPos = x+(y+1)*w2;
                if(aPos > pixels2) aPos = pixels2;
                if(aPos < 0) aPos = 0;
                if(bPos > pixels2) bPos = pixels2;
                if(bPos < 0) aPos = 0;
                if(cPos > pixels2) cPos = pixels2;
                if(cPos < 0) aPos = 0;
                if(dPos > pixels2) dPos = pixels2;
                if(dPos < 0) aPos = 0;
                
                float val = (odata[(x-1)+y*w2] + odata[(x+1)+y*w2] + odata[x+(y-1)*w2] + odata[x+(y+1)*w2]) / 2;
                
                if (val > 6.0f) {//STOPPER
                    //printf("val: %f \n", val);
                    val = 6.0f;
                }
                
                val = val - ndata[x+y*w2];
                if(val > 100.0f || val < -100.0f){
                    val *= 0.2f;
                }else{
                    val *= 0.85f;
                    //val *= 0.96875;
                }
                ndata[x+y*w2] = val;
            }
        }
        
        //--------------------------------------------------------------
        //Progress
        for (int i=0; i<pixels2; i++) {
            int x = i % w2;
            int y = i / w2;
            
            ofVec3f n = ofVec3f(getVal(x - eps, y) - getVal(x + eps, y), getVal(x, y - eps) - getVal(x, y + eps), eps * 2.0);
            
            n.normalize();
            
            float spec = (1 - (light.x + n.x)) + (1 - (light.y + n.y));
            spec /= 2.0f;
            if (spec > z){
                spec = (spec - z) / (1 - z);
            }else{
                spec = 0.0f;
            }
            spec *= 255.0f;
            
            int xPos = x*2 + n.x * 60;
            int yPos = y*2 + n.y * 60;
            if(xPos > openNIDevice.getWidth()) xPos = openNIDevice.getWidth() - 1;
            if(xPos < 0) xPos = 1;
            if(yPos > openNIDevice.getHeight()) yPos = openNIDevice.getHeight() - 1;
            if(yPos < 0) yPos = 1;
            
            ofColor c = openNIDevicePixel.getColor(xPos, yPos);
            c += spec;
            cdata[i] = c;
        }
        
        //Integrate
        for (int i = 0; i < w; i++){
            for (int j = 0; j < h; j++){
                int ii = (float)i/(float)2;
                int jj = (float)j/(float)2;
                
                if(ii * jj <= pixels2){
                    updatedImage.setColor(i, j, cdata[ii + w2*jj]);
                }
            }
        }
        updatedImage.update();
        
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    //printf("Framerate: %f \n", ofGetFrameRate());
    ofBackgroundGradient( ofColor( 255 ), ofColor( 128 ) );

    ofEnableBlendMode(OF_BLENDMODE_ALPHA);

    ofSetColor(255, 255, 255);
    updatedImage.draw(ofGetWidth()/2-w/2, ofGetHeight()/2-h/2);
    humanImage.draw(ofGetWidth()/2-w/2, ofGetHeight()/2-h/2);
    
    float h = (float)ofGetHeight()/(float)fft_size;
    ofColor colorL,colorR;
    for (int i=0; i<fft_size; i++) {
        if ( i == bandRad || i == bandVel ) {
            colorL = ofColor(0, 0, 0);
            colorR = ofColor(0, 0, 0);
        }else {
            colorL = ofColor(200, 0, 0);
            colorR = ofColor(0, 200, 0);
        }
        ofSetColor(colorL);
        ofRect(ofGetWidth()/4 + ofGetWidth()/2, h*i, -tempL[i]*3.0, h);
        ofSetColor(colorR);
        ofRect(ofGetWidth()/4 + ofGetWidth()/2, h*i, tempR[i]*3.0, h);
    }
}

//--------------------------------------------------------------
float ofApp::getVal(int x, int y){
    int threathold = 1;
    if (x > threathold && y > threathold && x < w2 - threathold && y < h2 - threathold){
        if(x+y*w2 > pixels2){
            //printf("x: %i, y: %i, e?: %d \n", x,y,x+y*w2);
            return 0;
        }else{
            float a = odata[x+y*w2];
            return a;
        }
    }else{
        return 0;
    }

}

//--------------------------------------------------------------
void ofApp::audioIn(float * input, int bufferSize, int nChannels){
    //printf("bufferSize: %i \n", bufferSize);
    //bufferSize = 4096
    
    for (int i = 0; i < bufferSize; i++){
        //printf("left: %f, %f \n", input[i],input[i*2]);
        
        left[i]		= input[i*2];
        right[i]	= input[i*2+1];
    }
}

//--------------------------------------------------------------
void ofApp::ripple(float handX, float handY, float depth){
    if (handX < w - 5 && handX > 10 && handY < h - 5 && handY > 10) {
        //printf("handX: %f, handY: %f, depth: %f \n", handX, handY, depth);

        for (int x = -5; x < 5; x++) {
            for (int y = -5; y < 5; y++) {
                int targetPix = ((int)handX/2 + x) + (w2 * ((int)handY/2 + y));
                //printf("x: %i, y: %i, targetPix: %i \n", x, y, targetPix);
                
                odata[targetPix] = depth;
            }
        }
    }
    //printf("handX: %f, handY: %f, depth: %f \n", handX, handY, depth);
}

//--------------------------------------------------------------
void ofApp::rippleFigure(float handX, float handY, float depth){
    if (handX < w - 5 && handX > 10 && handY < h - 5 && handY > 10) {
        //printf("handX: %f, handY: %f, depth: %f \n", handX, handY, depth);
        
        for (int x = -5; x < 5; x++) {
            for (int y = -5; y < 5; y++) {
                int targetPix = ((int)handX/2 + x) + (w2 * ((int)handY/2 + y));
                //printf("x: %i, y: %i, targetPix: %i \n", x, y, targetPix);
                
                odata[targetPix] = depth;
            }
        }
    }
    //printf("handX: %f, handY: %f, depth: %f \n", handX, handY, depth);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    //Full Screen
    if(key == 'f'){
        ofToggleFullscreen();
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    /*//Make ripple
    for (int x = -10; x < 10; x++) {
        for (int y = -10; y < 10; y++) {
            int targetPix = (mouseX/2 + x) + (w/2 * (mouseY/2 + y));
            odata[targetPix] = 5.0;
        }
    }*/
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    //Make ripple
    for (int x = -10; x < 10; x++) {
        for (int y = -10; y < 10; y++) {
            int targetPix = (mouseX/2 + x) + (w/2 * (mouseY/2 + y));
            odata[targetPix] = 5.0;
        }
    }
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
}
