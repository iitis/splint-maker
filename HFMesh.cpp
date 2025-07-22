#define _CRT_SECURE_NO_DEPRECATE
#include "HFMesh.h"

#include <iostream>
#include <fstream>
#include <functional>
#include <queue>
#include <cmath>
#include <limits>

void HFMesh::loadOff(const char* meshFile)
{

	this->filename = meshFile;

	cout << "Mesh initializing (to " << meshFile << ")..\n";

	FILE* fPtr;
	if (! (fPtr = fopen(meshFile, "r")))
	{
		cout << "cannot read " << meshFile << endl;
		exit(0);
	}

	char off[25];
	fscanf(fPtr, "%s\n", &off);
	float a, b, c, d; //for face lines and the 2nd line (that gives # of verts, faces, and edges) casting these to int will suffice
	fscanf(fPtr, "%f %f %f\n", &a, &b, &c);
	int nVerts = (int) a, v = 0;
	//fill vertices
	while (v++ < nVerts)
	{
		fscanf(fPtr, "%f %f %f\n", &a, &b, &c);
		float* coords = new float[3];
		coords[0] = a;
		coords[1] = b;
		coords[2] = c;
		addVertex(coords);
	}
	//fill triangles
	while (fscanf(fPtr, "%f %f %f %f\n", &d, &a, &b, &c) != EOF)
	{
		addTriangle((int) a, (int) b, (int) c);
	}
	fclose(fPtr);
	cout << "Mesh has " << (int) tris.size() << " tris, " << (int) verts.size() << " verts, " << (int) edges.size() << " edges\nInitialization done\n";
}

void HFMesh::exportOff(const char* out)
{
	std::ofstream file(out);
	cout << "Exporting mesh to " << out << ")..\n";
	if(!file)
    {
        std::cerr << "Cannot open the output file." << std::endl;
        return;
    }
	file << "OFF" << endl;
	file << this->verts.size() << " " << this->tris.size() << " " << this->edges.size() << endl;
	for(auto v : this->verts) {
		file << v->coords[0] << " " << v->coords[1] << " " << v->coords[2] << endl;
	}
	for(auto t : this-> tris) {
		file << "3 " << t->v1i << " " << t->v2i << " " << t->v3i << endl;
	}
}

int HFMesh::addVertex(float* coords)
{
	int idx;
	if (verts.size() == 0)
		idx = 0;
	else
		idx = (int) verts[verts.size()-1]->idx + 1;
	verts.push_back(new HFVertex(idx, coords));
	return idx;
}

void HFMesh::addTriangle(int v1i, int v2i, int v3i)
{
	int idx;
	if (tris.size() == 0)
		idx = 0;
	else
		idx = (int)tris[tris.size() - 1]->idx + 1;

	tris.push_back(new HFTriangle(idx, v1i, v2i, v3i));

	verts[v1i]->triList.push_back(idx);
	verts[v2i]->triList.push_back(idx);
	verts[v3i]->triList.push_back(idx);

	if (!makeVertsNeighbors(v1i, v2i))
		addEdge(v1i, v2i);

	if (!makeVertsNeighbors(v1i, v3i))
		addEdge(v1i, v3i);

	if (!makeVertsNeighbors(v2i, v3i))
		addEdge(v2i, v3i);
}

bool HFMesh::triangleExists(int v1i, int v2i, int v3i) {
	for (int i = 0; i < tris.size(); i++) {
		if (tris[i]->v1i == v1i && tris[i]->v2i == v2i && tris[i]->v3i == v3i) {
			return true;
		}
	}
	return false;
}

void HFMesh::removeTriangle(int v1i, int v2i, int v3i)
{
	int idx = -1;
	for (int i = 0; i < tris.size(); i++) {
		if (tris[i]->v1i == v1i && tris[i]->v2i == v2i && tris[i]->v3i == v3i) {
			idx = tris[i]->idx;
			tris.erase(tris.begin() + i);
			break;
		}
	}

	if (idx == -1) {
		cout << "no such triangle " << endl;
		return;
	}

	for (int i = 0; i < verts[v1i]->triList.size(); i++) {
		if (idx == verts[v1i]->triList[i]) {
			verts[v1i]->triList.erase(verts[v1i]->triList.begin() + i);
		}
	}
	for (int i = 0; i < verts[v2i]->triList.size(); i++) {
		if (idx == verts[v2i]->triList[i]) {
			verts[v2i]->triList.erase(verts[v2i]->triList.begin() + i);
		}
	}
	for (int i = 0; i < verts[v3i]->triList.size(); i++) {
		if (idx == verts[v3i]->triList[i]) {
			verts[v3i]->triList.erase(verts[v3i]->triList.begin() + i);
		}
	}

	if (!makeVertsUnneighbors(v1i, v2i))
		removeEdge(v1i, v2i);

	if (!makeVertsUnneighbors(v1i, v3i))
		removeEdge(v1i, v3i);

	if (!makeVertsUnneighbors(v2i, v3i))
		removeEdge(v2i, v3i);
}

void HFMesh::splitTriangle(int idx) {
	HFTriangle *tri = tris[idx];

	float coord_centroid[3] = {
		(verts[tri->v1i]->coords[0] + verts[tri->v2i]->coords[0] + verts[tri->v3i]->coords[0]) / 3,
		(verts[tri->v1i]->coords[1] + verts[tri->v2i]->coords[1] + verts[tri->v3i]->coords[1]) / 3,
		(verts[tri->v1i]->coords[2] + verts[tri->v2i]->coords[2] + verts[tri->v3i]->coords[2]) / 3 };

	int idx_centroid = addVertex(coord_centroid);

	cout << coord_centroid[0] << " " << coord_centroid[1] << " " << coord_centroid[2] << " " << idx_centroid << endl;

	addTriangle(verts[tri->v3i]->idx, verts[tri->v1i]->idx, idx_centroid);
	addTriangle(verts[tri->v2i]->idx, verts[tri->v3i]->idx, idx_centroid);
	addTriangle(verts[tri->v1i]->idx, verts[tri->v2i]->idx, idx_centroid);

	removeTriangle(verts[tri->v1i]->idx, verts[tri->v2i]->idx, verts[tri->v3i]->idx);
}

bool HFMesh::makeVertsNeighbors(int v, int w)
{
	//try to make v and w neighbors; return true if already neigbors

	for (int check = 0; check < (int)verts[v]->vertList.size(); check++)
		if (verts[v]->vertList[check] == w)
			return true;

	verts[v]->vertList.push_back(w);
	verts[w]->vertList.push_back(v);
	return false;
}

bool HFMesh::makeVertsUnneighbors(int v, int w)
{
	//try to make v and w unneighbors; return true if not neigbors

	for (int check = 0; check < (int)verts[v]->vertList.size(); check++)
		if (verts[v]->vertList[check] == w) {
			verts[v]->vertList.erase(verts[v]->vertList.begin() + check);
		}

	for (int check = 0; check < (int)verts[w]->vertList.size(); check++)
		if (verts[w]->vertList[check] == v) {
			verts[w]->vertList.erase(verts[w]->vertList.begin() + check);
			return false;
		}

	return true;
}

float HFMesh::distanceBetween(float* a, float* b)
{
	return sqrt( (a[0]-b[0])*(a[0]-b[0]) + (a[1]-b[1])*(a[1]-b[1]) + (a[2]-b[2])*(a[2]-b[2]));
}

void HFMesh::addEdge(int a, int b)
{
	int idx;
	if (edges.size() == 0)
		idx = 0;
	else
		idx = (int)edges[edges.size() - 1]->idx + 1;

	edges.push_back(new HFEdge(idx, a, b, distanceBetween(verts[a]->coords, verts[b]->coords)));

	verts[a]->edgeList.push_back(idx);
	verts[b]->edgeList.push_back(idx);
}
void HFMesh::removeEdge(int a, int b)
{
	int idx = -1;
	for (int i = 0; i < edges.size(); i++) {
		if ((edges[i]->v1i == a && edges[i]->v2i == b) || (edges[i]->v1i == b && edges[i]->v2i == a)) {
			idx = edges[i]->idx;
			edges.erase(edges.begin() + i);
		}
	}
	if (idx == -1)
		return;

	for (int i = 0; i < verts[a]->edgeList.size(); i++) {
		if (verts[a]->edgeList[i] == idx)
			verts[a]->edgeList.erase(verts[a]->edgeList.begin() + i);
	}
	for (int i = 0; i < verts[b]->edgeList.size(); i++) {
		if (verts[b]->edgeList[i] == idx)
			verts[b]->edgeList.erase(verts[b]->edgeList.begin() + i);
	}
}