
//--------------------------------------------------------------
//
//  CS134 - Game Development
//
//  3D Model Drag and Drop and Ray Tracing Selection - startup scene
// 
//  This is an openFrameworks 3D scene that includes an EasyCam
//  and example 3D geometry which I have modelled in Houdini that
//  represents lunar terrain.
//
//  You will use this source file (and include file) as a starting point
// 
//  Please do not modify any of the keymappings.  I would like 
//  the input interface to be the same for each student's 
//  work.  Please also add your name/date below.
//
//  Please document/comment all of your work !
//  Have Fun !!
//
//  Kevin Smith   10-20-19
//
//  Student Name:   < Your Name goes Here >
//  Date: <date of last version>


#include "ofApp.h"
#include "Util.h"



//--------------------------------------------------------------
// setup scene, lighting, state and load geometry
//
void ofApp::setup(){

	bWireframe = false;
	bDisplayPoints = false;
	bAltKeyDown = false;
	bCtrlKeyDown = false;
	bLanderLoaded = false;
	bTerrainSelected = true;
//	ofSetWindowShape(1024, 768);
	cam.setDistance(10);
	cam.setNearClip(.1);
	cam.setFov(65.5);   // approx equivalent to 28mm in 35mm format
	ofSetVerticalSync(true);
	cam.disableMouseInput();
	ofEnableSmoothing();
	ofEnableDepthTest();

	top.setPosition(0, 25, 0);
	top.lookAt(glm::vec3(0, 0, 0));
	top.setNearClip(.1);
	top.setFov(65.5);   // approx equivalent to 28mm in 35mm format

	theCam = &cam;

	// setup rudimentary lighting 
	//
	initLightingAndMaterials();

	//terrain.loadModel("geo/moon-houdini.obj");
	terrain.loadModel("geo/mars-5k.obj");
	terrain.setScaleNormalization(false);

	boundingBox = meshBounds(terrain.getMesh(0));

	kdtree.bMedian = false;

	
	kdtree.create(terrain.getMesh(0), 12);

	// load lander model
	//
	if (lander.loadModel("geo/lander.obj")) {
		lander.setScaleNormalization(false);
		landerScale = 0.5;
		lander.setScale(landerScale, landerScale, landerScale);
		lander.setPosition(0, 0, 0);
		bLanderLoaded = true;
	}
	else {
		cout << "Error: Can't load model" << "geo/lander.obj" << endl;
		ofExit(0);
	}

	// setup LEM
	//
	landerEmitter = ParticleEmitter(new ParticleSystem());
	landerEmitter.setLifespan(-1);
	landerEmitter.setVelocity(ofVec3f(0, 0, 0));
	landerEmitter.setPosition(ofVec3f(0, 0, 0));
	landerEmitter.setParticleRadius(0.02);
	landerEmitter.visible = false;
	landerEmitter.start();
	landerEmitter.oneShot = true;

	diskEmitter = ParticleEmitter(new ParticleSystem());
	diskEmitter.setLifespan(.5);
	diskEmitter.setRate(15);
	diskEmitter.setEmitterType(DiscEmitter);
	diskEmitter.setPosition(ofVec3f(0, 0, 0));
	diskEmitter.setVelocity(ofVec3f(0, -10, 0));
	diskEmitter.groupSize = 20;
	diskEmitter.setParticleRadius(0.05);
	diskEmitter.radius = .2;
	diskEmitter.oneShot = true;

	//setup initial Forces
	thrustForce = new ThrustForce(ofVec3f(0, 0, 0));
	landerEmitter.sys->addForce(thrustForce);
	//turbulenceForce = new TurbulenceForce(ofVec3f(-1, -0.2, -1), ofVec3f(1, 0.2, 1));
	//landerEmitter.sys->addForce(turbulenceForce);

	gui.setup();
	gui.add(levels.setup("Levels", 1, 0, 40));
	bHide = true;

	//UI stuff
	agl = 0;
	if (!myFont.load("fonts/slkscr.ttf", 10))
		ofLogFatalError("can't load font: fonts/slkscr.ttf");
}

//--------------------------------------------------------------
// incrementally update scene (animation)
//
void ofApp::update() {

	//Find direction that lander is facing and create heading vector for horizontal movement
	calculateHeadingVector();
	
	//update forces 
	landerEmitter.sys->forces[0] = thrustForce;
	//landerEmitter.sys->forces[1] = turbulenceForce;
	landerEmitter.update();

	//Get height of lander
	agl = calculateAGL(Vector3(landerEmitter.sys->particles[0].position.x,
	landerEmitter.sys->particles[0].position.y, landerEmitter.sys->particles[0].position.z));
	
	//check terrain collision
	bBelowGround = terrainCollision(agl);
	if (bBelowGround) {
		landerEmitter.sys->particles[0].position = ofVec3f(landerEmitter.sys->particles[0].position.x,
			terrain.getMesh(0).getVertex(landerTerrainIntersectionPoint.points[0]).y, landerEmitter.sys->particles[0].position.z);
	}

	//Move lander according to particle position
	lander.setPosition(landerEmitter.sys->particles[0].position.x,
		landerEmitter.sys->particles[0].position.y, landerEmitter.sys->particles[0].position.z);

	//Move DiskEmitter to lander position
	diskEmitter.setPosition(ofVec3f(lander.getPosition().x, lander.getPosition().y + .3, lander.getPosition().z));
	diskEmitter.update();
}
//--------------------------------------------------------------
void ofApp::draw(){
	
	ofEnableDepthTest();
	ofBackground(ofColor::black);

	theCam->begin();

	ofPushMatrix();
	if (bWireframe) {                    // wireframe mode  (include axis)
		ofDisableLighting();
		ofSetColor(ofColor::slateGray);
		terrain.drawWireframe();
		if (bLanderLoaded) {
			//  Note this is how to rotate LEM modlander.setRotation(0, angle, 0, 1, 0);el about the "Y" or "UP" axis
			lander.setRotation(0, angle, 0, 1, 0);
			lander.drawWireframe();
			if (!bTerrainSelected) drawAxis(lander.getPosition());
		}
		if (bTerrainSelected) drawAxis(ofVec3f(0, 0, 0));
	}
	else {
		ofEnableLighting();              // shaded mode
		terrain.drawFaces();

		if (bLanderLoaded) {
			//  Note this is how to rotate LEM modlander.setRotation(0, angle, 0, 1, 0);el about the "Y" or "UP" axis
			lander.setRotation(0, angle, 0, 1, 0);
			lander.drawFaces();
			if (!bTerrainSelected) drawAxis(lander.getPosition());

			
			ofVec3f min = lander.getSceneMin(true) + lander.getPosition();
			ofVec3f max = lander.getSceneMax(true) + lander.getPosition();

			Box bounds = Box((Vector3(min.x, min.y, min.z)), Vector3(max.x, max.y, max.z));

			landerBounds = bounds;

			if (bLanderSelected) ofSetColor(ofColor::red);
			else ofSetColor(ofColor::white);

			drawBox(bounds);
		}
		if (bTerrainSelected) drawAxis(ofVec3f(0, 0, 0));
	}


	if (bDisplayPoints) {                // display points as an option    
		glPointSize(3);
		ofSetColor(ofColor::green);
		terrain.drawVertices();
	}

	
	ofNoFill();
	ofSetColor(ofColor::white);
	//drawBox(boundingBox);


	kdtree.draw(kdtree.root,levels,0);

	// highlight selected point (draw sphere around selected point)
	//
	if (bPointSelected) {
		ofSetColor(ofColor::blue);
		ofDrawSphere(selectedPoint, .1);
	}

	diskEmitter.draw();
	ofPopMatrix();

	theCam->end();
	ofDisableDepthTest();

	if (!bHide) {
		gui.draw();
	}

	myFont.drawString("Altitude (AGL): " + to_string(agl), 10 , 20);
	

}

// 

// Draw an XYZ axis in RGB at world (0,0,0) for reference.
//
void ofApp::drawAxis(ofVec3f location) {

	ofPushMatrix();
	ofTranslate(location);

	ofSetLineWidth(1.0);

	// X Axis
	ofSetColor(ofColor(255, 0, 0));
	ofDrawLine(ofPoint(0, 0, 0), ofPoint(1, 0, 0));
	

	// Y Axis
	ofSetColor(ofColor(0, 255, 0));
	ofDrawLine(ofPoint(0, 0, 0), ofPoint(0, 1, 0));

	// Z Axis
	ofSetColor(ofColor(0, 0, 255));
	ofDrawLine(ofPoint(0, 0, 0), ofPoint(0, 0, 1));

	ofPopMatrix();
}

void ofApp::calculateHeadingVector() {
	//(270 - angle) because lander is traveling on xz plane instead of xy plane. Unit circle is now clockwise
	headingVector = ofVec3f(glm::cos(glm::radians(270 - angle)), 0, (glm::sin(glm::radians(270 - angle))));
}

float ofApp::calculateAGL(Vector3 v) {
	Ray ray = Ray(v, Vector3(0, -1, 0)); //Is this right?
	float distance = agl;

	if (kdtree.intersect(ray, kdtree.root, landerTerrainIntersectionPoint)) {
		distance = v.y() - terrain.getMesh(0).getVertex(landerTerrainIntersectionPoint.points[0]).y;
		/*selectedPoint = terrain.getMesh(0).getVertex(nodeRtn.points[0]);
		bPointSelected = true;*/


		return distance * 100;
	}
	else {
		return agl;
	}
}

bool ofApp::terrainCollision(int distance) {

	if (distance <= 0) {
		cout << "below" << endl;
		return true;
	}
	else {
		return false;
	}
}

void ofApp::keyPressed(int key) {

	switch (key) {
	case 'C':
	case 'c':
		if (cam.getMouseInputEnabled()) cam.disableMouseInput();
		else cam.enableMouseInput();
		break;
	case 'F':
	case 'f':
		ofToggleFullscreen();
		break;
	case 'H':
	case 'h':
		bHide = !bHide;
		break;
	case 'r':
		cam.reset();
		break;
		break;
	case 't':
		setCameraTarget();
		break;
	case 'u':
		toggleWireframeMode();
		break;
	case 'v':
		togglePointsDisplay();
		break;
	case 'p':
		savePicture();
		break;
	case 'V':
		break;
	case 'd':
		angle -= 1;  // rotate spacecraft clockwise (about Y (UP) axis)
		break;
	case 'a':
		angle += 1;  // rotate spacecraft counter-clockwise (about Y (UP) axis)
		break;
	case 'w':
		thrustForce->set(ofVec3f(0, 1, 0)); // spacecraft thrust UP
		diskEmitter.loopedFire = true;
		break;
	case 's':
		thrustForce->set(ofVec3f(0, -1, 0));// spacefraft thrust DOWN
		diskEmitter.loopedFire = true;

	case OF_KEY_ALT:
		cam.enableMouseInput();
		bAltKeyDown = true;
		break;
	case OF_KEY_CONTROL:
		bCtrlKeyDown = true;
		break;
	case OF_KEY_SHIFT:
		break;
	case OF_KEY_DEL:
		break;
	case OF_KEY_F1:
		theCam = &cam;
		break;
	case OF_KEY_F2:
		theCam = &top;
		break;
	case OF_KEY_UP:
		thrustForce->set(headingVector); // move forward
		diskEmitter.loopedFire = true;
		break;
	case OF_KEY_DOWN:
		if (!bBelowGround) {
			thrustForce->set(-headingVector);// move backward
			diskEmitter.loopedFire = true;
		}
		break;
	case OF_KEY_LEFT:
		//switch x and z components of heading vector, and make thrust z component negative 
		//to create thrust in negative x direction 
		thrustForce->set(ofVec3f(headingVector.z, headingVector.y, -headingVector.x));// move left
		diskEmitter.loopedFire = true;
		break;
	case OF_KEY_RIGHT:
		// move right
		//switch x and z components of heading vector, and make thrust x component negative 
		//to create velocity in positive x direction
		thrustForce->set(ofVec3f(-headingVector.z, headingVector.y, headingVector.x));// move left
		diskEmitter.loopedFire = true;
		break;
	default:
		break;
	}
}

void ofApp::toggleWireframeMode() {
	bWireframe = !bWireframe;
}

void ofApp::toggleSelectTerrain() {
	bTerrainSelected = !bTerrainSelected;
}

void ofApp::togglePointsDisplay() {
	bDisplayPoints = !bDisplayPoints;
}

void ofApp::keyReleased(int key) {

	switch (key) {
	
	case OF_KEY_ALT:
		cam.disableMouseInput();
		bAltKeyDown = false;
		break;
	case OF_KEY_CONTROL:
		bCtrlKeyDown = false;
		break;
	case OF_KEY_SHIFT:
		break;
	case 'w':
		thrustForce->set(ofVec3f(0, 0, 0)); // spacecraft stop thrust UP
		diskEmitter.loopedFire = false;
		break;
	case 's':
		thrustForce->set(ofVec3f(0, 0, 0));// spacecraft stop thrust DOWN
		diskEmitter.loopedFire = false;
		break;
	case OF_KEY_UP:
		thrustForce->set(ofVec3f(0, 0, 0));// spacecraft forward stop
		diskEmitter.loopedFire = false;
		break;
	case OF_KEY_DOWN:
		thrustForce->set(ofVec3f(0, 0, 0)); //spacecraft backward stop
		diskEmitter.loopedFire = false;
		break;
	case OF_KEY_RIGHT:
		thrustForce->set(ofVec3f(0, 0, 0)); //spacecraft right stop
		diskEmitter.loopedFire = false;
		break;
	case OF_KEY_LEFT:
		thrustForce->set(ofVec3f(0, 0, 0)); //spacecraft left stop
		diskEmitter.loopedFire = false;
		break;
	default:
		break;

	}
}



//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

	

}


//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

	// if moving camera, don't allow mouse interaction
	//
	if (cam.getMouseInputEnabled()) return;

	glm::vec3 p = theCam->screenToWorld(glm::vec3(mouseX, mouseY, 0));
	mouseDownPos = p;
	//creates ray
	glm::vec3 rayDir = glm::normalize(p - theCam->getPosition());


	//compute bounds for lander
	glm::vec3 min = lander.getSceneMin() + lander.getPosition();
	glm::vec3 max = lander.getSceneMax() + lander.getPosition();
	Box bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));

	//test intersection with ray
	if (bounds.intersect(Ray(Vector3(p.x, p.y, p.z), Vector3(rayDir.x, rayDir.y, rayDir.z)), 0, 1000)) {
		bLanderSelected = true;
	}
	else {
		bLanderSelected = false;
	}
	
	//test intersection of ray and terrain mesh
	TreeNode selectedNode;

	auto start = chrono::high_resolution_clock::now();

	if (kdtree.intersect(Ray(Vector3(p.x, p.y, p.z), Vector3(rayDir.x, rayDir.y, rayDir.z)), kdtree.root, selectedNode)) {
		selectedPoint = terrain.getMesh(0).getVertex(selectedNode.points[0]);
		bPointSelected = true;
		cout << "Node Selected" << endl;
		cout << "Children in node: " << selectedNode.children.size() << endl;
		cout << "Points in node: " << selectedNode.points.size() << endl;
	}
	else { cout << selectedNode.points.size() << endl; }

	auto stop = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
	cout << "Time taken by intersect function: "
		<< duration.count() << " milliseconds" << endl;



}


//draw a box from a "Box" class  
//
void ofApp::drawBox(const Box &box) {
	Vector3 min = box.parameters[0];
	Vector3 max = box.parameters[1];
	Vector3 size = max - min;
	Vector3 center = size / 2 + min;
	ofVec3f p = ofVec3f(center.x(), center.y(), center.z());
	float w = size.x();
	float h = size.y();
	float d = size.z();
	ofDrawBox(p, w, h, d);
}

// return a Mesh Bounding Box for the entire Mesh
//
Box ofApp::meshBounds(const ofMesh & mesh) {
	int n = mesh.getNumVertices();
	ofVec3f v = mesh.getVertex(0);
	ofVec3f max = v;
	ofVec3f min = v;
	for (int i = 1; i < n; i++) {
		ofVec3f v = mesh.getVertex(i);

		if (v.x > max.x) max.x = v.x;
		else if (v.x < min.x) min.x = v.x;

		if (v.y > max.y) max.y = v.y;
		else if (v.y < min.y) min.y = v.y;

		if (v.z > max.z) max.z = v.z;
		else if (v.z < min.z) min.z = v.z;
	}
	return Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
}



//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
	
	/*if (bLanderSelected) {

		glm::vec3 origin = theCam->getPosition();
		glm::vec3 camAxis = theCam->getZAxis();
		glm::vec3 mouseWorld = theCam->screenToWorld(glm::vec3(mouseX, mouseY, 0));
		glm::vec3 mouseDir = glm::normalize(mouseWorld - origin);
		float distance;

		glm::intersectRayPlane(origin, mouseDir, glm::vec3(lander.getPosition().x, lander.getPosition().y, lander.getPosition().z), camAxis, distance);

		glm::vec3 intersectPoint = origin + distance * mouseDir;

		lander.setPosition(intersectPoint.x, intersectPoint.y, intersectPoint.z);
	}*/
}
	// 
	//  implement your code here to drag the lander around



//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
	bLanderSelected = false;
	bPointSelected = false;
}



// Set the camera to use the selected point as it's new target
//  
void ofApp::setCameraTarget() {

}


//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}



//--------------------------------------------------------------
// setup basic ambient lighting in GL  (for now, enable just 1 light)
//
void ofApp::initLightingAndMaterials() {

	static float ambient[] =
	{ .5f, .5f, .5, 1.0f };
	static float diffuse[] =
	{ 1.0f, 1.0f, 1.0f, 1.0f };

	static float position[] =
	{5.0, 5.0, 5.0, 0.0 };

	static float lmodel_ambient[] =
	{ 1.0f, 1.0f, 1.0f, 1.0f };

	static float lmodel_twoside[] =
	{ GL_TRUE };


	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, position);

	glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, position);


	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
	glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
//	glEnable(GL_LIGHT1);
	glShadeModel(GL_SMOOTH);
} 

void ofApp::savePicture() {
	ofImage picture;
	picture.grabScreen(0, 0, ofGetWidth(), ofGetHeight());
	picture.save("screenshot.png");
	cout << "picture saved" << endl;
}

//--------------------------------------------------------------
//
// support drag-and-drop of model (.obj) file loading.  when
// model is dropped in viewport, place origin under cursor
//
void ofApp::dragEvent(ofDragInfo dragInfo) {
	if (lander.loadModel(dragInfo.files[0])) {
		bLanderLoaded = true;
		lander.setScaleNormalization(false);
	//	lander.setScale(.5, .5, .5);
		lander.setPosition(0, 0, 0);
//		lander.setRotation(1, 180, 1, 0, 0);

		// We want to drag and drop a 3D object in space so that the model appears 
		// under the mouse pointer where you drop it !
		//
		// Our strategy: intersect a plane parallel to the camera plane where the mouse drops the model
		// once we find the point of intersection, we can position the lander/lander
		// at that location.
		//

		// Setup our rays
		//
		glm::vec3 origin = theCam->getPosition();
		glm::vec3 camAxis = theCam->getZAxis();
		glm::vec3 mouseWorld = theCam->screenToWorld(glm::vec3(mouseX, mouseY, 0));
		glm::vec3 mouseDir = glm::normalize(mouseWorld - origin);
		float distance;

		bool hit = glm::intersectRayPlane(origin, mouseDir, glm::vec3(0, 0, 0), camAxis, distance);
		if (hit) {
			// find the point of intersection on the plane using the distance 
			// We use the parameteric line or vector representation of a line to compute
			//
			// p' = p + s * dir;
			//
			glm::vec3 intersectPoint = origin + distance * mouseDir;

			// Now position the lander's origin at that intersection point
			//
		    glm::vec3 min = lander.getSceneMin();
			glm::vec3 max = lander.getSceneMax();
			float offset = (max.y - min.y) / 2.0;
			lander.setPosition(intersectPoint.x, intersectPoint.y - offset, intersectPoint.z);

			// set up bounding box for lander while we are at it
			//
			landerBounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
		}
	}
	

}


//  intersect the mouse ray with the plane normal to the camera 
//  return intersection point.   (package code above into function)
//
glm::vec3 ofApp::getMousePointOnPlane() {
	// Setup our rays
	//
	glm::vec3 origin = theCam->getPosition();
	glm::vec3 camAxis = theCam->getZAxis();
	glm::vec3 mouseWorld = theCam->screenToWorld(glm::vec3(mouseX, mouseY, 0));
	glm::vec3 mouseDir = glm::normalize(mouseWorld - origin);
	float distance;

	bool hit = glm::intersectRayPlane(origin, mouseDir, glm::vec3(0, 0, 0), camAxis, distance);

	if (hit) {
		// find the point of intersection on the plane using the distance 
		// We use the parameteric line or vector representation of a line to compute
		//
		// p' = p + s * dir;
		//
		glm::vec3 intersectPoint = origin + distance * mouseDir;
		
		return intersectPoint;
	}
	else return glm::vec3(0, 0, 0);
}
