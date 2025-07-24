#pragma once

#include "Workspace.h"
//#include "../api/AP.h"
#include "AP.h"

#include "Siateczka1.h"

class CMesh;

class BiteSim
{
public:
	std::shared_ptr<CModel3D> szczeka_obj;
	std::shared_ptr<CModel3D> szczeka_zeby;
	std::shared_ptr<CModel3D> szczeka_oryginalneOdcieteZeby;
	std::shared_ptr<CModel3D> szczeka_okluzja;
	std::map<std::pair<int, int>, double> szczeka_mapaOkluzji;	 // < < int x, int y>, double z >

	void szczeka_inicjuj2(std::shared_ptr<CMesh> mesh);
	
	void szczeka_wytnijZebyNEW(std::shared_ptr<CPlane> cutPlane);
	

	int szczeka_nalezyDoPowierzchniOkluzji(CTriple<double>);
	void szczeka_tworzMapeOkluzji2(std::shared_ptr<CMesh> mesh, CTransform tFrom = CTransform(), CTransform tTo = CTransform());
	void szczeka_rozepchajZeby(float d, bool wierzch = false);
	
	void szczeka_rozepchajZebyRegular(float d);

	CSiateczka1* wierzch;
	CSiateczka1* wnetrze;

	//std::shared_ptr<CMesh> mBD;
	//std::shared_ptr<CMesh> mZD;

	int m_divider;

	BiteSim(void) {};
	~BiteSim(void) {};

	void create_inner_surface(CSiateczka1* wnetrze, float d = 0.2f);
	void create_outer_surface(double dVal, std::shared_ptr<CMesh> zuch);
};
