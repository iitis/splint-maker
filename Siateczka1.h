#pragma once

#include "Punkt3D.h"
#include <map>
#include <set>
#include <memory>

class CVertex;
class CModel3D;
class CMesh;

class CSiateczka1
{
public:
	std::shared_ptr<CModel3D> obj;

	CPoint3d pMin, pMax;

	int sMinX, sMaxX;
	int sMinY, sMaxY;

	double div;

	std::map< std::pair<int, int>, CVertex * > siateczka; // indeks*10
	std::vector<float> stareZ;

	float testRay(CPoint3d p0, CPoint3d p1, std::shared_ptr<CModel3D> o);
	
	void getTrueMinMax(std::shared_ptr<CModel3D> o, CPoint3d &zMin, CPoint3d &zMax);

	void tworzMesh(std::shared_ptr<CMesh> rzutnia);

	void zbudujSiatke(std::shared_ptr<CModel3D> o);

	void zrobWycisk2(std::shared_ptr<CModel3D> o);

	void zrobWyciskSumy4(std::shared_ptr<CModel3D> o1, std::shared_ptr<CModel3D> o2 = nullptr, bool test2 = false);
	
	void zrob_wycisk_z_dziurom(std::shared_ptr<CModel3D> o1, std::shared_ptr<CModel3D> o2);

	void zrobWyciskZuchwy_v1(std::shared_ptr<CModel3D> zuchwa, double dist, double ndir = -1, std::set<CVertex>* bledy = nullptr, std::set<int>* bledy2 = nullptr);

	void odwrocNormalne();

	bool flood(int ix, int iy, int limit, std::set<std::pair<int, int>> *dziura);

	void klejDziury3();
	void usunNadmiaroweScianki();

	void usunSkrajneScianki();

	inline size_t vIndex(int x, int y)
	{
		return ( (x-sMinX) * ( sMaxY - sMinY + 2 ) + (y-sMinY) );
	};

	static void close(int id, CSiateczka1 *s);

	CSiateczka1( unsigned int, unsigned int, unsigned int, unsigned int);
	~CSiateczka1() {};
};

