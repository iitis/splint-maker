#pragma once

#include "Workspace.h"
#include "../api/AP.h"

#include "Siateczka1.h"
//#include "Siateczka2.h"

#include "Szczeka.h"

class CMesh;

class CSymulatorZgryzu
{
public:
	CSzczeka szczeka;
	CSzczeka zuchwa;

	CSiateczka1* wierzch;
	CSiateczka1* wnetrze;

	std::shared_ptr<CMesh> mBD;
	std::shared_ptr<CMesh> mZD;

	int m_divider;

	CSymulatorZgryzu(void) {};
	~CSymulatorZgryzu(void);

	void generujWnetrzeNEW(CSiateczka1* wnetrze, float d = 0.2f);
	void generujWnetrze( CSiateczka1 * wnetrze, float d = 0.2f );

	void generujWierzchOdRazu3_CSiateczka1_v2(double dVal, std::shared_ptr<CMesh> zuch);
	void generujWierzchOdRazu3_CSiateczka1(double dVal, std::shared_ptr<CMesh> zuch);

	void usunZeby();
};
