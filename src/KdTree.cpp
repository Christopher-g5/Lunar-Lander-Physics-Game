//  KdTree Template - Simple KdTree class
//
//  SJSU  - CS134 Game Development
//
//  Kevin M. Smith   04/19/20

//  **Important:  Vertices (x, y, z) in the mesh are stored in the Tree node as an integer index.
//  to read the 3D vertex point from the mesh given index i,  use the function ofMesh::getVertex(i);  See
//  KdTree::meshBounds() for an example of usage;
//
//

#include "KdTree.h"
 
// draw KdTree (recursively)
//
void KdTree::draw(TreeNode & node, int numLevels, int level) {
	if (level >= numLevels) return;
	drawBox(node.box);
	level++;
	for (int i = 0; i < node.children.size(); i++) {
		draw(node.children[i], numLevels, level);
	}
}

// draw only leaf Nodes
//
void KdTree::drawLeafNodes(TreeNode & node) {
	if (node.children.size() != 0) {
		for (int i = 0; i < node.children.size(); i++) {
			drawLeafNodes(node.children[i]);
		}
	}
	else {
		drawBox(node.box);
	}
}


//draw a box from a "Box" class  
//
void KdTree::drawBox(const Box &box) {
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
Box KdTree::meshBounds(const ofMesh & mesh) {
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
	cout << "vertices: " << n << endl;
//	cout << "min: " << min << "max: " << max << endl;
	return Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
}

// getMeshPointsInBox:  return an array of indices to points in mesh that are contained 
//                      inside the Box.  Return count of points found;
//
int KdTree::getMeshPointsInBox(const ofMesh & mesh, const vector<int>& points,
	Box & box, vector<int> & pointsRtn)
{
	int count = 0;
	for (int i = 0; i < points.size(); i++) {
		ofVec3f v = mesh.getVertex(points[i]);
		if (box.inside(Vector3(v.x, v.y, v.z))) {
			count++;
			pointsRtn.push_back(points[i]);
		}
	}
	return count;
}

void KdTree::create(const ofMesh & geo, int numLevels) {
	//initialize KdTree structure
	mesh = geo;
	int level = 0;
	vector<float> boxYvalue;
	int it = 0;

	root = TreeNode();
	root.box = meshBounds(mesh);

	vector <ofIndexType> indices = geo.getIndices();
	vector <int> meshpoints(indices.size());
	vector <int> rootpoints;

	//fill meshpoints vector with all indices from mesh
	for (int i = 0; i < indices.size(); i++) {
		meshpoints[i] = indices[i];
	}

	//fill root points vector with all indices from mesh
	getMeshPointsInBox(geo, meshpoints, root.box, rootpoints);
	root.points = rootpoints;


	/*cout << "Level: " << level << endl;
	cout << "Root points: " << root.points.size() << endl;
	cout << "Max: " << root.box.max().x() << " " << root.box.max().y() << " " << root.box.max().z() << endl;
	cout << "Min: " << root.box.min().x() << " " << root.box.min().y() << " " << root.box.min().z() << endl;
	cout << endl;*/
	

	//divide original box into two
	subdivide(mesh, root, numLevels ,level);

}

void KdTree::subdivide(const ofMesh & mesh, TreeNode & node, int numLevels, int level) {
	level++;
	if (level >= numLevels) return;


	vector<Box> boxList;
	vector<float> boxYValue;
	float maxY = mesh.getVertex(node.points[0]).y;
	float minY = mesh.getVertex(node.points[0]).y;

	//find min and max y vertex values
	for (int i = 0; i < node.points.size(); i++) {
		if (mesh.getVertex(node.points[i]).y > maxY) {
			maxY = mesh.getVertex(node.points[i]).y;
		}
		else if (mesh.getVertex(node.points[i]).y < minY) {
			minY = mesh.getVertex(node.points[i]).y;
		}
	}
	boxYValue.push_back(minY); 
	boxYValue.push_back(maxY);


	subDivideBox(node.box, boxList, boxYValue ,level);

	TreeNode leftChild = TreeNode();
	TreeNode rightChild = TreeNode();
	
	Vector3 vertex;
	vector<int> rtnPoints;

	//left child
	leftChild.box = boxList[0];
	if (getMeshPointsInBox(mesh, node.points, leftChild.box, rtnPoints) > 0) {
		leftChild.points = rtnPoints;
		subdivide(mesh, leftChild, numLevels, level);
		node.children.push_back(leftChild);
	}

	rtnPoints.clear();

	//right child
	rightChild.box = boxList[1];
	if (getMeshPointsInBox(mesh, node.points, rightChild.box, rtnPoints) > 0) {
		rightChild.points = rtnPoints;
		subdivide(mesh, rightChild, numLevels, level);
		node.children.push_back(rightChild);
	}


	/*cout << "Children in Node: " << node.children.size() << endl;
	cout << "Points in Parent: " << node.points.size() << endl;
	cout << endl;*/
	
}

//  Subdivide a Box; return children in  boxList
//
void KdTree::subDivideBox(const Box &box, vector<Box> & boxList, vector<float> boxYvalue, int level) {
	Vector3 newMax;
	Vector3 newMin;
	Box b = box;


	//subdivide this node by cutting the box into equal sizes
	//alternate box division orientation
	//cut along z axis
	if (level % 2 == 0) {
		newMin = Vector3((b.max().x() + b.min().x()) / 2, boxYvalue[0], b.min().z());
		newMax = Vector3((b.max().x() + b.min().x()) / 2, boxYvalue[1], b.max().z());
	}

	//cut along x axis
	else {
			newMin = Vector3(b.min().x(), boxYvalue[0], (b.max().z() + b.min().z()) / 2);
			newMax = Vector3(b.max().x(), boxYvalue[1], (b.max().z() + b.min().z()) / 2);
	}

	Vector3 modifyMin = Vector3(b.min().x(), boxYvalue[0] , b.min().z());
	Vector3 modifyMax = Vector3(b.max().x(), boxYvalue[1] , b.max().z());

	boxList.push_back(Box(modifyMin, newMax));
	boxList.push_back(Box(newMin, modifyMax));
}


bool KdTree::intersect(const Ray &ray, const TreeNode & node, TreeNode & nodeRtn) {

	if (node.box.intersect(ray, -50000, 50000)) {

		if (node.children.size() > 0) {

			for (int i = 0; i < node.children.size(); i++) {
				if (intersect(ray, node.children[i], nodeRtn)) {
					return true;
				}
			}
		}
		else {
			//cout << "Child Node found" << endl ;
			nodeRtn = node;
			//cout << "LOOK: "<<nodeRtn.points.size() << endl ;
			return true;
		}
	}
	return false;
}

		

void KdTree::insertionSortX(vector<int> & vec, const ofMesh & mesh, int n)
{
	int i, j , keyVal;
	float keyX;

	for (i = 1; i < n; i++)
	{
		keyX = mesh.getVertex(vec[i]).x;
		keyVal = vec[i];
		j = i - 1;

		while (j >= 0 && mesh.getVertex(vec[j]).x > keyX)
		{
			vec[j + 1] = vec[j];
			j = j - 1;
		}
		vec[j + 1] = keyVal;
	}
}

void KdTree::insertionSortZ(vector<int> & vec, const ofMesh & mesh, int n)
{
	int i, j, keyVal;
	float keyZ;

	for (i = 1; i < n; i++)
	{
		keyZ = mesh.getVertex(vec[i]).z;
		keyVal = vec[i];
		j = i - 1;

		while (j >= 0 && mesh.getVertex(vec[j]).z > keyZ)
		{
			vec[j + 1] = vec[j];
			j = j - 1;
		}
		vec[j + 1] = keyVal;
	}
}



