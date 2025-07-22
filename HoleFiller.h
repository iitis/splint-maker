#ifndef HOLEFILLER_H
#define HOLEFILLER_H

#include "HFMesh.h"

typedef vector<HFVertex *> HoleVertices;
typedef vector<HFEdge *> HoleEdges;
typedef vector<HFTriangle *> HoleTriangles;

class HoleFiller
{
public:
	HoleFiller(HFMesh & m);
	~HoleFiller();
	void fillHoles();

private:
	void identifyHoles();
	void trianglulateHoles();
	void refine(int idx1 , int idx2 ,int idx3);

	//identifyHoles
	bool isBoundary(HFVertex *v1 , HFVertex *v2);
	bool isChecked(HFEdge *edge);

	//trianglulateHoles
	pair<float, float> computeWeight(HFVertex *v1, HFVertex *v2, HFVertex *v3);
	float computeArea(float* coordsV1, float* coordsV2, float* coordsV3);
	float maxDihedralAngle(HFVertex *v1, HFVertex *v2, HFVertex *v3);
	void addToMesh(int hole_id, vector< vector<int> > &minimum_weight_index, int begin, int end );

private:
	vector<HoleVertices *> v_holes;
	vector<HoleEdges *> e_holes;
	vector<HoleTriangles *> t_holes;
	HFMesh *mesh;
};

#endif