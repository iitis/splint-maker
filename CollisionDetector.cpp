#include "CollisionDetector.h"
#include "CollisionDetector_definitions.h"

#include "Triangle.h"
#include "Mesh.h"

#include "../api/UI.h"

bool CollisionDetector::testIntersectionOfTriangleWithTriangle3d(CPoint3d p1, CPoint3d q1, CPoint3d r1, CPoint3d p2, CPoint3d q2, CPoint3d r2)
{
	return 1 == tri_tri_overlap_test_3d(    p1.toVector(), q1.toVector(), r1.toVector(),
                                            p2.toVector(), q2.toVector(), r2.toVector());
}

bool CollisionDetector::testIntersectionOfTriangleWithTriangle3d(CTriangle &t1, CTriangle &t2)
{
    return testIntersectionOfTriangleWithTriangle3d(    t1[0], t1[1], t1[2],
                                                        t2[0], t2[1], t2[2]);
}

bool CollisionDetector::testIntersectionOfTriangleWithTriangle3d(CMesh* mesh1, INDEX_TYPE i1, CMesh* mesh2, INDEX_TYPE i2)
{
    return testIntersectionOfTriangleWithTriangle3d(        mesh1->vertex( i1, 0 ), mesh1->vertex( i1, 1 ), mesh1->vertex(i1, 2),
                                                            mesh2->vertex( i2, 0 ), mesh2->vertex( i2, 1 ), mesh2->vertex(i2, 2));
}

bool CollisionDetector::getIntersectionOfTriangleWithTriangle3d(CPoint3d p1, CPoint3d q1, CPoint3d r1, CPoint3d p2, CPoint3d q2, CPoint3d r2, int &coplanar, CPoint3d &source, CPoint3d &target)
{
    return 1 == tri_tri_intersection_test_3d(   p1.toVector(), q1.toVector(), r1.toVector(),
                                                p2.toVector(), q2.toVector(), r2.toVector(),
                                                &coplanar, source.toVector(), target.toVector() );
}

bool CollisionDetector::getIntersectionOfTriangleWithTriangle3d(CTriangle& t1, CTriangle& t2, int &coplanar, CPoint3d &source, CPoint3d &target)
{
    return getIntersectionOfTriangleWithTriangle3d(     t1[0], t1[1], t1[2],
                                                        t2[0], t2[1], t2[2],
                                                        coplanar, source, target );
}

bool CollisionDetector::getIntersectionOfTriangleWithTriangle3d(CMesh* mesh1, INDEX_TYPE i1, CMesh* mesh2, INDEX_TYPE i2, int& coplanar, CPoint3d& source, CPoint3d& target)
{
    return getIntersectionOfTriangleWithTriangle3d(     mesh1->vertex(i1, 0), mesh1->vertex(i1, 1), mesh1->vertex(i1, 2),
                                                        mesh2->vertex(i2, 0), mesh2->vertex(i2, 1), mesh2->vertex(i2, 2),
                                                        coplanar, source, target);
}

#include "KDNode2.h"

bool CollisionDetector::getIntersectionOfTriangleWithMesh3d(CTriangle& testedTriangle, CMesh* testedMesh, std::set<INDEX_TYPE>& foundCrossedFaces)
{
    CBoundingBox bb1 = testedMesh->getBoundingBox();
    CBoundingBox bb2 = testedTriangle.getBoundingBox();

    //if (CBoundingBox::intersection(mesh1BB, currentTriangleBB).isValid())
    if ( bb1.intersects(bb2) )
    {
        KDNode2* node = KDNode2::build(testedMesh);

        bool hit = node->findCrossedBB(bb2, testedMesh, foundCrossedFaces);

        if (! foundCrossedFaces.empty())
        {
            std::set<INDEX_TYPE>::iterator it = foundCrossedFaces.begin();

            while (it != foundCrossedFaces.end())
            {
                CTriangle potentiallyFoundTriangle(*it, *testedMesh);

                if (CollisionDetector::testIntersectionOfTriangleWithTriangle3d(potentiallyFoundTriangle, testedTriangle))
                {
                    it++;
                }
                else
                {
                    INDEX_TYPE indexToBeErased = *it;
                    it++;
                    foundCrossedFaces.erase(indexToBeErased);
                }
            }

            if (!foundCrossedFaces.empty())
            {
                return true;
            }
        }

        delete node;
    }

    return false;
}

//#include "ProgressIndicator.h"

bool CollisionDetector::getIntersectionOfMeshWithMesh3d(std::shared_ptr<CMesh> mesh2, std::shared_ptr<CMesh> mesh1, std::map<INDEX_TYPE, std::set<INDEX_TYPE>*> &crossed)
{
    CBoundingBox mesh1BB = mesh1->getBoundingBox();
    CBoundingBox mesh2BB = mesh2->getBoundingBox();

    if ( mesh1BB.intersects(mesh2BB) )
    {
        UI::STATUSBAR::setText("Building tree of faces");
        KDNode2* node = KDNode2::build(mesh1.get());

        int progress = 0;

        //ProgressIndicator* progressIndicator = UI::PROGRESSBAR::instance();
        //progressIndicator->init(0, 100, 0, "Finding intersections...");
        UI::PROGRESSBAR::init(0, 100, 0);
        //connect(this, SIGNAL(sendProgress(int)), progressIndicator, SLOT(setValue(int)));
        //connect(progressIndicator->cancelButton(), SIGNAL(clicked()), this, SLOT(onLoadCancelled()));
//        progressIndicator->cancelButton()->show();
        
        //cancelled = false;

        int i = 0;
        size_t nbOfFaces = mesh2->faces().size();

        while (i < nbOfFaces)
        //        while ( !progressIndicator->actionCancelled && (i < nbOfFaces) )
        {
            UI::STATUSBAR::printfTimed(1000, "Iteration: %d", i);

            CTriangle t2(i, *mesh2);
            CBoundingBox currentTriangleBB = t2.getBoundingBox();

            //if (CBoundingBox::intersection(mesh1BB, currentTriangleBB).isValid())
            if (mesh1BB.intersects(currentTriangleBB))
            {
                std::set<INDEX_TYPE>* foundCrossedFaces = new std::set<INDEX_TYPE>;

                bool hit = node->findCrossedBB(currentTriangleBB, mesh1.get(), *foundCrossedFaces);

                if (foundCrossedFaces->empty())
                {
                    delete foundCrossedFaces;
                }
                else
                {
                    std::set<INDEX_TYPE>::iterator it = foundCrossedFaces->begin();

                    while (it != foundCrossedFaces->end())
                    {
                        CTriangle t1(*it, *mesh1);

                        if (CollisionDetector::testIntersectionOfTriangleWithTriangle3d(t1, t2))
                        {
                            it++;
                        }
                        else
                        {
                            INDEX_TYPE indexToBeErased = *it;
                            it++;
                            foundCrossedFaces->erase(indexToBeErased);
                        }
                    }

                    if (foundCrossedFaces->empty())
                    {
                        delete foundCrossedFaces;
                    }
                    else
                    {
                        crossed[i] = foundCrossedFaces;
                    }
                }

            }
            

            if ( progress < ( (100 * i) / nbOfFaces) )
            {
                //emit(sendProgress(i));
                //progressIndicator->setValue(progress);
                UI::PROGRESSBAR::setValue(progress);
                progress = (100 * i) / nbOfFaces;
            }

            i++;
        }

        //progressIndicator->hide();
        UI::PROGRESSBAR::hide();
        delete node;
    }

    return !crossed.empty();
}






/************************************* PRIVATE *************************************************/

/*
*
*  Three-dimensional Triangle-Triangle Overlap Test
*
*/


int CollisionDetector::tri_tri_overlap_test_3d(double p1[3], double q1[3], double r1[3], double p2[3], double q2[3], double r2[3])
{
    double dp1, dq1, dr1, dp2, dq2, dr2;
    double v1[3], v2[3];
    double N1[3], N2[3];

    /* Compute distance signs  of p1, q1 and r1 to the plane of
       triangle(p2,q2,r2) */


    SUB(v1, p2, r2)
        SUB(v2, q2, r2)
        CROSS(N2, v1, v2)

        SUB(v1, p1, r2)
        dp1 = DOT(v1, N2);
    SUB(v1, q1, r2)
        dq1 = DOT(v1, N2);
    SUB(v1, r1, r2)
        dr1 = DOT(v1, N2);

    if (((dp1 * dq1) > 0.0f) && ((dp1 * dr1) > 0.0f))  return 0;

    /* Compute distance signs  of p2, q2 and r2 to the plane of
       triangle(p1,q1,r1) */


    SUB(v1, q1, p1)
        SUB(v2, r1, p1)
        CROSS(N1, v1, v2)

        SUB(v1, p2, r1)
        dp2 = DOT(v1, N1);
    SUB(v1, q2, r1)
        dq2 = DOT(v1, N1);
    SUB(v1, r2, r1)
        dr2 = DOT(v1, N1);

    if (((dp2 * dq2) > 0.0f) && ((dp2 * dr2) > 0.0f)) return 0;

    /* Permutation in a canonical form of T1's vertices */




    if (dp1 > 0.0f) {
        if (dq1 > 0.0f) TRI_TRI_3D(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2)
        else if (dr1 > 0.0f) TRI_TRI_3D(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2)
        else TRI_TRI_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2)
    }
    else if (dp1 < 0.0f) {
        if (dq1 < 0.0f) TRI_TRI_3D(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2)
        else if (dr1 < 0.0f) TRI_TRI_3D(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2)
        else TRI_TRI_3D(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2)
    }
    else {
        if (dq1 < 0.0f) {
            if (dr1 >= 0.0f) TRI_TRI_3D(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2)
            else TRI_TRI_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2)
        }
        else if (dq1 > 0.0f) {
            if (dr1 > 0.0f) TRI_TRI_3D(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2)
            else TRI_TRI_3D(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2)
        }
        else {
            if (dr1 > 0.0f) TRI_TRI_3D(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2)
            else if (dr1 < 0.0f) TRI_TRI_3D(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2)
            else return coplanar_tri_tri3d(p1, q1, r1, p2, q2, r2, N1, N2);
        }
    }
};



int CollisionDetector::coplanar_tri_tri3d(double p1[3], double q1[3], double r1[3],
    double p2[3], double q2[3], double r2[3],
    double normal_1[3], double normal_2[3]) {

    double P1[2], Q1[2], R1[2];
    double P2[2], Q2[2], R2[2];

    double n_x, n_y, n_z;

    n_x = ((normal_1[0] < 0) ? -normal_1[0] : normal_1[0]);
    n_y = ((normal_1[1] < 0) ? -normal_1[1] : normal_1[1]);
    n_z = ((normal_1[2] < 0) ? -normal_1[2] : normal_1[2]);


    /* Projection of the triangles in 3D onto 2D such that the area of
       the projection is maximized. */


    if ((n_x > n_z) && (n_x >= n_y)) {
        // Project onto plane YZ

        P1[0] = q1[2]; P1[1] = q1[1];
        Q1[0] = p1[2]; Q1[1] = p1[1];
        R1[0] = r1[2]; R1[1] = r1[1];

        P2[0] = q2[2]; P2[1] = q2[1];
        Q2[0] = p2[2]; Q2[1] = p2[1];
        R2[0] = r2[2]; R2[1] = r2[1];

    }
    else if ((n_y > n_z) && (n_y >= n_x)) {
        // Project onto plane XZ

        P1[0] = q1[0]; P1[1] = q1[2];
        Q1[0] = p1[0]; Q1[1] = p1[2];
        R1[0] = r1[0]; R1[1] = r1[2];

        P2[0] = q2[0]; P2[1] = q2[2];
        Q2[0] = p2[0]; Q2[1] = p2[2];
        R2[0] = r2[0]; R2[1] = r2[2];

    }
    else {
        // Project onto plane XY

        P1[0] = p1[0]; P1[1] = p1[1];
        Q1[0] = q1[0]; Q1[1] = q1[1];
        R1[0] = r1[0]; R1[1] = r1[1];

        P2[0] = p2[0]; P2[1] = p2[1];
        Q2[0] = q2[0]; Q2[1] = q2[1];
        R2[0] = r2[0]; R2[1] = r2[1];
    }

    return tri_tri_overlap_test_2d(P1, Q1, R1, P2, Q2, R2);

};

/*
   The following version computes the segment of intersection of the
   two triangles if it exists.
   coplanar returns whether the triangles are coplanar
   source and target are the endpoints of the line segment of intersection
*/

int CollisionDetector::tri_tri_intersection_test_3d(double p1[3], double q1[3], double r1[3],
    double p2[3], double q2[3], double r2[3],
    int* coplanar,
    double source[3], double target[3])

{
    double dp1, dq1, dr1, dp2, dq2, dr2;
    double v1[3], v2[3], v[3];
    double N1[3], N2[3], N[3];
    double alpha;

    // Compute distance signs  of p1, q1 and r1 
    // to the plane of triangle(p2,q2,r2)


    SUB(v1, p2, r2)
        SUB(v2, q2, r2)
        CROSS(N2, v1, v2)

        SUB(v1, p1, r2)
        dp1 = DOT(v1, N2);
    SUB(v1, q1, r2)
        dq1 = DOT(v1, N2);
    SUB(v1, r1, r2)
        dr1 = DOT(v1, N2);

    if (((dp1 * dq1) > 0.0f) && ((dp1 * dr1) > 0.0f))  return 0;

    // Compute distance signs  of p2, q2 and r2 
    // to the plane of triangle(p1,q1,r1)


    SUB(v1, q1, p1)
        SUB(v2, r1, p1)
        CROSS(N1, v1, v2)

        SUB(v1, p2, r1)
        dp2 = DOT(v1, N1);
    SUB(v1, q2, r1)
        dq2 = DOT(v1, N1);
    SUB(v1, r2, r1)
        dr2 = DOT(v1, N1);

    if (((dp2 * dq2) > 0.0f) && ((dp2 * dr2) > 0.0f)) return 0;

    // Permutation in a canonical form of T1's vertices


    //  printf("d1 = [%f %f %f], d2 = [%f %f %f]\n", dp1, dq1, dr1, dp2, dq2, dr2);
    /*
    // added by Aaron
    if (ZERO_TEST(dp1) || ZERO_TEST(dq1) ||ZERO_TEST(dr1) ||ZERO_TEST(dp2) ||ZERO_TEST(dq2) ||ZERO_TEST(dr2))
      {
        coplanar = 1;
        return 0;
      }
    */


    if (dp1 > 0.0f) {
        if (dq1 > 0.0f) TRI_TRI_INTER_3D(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2)
        else if (dr1 > 0.0f) TRI_TRI_INTER_3D(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2)

        else TRI_TRI_INTER_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2)
    }
    else if (dp1 < 0.0f) {
        if (dq1 < 0.0f) TRI_TRI_INTER_3D(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2)
        else if (dr1 < 0.0f) TRI_TRI_INTER_3D(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2)
        else TRI_TRI_INTER_3D(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2)
    }
    else {
        if (dq1 < 0.0f) {
            if (dr1 >= 0.0f) TRI_TRI_INTER_3D(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2)
            else TRI_TRI_INTER_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2)
        }
        else if (dq1 > 0.0f) {
            if (dr1 > 0.0f) TRI_TRI_INTER_3D(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2)
            else TRI_TRI_INTER_3D(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2)
        }
        else {
            if (dr1 > 0.0f) TRI_TRI_INTER_3D(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2)
            else if (dr1 < 0.0f) TRI_TRI_INTER_3D(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2)
            else {
                // triangles are co-planar

                *coplanar = 1;
                return coplanar_tri_tri3d(p1, q1, r1, p2, q2, r2, N1, N2);
            }
        }
    }
};


int CollisionDetector::ccw_tri_tri_intersection_2d(double p1[2], double q1[2], double r1[2],
    double p2[2], double q2[2], double r2[2]) {
    if (ORIENT_2D(p2, q2, p1) >= 0.0f) {
        if (ORIENT_2D(q2, r2, p1) >= 0.0f) {
            if (ORIENT_2D(r2, p2, p1) >= 0.0f) return 1;
            else INTERSECTION_TEST_EDGE(p1, q1, r1, p2, q2, r2)
        }
        else {
            if (ORIENT_2D(r2, p2, p1) >= 0.0f)
                INTERSECTION_TEST_EDGE(p1, q1, r1, r2, p2, q2)
            else INTERSECTION_TEST_VERTEX(p1, q1, r1, p2, q2, r2)
        }
    }
    else {
        if (ORIENT_2D(q2, r2, p1) >= 0.0f) {
            if (ORIENT_2D(r2, p2, p1) >= 0.0f)
                INTERSECTION_TEST_EDGE(p1, q1, r1, q2, r2, p2)
            else  INTERSECTION_TEST_VERTEX(p1, q1, r1, q2, r2, p2)
        }
        else INTERSECTION_TEST_VERTEX(p1, q1, r1, r2, p2, q2)
    }
};


int CollisionDetector::tri_tri_overlap_test_2d(double p1[2], double q1[2], double r1[2],
    double p2[2], double q2[2], double r2[2]) {
    if (ORIENT_2D(p1, q1, r1) < 0.0f)
        if (ORIENT_2D(p2, q2, r2) < 0.0f)
            return ccw_tri_tri_intersection_2d(p1, r1, q1, p2, r2, q2);
        else
            return ccw_tri_tri_intersection_2d(p1, r1, q1, p2, q2, r2);
    else
        if (ORIENT_2D(p2, q2, r2) < 0.0f)
            return ccw_tri_tri_intersection_2d(p1, q1, r1, p2, r2, q2);
        else
            return ccw_tri_tri_intersection_2d(p1, q1, r1, p2, q2, r2);

};
