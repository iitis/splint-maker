#include "Siateczka1.h"
#include "Model3D.h"
#include "Workspace.h"
#include "KDNode2.h"
#include "AnnotationPoint.h"

#include "../api/AP.h"
#include "../api/UI.h"

void CSiateczka1::close(int id, CSiateczka1 *s)
{
	if (NULL != s)
	{
		AP::WORKSPACE::removeModel(id);
	}
}

CSiateczka1::CSiateczka1( unsigned int sizeXmm , unsigned int sizeYmm, unsigned int sizeZmm, unsigned int divider )
{
	obj = std::make_shared<CModel3D>();

	obj->addChild(obj, std::make_shared<CMesh>());

	int tmpX = (sizeXmm+1) / 2;
	if ( tmpX % 2) tmpX++;

	int tmpY = (sizeYmm+1) / 2;
	if ( tmpY % 2) tmpY++;

	int tmpZ = sizeZmm;
	if (tmpZ % 2) tmpZ++;

	div = std::min( 50.0, double(divider) );

	pMin = CPoint3d( double(-tmpX), double(-tmpY),  0.0 );
	pMax = CPoint3d( double( tmpX), double( tmpY), double( tmpZ) );

	sMinX = pMin.X()*div;
	sMaxX = pMax.X()*div;
	sMinY = pMin.Y()*div;
	sMaxY = pMax.Y()*div;

	obj->setMin(pMin);
	obj->setMax(pMax);
}




float CSiateczka1::testRay(CPoint3d p0, CPoint3d p1, std::shared_ptr<CModel3D> o)
{
	double wx = 0.0, wy = 0.0, wz = 0.0;
	double wx0 = 0.0, wy0 = 0.0, wz0 = 0.0;

	// punkt "pocz�tkowy"
	CPoint3f pkt0(o->getTransform().w2l(p0));

	// punkt "ko�cowy"
	CPoint3f pkt1(o->getTransform().w2l(p1));


	CVector3f vRay(pkt0, pkt1);
	vRay.normalize();

	CPoint3f IntersectionPoint;
	INDEX_TYPE faceIdx;

	std::shared_ptr<CMesh> mesh = std::dynamic_pointer_cast<CMesh>(o->getData());

	if (mesh->getClosestFace(pkt0, vRay, /*ref*/IntersectionPoint, /*ref*/faceIdx))
	{
		CAnnotationPoint pt(IntersectionPoint);

		pt.setParent(o);

		pt.setRay(vRay);

		pt.setFace(
			faceIdx,
			mesh->vertices()[mesh->faces()[faceIdx].A()],
			mesh->vertices()[mesh->faces()[faceIdx].B()],
			mesh->vertices()[mesh->faces()[faceIdx].C()]);

		CVector3d w(pkt0, IntersectionPoint);

		return w.length();
	}
	else
	{
		return 1000000.0f;
	}
}


void CSiateczka1::getTrueMinMax(std::shared_ptr<CModel3D> o, CPoint3d &zMin, CPoint3d &zMax)
{
	std::shared_ptr<CMesh> zeby = std::dynamic_pointer_cast<CMesh>(o->getData());

	if (zeby->vertices().size() > 0)
	{
		CPoint3d v = zeby->vertices()[0];

		zMin = zMax = obj->getTransform().w2l( o->getTransform().l2w( v ));

		for (int i = 1; i < zeby->vertices().size(); i++)
		{
			v = zeby->vertices()[i];

			CPoint3d t = obj->getTransform().w2l( o->getTransform().l2w( v ));
			
			zMin.SetIfSmaller(t.X(), t.Y(), t.Z());
			zMax.SetIfBigger(t.X(), t.Y(), t.Z());
		}
	}

}



void CSiateczka1::tworzMesh(std::shared_ptr<CMesh> rzutnia)
{
	rzutnia->vertices().resize((sMaxX - sMinX + 2) * (sMaxY - sMinY + 2));

	for (int i = sMinX; i <= sMaxX; i += 2)
		for (int j = sMinY; j <= sMaxY; j += 2)
		{
			double x0 = (double)i / div;
			double x1 = (double)(i + 1) / div;
			double y0 = (double)j / div;
			double y1 = (double)(j + 1) / div;

			size_t index00 = vIndex(i, j);
			size_t index01 = vIndex(i, j + 1);
			size_t index11 = vIndex(i + 1, j + 1);
			size_t index10 = vIndex(i + 1, j);

			rzutnia->vertices()[index00] = CVertex(x0, y0, (double)pMin.Z());
			rzutnia->vertices()[index01] = CVertex(x0, y1, (double)pMin.Z());
			rzutnia->vertices()[index11] = CVertex(x1, y1, (double)pMin.Z());
			rzutnia->vertices()[index10] = CVertex(x1, y0, (double)pMin.Z());

			rzutnia->faces().push_back(CFace(index01, index00, index11));
			rzutnia->faces().push_back(CFace(index10, index11, index00));

			rzutnia->fnormals().push_back(CVector3d(0.0, 0.0, 1.0));
			rzutnia->fnormals().push_back(CVector3d(0.0, 0.0, 1.0));

			if (i > sMinX)
			{
				size_t index21 = vIndex(i - 1, j + 1);
				size_t index20 = vIndex(i - 1, j);

				rzutnia->faces().push_back(CFace(index21, index20, index01));
				rzutnia->faces().push_back(CFace(index00, index01, index20));

				rzutnia->fnormals().push_back(CVector3d(0.0, 0.0, 1.0));
				rzutnia->fnormals().push_back(CVector3d(0.0, 0.0, 1.0));
			}

			if ((i > sMinX) && (j > sMinY))
			{
				size_t index22 = vIndex(i - 1, j - 1);
				size_t index20 = vIndex(i - 1, j);

				size_t index02 = vIndex(i, j - 1);

				rzutnia->faces().push_back(CFace(index20, index22, index02));
				rzutnia->faces().push_back(CFace(index02, index00, index20));

				rzutnia->fnormals().push_back(CVector3d(0.0, 0.0, 1.0));
				rzutnia->fnormals().push_back(CVector3d(0.0, 0.0, 1.0));
			}

			if (j > sMinY)
			{
				size_t index02 = vIndex(i, j - 1);
				size_t index12 = vIndex(i + 1, j - 1);

				rzutnia->faces().push_back(CFace(index00, index02, index12));
				rzutnia->faces().push_back(CFace(index12, index10, index00));

				rzutnia->fnormals().push_back(CVector3d(0.0, 0.0, 1.0));
				rzutnia->fnormals().push_back(CVector3d(0.0, 0.0, 1.0));
			}

		}
}

void CSiateczka1::zbudujSiatke(std::shared_ptr<CModel3D> zebyObj)
{
	int margin = 10*div;

	CPoint3d zebyMin, zebyMax;
	
	getTrueMinMax( zebyObj, zebyMin, zebyMax );

	sMinX = floor(div * zebyMin.X()) - margin;
	if (sMinX % 2) sMinX -= 1;

	sMaxX = ceil(div * zebyMax.X()) + margin;
	if (sMaxX % 2) sMaxX += 1;

	sMinY = floor(div * zebyMin.Y()) - margin;
	if (sMinY % 2) sMinY -= 1;
	
	sMaxY = ceil(div * zebyMax.Y()) + margin;
	if (sMaxY % 2) sMaxY += 1;

	pMin.X((double)sMinX / div);
	pMin.Y((double)sMinY / div);
	pMax.X((double)sMaxX / div);
	pMax.Y((double)sMaxY / div);

	std::shared_ptr<CMesh> rzutnia = std::dynamic_pointer_cast<CMesh>(obj->getChild());

	rzutnia->faces().clear();
	rzutnia->vertices().clear();
	rzutnia->vnormals().clear();
	rzutnia->fnormals().clear();

	tworzMesh(rzutnia);

	//szczeka_obj->removeChild(0);
	//rzutnia = CMesh::createPrimitivePlane(sMaxX - sMinX + 2, sMaxY - sMinY + 2, 1, 1);
	//szczeka_obj->addChild(rzutnia);

	rzutnia->getMaterial().FrontColor.ambient.SetFloat(0.6f, 0.6f, 0.5f, 0.95f);
	rzutnia->getMaterial().FrontColor.diffuse.SetFloat(0.6f, 0.6f, 0.5f, 0.95f);
	rzutnia->getMaterial().FrontColor.emission.SetFloat(0.1f, 0.1f, 0.1f, 0.0f);

	rzutnia->setMin(pMin);
	rzutnia->setMax(pMax);
	obj->setMin(pMin);
	obj->setMax(pMax);

	UI::updateAllViews();
}


// o1 - to jest rozepchana szczęka
// o2 - to jest powierzchnia okluzji
void CSiateczka1::zrobWyciskSumy4(std::shared_ptr<CModel3D> o1, std::shared_ptr<CModel3D> o2, bool test2)
{
//	unsigned long t1 = GetTickCount();

	std::shared_ptr<CMesh> rzutnia = std::dynamic_pointer_cast<CMesh>(obj->getData());

	std::shared_ptr<CMesh> zeby1 = std::dynamic_pointer_cast<CMesh>(o1->getData());

	CTransform trans10 = CTransform::fromTo(o1->getTransform(), obj->getTransform());

	// SIATKA SZCZEKI (ROZEPCHANA)
	for (int i = 0; i < zeby1->faces().size(); i++)
	{
		//UI::STATUSBAR::printfTimed(1000, L"Tworzę wycisk 1. Zostało %d ścian.", i);

		CTriangle t = CTriangle(i, zeby1.get()).transformByMatrix(trans10.toEigenMatrix4d());

		CPoint3d tmin = t.getBoundingBox().getMin();
		CPoint3d tmax = t.getBoundingBox().getMax();

		int rMinX = std::max((int)floor(div * tmin.x - 1.0), sMinX);
		int rMinY = std::max((int)floor(div * tmin.y - 1.0), sMinY);

		int rMaxX = std::min((int)ceil(div * tmax.x + 1.0), sMaxX);
		int rMaxY = std::min((int)ceil(div * tmax.y + 1.0), sMaxY);

		for (int iy = rMinY; iy <= rMaxY; iy++)
		{
			double y = (double)iy / div;

			for (int ix = rMinX; ix <= rMaxX; ix++)
			{
				double x = (double)ix / div;

				CTriple<double> p0r(x, y, (double)pMax.Z());
				CVector3d vRayR(0.0, 0.0, -1.0);

				CPoint3d pIP;
				double odl;

				int res = CMesh::rayTriangleIntersect3D(t[0], t[1], t[2], CVector3d(), vRayR, p0r, pIP, odl);

				if (res == 1)
				{
					if (siateczka.end() == siateczka.find(std::pair<int, int>(ix, iy)))
					{
						siateczka[std::pair<int, int>(ix, iy)] = &rzutnia->vertices()[vIndex(ix, iy)];
						siateczka[std::pair<int, int>(ix, iy)]->Z(pIP.Z());
					}
					else if (siateczka[std::pair<int, int>(ix, iy)]->Z() < pIP.Z())
					{
						siateczka[std::pair<int, int>(ix, iy)]->Z(pIP.Z());
					}
				}
			}
		}
	}

	if (o2 != nullptr)
	{
		std::shared_ptr<CMesh> zeby2 = std::dynamic_pointer_cast<CMesh>(o2->getData());

		CTransform trans20 = CTransform::fromTo(o2->getTransform(), obj->getTransform());

		// SIATKA OKLUZJI
		for (int i = 0; i < zeby2->faces().size(); i++)
		{
			//UI::STATUSBAR::printfTimed(1000, L"Tworzę wycisk 2. Zostało %d ścian.", i);

			CTriangle t = CTriangle(i, zeby2.get()).transformByMatrix(trans20.toEigenMatrix4d());

			CPoint3d tmin = t.getBoundingBox().getMin();
			CPoint3d tmax = t.getBoundingBox().getMax();

			int rMinX = std::max((int)floor(div * tmin.x - 1.0), sMinX);
			int rMinY = std::max((int)floor(div * tmin.y - 1.0), sMinY);

			int rMaxX = std::min((int)ceil(div * tmax.x + 1.0), sMaxX);
			int rMaxY = std::min((int)ceil(div * tmax.y + 1.0), sMaxY);

			for (int iy = rMinY; iy <= rMaxY; iy++)
			{
				double y = (double)iy / div;

				for (int ix = rMinX; ix <= rMaxX; ix++)
				{
					double x = (double)ix / div;

					CTriple<double> p0r(x, y, (double)pMax.Z());
					CVector3d vRayR(0.0, 0.0, -1.0);

					CPoint3d pIP;
					double odl;

					int res = CMesh::rayTriangleIntersect3D(t[0], t[1], t[2], CVector3d(), vRayR, p0r, pIP, odl);

					if (res == 1)
					{
						if (siateczka.end() == siateczka.find(std::pair<int, int>(ix, iy)))
						{
							siateczka[std::pair<int, int>(ix, iy)] = &rzutnia->vertices()[vIndex(ix, iy)];
							siateczka[std::pair<int, int>(ix, iy)]->Z(pIP.Z());
						}
						else if (test2 || (siateczka[std::pair<int, int>(ix, iy)]->Z() < pIP.Z()))
						{
							siateczka[std::pair<int, int>(ix, iy)]->Z(pIP.Z());
						}
					}
				}
			}
		}
	}

//	UI::STATUSBAR::printf(L"Wycisk gotowy. Czas wykonania: %d ms", GetTickCount() - t1);

	UI::updateAllViews();
}




// o1 - to jest rozepchana szczęka
// o2 - to jest powierzchnia okluzji
void CSiateczka1::zrob_wycisk_z_dziurom(std::shared_ptr<CModel3D> o1, std::shared_ptr<CModel3D> o2)
{
	if (o1 == nullptr) {
		return;
	}

	if (o2 == nullptr) {
		return;
	}

//	unsigned long t1 = GetTickCount();

	std::shared_ptr<CMesh> rzutnia = std::dynamic_pointer_cast<CMesh>(obj->getData());
	std::shared_ptr<CMesh> zeby1 = std::dynamic_pointer_cast<CMesh>(o1->getData());

	CTransform trans10 = CTransform::fromTo(o1->getTransform(), obj->getTransform());

	// SIATKA SZCZEKI (ROZEPCHANA)
	for (int i = 0; i < zeby1->faces().size(); i++)
	{
		//UI::STATUSBAR::printfTimed(1000, L"Tworzę wycisk 1. Zostało %d ścian.", i);

		CTriangle t = CTriangle(i, zeby1.get()).transformByMatrix(trans10.toEigenMatrix4d());

		CPoint3d tmin = t.getBoundingBox().getMin();
		CPoint3d tmax = t.getBoundingBox().getMax();

		int rMinX = std::max((int)floor(div * tmin.x - 1.0), sMinX);
		int rMinY = std::max((int)floor(div * tmin.y - 1.0), sMinY);

		int rMaxX = std::min((int)ceil(div * tmax.x + 1.0), sMaxX);
		int rMaxY = std::min((int)ceil(div * tmax.y + 1.0), sMaxY);

		for (int iy = rMinY; iy <= rMaxY; iy++)
		{
			double y = (double)iy / div;

			for (int ix = rMinX; ix <= rMaxX; ix++)
			{
				double x = (double)ix / div;

				CTriple<double> p0r(x, y, (double)pMax.Z());
				CVector3d vRayR(0.0, 0.0, -1.0);

				CPoint3d pIP;
				double odl;

				int res = CMesh::rayTriangleIntersect3D(t[0], t[1], t[2], CVector3d(), vRayR, p0r, pIP, odl);

				if (res == 1)
				{
					if (siateczka.end() == siateczka.find(std::pair<int, int>(ix, iy)))
					{
						siateczka[std::pair<int, int>(ix, iy)] = &rzutnia->vertices()[vIndex(ix, iy)];
						siateczka[std::pair<int, int>(ix, iy)]->Z(pIP.Z());
					}
					else if (siateczka[std::pair<int, int>(ix, iy)]->Z() < pIP.Z())
					{
						siateczka[std::pair<int, int>(ix, iy)]->Z(pIP.Z());
					}
				}
			}
		}
	}

	std::shared_ptr<CMesh> zeby2 = std::dynamic_pointer_cast<CMesh>(o2->getData());

	CTransform trans20 = CTransform::fromTo(o2->getTransform(), obj->getTransform());

	// SIATKA OKLUZJI
	for (int i = 0; i < zeby2->faces().size(); i++)
	{
		//UI::STATUSBAR::printfTimed(1000, L"Tworzę wycisk 2. Zostało %d ścian.", i);

		CTriangle t = CTriangle(i, zeby2.get()).transformByMatrix(trans20.toEigenMatrix4d());

		CPoint3d tmin = t.getBoundingBox().getMin();
		CPoint3d tmax = t.getBoundingBox().getMax();

		int rMinX = std::max((int)floor(div * tmin.x - 1.0), sMinX);
		int rMinY = std::max((int)floor(div * tmin.y - 1.0), sMinY);

		int rMaxX = std::min((int)ceil(div * tmax.x + 1.0), sMaxX);
		int rMaxY = std::min((int)ceil(div * tmax.y + 1.0), sMaxY);

		for (int iy = rMinY; iy <= rMaxY; iy++)
		{
			double y = (double)iy / div;

			for (int ix = rMinX; ix <= rMaxX; ix++)
			{
				double x = (double)ix / div;

				CTriple<double> p0r(x, y, (double)pMax.Z());
				CVector3d vRayR(0.0, 0.0, -1.0);

				CPoint3d pIP;
				double odl;

				int res = CMesh::rayTriangleIntersect3D(t[0], t[1], t[2], CVector3d(), vRayR, p0r, pIP, odl);

				if (res == 1)
				{
					if (siateczka.end() == siateczka.find(std::pair<int, int>(ix, iy)))
					{
						siateczka[std::pair<int, int>(ix, iy)] = &rzutnia->vertices()[vIndex(ix, iy)];
						siateczka[std::pair<int, int>(ix, iy)]->z = pMin.z-1;
					}
					else // if (siateczka[std::pair<int, int>(ix, iy)]->Z() < pIP.Z())
					{
						siateczka[std::pair<int, int>(ix, iy)]->z = pMin.z-1;
					}
				}
			}
		}
	}
	

	// UWAGA - to desynchronizuje siateczkę, uzyć tylko jako ostatni wycisk

	CMesh::Faces temp;
	std::set<std::pair<int, int>> toRemove;

	for (int j = 0; j < rzutnia->faces().size(); j++) {
		CFace &f = rzutnia->faces()[j]; 
		CVertex vA = rzutnia->vertices()[f.A()];
		CVertex vB = rzutnia->vertices()[f.B()];
		CVertex vC = rzutnia->vertices()[f.C()];
		if ((vA.z < pMin.z) || (vB.z < pMin.z) || (vC.z < pMin.z)) {
			if (vA.z < pMin.z) toRemove.insert(std::pair<int, int>(vA.x, vA.y));
			if (vB.z < pMin.z) toRemove.insert(std::pair<int, int>(vB.x, vB.y));
			if (vC.z < pMin.z) toRemove.insert(std::pair<int, int>(vC.x, vC.y));
		}
		else {
			temp.push_back(f);
			//rzutnia->faces().erase(rzutnia->faces().begin() + j);
		}
	}

	rzutnia->faces() = temp;

	for (auto p : toRemove) {
		siateczka.erase(p);
	}

	rzutnia->removeUnusedVertices();


//	UI::STATUSBAR::printf(L"Wycisk gotowy. Czas wykonania: %d ms", GetTickCount() - t1);

	UI::updateAllViews();
}








void CSiateczka1::zrobWyciskZuchwy_v1(std::shared_ptr<CModel3D> o4, double _distMax, double ndir, std::set<CVertex>* bledy, std::set<int>* bledy2)
{
//	unsigned long t1 = GetTickCount();

	std::shared_ptr<CMesh> zeby4 = std::dynamic_pointer_cast<CMesh>(o4->getData());

	Eigen::Matrix4d M = CTransform::fromTo(o4->getTransform(), obj->getTransform()).toEigenMatrix4d();

	o4->applyTransformation(o4->getTransform(), obj->getTransform());
	o4->setTransform(obj->getTransform());

	KDNode2* tree = KDNode2::build(zeby4.get());
	KDNode2::HitMap hmap;

	std::shared_ptr<CMesh> rzutnia = std::dynamic_pointer_cast<CMesh>(obj->getData());
	rzutnia->calcVN();

	CPoint3d mid = rzutnia->getCenterOfWeight();

	for (int i = 0; i < rzutnia->vertices().size(); i++) {
		//UI::STATUSBAR::printfTimed(1000, L"Tworzę wycisk zuchwy. Wertex %d z %d.", i, rzutnia->vertices().size());

		CVertex p0 = rzutnia->vertices()[i];

		KDNode2* ptr = tree;

		double minDist = DBL_MAX;
		double minDir = 0.0;
		CVector3d minV;

		bool found = false;

		CVector3d vr = rzutnia->vnormals()[i];
		//CVector3d vr(0,0,) = rzutnia->vnormals()[i];

		CVector3d mv(0, 0, -1);
		CPoint3d p0mv = p0 - mv;

		hmap.clear();
		bool hit = tree->hit(zeby4.get(), p0mv, mv, hmap);

		if (hit) {
			//qInfo() << "hmap size = " << hmap.size();

			double dist = (*hmap.begin()).second.first;
			CPoint3d p1 = (*hmap.begin()).second.second;
			int idx = (*hmap.begin()).first;

			for (auto h : hmap) {
				double dd = h.second.first;

				if (dd < dist) {
					dist = dd;
					p1 = h.second.second;
					idx = h.first;
				}
			}

			if (dist >= 1.0 + _distMax)
			{
				qInfo() << "i = " << i << " dist = " << dist;

				rzutnia->vertices()[i] = p1;
				//found = true;
				//break;
				if (bledy) {
					bledy->insert(p0);
				}

				if (bledy2) {
					bledy2->insert(idx);
				}
			}
		}

	}


//	UI::STATUSBAR::printf(L"Wycisk zuchwy gotowy. Czas wykonania: %d ms", GetTickCount() - t1);

	UI::updateAllViews();
}



void CSiateczka1::zrobWycisk2(std::shared_ptr<CModel3D> zebyObj)
{
//	unsigned long t1 = GetTickCount();

	qInfo() << "CSiateczka1::zrobWycisk2()";

	std::shared_ptr<CMesh> zeby = std::dynamic_pointer_cast<CMesh>(zebyObj->getData());
	std::shared_ptr<CMesh> rzutnia = std::dynamic_pointer_cast<CMesh>(obj->getData());

	if (!zeby || !rzutnia) qInfo() << "BLAD";

	for (int i = 0; i < zeby->faces().size(); i++)
	{
		//UI::STATUSBAR::printfTimed(1000, L"Tworzę wycisk. Zostało %d ścian.", i );
		// UI::STATUSBAR::printf(L"Tworzę wycisk. Zostało %d ścian.", i );

		CFace f = zeby->faces().at(i);
		
		CVertex za = zeby->vertices().at(f.A());
		CVertex zb = zeby->vertices().at(f.B());
		CVertex zc = zeby->vertices().at(f.C());

		CPoint3d a = obj->getTransform().w2l( zebyObj->getTransform().l2w( za ) );
		CPoint3d b = obj->getTransform().w2l( zebyObj->getTransform().l2w( zb ) );
		CPoint3d c = obj->getTransform().w2l( zebyObj->getTransform().l2w( zc ) );

		int rMinX = std::max( (int) floor( div * MIN3(a.X(), b.X(), c.X()) - 1.0 ), sMinX );
		int rMinY = std::max( (int) floor( div * MIN3(a.Y(), b.Y(), c.Y()) - 1.0 ), sMinY );

		int rMaxX = std::min( (int) ceil( div * MAX3(a.X(), b.X(), c.X()) + 1.0 ), sMaxX );
		int rMaxY = std::min( (int) ceil( div * MAX3(a.Y(), b.Y(), c.Y()) + 1.0 ), sMaxY );


		for (int iy = rMinY; iy <= rMaxY; iy++)
		{

			double y = (double)iy / div;

			for (int ix = rMinX; ix <= rMaxX; ix++)
			{
				double x = (double)ix / div;


				CPoint3d p0r( x, y, (double)pMax.Z() );

				CVector3d vRayR( 0.0, 0.0, -1.0 );

				CPoint3d pIP;
				double odl;

				int res = CMesh::rayTriangleIntersect3D( a, b, c, CVector3d(), vRayR, p0r, pIP, odl );

				if (res == 1)
				{
					if (siateczka.end() == siateczka.find(std::pair<int, int>(ix, iy)))
					{
						siateczka[std::pair<int, int>(ix, iy)] = &rzutnia->vertices()[ vIndex(ix, iy) ];
						siateczka[std::pair<int, int>(ix, iy)]->z = pIP.z;
					}
					else if (siateczka[std::pair<int, int>(ix, iy)]->z < pIP.z )
					{
						siateczka[std::pair<int, int>(ix, iy)]->z = pIP.z;
					}

					//fprintf( plik, "%d, %d : %lf\n", ix, iy, pIP.Z() );
				}
			}
		}
	}
	//fclose(plik);

//	UI::STATUSBAR::printf(L"Wycisk gotowy. Czas wykonania: %d ms", GetTickCount() - t1);


	UI::updateAllViews();
}



bool CSiateczka1::flood( int ix, int iy, int limit, std::set<std::pair<int,int>> *dziura )
{
	// ten w�ze� ju� jest na li�cie
	if (dziura->end() != dziura->find(std::pair<int, int>(ix, iy))) return true;

	// ograniczam "od g�ry" dopuszczaln� wielko�� dziury
	bool test = (dziura->size() <= limit);

	if (test)
	{
		// szukam w�z�a w siateczce
		std::map< std::pair<int, int>, CVertex* >::iterator it = siateczka.find(std::pair<int, int>(ix, iy));

		// nie ma w�z�a => jest tu dziura lub cz�� dziury
		if (it == siateczka.end())
		{
			dziura->insert(std::pair<int, int>(ix, iy));

			test = (dziura->size() <= limit);

			// je�li nie przekroczy�em limitu, to sprawdzam s�siad�w:
			if (test && (ix < sMaxX)) test = flood(ix + 1, iy, limit, dziura);

			if (test && (iy < sMaxY)) test = flood(ix, iy + 1, limit, dziura);

			if (test && (ix > sMinX)) test = flood(ix - 1, iy, limit, dziura);

			if (test && (iy > sMinY)) test = flood(ix, iy - 1, limit, dziura);
		}
	}
	return test;
}


void CSiateczka1::klejDziury3()
{
	std::shared_ptr<CMesh> rzutnia = std::dynamic_pointer_cast<CMesh>(obj->getData());

	std::map< std::pair<int, int>, CVertex* >::iterator it;
	std::set<std::pair<int, int>> dziura;

	//FILE * plik = fopen("siateczka_dziury.txt", "w");

	int limit = 10;

	for (int iy = sMinY; iy <= sMaxY; iy++)
	{
		double y = (double)iy / div;

		for (int ix = sMinX; ix <= sMaxX; ix++)
		{
			double x = (double)ix / div;

			//UI::STATUSBAR::printfTimed(1000, L"Kleję dziury (v.3) [ x:%d, y:%d ]", ix, iy);

			dziura.clear();

			if (flood(ix, iy, limit, &dziura))
			{
				size_t s = dziura.size();
				std::set<std::pair<int, int>>::iterator di;

				if ((s > 0) && (s <= limit))
				{
					for (di = dziura.begin(); di != dziura.end(); di++)
					{
						it = siateczka.find(*di);

						if (siateczka.end() == it)
						{
							siateczka[*di] = &rzutnia->vertices()[vIndex(di->first, di->second)];
						}

						if (ix > sMinX) siateczka[*di]->Z(siateczka[std::pair<int, int>(ix - 1, iy)]->Z());
						else siateczka[*di]->Z(10);
					}
				}
			}
		}
	}
	//fclose(plik);

	UI::updateAllViews();

	rzutnia->correctNormals();
	//UI::STATUSBAR::printf(L"Dziury zostały zaklejone.");

	UI::updateAllViews();
}



void CSiateczka1::odwrocNormalne()
{
	std::shared_ptr<CMesh> m = std::dynamic_pointer_cast<CMesh>(obj->getChild());

	m->correctNormals();
	m->invertNormals();

	//UI::STATUSBAR::printf(L"Normalne ścian zostały odwrócone");

	UI::updateAllViews();
}



void CSiateczka1::usunNadmiaroweScianki()
{
	std::shared_ptr<CMesh> rzutnia = std::dynamic_pointer_cast<CMesh>(obj->getData());

	double prog = pMin.Z() + 0.1;

	std::vector<CFace> newFaces;
	std::vector<CVector3f> newNormals;

	qInfo() << rzutnia->faces().size();

	for (int i = rzutnia->faces().size() - 1; i >= 0; i--)
	{
		//UI::STATUSBAR::printfTimed( 1000, L"Usuwam niepotrzebne scianki. Zostało:%d", i);

		CFace f = rzutnia->faces()[i];

		if ((rzutnia->vertices()[f.A()].Z() > prog) || (rzutnia->vertices()[f.B()].Z() > prog) || (rzutnia->vertices()[f.C()].Z() > prog))
		//if ( (rzutnia->vertices()[f.A()].Z() > prog) && (rzutnia->vertices()[f.B()].Z() > prog) && (rzutnia->vertices()[f.C()].Z() > prog) )
		{
			newFaces.push_back(f);
			newNormals.push_back(f.getNormal(rzutnia->vertices()));

			//rzutnia->faces().erase(rzutnia->faces().begin() + i);
			//rzutnia->fnormals().erase(rzutnia->fnormals().begin() + i);
		}
	}

	qInfo() << "TEST4";

	rzutnia->faces() = std::vector<CFace>(newFaces);
	rzutnia->fnormals() = std::vector<CVector3f>(newNormals);

	rzutnia->removeUnusedVertices();

	//UI::STATUSBAR::printf(L"Niepotrzebne ścianki i wierzchołki zostały usuniete.");

	UI::updateAllViews();
}

