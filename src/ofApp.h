#pragma once

#include "ofMain.h"
#include  "ofxAssimpModelLoader.h"
#include "box.h"
#include "ray.h"
#include "Kdtree.h"
#include <glm/gtx/intersect.hpp>
#include "ofxGui.h"
#include "ParticleEmitter.h"
#include "ParticleSystem.h"


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
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		void drawAxis(ofVec3f);
		void initLightingAndMaterials();
		void savePicture();
		void toggleWireframeMode();
		void togglePointsDisplay();
		void toggleSelectTerrain();
		void setCameraTarget();
		glm::vec3 getMousePointOnPlane();
		void drawBox(const Box &box);
		Box meshBounds(const ofMesh &);
		

		ofEasyCam cam;
		ofCamera top;
		ofCamera *theCam;
		ofxAssimpModelLoader terrain, lander;
		ofLight light;
		Box boundingBox;
		Box landerBounds;
	
		bool bAltKeyDown;
		bool bCtrlKeyDown;
		bool bWireframe;
		bool bDisplayPoints;
		bool bPointSelected;
		
		bool bLanderLoaded = false;
		bool bTerrainSelected;
		bool bLanderSelected = false;
	
		ofVec3f selectedPoint;
		ofVec3f intersectPoint;
		
		glm::vec3 mouseDownPos;

		ofxPanel gui;
		ofxIntSlider levels;
		bool bHide;

		const float selectionRange = 4.0;

		KdTree kdtree;

		ParticleEmitter landerEmitter;
		ParticleEmitter diskEmitter;
		ThrustForce* thrustForce;
		TurbulenceForce* turbulenceForce;
		float landerScale;
		void calculateHeadingVector();
		ofVec3f headingVector;
		float angle;

		float agl;
		TreeNode nodeRtn;
		float calculateAGL(Vector3 v);
		ofTrueTypeFont myFont;

		bool terrainCollision(int distance);
		bool bBelowGround;
		TreeNode landerTerrainIntersectionPoint;
};
