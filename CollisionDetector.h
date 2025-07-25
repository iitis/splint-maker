#pragma once
#include "Global.h"

#include <set>
#include <map>

#include "Point3.h"

class CTriangle;
class CMesh;

class CollisionDetector
{
public:
	static bool testIntersectionOfTriangleWithTriangle3d( CPoint3d p1, CPoint3d q1, CPoint3d r1, CPoint3d p2, CPoint3d q2, CPoint3d r2 );
    static bool testIntersectionOfTriangleWithTriangle3d( CTriangle &t1, CTriangle &t2);
    static bool testIntersectionOfTriangleWithTriangle3d( CMesh *mesh1, INDEX_TYPE i1, CMesh *mesh2, INDEX_TYPE i2 );

	static bool getIntersectionOfTriangleWithTriangle3d(CPoint3d p1, CPoint3d q1, CPoint3d r1, CPoint3d p2, CPoint3d q2, CPoint3d r2, int &coplanar, CPoint3d &source, CPoint3d &target);
	static bool getIntersectionOfTriangleWithTriangle3d(CTriangle& t1, CTriangle& t2, int &coplanar, CPoint3d &source, CPoint3d &target);
	static bool getIntersectionOfTriangleWithTriangle3d(CMesh* mesh1, INDEX_TYPE i1, CMesh* mesh2, INDEX_TYPE i2, int &coplanar, CPoint3d &source, CPoint3d &target);


	static bool getIntersectionOfTriangleWithMesh3d(CTriangle& t, CMesh* mesh, std::set<INDEX_TYPE>& crossed);

	static bool getIntersectionOfMeshWithMesh3d(std::shared_ptr<CMesh> mesh1, std::shared_ptr<CMesh> mesh2, std::map<INDEX_TYPE, std::set<INDEX_TYPE>*> &crossed);

private:
	static int tri_tri_overlap_test_3d(double p1[3], double q1[3], double r1[3],
		double p2[3], double q2[3], double r2[3]);


	static int coplanar_tri_tri3d(double  p1[3], double  q1[3], double  r1[3],
		double  p2[3], double  q2[3], double  r2[3],
		double  N1[3], double  N2[3]);


	static int tri_tri_overlap_test_2d(double p1[2], double q1[2], double r1[2],
		double p2[2], double q2[2], double r2[2]);


	static int tri_tri_intersection_test_3d(double p1[3], double q1[3], double r1[3],
		double p2[3], double q2[3], double r2[3],
		int* coplanar,
		double source[3], double target[3]);

	static int ccw_tri_tri_intersection_2d(double p1[2], double q1[2], double r1[2], double p2[2], double q2[2], double r2[2]);

};

