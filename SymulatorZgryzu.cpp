#include "SymulatorZgryzu.h"
#include "AnnotationEdges.h"

#include "../api/UI.h"


CSymulatorZgryzu::~CSymulatorZgryzu(void)
{
	AP::WORKSPACE::removeModel( szczeka.obj->id() );
	AP::WORKSPACE::removeModel( zuchwa.obj->id() );
	
	if ( NULL != wnetrze )	AP::WORKSPACE::removeModel(wnetrze->obj->id());
	if ( NULL != wierzch )	AP::WORKSPACE::removeModel(wierzch->obj->id());
}



void CSymulatorZgryzu::generujWnetrzeNEW(CSiateczka1* wnetrze, float d)
{
	if ((NULL == wnetrze) || (NULL == wnetrze->obj)) return;

	std::shared_ptr<CModel3D> obj = wnetrze->obj;
	obj->setName(L"wnetrze_" + std::to_wstring(d));

	obj->getData()->setLabel("wnetrze_mesh");


	szczeka.oryginalneOdcieteZeby = std::dynamic_pointer_cast<CModel3D>(szczeka.zeby->getCopy());

	szczeka.oryginalneOdcieteZeby->setName(L"wycięte zęby");
	//szczeka.oryginalneOdcieteZeby->setTransform(szczeka.zeby->getTransform());

	AP::WORKSPACE::addModel(szczeka.oryginalneOdcieteZeby);

	szczeka.rozepchajZeby(d);

	std::shared_ptr<CMesh> tmp = std::dynamic_pointer_cast<CMesh>(szczeka.zeby->getData()->getCopy());
	tmp->setLabel(QString("rozepch1"));
	tmp->addKeyword(QString("dVal=%1").arg(d));
	AP::WORKSPACE::addObject(tmp);
	QString val = QString::number(d).replace(".", "_");
	tmp->getParent()->setLabel(QString("rozepchane_%1").arg(val));

	//CModel3D *rozph = szczeka.zeby->getCopy();
	//rozph->setLabel()

	szczeka.zeby->setLocked(true);

	wnetrze->zbudujSiatke(szczeka.zeby);

	//return;

	wnetrze->zrobWycisk2(szczeka.zeby);

	wnetrze->klejDziury3();

	wnetrze->usunNadmiaroweScianki();

	wnetrze->odwrocNormalne();
}



void CSymulatorZgryzu::generujWnetrze( CSiateczka1 * wnetrze, float d )
{
	if ((NULL == wnetrze) || (NULL == wnetrze->obj)) return;
	
	std::shared_ptr<CModel3D> obj = wnetrze->obj;
	obj->setName(L"wnetrze_"+std::to_wstring(d));
	
	obj->setLocked(true);

	szczeka.wytnijZeby(szczeka.obj);
	
	szczeka.oryginalneOdcieteZeby = std::dynamic_pointer_cast<CModel3D>(szczeka.zeby->getCopy());

	szczeka.oryginalneOdcieteZeby->setName(L"wycięte zęby");
		
	AP::WORKSPACE::addModel(szczeka.oryginalneOdcieteZeby);

	//return;
		
	szczeka.rozepchajZeby( d );

	szczeka.zeby->setLocked(true);

	wnetrze->zbudujSiatke(szczeka.zeby);

	return;

	wnetrze->zrobWycisk2(szczeka.zeby);

	wnetrze->klejDziury3();

	wnetrze->usunNadmiaroweScianki();
	wnetrze->odwrocNormalne();
}


//void CSymulatorZgryzu::generujWierzch(int krok)
//{
//	wierzch = new CSiateczka1(80, 80, 30, m_divider); // arguments: sizeX, sizeY, depth, divider
//
//	if ((NULL == wierzch) || (NULL == wierzch->obj)) return;
//
//	if ( krok == 0 )
//	{
//		wierzch->obj->setName(L"wierzch");
//
//		wierzch->obj->getTransform() = wnetrze->obj->getTransform();
//
//		AP::WORKSPACE::addModel(wierzch->obj);
//
//		AP::WORKSPACE::removeModel(szczeka.zeby->id() );
//
//		szczeka.zeby = szczeka.oryginalneOdcieteZeby->getCopy();
//		
//		AP::WORKSPACE::addModel(szczeka.zeby);
//
//		szczeka.rozepchajZeby2( 0.4, 1.2, true );
//
//		wierzch->zbudujSiatke(szczeka.zeby);
//
//		wierzch->zrobWycisk2(szczeka.zeby);
//
//		wierzch->klejDziury3();
//		wierzch->usunNadmiaroweScianki();
//	}
//}
//
//void CSymulatorZgryzu::generujWi2(int krok)
//{
//	wi2 = new CSiateczka1(80, 80, 30, m_divider); // arguments: sizeX, sizeY, depth, divider
//
//	if ((NULL == wi2) || (NULL == wi2->obj)) return;
//
//	if (krok == 0)
//	{
//		wi2->obj->setName(L"wi_2");
//
//		wi2->obj->getTransform() = wnetrze->obj->getTransform();
//
//		AP::WORKSPACE::addModel(wi2->obj);
//
//		AP::WORKSPACE::removeModel(szczeka.zeby->id());
//
//		szczeka.zeby = szczeka.oryginalneOdcieteZeby->getCopy();
//		AP::WORKSPACE::addModel(szczeka.zeby);
//
//		szczeka.rozepchajZeby(0.7);
//		szczeka.korygujDoOkluzji(1.2);
//
//		wi2->zbudujSiatke(szczeka.zeby);
//
//		wi2->zrobWycisk2(szczeka.zeby);
//
//		wi2->klejDziury3();
//		wi2->usunNadmiaroweScianki();
//	}
//}
//
//
//void CSymulatorZgryzu::zalejSume(int krok)
//{
//	suma = new CSiateczka1(80, 80, 30, m_divider); // arguments: sizeX, sizeY, depth, divider
//
//	if ((NULL == suma) || (NULL == suma->obj)) return;
//
//	if (krok == 0)
//	{
//		suma->obj->setName(L"suma");
//
//		suma->obj->getTransform() = wnetrze->obj->getTransform();
//
//		AP::WORKSPACE::addModel(suma->obj);
//
//		//AP::getWorkspace()->close(szczeka.zebyID);
//		//szczeka.zeby = szczeka.oryginalneOdcieteZeby->getCopy();
//		//szczeka.zebyID = AP::getWorkspace()->addModel(szczeka.zeby);
//
//		//szczeka.rozepchajZeby(0.7);
//		//szczeka.korygujDoOkluzji(1.2);
//
//		suma->zbudujSiatke(szczeka.zeby);
//
//		suma->zrobWyciskSumy( wierzch->obj, wi2->obj );
//
//		suma->klejDziury3();
//		suma->usunNadmiaroweScianki();
//	}
//}
//
//
//void CSymulatorZgryzu::generujWierzchOdRazu(int krok)
//{
//	wierzch = new CSiateczka1(80, 80, 30, m_divider); // arguments: sizeX, sizeY, depth, divider
//
//	if ((NULL == wierzch) || (NULL == wierzch->obj)) return;
//
//	if (krok == 0)
//	{
//		wierzch->obj->setName(L"wierzch");
//
//		wierzch->obj->getTransform() = wnetrze->obj->getTransform();
//		AP::WORKSPACE::addModel(wierzch->obj);
//
//		AP::WORKSPACE::removeModel(szczeka.zeby->id());
//
//		szczeka.zeby = szczeka.oryginalneOdcieteZeby->getCopy();
//		AP::WORKSPACE::addModel(szczeka.zeby);
//
//		szczeka.rozepchajZeby(0.7); //0.7
//
//		szczeka.korygujDoOkluzji(20.0); //1.2
//
//		CModel3D *z07 = szczeka.zeby->getCopy();
//		z07->setLabel(L"z07");
//		AP::WORKSPACE::addModel(z07);
//
//		AP::WORKSPACE::removeModel(szczeka.zeby->id() );
//
//		szczeka.zeby = szczeka.oryginalneOdcieteZeby->getCopy();
//		AP::WORKSPACE::addModel(szczeka.zeby);
//
//		szczeka.rozepchajZeby2(0.4, 1.2, true);
//
//		CModel3D *z12 = szczeka.zeby->getCopy();
//		z12->setLabel(L"z12");
//		AP::WORKSPACE::addModel(z12);
//
//		
//
//		wierzch->zbudujSiatke(szczeka.zeby);
//
//		wierzch->zrobWyciskSumy2( z12, z07 );
//
//		wierzch->klejDziury3();
//		wierzch->usunNadmiaroweScianki();
//	}
//
//}
//
//
//void CSymulatorZgryzu::generujWierzchOdRazu2(double dVal, int krok)
//{
//	wierzch = new CSiateczka1(80, 80, 30, m_divider); // arguments: sizeX, sizeY, depth, divider
//
//	if ((NULL == wierzch) || (NULL == wierzch->obj)) return;
//
//	if (krok == 0)
//	{
//		wierzch->obj->setName(L"wierzch");
//
//		wierzch->obj->getTransform() = wnetrze->obj->getTransform();
//		AP::WORKSPACE::addModel(wierzch->obj);
//
//		AP::WORKSPACE::removeModel(szczeka.zeby->id());
//
//		szczeka.zeby = szczeka.oryginalneOdcieteZeby->getCopy();
//		AP::WORKSPACE::addModel(szczeka.zeby);
//
//		szczeka.rozepchajZeby(dVal); //0.7
//		//szczeka.rozepchajZeby2(0.7, 2.5, true);
//
//		wierzch->zbudujSiatke(szczeka.zeby); 
//		wierzch->zrobWyciskSumy4(szczeka.zeby, szczeka.okluzja);  /* (DP) TO TUTAJ !!!*/
//
//		wierzch->klejDziury3();
//		wierzch->usunNadmiaroweScianki();
//		return;
///********************************************************************/
//		szczeka.korygujDoOkluzji(20.0); //1.2
//
//		CModel3D* z07 = szczeka.zeby->getCopy();
//		z07->setLabel(L"z07");
//		AP::WORKSPACE::addModel(z07);
//
//		AP::WORKSPACE::removeModel(szczeka.zeby->id());
//
//		szczeka.zeby = szczeka.oryginalneOdcieteZeby->getCopy();
//		AP::WORKSPACE::addModel(szczeka.zeby);
//
//		//szczeka.rozepchajZeby2(0.4, 1.2, true);
//		szczeka.rozepchajZeby2(2.0, 2.0, true);
//
//		CModel3D* z12 = szczeka.zeby->getCopy();
//		z12->setLabel(L"z12");
//		AP::WORKSPACE::addModel(z12);
//
//
//
//		wierzch->zbudujSiatke(szczeka.zeby);
//
//		wierzch->zrobWyciskSumy2(z12, z07);
//
//		wierzch->klejDziury3();
//		wierzch->usunNadmiaroweScianki();
//	}
//
//}


#include "AnnotationPoints.h"
#include "AnnotationSetOfFaces.h"



void rozepchajZeby_simple(CMesh *dstMesh, float d)
{
	dstMesh->calcVN();

	for (int i = 0; i < dstMesh->vnormals().size(); i++)
	{
		CVertex &v1 = dstMesh->vertices()[i];

		CVector3f &vn1 = dstMesh->vnormals()[i];

		v1 = CVertex( v1.x + d * vn1.x, v1.y + d * vn1.y, v1.z + d * vn1.z );

		AP::processEvents();
	}
}

#include "CollisionDetector.h"
#include "triangulacja.h"


std::shared_ptr<CMesh> create_mesh_from_faces(std::shared_ptr<CMesh> mesh1, std::set<INDEX_TYPE> s1) {
	std::shared_ptr<CMesh> mesh = std::make_shared<CMesh>();

	for (auto fidx : s1) {
		CFace f = mesh1->faces()[fidx];
		CVertex v1 = mesh1->vertices()[f.x];
		CVertex v2 = mesh1->vertices()[f.y];
		CVertex v3 = mesh1->vertices()[f.z];

		int idx = mesh->vertices().size();
		mesh->addVertex(v1);
		mesh->addVertex(v2);
		mesh->addVertex(v3);
		
		mesh->addFace(CFace(idx, idx + 1, idx + 2));
	}

	mesh->removeDuplicateVertices();

	return mesh;
}

void przeciecia(std::shared_ptr<CMesh> mesh1, std::shared_ptr<CMesh> mesh2) {

	std::map<INDEX_TYPE, std::set<INDEX_TYPE>*> crossed;

	if (CollisionDetector::getIntersectionOfMeshWithMesh3d(mesh1, mesh2, crossed))
	{
		std::set<INDEX_TYPE> s1, s2;

		for (auto pair : crossed)
		{
			s1.insert(pair.first);
			s2.insert(pair.second->begin(), pair.second->end());
		}

		std::shared_ptr<CMesh> m1 = create_mesh_from_faces(mesh1, s1);
		std::shared_ptr<CMesh> m2 = create_mesh_from_faces(mesh2, s2);

		//CAnnotationSetOfFaces* cavf2 = new CAnnotationSetOfFaces(s2);
		//AP::MODEL::addAnnotation(wnetrze->obj, cavf2);

		//CAnnotationSetOfFaces* cavf1 = new CAnnotationSetOfFaces(s1);
		//cavf1->setColor(CRGBA(0.0f, 1.0f, 0.0f));
		//AP::MODEL::addAnnotation(wierzch->obj, cavf1);

		//UI::STATUSBAR::setText("Ready. You can see interecting faces as annotations.");

		//CAnnotationEdges* ed = new CAnnotationEdges();

		//SplitTriangles((CMesh*)wierzch->obj->getData(), (CMesh*)wnetrze->obj->getData(), *cavf1, *cavf2, *ed);


		//SplitTriangles2(mesh1, mesh2, crossed, *ed);

		//AP::OBJECT::addChild(mesh1->getParent(), ed);
		AP::OBJECT::addChild(mesh1, m1);
		AP::OBJECT::addChild(mesh2, m2);

		UI::STATUSBAR::setText("Ready. You can see sugested faces.");
	}
	else
	{
		UI::STATUSBAR::setText("No intersections found");
	}



}

//void CSymulatorZgryzu::generujWierzchOdRazu3(double dVal, CMesh* zuch)
//{
//	zuch->setLabel("testowa zuchwa");
//
//	AP::WORKSPACE::removeModel(szczeka.zeby);
//
//	szczeka.zeby = szczeka.oryginalneOdcieteZeby->getCopy();
//	szczeka.rozepchajZeby(dVal);
//
//	szczeka.zeby->getData()->setLabel(QString("rozepch2"));
//	szczeka.zeby->getData()->addKeyword(QString("dVal=%1").arg(dVal));
//	QString val = QString::number(dVal).replace(".", "_");
//	szczeka.zeby->setLabel(QString("rozepchane_%1").arg(val));
//	AP::WORKSPACE::addModel(szczeka.zeby);
//
//
//	CSiateczka2 *wierzchS2 = new CSiateczka2(80, 80, 30, m_divider);
//	if ((NULL == wierzchS2) || (NULL == wierzchS2->obj)) return;
//
//	wierzchS2->obj->getTransform() = wnetrze->obj->getTransform();
//	wierzchS2->zbudujSiatke(szczeka.zeby);
//
//	// wyciskam rozepchana powierzchnie szczęki bez okluzji
//	wierzchS2->daj_mi_wycisk(szczeka.zeby, 0);
//	wierzchS2->daj_mi_wycisk(szczeka.okluzja, 0);
//	wierzchS2->daj_mi_wycisk((CModel3D*)zuch->getParent(), -1);
//	wierzchS2->usunNadmiaroweScianki();
//
//	wierzchS2->obj->setName(L"wierzch");
//	wierzchS2->obj->getData()->setLabel("wierzch_mesh");
//	AP::WORKSPACE::addModel(wierzchS2->obj);
//
//
//	CSiateczka2 *wierzchBD = new CSiateczka2(80, 80, 30, m_divider);
//	if ((NULL == wierzchBD) || (NULL == wierzchBD->obj)) return;
//
//	wierzchBD->obj->getTransform() = wnetrze->obj->getTransform();
//	wierzchBD->zbudujSiatke(szczeka.zeby);
//
//	// wyciskam rozepchana powierzchnie szczęki bez okluzji
//	wierzchBD->daj_mi_wycisk(szczeka.zeby, 0);
//	wierzchBD->usunNadmiaroweScianki();
//
//	mBD = (CMesh*)wierzchBD->obj->getData();
//
//	wierzchBD->obj->setName(L"wierzch bez dziury");
//	mBD->setLabel("wierzchBD_mesh");
//	AP::WORKSPACE::addModel(wierzchBD->obj);
//
//
//
//	CSiateczka2* wierzchZD = new CSiateczka2(80, 80, 30, m_divider);
//	if ((NULL == wierzchZD) || (NULL == wierzchZD->obj)) return;
//
//	wierzchZD->obj->getTransform() = wnetrze->obj->getTransform();
//	wierzchZD->zbudujSiatke(szczeka.zeby);
//
//	// wyciskam rozepchana powierzchnie szczęki bez okluzji
//	wierzchZD->daj_mi_wycisk(szczeka.zeby, 0);
//	wierzchZD->zrob_dziure(szczeka.okluzja);
//	wierzchZD->usunNadmiaroweScianki();
//
//	mZD = (CMesh*)wierzchZD->obj->getData();
//
//	wierzchZD->obj->setName(L"wierzch z dziurom");
//	mZD->setLabel("wierzchZD_mesh");
//	AP::WORKSPACE::addModel(wierzchZD->obj);
//}
//
//
void CSymulatorZgryzu::generujWierzchOdRazu3_CSiateczka1_v2(double dVal, std::shared_ptr<CMesh> zuch)
{
	zuch->setLabel("testowa zuchwa");

	wierzch = new CSiateczka1(80, 80, 30, m_divider); // arguments: sizeX, sizeY, depth, divider

	if ((NULL == wierzch) || (NULL == wierzch->obj)) return;

	wierzch->obj->setName(L"wierzch");
	wierzch->obj->getTransform() = wnetrze->obj->getTransform();
	wierzch->obj->getData()->setLabel("wierzch_mesh");
	AP::WORKSPACE::addModel(wierzch->obj);



	AP::WORKSPACE::removeModel(szczeka.zeby);

	szczeka.zeby = std::dynamic_pointer_cast<CModel3D>(szczeka.oryginalneOdcieteZeby->getCopy());
	szczeka.rozepchajZeby(dVal);

	szczeka.zeby->getData()->setLabel(QString("rozepch2"));
	szczeka.zeby->getData()->addKeyword(QString("dVal=%1").arg(dVal));
	QString val = QString::number(dVal).replace(".", "_");
	szczeka.zeby->setLabel(QString("rozepchane_%1").arg(val));
	AP::WORKSPACE::addModel(szczeka.zeby);


	wierzch->zbudujSiatke(szczeka.zeby);

	// wyciskam rozepchana powierzchnie szczęki + powierzchnie okluzji
	wierzch->zrobWyciskSumy4(szczeka.zeby, szczeka.okluzja, false);

	// wyciskam żuchwę, żeby usunąć potencjalnie wystajace fragmenty
	//wierzch->zrobWyciskZuchwy_v1((CModel3D*)zuch->getParent(), 0.0, 1);

	wierzch->klejDziury3();
	wierzch->usunNadmiaroweScianki();



	CSiateczka1* wierzch_bez_dziury = new CSiateczka1(80, 80, 30, m_divider); // arguments: sizeX, sizeY, depth, divider

	if ((NULL == wierzch_bez_dziury) || (NULL == wierzch_bez_dziury->obj)) return;

	wierzch_bez_dziury->obj->setName(L"wierzch bez dziury");
	wierzch_bez_dziury->obj->getTransform() = wnetrze->obj->getTransform();
	wierzch_bez_dziury->obj->getData()->setLabel("wierzchBD_mesh");
	AP::WORKSPACE::addModel(wierzch_bez_dziury->obj);

	wierzch_bez_dziury->zbudujSiatke(szczeka.zeby);

	// wyciskam rozepchana powierzchnie szczęki bez okluzji
	wierzch_bez_dziury->zrobWyciskSumy4(szczeka.zeby);

	wierzch_bez_dziury->klejDziury3();
	wierzch_bez_dziury->usunNadmiaroweScianki();

	mBD = std::dynamic_pointer_cast<CMesh>(wierzch_bez_dziury->obj->getData());

	CSiateczka1* wierzch_z_dziurom = new CSiateczka1(80, 80, 30, m_divider); // arguments: sizeX, sizeY, depth, divider

	if ((NULL == wierzch_z_dziurom) || (NULL == wierzch_z_dziurom->obj)) return;

	wierzch_z_dziurom->obj->setLabel(L"wierzch z dziurom");
	wierzch_z_dziurom->obj->getTransform() = wnetrze->obj->getTransform();
	wierzch_z_dziurom->obj->getData()->setLabel("wierzchZD_mesh");
	AP::WORKSPACE::addModel(wierzch_z_dziurom->obj);

	wierzch_z_dziurom->zbudujSiatke(szczeka.zeby);

	// wyciskam rozepchana powierzchnie szczęki i robię dziurę za pomocą okluzji

	wierzch_z_dziurom->zrob_wycisk_z_dziurom(szczeka.zeby, szczeka.okluzja);

	wierzch_z_dziurom->klejDziury3();
	wierzch_z_dziurom->usunNadmiaroweScianki();

	mZD = std::dynamic_pointer_cast<CMesh>(wierzch_z_dziurom->obj->getData());
}


void CSymulatorZgryzu::generujWierzchOdRazu3_CSiateczka1(double dVal, std::shared_ptr<CMesh> zuch)
{
	zuch->setLabel("testowa zuchwa");

	wierzch = new CSiateczka1(80, 80, 30, m_divider); // arguments: sizeX, sizeY, depth, divider

	if ((NULL == wierzch) || (NULL == wierzch->obj)) return;

	wierzch->obj->setName(L"wierzch");
	wierzch->obj->getTransform() = wnetrze->obj->getTransform();
	wierzch->obj->getData()->setLabel("wierzch_mesh");
	AP::WORKSPACE::addModel(wierzch->obj);



	AP::WORKSPACE::removeModel(szczeka.zeby);

	szczeka.zeby = std::dynamic_pointer_cast<CModel3D>(szczeka.oryginalneOdcieteZeby->getCopy());
	szczeka.rozepchajZeby(dVal);

	szczeka.zeby->getData()->setLabel(QString("rozepch2"));
	szczeka.zeby->getData()->addKeyword(QString("dVal=%1").arg(dVal));
	QString val = QString::number(dVal).replace(".", "_");
	szczeka.zeby->setLabel(QString("rozepchane_%1").arg(val));
	AP::WORKSPACE::addModel(szczeka.zeby);


	wierzch->zbudujSiatke(szczeka.zeby);

	// wyciskam rozepchana powierzchnie szczęki + powierzchnie okluzji
	wierzch->zrobWyciskSumy4(szczeka.zeby, szczeka.okluzja, true);

	// wyciskam żuchwę, żeby usunąć potencjalnie wystajace fragmenty
	wierzch->zrobWyciskZuchwy_v1(std::dynamic_pointer_cast<CModel3D>(zuch->getParentPtr()), 0.0, 1);

	wierzch->klejDziury3();
	wierzch->usunNadmiaroweScianki();



	CSiateczka1* wierzch_bez_dziury = new CSiateczka1(80, 80, 30, m_divider); // arguments: sizeX, sizeY, depth, divider

	if ((NULL == wierzch_bez_dziury) || (NULL == wierzch_bez_dziury->obj)) return;

	wierzch_bez_dziury->obj->setName(L"wierzch bez dziury");
	wierzch_bez_dziury->obj->getTransform() = wnetrze->obj->getTransform();
	wierzch_bez_dziury->obj->getData()->setLabel("wierzchBD_mesh");
	AP::WORKSPACE::addModel(wierzch_bez_dziury->obj);

	wierzch_bez_dziury->zbudujSiatke(szczeka.zeby);

	// wyciskam rozepchana powierzchnie szczęki bez okluzji
	wierzch_bez_dziury->zrobWyciskSumy4(szczeka.zeby);

	wierzch_bez_dziury->klejDziury3();
	wierzch_bez_dziury->usunNadmiaroweScianki();

	mBD = std::dynamic_pointer_cast<CMesh>(wierzch_bez_dziury->obj->getData());

	CSiateczka1* wierzch_z_dziurom = new CSiateczka1(80, 80, 30, m_divider); // arguments: sizeX, sizeY, depth, divider

	if ((NULL == wierzch_z_dziurom) || (NULL == wierzch_z_dziurom->obj)) return;

	wierzch_z_dziurom->obj->setLabel(L"wierzch z dziurom");
	wierzch_z_dziurom->obj->getTransform() = wnetrze->obj->getTransform();
	wierzch_z_dziurom->obj->getData()->setLabel("wierzchZD_mesh");
	AP::WORKSPACE::addModel(wierzch_z_dziurom->obj);

	wierzch_z_dziurom->zbudujSiatke(szczeka.zeby);

	// wyciskam rozepchana powierzchnie szczęki i robię dziurę za pomocą okluzji

	wierzch_z_dziurom->zrob_wycisk_z_dziurom(szczeka.zeby, szczeka.okluzja);

	wierzch_z_dziurom->klejDziury3();
	wierzch_z_dziurom->usunNadmiaroweScianki();

	mZD = std::dynamic_pointer_cast<CMesh>(wierzch_z_dziurom->obj->getData());
}


//void CSymulatorZgryzu::generujWierzchOdRazu3_backup(double dVal, CMesh* zuch, int krok)
//{
//	zuch->setLabel("testowa zuchwa");
//
//	wierzch = new CSiateczka1(80, 80, 30, m_divider); // arguments: sizeX, sizeY, depth, divider
//
//	if ((NULL == wierzch) || (NULL == wierzch->obj)) return;
//
//	if (krok == 0)
//	{
//		wierzch->obj->setName(L"wierzch");
//
//		wierzch->obj->getTransform() = wnetrze->obj->getTransform();
//		AP::WORKSPACE::addModel(wierzch->obj);
//
//		wierzch->obj->getData()->setLabel("wierzch_mesh");
//
//
//		/*dp*///CModel3D* pow_wewn = szczeka.zeby->getCopy();
//
//
//		AP::WORKSPACE::removeModel(szczeka.zeby);
//
//		szczeka.zeby = szczeka.oryginalneOdcieteZeby->getCopy();
//
//		/*dp*///CModel3D* zeby_01 = szczeka.zeby->getCopy();
//		//rozepchajZeby_simple((CMesh*)zeby_01->getData(), 0.4); // 0.3 + 0.1
//
//
//		szczeka.rozepchajZeby(dVal);
//		//szczeka.rozepchajZeby2(1.3, 0.4, true);
//		szczeka.zeby->getData()->setLabel(QString("rozepch2"));
//		szczeka.zeby->getData()->addKeyword(QString("dVal=%1").arg(dVal));
//		QString val = QString::number(dVal).replace(".", "_");
//		szczeka.zeby->setLabel(QString("rozepchane_%1").arg(val));
//		AP::WORKSPACE::addModel(szczeka.zeby);
//
//
//
//		std::set<CVertex> bledy;
//		std::set<int> bledy2;
//
//
//		//wierzch->zbudujSiatke(zeby_01);
//		//wierzch->zrobWyciskSumy4(zeby_01, szczeka.okluzja, false);
//
//		//wierzch->zrobWyciskZuchwy(szczeka.zeby, 1, nullptr, nullptr);
//		wierzch->zbudujSiatke(szczeka.zeby);
//		wierzch->zrobWyciskSumy4(szczeka.zeby, szczeka.okluzja, true);
//
//		//wierzch->zrobWyciskSumy4_aaa(szczeka.zeby, szczeka.okluzja, false);
//
//		//wierzch->zrobWyciskSumy4(szczeka.zeby, szczeka.zeby, true);
//
//		//wierzch->zrobWyciskSumy4(zeby_01, szczeka.okluzja, true);
//		//wierzch->zbudujSiatke(szczeka.zeby);
//		//wierzch->zrobWyciskSumy4(szczeka.zeby, szczeka.okluzja, true);
//
//		//wierzch->zrobWyciskSumy5(szczeka.zeby, szczeka.okluzja, pow_wewn, &bledy);  /* (DP) TO TUTAJ !!!*/
//		//wierzch->zrobWyciskSumy6(szczeka.zeby, szczeka.okluzja, pow_wewn, (CModel3D*)zuch->getParent(), &bledy);  /* (DP) TO TUTAJ !!!*/
//
//
//		wierzch->zrobWyciskZuchwy_v1((CModel3D*)zuch->getParent(), 0.0, 1, nullptr, &bledy2);
//
//		//wierzch->zrobWyciskSumy1111((CModel3D*)zuch->getParent());
//
//
//		wierzch->klejDziury3();
//		wierzch->usunNadmiaroweScianki();
//
//
//		//rozepchajZeby_simple(zuch, 0.2);
//
//
//		//przeciecia((CMesh*)wierzch->obj->getData(), (CMesh*)wnetrze->obj->getData());
//
//		/*dp1*///wierzch->zrobWyciskZuchwy(pow_wewn, 1, &bledy, nullptr);  /* (DP) TO TUTAJ !!!*/
//
//		/*dp1*///wnetrze->zrobWyciskZuchwy((CModel3D*)zuch->getParent(), 1, &bledy, nullptr);  /* (DP) TO TUTAJ !!!*/
//
//		/*dp*///delete pow_wewn;
//		//delete zeby_01;
//
//		//CAnnotationPoints* pts = new CAnnotationPoints();
//
//		//for (auto v : bledy) {
//		//	pts->addPoint(v);
//		//}
//
//		//AP::OBJECT::addChild(wierzch->obj, pts);
//
//
//		CAnnotationSetOfFaces* vtcs = new CAnnotationSetOfFaces();
//
//		for (auto fi : bledy2) {
//			vtcs->insert(fi);
//		}
//
//		vtcs->setDest(zuch);
//		AP::OBJECT::addChild(zuch->getParent(), vtcs);
//
//	}
//
//}
//
//
//
void CSymulatorZgryzu::usunZeby()
{
	if (NULL != szczeka.zeby)
	{
		AP::WORKSPACE::removeModel(szczeka.zeby->id());
	}

	if (NULL != szczeka.obj)
	{
		AP::WORKSPACE::removeModel(szczeka.obj->id());
	}
}





bool myGetClosestFace(CMesh* m, size_t vIndex, CWektor3D vRay, CPunkt3D& IntersectionPoint, size_t& indx, bool onlyFront, std::set<size_t> podzbior)
{
	size_t id = 0;
	double dist = DBL_MAX;
	CPunkt3D tmpP;
	double r;
	bool foundAny = false;

	CPunkt3D pkt0(m->vertices()[vIndex]);

	for (std::set<size_t>::iterator si = podzbior.begin(); si != podzbior.end(); si++)
	{
		size_t j = *si;
		//for (size_t j = 0; j<static_cast<size_t>(m->faces().size()); j++)
		//{
		CFace* f = &m->faces()[j];
		if ((f->A() == vIndex) || (f->B() == vIndex) || (f->C() == vIndex))
		{
			continue; //pomijam trójkąty zawierające punkt początkowy
		}

		int result = m->rayTriangleIntersect3D(j, vRay, pkt0, tmpP, r);

		// result == -2 ---> promień leży na trójkącie (ma wiecej punktów wspólnych niż 1)
		// result == -1 ---> trójkąt jest zdegenerowany (co najmniej jedna krawędź ma zerową długość)
		// result == 0 ---> promień nie trafił w trójkąt
		// result == 1 ---> trójkąt trafiony od przodu (punkt przecięcia zwracany w tmpP)
		// result == 2 ---> trójkąt trafiony od tyłu (punkt przecięcia zwracany w tmpP)

		if (tmpP.Z() < pkt0.Z())
		{
			continue; // to jest ścianka leżąca ZA badanym wierzchołkiem
		}

		if (result > 0)
		{
			double tmp = CWektor3D(tmpP, pkt0).length();

			if (tmp < dist)
			{
				dist = tmp;
				IntersectionPoint = tmpP;
				indx = j;

				if (result == 1)
				{
					foundAny = true;
				}
				else if (result == 2)
				{
					if (onlyFront)
						foundAny = false;				 // jeśli najbliżej jest ściana odwrócona tyłem, to nie powinno mi zwrócić punktu
					else
						foundAny = true;				// chyba że dokłądnie tego chcę
				}
			}
		}

		/* UWAGA
		W przypadku gdy result == -2 możemy mieć do czynienia z sytuacjami:
		a) ściana jest ustawiona bokiem, ale ma sąsiadów więc promień powinien w któregoś z nich trafić i temu sąsiadowi zostanie przypisane trafienie
		b) j.w., ale wszyscy trafieni sąsiedzi sa odwróceni "tyłem" to zn. że promień prześliznął sie po wewn. stronie siatki - można punkt pominąć.
		c) ściana nie ma żadnego sąsiada - nieprawidłowa siatka ??? - mozna punkt pominąć
		d) ściana jest na brzegu siatki - jedynie tu należałoby jakoś wyznaczyć punkt przecięcia, ale to jest chyba przypadek marginalny
		*/

	}

	return foundAny;
}


void createXYMap(CMesh* zeby, std::map<std::pair<int, int>, std::set<size_t>>& xyMap)
{
	xyMap.clear();
	for (int i = 0; i < zeby->faces().size(); i++)
	{
		CFace* f = &zeby->faces()[i];
		CVertex* vA = &zeby->vertices()[f->A()];
		CVertex* vB = &zeby->vertices()[f->B()];
		CVertex* vC = &zeby->vertices()[f->C()];

		xyMap[std::pair<int, int>(floor(vA->X()), floor(vA->Y()))].insert(i);
		xyMap[std::pair<int, int>(floor(vB->X()), floor(vB->Y()))].insert(i);
		xyMap[std::pair<int, int>(floor(vC->X()), floor(vC->Y()))].insert(i);
	}
}


void findGoodVertices(CMesh* zeby, std::map<std::pair<int, int>, std::set<size_t>>& xyMap, std::set<size_t>& dobre)
{
	dobre.clear();
	zeby->calcVN();

	for (int i = 0; i < zeby->vertices().size(); i++)
	{
		UI::STATUSBAR::printfTimed(100, L"Sprawdzam czy wierzchołek jest zasłonięty. Zostało:%d", zeby->vertices().size() - i);

		CVertex v(zeby->vertices()[i]);
		CWektor3D vn(zeby->vnormals()[i]);

		// 1 - spr. czy skladowa Z normalnej jest w kierunku promienia
		if (vn.Z() > 0)
		{
			// 2 - spr. czy promień przecina inne ścianki
			CPunkt3D pkt0(v), pkt1(v), IntersectionPoint;
			size_t faceIdx;

			pkt1.Z(pkt1.Z() + 1000000);

			CWektor3D vRay(pkt0, pkt1);

			bool isBehind = myGetClosestFace(zeby, i, vRay, /*ref*/IntersectionPoint, /*ref*/faceIdx, false, xyMap[std::pair<int, int>(floor(v.X()), floor(v.Y()))]);

			if (!isBehind)
			{
				dobre.insert(i);
			}
		}
	}
}




void kopiujScianki(CMesh* src, CMesh* zeby, CMesh* zeby2, std::set<size_t>& dobre, bool tworzDopelnienie)
{
	//zeby->resetMinMax();
	for (int i = 0; i < src->faces().size(); i++)
	{
		UI::STATUSBAR::printfTimed(100, L"Kopiuję ścianki. Wykonano:%d", i);

		CFace f = src->faces()[i];

		if ((dobre.end() != dobre.find(f.A())) && (dobre.end() != dobre.find(f.B())) && (dobre.end() != dobre.find(f.C())))
		{
			if (src->hasFaceNormals())
				zeby->fnormals().push_back(src->fnormals()[i]);
			zeby->faces().push_back(f);
			//zeby->calcMinMax(src->vertices()[f.A()]);
			//zeby->calcMinMax(src->vertices()[f.B()]);
			//zeby->calcMinMax(src->vertices()[f.C()]);
		}
		else if (tworzDopelnienie)
		{
			if (src->hasFaceNormals())
				zeby2->fnormals().push_back(src->fnormals()[i]);
			zeby2->faces().push_back(f);
		}
	}
}



//void CSymulatorZgryzu::usuwamyOdwrocone(CModel3D* src, bool tworzDopelnienie)
//{
//	wycinek = src->getCopy();
//	AP::WORKSPACE::addModel(wycinek);
//	wycinek->setLabel(L"wycinek");
//
//	CMesh* zeby = (CMesh*)wycinek->getChild();
//	zeby->faces().clear();
//	zeby->fnormals().clear();
//
//	CMesh* zeby2 = NULL;
//
//	if (tworzDopelnienie)
//	{
//		dopelnienie = src->getCopy();
//		AP::WORKSPACE::addModel(dopelnienie);
//		dopelnienie->setLabel(L"dopełnienie");
//
//		zeby2 = (CMesh*)dopelnienie->getChild();
//		zeby2->faces().clear();
//		zeby2->fnormals().clear();
//	}
//
//	std::map<std::pair<int, int>, std::set<size_t>> xyMap; // [floor(vertex_x),floor(vertex_y)] -> set of face_index
//	createXYMap((CMesh*)src->getChild(), xyMap);
//
//	std::set<size_t> dobre;
//
//	findGoodVertices((CMesh*)src->getChild(), xyMap, dobre);
//
//	//wycinek->setVisible(false);	dopelnienie->setVisible(false);
//
//	kopiujScianki((CMesh*)src->getChild(), zeby, zeby2, dobre, tworzDopelnienie);
//
//	//wycinek->setVisible(true); dopelnienie->setVisible(true);
//
//	zeby->getMaterial().FrontColor.ambient.Set(255, 192, 192, 255);
//	if (tworzDopelnienie)
//	{
//		zeby2->getMaterial().FrontColor.ambient.Set(164, 164, 255, 255);
//	}
//
//	UI::STATUSBAR::printf(L"gotowe, w siatce zostało:%d ścianek.", zeby->faces().size());
//}
//
