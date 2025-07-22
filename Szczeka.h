#pragma once

#include <string>
#include <map>
#include "Triple.h"
#include "Mesh.h"

class CModel3D;

class CSzczeka
{
public:
	std::shared_ptr<CModel3D> obj;
	std::shared_ptr<CModel3D> zeby;

	std::shared_ptr<CModel3D> oryginalneOdcieteZeby;

	std::shared_ptr<CModel3D> okluzja;

	std::map<std::pair<int,int>,double> mapaOkluzji;	 // < < int x, int y>, double z >

	CSzczeka(void);
	~CSzczeka(void);

	void inicjuj2(std::shared_ptr<CMesh> mesh);

	void wytnijZebyNEW(std::shared_ptr<CPlane> cutPlane);

	int nalezyDoPowierzchniOkluzji( CTriple<double> );

	void tworzMapeOkluzji2(std::shared_ptr<CMesh> mesh, CTransform tFrom = CTransform(), CTransform tTo = CTransform());

	void wytnijZeby(std::shared_ptr<CModel3D> obP );
	void rozepchajZeby( float d, bool wierzch = false );
	void rozepchajZebyRegular(float d);
};

