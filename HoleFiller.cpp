#include "HoleFiller.h"

#include <set>
#include <iterator>
#include <limits>
#include <tuple>

#include "../api/UI.h"

HoleFiller::HoleFiller(HFMesh & m)
{
	mesh = &m;
}

HoleFiller::~HoleFiller()
{
	for(int i = 0 ; i < v_holes.size() ; i++)
		for(int j = 0 ; j < v_holes[i]->size() ; j++)
			delete (*v_holes[i])[j];
}

void HoleFiller::fillHoles()
{
	UI::STATUSBAR::setText("identifying holes...");
	identifyHoles();
	UI::STATUSBAR::printf("number of holes: %d",v_holes.size());
	for (int i = 0; i < v_holes.size(); i++) {
		HoleTriangles *t_hole = new HoleTriangles;
		t_holes.push_back(t_hole);
	}

	trianglulateHoles();

	
	for (int i = 0; i<t_holes.size(); i++) {
		HoleTriangles ht = *t_holes[i];
		for (int k = 0; k<ht.size(); k++) {
			UI::STATUSBAR::printfTimed(1000, "refining hole: i = %d, k = %d", i, k);
			// TODO refine does not work!
			refine(ht[k]->v1i, ht[k]->v2i, ht[k]->v3i);
		}
	}
}

void HoleFiller::identifyHoles()
{
	for(int i = 0; i < mesh->edges.size() ; i++)
	{
		if( !isChecked(mesh->edges[i]) && isBoundary(mesh->verts[mesh->edges[i]->v1i] , mesh->verts[mesh->edges[i]->v2i]) )
		{
			HoleVertices *v_hole = new HoleVertices;
			HoleEdges *e_hole = new HoleEdges;

			HFVertex *current_vertex = mesh->verts[mesh->edges[i]->v1i];
			HFEdge *current_edge = mesh->edges[i];
			
			int aaa = 0;
			do
			{
				//UI::STATUSBAR::printfTimed(1000, "identifying holes... i = %d, while %d", i, aaa++);

				v_hole->push_back(current_vertex);
				e_hole->push_back(current_edge);
				
				for(int j = 0 ; j < current_vertex->edgeList.size() ; j++)
				{
					UI::STATUSBAR::printfTimed(500, "identifying holes... i = %d of %d, aaa = %d, j = %d of %d", i, mesh->edges.size(), aaa, j, current_vertex->edgeList.size());
					if(current_vertex->edgeList[j] != current_edge->idx )
					{

						HFVertex *v1 = mesh->verts[mesh->edges[current_vertex->edgeList[j]]->v1i];
						HFVertex *v2 = mesh->verts[mesh->edges[current_vertex->edgeList[j]]->v2i];
						if (isBoundary(v1 , v2))
						{
							current_edge = mesh->edges[current_vertex->edgeList[j]];
							current_vertex = (v1 == current_vertex) ? v2 : v1;
							break;
						}
					}
				}

				aaa++;
			}
			while(mesh->edges[i]->idx != current_edge->idx);

			v_holes.push_back(v_hole);
			e_holes.push_back(e_hole);
		}
	}
}

void HoleFiller::trianglulateHoles()
{
	for(int hole_id = 0; hole_id < v_holes.size(); hole_id++)
	{
		HoleVertices holeVertices = *v_holes[hole_id];
		int size = holeVertices.size();

		vector<vector<pair<float, float>>> minimum_weight(size, vector<pair<float, float>>(size , {0, 0} ));
		vector<vector<int>> minimum_weight_index(size , vector<int>(size , -1));
		for(int j = 2 ; j < size ; j++)
		{
			for(int i = 0 ; i < size - j ; i++)
			{
				UI::STATUSBAR::printfTimed(1000, "triangulating holes... id = %d, i = %d, j = %d", hole_id, i, j);

				pair<float, float> min = {FLT_MAX, FLT_MAX};
				int index = -1;
				int k = i + j;

				for(int m = i + 1 ; m < k ; m++)
				{
					pair<float, float> w = computeWeight(holeVertices[i], holeVertices[m], holeVertices[j]);
					pair<float, float> val = { minimum_weight[i][m].first + minimum_weight[m][j].first +  w.first,
						minimum_weight[i][m].second + minimum_weight[m][j].second +  w.second};
					if( val.first < min.first || (val.first == min.first && val.second < min.second))
					{
						min = val;
						index = m;
					}
				}
				minimum_weight[i][j] = min;
				minimum_weight_index[i][j] = index;
			}
		}

		addToMesh(hole_id, minimum_weight_index, 0, size - 1);
	}
}

void HoleFiller::refine(int idx1, int idx2, int idx3)
{
	if (!mesh->triangleExists(idx1, idx2, idx3)) {
		cout << "triangle does not exist: " << idx1 << " " << idx2 << " " << idx3 << endl;
		return;
	}

	HFVertex *v1 =  mesh->verts[idx1];
	HFVertex *v2 =  mesh->verts[idx2];
	HFVertex *v3 =  mesh->verts[idx3];

	float *coord1 = v1->coords;
	float *coord2 = v2->coords;
	float *coord3 = v3->coords;

	float coord_centroid[3] = { (coord1[0] + coord2[0] + coord3[0]) / 3,
								(coord1[1] + coord2[1] + coord3[1]) / 3,
								(coord1[2] + coord2[2] + coord3[2]) / 3 };

	float sigma_vertices[3] = {0};
	float sigma_centroid;

	for(int j = 0 ; j < v1->edgeList.size() ; j++)
	{
		if(mesh->edges.size() >= v1->edgeList[j] && mesh->edges[v1->edgeList[j]] != NULL) {
			sigma_vertices[0] += mesh->edges[v1->edgeList[j]]->length;
		}
	}
	sigma_vertices[0] /= v1->edgeList.size();

	for(int j = 0 ; j < v2->edgeList.size() ; j++)
	{
		if(mesh->edges.size() >= v2->edgeList[j] && mesh->edges[v2->edgeList[j]] != NULL) {
			sigma_vertices[1] += mesh->edges[v2->edgeList[j]]->length;
		}
	}
	sigma_vertices[1] /= v2->edgeList.size();

	for(int j = 0 ; j < v3->edgeList.size() ; j++)
	{
		if(mesh->edges.size() >= v3->edgeList[j] && mesh->edges[v3->edgeList[j]] != NULL) {
			sigma_vertices[2] += mesh->edges[v3->edgeList[j]]->length;
		}
	}
	sigma_vertices[2] /= v3->edgeList.size();
	sigma_centroid = (sigma_vertices[2] + sigma_vertices[1] + sigma_vertices[0])/3;

	
	float scale = sqrt(2.0);

	float temp1 = scale * mesh->distanceBetween(coord_centroid, v1->coords);
	float temp2 = scale * mesh->distanceBetween(coord_centroid, v2->coords);
	float temp3 = scale * mesh->distanceBetween(coord_centroid, v3->coords);
	if( ( temp1 > sigma_vertices[0] && temp1 > sigma_centroid) ||
		(temp2 > sigma_vertices[1] && temp2 > sigma_centroid) ||
		(temp3 > sigma_vertices[2] && temp3 > sigma_centroid) )
	{
		int idx_centroid = mesh->addVertex(coord_centroid);

		mesh->removeTriangle(idx1, idx2, idx3);

		mesh->addTriangle(idx1, idx_centroid, idx3);
		mesh->addTriangle(idx2, idx_centroid, idx3);
		mesh->addTriangle(idx1, idx_centroid, idx2);
	}
}



bool HoleFiller::isChecked(HFEdge *edge)
{
	for(int i= 0 ; i < e_holes.size() ; i++)
	{
		for(int j= 0 ; j < e_holes[i]->size() ; j++)
		{
			if((*e_holes[i])[j]->idx == edge->idx)
				return 1;
		}
	}
	return 0;
}

bool HoleFiller::isBoundary(HFVertex *v1 , HFVertex *v2)
{
	int counter = 0;
	for(int i = 0 ; i < v1->triList.size() && counter < 2; i++)
	{
		for(int j = 0 ; j < v2->triList.size() ; j++)
		{
			if(v2->triList[j] == v1->triList[i])
			{
				counter++;
				break;
			}
		}
	}
	return (counter == 1);
}


float HoleFiller::computeArea(float* coordsV1, float* coordsV2, float* coordsV3)
{
	float a = sqrt((coordsV1[0] - coordsV2[0])*(coordsV1[0] - coordsV2[0]) + (coordsV1[1] - coordsV2[1])*(coordsV1[1] - coordsV2[1]) + (coordsV1[2] - coordsV2[2])*(coordsV1[2] - coordsV2[2]));
	float b = sqrt((coordsV1[0] - coordsV3[0])*(coordsV1[0] - coordsV3[0]) + (coordsV1[1] - coordsV3[1])*(coordsV1[1] - coordsV3[1]) + (coordsV1[2] - coordsV3[2])*(coordsV1[2] - coordsV3[2]));
	float c = sqrt((coordsV3[0] - coordsV2[0])*(coordsV3[0] - coordsV2[0]) + (coordsV3[1] - coordsV2[1])*(coordsV3[1] - coordsV2[1]) + (coordsV3[2] - coordsV2[2])*(coordsV3[2] - coordsV2[2]));
	float p = (a + b + c) / 2;
	return sqrt(p*(p-a)*(p-b)*(p-c));
}


float* planarEquation(float* v1, float *v2, float *v3)
{
	// planar equation of our triangle
	float vector_v1_v2[3] = {v2[0] - v1[0], v2[1] - v1[1], v2[2] - v1[2]};
	float vector_v1_v3[3] = {v3[0] - v1[0], v3[1] - v1[1], v3[2] - v1[2]};

	// cross product => A*i + B*j + C*k
	float A = vector_v1_v2[1]*vector_v1_v3[2] - vector_v1_v2[2]*vector_v1_v3[1];
	float B = vector_v1_v2[1]*vector_v1_v3[2] - vector_v1_v2[2]*vector_v1_v3[1];
	float C = vector_v1_v2[0]*vector_v1_v3[1] - vector_v1_v2[1]*vector_v1_v3[0];

	// Planar equation => A(x-v1[0]) + B(y-v1[1]) + C(z-v1[2]) = 0
	return new float[3] {A, B, C};
}

// Dihedral angle between two planes.
// p = [A, B, C] where Ax + By + Cz = D
float dihedralAngle(float* p1, float* p2) {
	return acos((p1[0] * p2[0] + p1[2] * p2[2] + p1[2] * p2[2]) / 
		(sqrt(p1[0]*p1[0] + p1[1]*p1[1] + p1[2]*p1[2]) * sqrt(p2[0]*p2[0] + p2[1]*p2[1] + p2[2]*p2[2])));
}

float HoleFiller::maxDihedralAngle(HFVertex* v1, HFVertex* v2, HFVertex* v3)
{
	// TODO max. dihedral angle between (v1, v2, v3) and existing adjacent triangles
	// collect all adjacent triangles
	set<int> adjacentTriangles;
	for(auto v : {v1, v2, v3}) {
		std::copy(v->triList.begin(), v->triList.end(), std::inserter(adjacentTriangles, adjacentTriangles.end()));
	}
	auto eq = planarEquation(v1->coords, v2->coords, v3->coords);
	auto maxDihedral = std::numeric_limits<float>::lowest();
	for (auto tri : adjacentTriangles) {
		auto t = mesh->tris[tri];
		auto eq2 = planarEquation(mesh->verts[t->v1i]->coords, mesh->verts[t->v1i]->coords, mesh->verts[t->v1i]->coords);
		maxDihedral = dihedralAngle(eq, eq2);
	}
	return maxDihedral;
}

pair<float, float> HoleFiller::computeWeight(HFVertex* v1, HFVertex* v2, HFVertex* v3)
{
	return make_pair(maxDihedralAngle(v1, v2, v3), computeArea(v1->coords, v2->coords, v3->coords));
}



void HoleFiller::addToMesh(int hole_id, vector<vector<int>> &minimum_weight_index, int begin, int end)
{
	cout << "addToMesh " <<  hole_id << " " << minimum_weight_index.size() << " " << begin << " " << end << endl;
	for(auto x : minimum_weight_index) {
		for(auto y : x) {
			cout << y << " ";
		}
		cout << endl;
	}
	if(end - begin > 1)
	{
		HoleVertices holeVertices = *v_holes[hole_id];
		int current = minimum_weight_index[begin][end];
		cout << "current " <<  current << " " << holeVertices.size() << endl;
		cout << "add triangle " <<  holeVertices[begin]->idx << " " << holeVertices[current]->idx << " " << holeVertices[end]->idx << endl;

		mesh->addTriangle(holeVertices[begin]->idx, holeVertices[current]->idx, holeVertices[end]->idx);
		(*t_holes[hole_id]).push_back(mesh->tris[mesh->tris.size()-1]);

		addToMesh( hole_id , minimum_weight_index , begin , current );
		addToMesh( hole_id , minimum_weight_index , current , end );
	}
}