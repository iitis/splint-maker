#pragma once

#include "Punkt3D.h"
#include <map>
#include <set>

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

	//std::map< std::pair<float, float>, float > siateczka;
	//std::map< std::pair<float, float>, CVertex * > siateczka;
	std::map< std::pair<int, int>, CVertex * > siateczka; // indeks*10
	std::vector<float> stareZ;

	//void set(CPoint3d v1, CPoint3d v2, CPoint3d v3, CPoint3d v4);
	/*void incX(float d);
	void incY(float d);
	void incZ(float d);*/

	float testRay(CPoint3d p0, CPoint3d p1, std::shared_ptr<CModel3D> o);
	
	void getTrueMinMax(std::shared_ptr<CModel3D> o, CPoint3d &zMin, CPoint3d &zMax);

	void tworzMesh(std::shared_ptr<CMesh> rzutnia);

	void zbudujSiatke(std::shared_ptr<CModel3D> o);

	//void zrobCienNaSiatce(CModel3D *o);

	void zrobWycisk(std::shared_ptr<CModel3D> o);
	void zrobWycisk2(std::shared_ptr<CModel3D> o);

	//void zrobWyciskSumy(CModel3D *o1, CModel3D *o2); // tu podajemy wyciski we wspó³rzêdnych siatki
	void zrobWyciskSumy4(std::shared_ptr<CModel3D> o1, std::shared_ptr<CModel3D> o2 = nullptr, bool test2 = false);
	
	void zrob_wycisk_z_dziurom(std::shared_ptr<CModel3D> o1, std::shared_ptr<CModel3D> o2);

	//void zrobWyciskSumy1111(CModel3D* o1);
	//void zrobWyciskSumy4_aaa(CModel3D* o1, CModel3D* o2, bool test2);
	//void zrobWyciskSumy5(CModel3D* o1, CModel3D* o2, CModel3D* o3, std::set<CVertex> *bledy = nullptr);
	//void zrobWyciskSumy6(CModel3D* o1, CModel3D* o2, CModel3D* o3, CModel3D* o4, std::set<CVertex>* bledy = nullptr);
	//void zrobWyciskZuchwy(CModel3D* zuchwa, double ndir = -1, std::set<CVertex>* bledy = nullptr, std::set<int>* bledy2 = nullptr);
	void zrobWyciskZuchwy_v1(std::shared_ptr<CModel3D> zuchwa, double dist, double ndir = -1, std::set<CVertex>* bledy = nullptr, std::set<int>* bledy2 = nullptr);
	//void zrobWyciskSumy3(CModel3D* o1, CModel3D* o2);
	//void zrobWyciskSumy2(CModel3D *o1, CModel3D *o2); // tu podajemy zêby we wspó³rzêdnych szczêki

	void odwrocNormalne();

	bool flood(int ix, int iy, int limit, std::set<std::pair<int, int>> *dziura);

	//void klejDziury();
	//void klejDziury2();
	void klejDziury3();
	void usunNadmiaroweScianki();

	inline size_t vIndex(int x, int y)
	{
		return ( (x-sMinX) * ( sMaxY - sMinY + 2 ) + (y-sMinY) );
	};

	static void close(int id, CSiateczka1 *s);

	CSiateczka1( unsigned int, unsigned int, unsigned int, unsigned int);
	~CSiateczka1() {};
};

