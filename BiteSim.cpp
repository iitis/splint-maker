#include "BiteSim.h"
#include "AnnotationEdges.h"

#include "../api/UI.h"

void BiteSim::create_inner_surface(CSiateczka1* wnetrze, float d)
{
	if ((NULL == wnetrze) || (NULL == wnetrze->obj)) return;

	std::shared_ptr<CModel3D> obj = wnetrze->obj;
	obj->setName(L"wnetrze_" + std::to_wstring(d));

	obj->getData()->setLabel("wnetrze_mesh");


	szczeka_oryginalneOdcieteZeby = std::dynamic_pointer_cast<CModel3D>(szczeka_zeby->getCopy());

	szczeka_oryginalneOdcieteZeby->setName(L"wycięte zęby");
	//szczeka_oryginalneOdcieteZeby->setTransform(szczeka_zeby->getTransform());

	AP::WORKSPACE::addModel(szczeka_oryginalneOdcieteZeby);

	szczeka_rozepchajZeby(d);

	std::shared_ptr<CMesh> tmp = std::dynamic_pointer_cast<CMesh>(szczeka_zeby->getData()->getCopy());
	tmp->setLabel(QString("rozepch1"));
	tmp->addKeyword(QString("dVal=%1").arg(d));
	AP::WORKSPACE::addObject(tmp);
	QString val = QString::number(d).replace(".", "_");
	tmp->getParent()->setLabel(QString("rozepchane_%1").arg(val));

	szczeka_zeby->setLocked(true);

	wnetrze->zbudujSiatke(szczeka_zeby);

	wnetrze->zrobWycisk2(szczeka_zeby);

	wnetrze->klejDziury3();

	wnetrze->usunNadmiaroweScianki();

	wnetrze->odwrocNormalne();
}



void BiteSim::create_outer_surface(double dVal, std::shared_ptr<CMesh> zuch)
{
	zuch->setLabel("testowa zuchwa");

	wierzch = new CSiateczka1(80, 80, 30, m_divider); // arguments: sizeX, sizeY, depth, divider

	if ((NULL == wierzch) || (NULL == wierzch->obj)) return;

	wierzch->obj->setName(L"wierzch");
	wierzch->obj->getTransform() = wnetrze->obj->getTransform();
	wierzch->obj->getData()->setLabel("wierzch_mesh");
	AP::WORKSPACE::addModel(wierzch->obj);

	AP::WORKSPACE::removeModel(szczeka_zeby);

	szczeka_zeby = std::dynamic_pointer_cast<CModel3D>(szczeka_oryginalneOdcieteZeby->getCopy());
	szczeka_rozepchajZeby(dVal);

	szczeka_zeby->getData()->setLabel(QString("rozepch2"));
	szczeka_zeby->getData()->addKeyword(QString("dVal=%1").arg(dVal));
	QString val = QString::number(dVal).replace(".", "_");
	szczeka_zeby->setLabel(QString("rozepchane_%1").arg(val));
	AP::WORKSPACE::addModel(szczeka_zeby);


	wierzch->zbudujSiatke(szczeka_zeby);

	// wyciskam rozepchana powierzchnie szczęki + powierzchnie okluzji
	wierzch->zrobWyciskSumy4(szczeka_zeby, szczeka_okluzja, false);

	// wyciskam żuchwę, żeby usunąć potencjalnie wystajace fragmenty
	//wierzch->zrobWyciskZuchwy_v1((CModel3D*)zuch->getParent(), 0.0, 1);

	wierzch->klejDziury3();
	wierzch->usunNadmiaroweScianki();



	// *** AKTUALNIE INTERESUJE NAS TYLKO ŁĄCZNY WYCISK SZCZEKI+OKLUZJI
	// 
	//CSiateczka1* wierzch_bez_dziury = new CSiateczka1(80, 80, 30, m_divider); // arguments: sizeX, sizeY, depth, divider

	//if ((NULL == wierzch_bez_dziury) || (NULL == wierzch_bez_dziury->obj)) return;

	//wierzch_bez_dziury->obj->setName(L"wierzch bez dziury");
	//wierzch_bez_dziury->obj->getTransform() = wnetrze->obj->getTransform();
	//wierzch_bez_dziury->obj->getData()->setLabel("wierzchBD_mesh");
	//AP::WORKSPACE::addModel(wierzch_bez_dziury->obj);

	//wierzch_bez_dziury->zbudujSiatke(szczeka_zeby);

	//// wyciskam rozepchana powierzchnie szczęki bez okluzji
	//wierzch_bez_dziury->zrobWyciskSumy4(szczeka_zeby);

	//wierzch_bez_dziury->klejDziury3();
	//wierzch_bez_dziury->usunNadmiaroweScianki();

	//mBD = std::dynamic_pointer_cast<CMesh>(wierzch_bez_dziury->obj->getData());



	// *** AKTUALNIE NIE ROBIMY WYCISKU Z DZIURAMI
	// 
	//CSiateczka1* wierzch_z_dziurom = new CSiateczka1(80, 80, 30, m_divider); // arguments: sizeX, sizeY, depth, divider

	//if ((NULL == wierzch_z_dziurom) || (NULL == wierzch_z_dziurom->obj)) return;

	//wierzch_z_dziurom->obj->setLabel(L"wierzch z dziurom");
	//wierzch_z_dziurom->obj->getTransform() = wnetrze->obj->getTransform();
	//wierzch_z_dziurom->obj->getData()->setLabel("wierzchZD_mesh");
	//AP::WORKSPACE::addModel(wierzch_z_dziurom->obj);

	//wierzch_z_dziurom->zbudujSiatke(szczeka_zeby);

	//// wyciskam rozepchana powierzchnie szczęki i robię dziurę za pomocą okluzji

	//wierzch_z_dziurom->zrob_wycisk_z_dziurom(szczeka_zeby, szczeka_okluzja);

	//wierzch_z_dziurom->klejDziury3();
	//wierzch_z_dziurom->usunNadmiaroweScianki();

	//mZD = std::dynamic_pointer_cast<CMesh>(wierzch_z_dziurom->obj->getData());
}

//
//void BiteSim::generujWierzchOdRazu3_CSiateczka1(double dVal, std::shared_ptr<CMesh> zuch)
//{
//	zuch->setLabel("testowa zuchwa");
//
//	wierzch = new CSiateczka1(80, 80, 30, m_divider); // arguments: sizeX, sizeY, depth, divider
//
//	if ((NULL == wierzch) || (NULL == wierzch->obj)) return;
//
//	wierzch->obj->setName(L"wierzch");
//	wierzch->obj->getTransform() = wnetrze->obj->getTransform();
//	wierzch->obj->getData()->setLabel("wierzch_mesh");
//	AP::WORKSPACE::addModel(wierzch->obj);
//
//
//
//	AP::WORKSPACE::removeModel(szczeka_zeby);
//
//	szczeka_zeby = std::dynamic_pointer_cast<CModel3D>(szczeka_oryginalneOdcieteZeby->getCopy());
//	szczeka_rozepchajZeby(dVal);
//
//	szczeka_zeby->getData()->setLabel(QString("rozepch2"));
//	szczeka_zeby->getData()->addKeyword(QString("dVal=%1").arg(dVal));
//	QString val = QString::number(dVal).replace(".", "_");
//	szczeka_zeby->setLabel(QString("rozepchane_%1").arg(val));
//	AP::WORKSPACE::addModel(szczeka_zeby);
//
//
//	wierzch->zbudujSiatke(szczeka_zeby);
//
//	// wyciskam rozepchana powierzchnie szczęki + powierzchnie okluzji
//	wierzch->zrobWyciskSumy4(szczeka_zeby, szczeka_okluzja, true);
//
//	// wyciskam żuchwę, żeby usunąć potencjalnie wystajace fragmenty
//	wierzch->zrobWyciskZuchwy_v1(std::dynamic_pointer_cast<CModel3D>(zuch->getParentPtr()), 0.0, 1);
//
//	wierzch->klejDziury3();
//	wierzch->usunNadmiaroweScianki();
//
//
//
//	CSiateczka1* wierzch_bez_dziury = new CSiateczka1(80, 80, 30, m_divider); // arguments: sizeX, sizeY, depth, divider
//
//	if ((NULL == wierzch_bez_dziury) || (NULL == wierzch_bez_dziury->obj)) return;
//
//	wierzch_bez_dziury->obj->setName(L"wierzch bez dziury");
//	wierzch_bez_dziury->obj->getTransform() = wnetrze->obj->getTransform();
//	wierzch_bez_dziury->obj->getData()->setLabel("wierzchBD_mesh");
//	AP::WORKSPACE::addModel(wierzch_bez_dziury->obj);
//
//	wierzch_bez_dziury->zbudujSiatke(szczeka_zeby);
//
//	// wyciskam rozepchana powierzchnie szczęki bez okluzji
//	wierzch_bez_dziury->zrobWyciskSumy4(szczeka_zeby);
//
//	wierzch_bez_dziury->klejDziury3();
//	wierzch_bez_dziury->usunNadmiaroweScianki();
//
//	mBD = std::dynamic_pointer_cast<CMesh>(wierzch_bez_dziury->obj->getData());
//
//	CSiateczka1* wierzch_z_dziurom = new CSiateczka1(80, 80, 30, m_divider); // arguments: sizeX, sizeY, depth, divider
//
//	if ((NULL == wierzch_z_dziurom) || (NULL == wierzch_z_dziurom->obj)) return;
//
//	wierzch_z_dziurom->obj->setLabel(L"wierzch z dziurom");
//	wierzch_z_dziurom->obj->getTransform() = wnetrze->obj->getTransform();
//	wierzch_z_dziurom->obj->getData()->setLabel("wierzchZD_mesh");
//	AP::WORKSPACE::addModel(wierzch_z_dziurom->obj);
//
//	wierzch_z_dziurom->zbudujSiatke(szczeka_zeby);
//
//	// wyciskam rozepchana powierzchnie szczęki i robię dziurę za pomocą okluzji
//
//	wierzch_z_dziurom->zrob_wycisk_z_dziurom(szczeka_zeby, szczeka_okluzja);
//
//	wierzch_z_dziurom->klejDziury3();
//	wierzch_z_dziurom->usunNadmiaroweScianki();
//
//	mZD = std::dynamic_pointer_cast<CMesh>(wierzch_z_dziurom->obj->getData());
//}
//
//
//void BiteSim::usunZeby()
//{
//	if (NULL != szczeka_zeby)
//	{
//		AP::WORKSPACE::removeModel(szczeka_zeby->id());
//	}
//
//	if (NULL != szczeka_obj)
//	{
//		AP::WORKSPACE::removeModel(szczeka_obj->id());
//	}
//}
//

void BiteSim::szczeka_inicjuj2(std::shared_ptr<CMesh> mesh)
{
	std::shared_ptr<CMesh> m = std::dynamic_pointer_cast<CMesh>(mesh->getCopy());
	m->setLabel("szczeka_mesh");

	szczeka_obj = std::make_shared<CModel3D>();
	szczeka_obj->addChild(szczeka_obj, m);
	szczeka_obj->importChildrenGeometry();
	szczeka_obj->setLabel("SZCZEKA");

	szczeka_obj->getTransform().translation() = CVector3d(0.000000, 0.000000, 0.000000);
	szczeka_obj->getTransform().rotation().setIdentity();
	szczeka_obj->getTransform().setScale(1.0);
	szczeka_obj->getTransform().moveTheOriginToTheCenterOfRotation(false);
	szczeka_obj->getTransform().lock(true);

	AP::WORKSPACE::addModel(szczeka_obj);
}


#include "Plane.h"

void BiteSim::szczeka_wytnijZebyNEW(std::shared_ptr<CPlane> cutPlane)
{
	std::shared_ptr<CMesh> robo = std::dynamic_pointer_cast<CMesh>(szczeka_obj->getChild()->getCopy());

	robo->cutPlane(*cutPlane);

	szczeka_zeby = std::make_shared<CModel3D>();
	szczeka_zeby->addChild(szczeka_zeby, robo);
	szczeka_zeby->setName(L"---zęby---");
	szczeka_zeby->setMin(robo->getMin());
	szczeka_zeby->setMax(robo->getMax());
	szczeka_zeby->setTransform(szczeka_obj->getTransform());
}

void BiteSim::szczeka_tworzMapeOkluzji2(std::shared_ptr<CMesh> mesh, CTransform tFrom, CTransform tTo)
{
	szczeka_mapaOkluzji.clear();

	std::shared_ptr<CMesh> o = std::dynamic_pointer_cast<CMesh>(mesh->getCopy());
	o->setLabel("okluzja_mesh2");

	szczeka_okluzja = std::make_shared<CModel3D>();
	szczeka_okluzja->addChild(szczeka_okluzja, o);
	szczeka_okluzja->importChildrenGeometry();
	szczeka_okluzja->setLabel("OKLUZJA");
	szczeka_okluzja->applyTransformation(tFrom, tTo);

	szczeka_okluzja->transform() = tTo;

	AP::WORKSPACE::addModel(szczeka_okluzja);

	//CMesh* mesh = (CMesh*)szczeka_okluzja->getData();
	for (int i = 0; i < mesh->vertices().size(); i++)
	{
		CVertex v(mesh->vertices().at(i));

		std::pair<int, int> indeks(round(10 * v.X()), round(10 * v.Y()));
		szczeka_mapaOkluzji[indeks] = v.Z();
	}
}

int BiteSim::szczeka_nalezyDoPowierzchniOkluzji(CTriple<double> v)
{
	int x = round(10 * v.X());
	int y = round(10 * v.Y());
	double z = v.Z();

	std::map< std::pair<int, int>, double >::iterator point = szczeka_mapaOkluzji.find(std::pair<int, int>(x, y));

	int result = 0;

	if (point != szczeka_mapaOkluzji.end())
	{
		//if ( (point->second < ( z + 0.1 ) ) && (point->second > ( z - 0.1 ) ) )
		if ((point->second < (z + 0.75)) && (point->second > (z - 0.75)))
		{
			return 1;
		}
	}
	else
	{
		std::map< std::pair<int, int>, double >::iterator left = szczeka_mapaOkluzji.find(std::pair<int, int>(x - 1, y));
		std::map< std::pair<int, int>, double >::iterator right = szczeka_mapaOkluzji.find(std::pair<int, int>(x + 1, y));
		std::map< std::pair<int, int>, double >::iterator top = szczeka_mapaOkluzji.find(std::pair<int, int>(x, y - 1));
		std::map< std::pair<int, int>, double >::iterator bottom = szczeka_mapaOkluzji.find(std::pair<int, int>(x, y + 1));

		if ((left != szczeka_mapaOkluzji.end()) ||
			(right != szczeka_mapaOkluzji.end()) ||
			(top != szczeka_mapaOkluzji.end()) ||
			(bottom != szczeka_mapaOkluzji.end()))
		{
			return 2;
		}
	}

	return 0;
}


/*
Należy zwrócić uwagę na tę funkcję. Wydaje mi sie, ze stare podejście,
gdy punkty należące do okluzji traktowaliśmy inaczej niż inne jest już nie aktualne.
Na razie tego nie zmieniam, bo generowane szyny różniły by sie od tych które testujemy,
a nie ma na czasu na robienie ich od nowa. DO PRZEMYŚLENIA I POPRAWY
*/
void BiteSim::szczeka_rozepchajZeby(float d, bool wierzch)
{
	std::shared_ptr<CMesh> dstMesh = std::dynamic_pointer_cast<CMesh>(szczeka_zeby->getData());

	dstMesh->calcVN();

	for (int i = 0; i < dstMesh->vnormals().size(); i++)
	{
		CVertex* v1 = &dstMesh->vertices()[i];

		CTriple<double> v(szczeka_okluzja->getTransform().w2l(szczeka_zeby->getTransform().l2w(*v1)));

		CVector3f* vn1 = &dstMesh->vnormals()[i];

		if (wierzch && szczeka_nalezyDoPowierzchniOkluzji(v))
		{
			if (vn1->Z() > 0)
			{
				v1->Z(v1->Z() + d); // !!!
			}

		}
		else
		{
			v1->Set(v1->X() + d * vn1->X(), v1->Y() + d * vn1->Y(), v1->Z() + d * vn1->Z());
		}

		AP::processEvents();
	}
}

// Regular mesh dilation
void BiteSim::szczeka_rozepchajZebyRegular(float d)
{
	std::shared_ptr<CMesh> dstMesh = std::dynamic_pointer_cast<CMesh>(szczeka_zeby->getData());

	dstMesh->calcVN();

	for (int i = 0; i < dstMesh->vnormals().size(); i++)
	{
		CVertex* v1 = &dstMesh->vertices()[i];
		CVector3f* vn1 = &dstMesh->vnormals()[i];

		v1->Set(v1->X() + d * vn1->X(), v1->Y() + d * vn1->Y(), v1->Z() + d * vn1->Z());

		AP::processEvents();
	}
}

