#pragma once

#include <vector>
#include <iostream>
using namespace std;

struct HFTriangle
{
	int idx; //tris[idx] is this triangle
	int v1i, v2i, v3i; //verts[v1i]-verts[v2i]-verts[v3i] forms this triangle
	float area, *normal;

	HFTriangle(int i, int a, int b, int c) : idx(i), v1i(a), v2i(b), v3i(c) {};
};

struct HFVertex
{
	int idx; //verts[idx] is this veryex
	float* coords, //coords[0] ~ x coordinate, ..
		 * normal;
	HFVertex(int i, float* c) : idx(i), coords(c) {};

	vector< int > vertList; //adjacent verts
	vector< int > edgeList; //adjacent edges
	vector< int > triList; //adjacent tris
};

struct HFEdge
{
	int idx; //edges[idx] is this edge
	int v1i, v2i; //verts[v1i]-verts[v2i] are the endpnts of this edge
	float length;
	HFEdge(int i, int a, int b, float l) : idx(i), v1i(a), v2i(b), length(l) {};
};

class HFMesh
{
public:
	vector< HFTriangle* > tris;
	vector< HFVertex* > verts;
	vector< HFEdge* > edges;

	void loadOff(const char* fName);
	void exportOff(const char* out);
	void createCube(float sideLength);

	float distanceBetween(float* a, float* b);
	int addVertex(float* coords);
	void addTriangle(int v1i, int v2i, int v3i);
	void removeTriangle(int v1i, int v2i, int v3i);
	bool triangleExists(int v1i, int v2i, int v3i);
	void splitTriangle(int idx);
	bool makeVertsNeighbors(int v, int w);
	bool makeVertsUnneighbors(int v, int w);
	void addEdge(int a, int b);
	void removeEdge(int a, int b);

	const char* filename;
};