#include "Siateczka2.h"
#include "KDNode2.h"

#include "Model3D.h"
#include "Workspace.h"
#include "../api/AP.h"
#include "../api/UI.h"

CSiateczka2::CSiateczka2(unsigned int sizeXmm, unsigned int sizeYmm, unsigned int sizeZmm, unsigned int divider) : CSiateczka1(sizeXmm, sizeYmm, sizeZmm, divider)
{
	obj = std::make_shared<CModel3D>();

	obj->addChild(obj, std::make_shared<CMesh>());

	int tmpX = (sizeXmm + 1) / 2;
	if (tmpX % 2) tmpX++;

	int tmpY = (sizeYmm + 1) / 2;
	if (tmpY % 2) tmpY++;

	int tmpZ = sizeZmm;
	if (tmpZ % 2) tmpZ++;

	div = std::min(50.0, double(divider));

	pMin = CPoint3d(double(-tmpX), double(-tmpY), 0.0);
	pMax = CPoint3d(double(tmpX), double(tmpY), double(tmpZ));

	sMinX = pMin.X() * div;
	sMaxX = pMax.X() * div;
	sMinY = pMin.Y() * div;
	sMaxY = pMax.Y() * div;

	obj->setMin(pMin);
	obj->setMax(pMax);
}



void CSiateczka2::removeZeroVertices(std::shared_ptr<CMesh> rzutnia) {
	int size = rzutnia->faces().size();

	std::set<int> to_remove;

#pragma omp parallel for
	for (int cnt = 0; cnt < size; cnt++)
	{
		if ((cnt % 1000) == 0) printf("SIATECZKA2: Usuwam ścianki na plaszczyxnie XY0. %d z %d\n", cnt, size);
		CFace f = rzutnia->faces()[cnt];

		CVertex& vA = rzutnia->vertices()[f.x];
		CVertex& vB = rzutnia->vertices()[f.y];
		CVertex& vC = rzutnia->vertices()[f.z];

		if ((vA.z <= pMin.z) && (vB.z <= pMin.z) && (vC.z <= pMin.z)) {
			to_remove.insert(cnt);
		}
	}

	for (std::set<int>::reverse_iterator rit = to_remove.rbegin(); rit != to_remove.rend(); rit++) {
		rzutnia->removeFace(*rit);
	}

	rzutnia->removeUnusedVertices();
}

void CSiateczka2::tworzMesh(std::shared_ptr<CMesh> rzutnia) {
	float width = pMax.x - pMin.x;
	float height = pMax.y - pMin.y;
	float a = 1.0 / div;
	float minZ = pMin.z;

	printf("SIATECZKA2: generuje siatke 2D\n");
	std::vector<Triangle2> grid2D = generateTriangularGrid2D(width, height, a);

	printf("SIATECZKA2: generuje mesh 3D\n");
	int cnt = 0;
	int size = grid2D.size();
	for (Triangle2 t : grid2D) {
		rzutnia->addFace(
			rzutnia->addVertex(pMin.x + (t.v1.x), pMin.y + (t.v1.y), minZ),
			rzutnia->addVertex(pMin.x + (t.v2.x), pMin.y + (t.v2.y), minZ),
			rzutnia->addVertex(pMin.x + (t.v3.x), pMin.y + (t.v3.y), minZ));
		if ((cnt % 1000) == 0) printf("SIATECZKA2: %d z %d\n", ++cnt, size);
	}

	rzutnia->removeDuplicateVertices();
	
	//removeZeroVertices(rzutnia);

	printf("SIATECZKA2: mesh utworzono\n");

}

void CSiateczka2::zbudujSiatke(std::shared_ptr<CModel3D> o) {
	int margin = 10 * div;

	CPoint3d zebyMin, zebyMax;

	getTrueMinMax(o, zebyMin, zebyMax);

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

	auto rzutnia = std::dynamic_pointer_cast<CMesh>(obj->getChild());

	rzutnia->faces().clear();
	rzutnia->vertices().clear();
	rzutnia->vnormals().clear();
	rzutnia->fnormals().clear();

	tworzMesh(rzutnia);

	//obj->removeChild(0);
	//rzutnia = CMesh::createPrimitivePlane(sMaxX - sMinX + 2, sMaxY - sMinY + 2, 1, 1);
	//obj->addChild(rzutnia);

	rzutnia->getMaterial().FrontColor.ambient.SetFloat(0.6f, 0.6f, 0.5f, 0.95f);
	rzutnia->getMaterial().FrontColor.diffuse.SetFloat(0.6f, 0.6f, 0.5f, 0.95f);
	rzutnia->getMaterial().FrontColor.emission.SetFloat(0.1f, 0.1f, 0.1f, 0.0f);

	rzutnia->setMin(pMin);
	rzutnia->setMax(pMax);
	obj->setMin(pMin);
	obj->setMax(pMax);

	UI::updateAllViews();
}

std::shared_ptr<CModel3D> CSiateczka2::daj_model() {
	auto backup = std::dynamic_pointer_cast<CModel3D>(obj->getCopy());
	usunNadmiaroweScianki();
	auto kopia = obj;
	obj = backup;
	return kopia;
}

void CSiateczka2::daj_mi_wycisk(std::shared_ptr<CModel3D> o1, int mode)
{
	unsigned long t1 = GetTickCount();

	auto rzutnia = std::dynamic_pointer_cast<CMesh>(obj->getData());

	auto _zeby1 = std::dynamic_pointer_cast<CMesh>(o1->getData());

	auto zeby1 = std::dynamic_pointer_cast<CMesh>(_zeby1->getCopy());

	zeby1->applyTransformation(o1->getTransform(), obj->getTransform());

	KDNode2 *kdnode = KDNode2::build(zeby1.get());

	int size = rzutnia->vertices().size();
	
	CVector3d vd1(0.3 / div, 0.3 / div, 0.0);
	CVector3d vd2(-0.3 / div, -0.3 / div, 0.0);

	#pragma omp parallel for
	for (int cnt = 0; cnt < size; cnt++)
	{
		CVertex& v = rzutnia->vertices()[cnt];

		if ((cnt % 10000) == 0) printf("SIATECZKA2: Tworze wycisk. %d z %d\n", cnt, size);

		CTriple<double> p0r((double)v.x, (double)v.y, 2.0 * pMax.z);
		CVector3d vRayR(0.0, 0.0, -1.0);

		std::set<double> sdbl;
		KDNode2::ShadeRec shr0, shr1, shr2;
		bool h0 = kdnode->hit(zeby1.get(), p0r, vRayR, shr0);
		if (h0) for (auto ff : shr0.fidxs) {
			sdbl.insert(ff.second.second.z);
		}
		
		//bool h1 = kdnode->hit(zeby1, p0r + vd1, vRayR, shr1);
		//if (h1) for (auto ff : shr1.fidxs) {
		//	sdbl.insert(ff.second.second.z);
		//}

		//bool h2 = kdnode->hit(zeby1, p0r + vd2, vRayR, shr2);
		//if (h2) for (auto ff : shr2.fidxs) {
		//	sdbl.insert(ff.second.second.z);
		//}

		if (!sdbl.empty()) {
			if (mode == 0) {
				double z = *std::prev(sdbl.end());
				v.z = z;
			}
			else if (mode > 0) {
				double z = *std::prev(sdbl.end());
				if (v.z < z) v.x = z;
			}
			else {
				//double z = *sdbl.begin();
				double z = *std::prev(sdbl.end());
				if (v.z > z) v.x = z;
			}

			sdbl.clear();
		}
	}

	delete kdnode;
	//delete zeby1;

	printf("SIATECZKA2: Wycisk gotowy\n");
	UI::STATUSBAR::printf(L"Wycisk gotowy. Czas wykonania: %d ms", GetTickCount() - t1);

	UI::updateAllViews();
}

inline void CSiateczka2::zrob_dziure(std::shared_ptr<CModel3D> o1)//, CModel3D* o2, bool test2)
{
	unsigned long t1 = GetTickCount();

	auto rzutnia = std::dynamic_pointer_cast<CMesh>(obj->getData());

	auto _zeby1 = std::dynamic_pointer_cast<CMesh>(o1->getData());

	auto zeby1 = std::dynamic_pointer_cast<CMesh>(_zeby1->getCopy());

	zeby1->applyTransformation(o1->getTransform(), obj->getTransform());

	KDNode2* kdnode = KDNode2::build(zeby1.get());

	int size = rzutnia->vertices().size();

#pragma omp parallel for
	for (int cnt = 0; cnt < size; cnt++)
	{
		CVertex& v = rzutnia->vertices()[cnt];

		if ((cnt % 10000) == 0) printf("SIATECZKA2: Robie dziure. %d z %d\n", cnt, size);

		CTriple<double> p0r((double)v.x, (double)v.y, pMax.z);
		CVector3d vRayR(0.0, 0.0, -1.0);

		KDNode2::ShadeRec shr;
		if (kdnode->hit(zeby1.get(), p0r, vRayR, shr)) {
			v.z = pMin.z;
		}
	}

	delete kdnode;
	//delete zeby1;

	printf("SIATECZKA2: Dziura gotowa\n");
	UI::STATUSBAR::printf(L"Dziura gotowa. Czas wykonania: %d ms", GetTickCount() - t1);

	UI::updateAllViews();
}
