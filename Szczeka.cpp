#include "Szczeka.h"
#include "Workspace.h"
#include "../api/AP.h"
#include "../api/UI.h"

CSzczeka::CSzczeka(void)
{
}


CSzczeka::~CSzczeka(void)
{
}

void CSzczeka::inicjuj2(std::shared_ptr<CMesh> mesh)
{
	std::shared_ptr<CMesh> m = std::dynamic_pointer_cast<CMesh>(mesh->getCopy());
	m->setLabel("szczeka_mesh");

	obj = std::make_shared<CModel3D>();
	obj->addChild(obj, m);
	obj->importChildrenGeometry();
	obj->setLabel("SZCZEKA");

	obj->getTransform().translation() = CVector3d(0.000000, 0.000000, 0.000000);
	obj->getTransform().rotation().setIdentity();
	obj->getTransform().setScale(1.0);
	obj->getTransform().moveTheOriginToTheCenterOfRotation(false);
	obj->getTransform().lock(true);

	AP::WORKSPACE::addModel(obj);
}

//void CSzczeka::inicjuj(std::wstring path)
//{
//	obj = AP::WORKSPACE::loadModel(path);
//
//	obj->getTransform().translation() = CVector3d(0.000000, 0.000000, 0.000000);
//	obj->getTransform().rotation().setIdentity();
//	obj->getTransform().setScale(1.0);
//	obj->getTransform().moveTheOriginToTheCenterOfRotation(false);
//	obj->getTransform().lock(true);
//}

#include "Plane.h"

void CSzczeka::wytnijZebyNEW(std::shared_ptr<CPlane> cutPlane)
{
	std::shared_ptr<CMesh> robo = std::dynamic_pointer_cast<CMesh>(obj->getChild()->getCopy());

	robo->cutPlane(*cutPlane);

	zeby = std::make_shared<CModel3D>();
	zeby->addChild(zeby, robo);
	zeby->setName(L"---zęby---");
	zeby->setMin(robo->getMin());
	zeby->setMax(robo->getMax());
	zeby->setTransform(obj->getTransform());
}


void CSzczeka::wytnijZeby(std::shared_ptr<CModel3D> obP )
{
	//UI::STATUSBAR::printf("Start... Id: %d", zeby->id());

	std::shared_ptr<CMesh> srcMesh = std::dynamic_pointer_cast<CMesh>(obj->getData());

	CPunkt3D pMin(obP->getMin());
	CPunkt3D pMax(obP->getMax());

	std::shared_ptr<CMesh> dstMesh = std::make_shared<CMesh>();

	dstMesh->setMin(pMin);
	dstMesh->setMax(pMax);

	zeby = std::make_shared<CModel3D>();
	zeby->addChild(zeby, dstMesh);
	zeby->setName(L"---zęby---");
	zeby->setSelfVisibility(false);
	zeby->setTransform( obP->getTransform() );

	if ( ! AP::WORKSPACE::addModel(zeby) ) return;

	std::map<size_t, size_t> indexmap;

	UI::updateAllViews();

	std::vector<CFace>::iterator fi = srcMesh->faces().begin();

	while (fi != srcMesh->faces().end())
	{
		CPunkt3D sA( srcMesh->vertices().at( (*fi).A() ) );
		CPunkt3D sB( srcMesh->vertices().at( (*fi).B() ) );
		CPunkt3D sC( srcMesh->vertices().at( (*fi).C() ) );

		CPunkt3D wA = obj->getTransform().l2w( sA );
		CPunkt3D wB = obj->getTransform().l2w( sB );
		CPunkt3D wC = obj->getTransform().l2w( sC );

		CPunkt3D dA = zeby->getTransform().w2l( wA );
		CPunkt3D dB = zeby->getTransform().w2l( wB );
		CPunkt3D dC = zeby->getTransform().w2l( wC );

		if ( dA.isInBox(pMin, pMax) && dB.isInBox(pMin, pMax) && dC.isInBox(pMin, pMax) ) {
			
			size_t ia;
			if (indexmap.end() == indexmap.find((*fi).A())) {
				ia = indexmap.size();
				indexmap.insert( std::map<size_t, size_t>::value_type((*fi).A(), ia ) );
				
				dstMesh->vertices().push_back( dA );
				//dstMesh->calcMinMax( dA );
			}
			else {
				ia = indexmap[(*fi).A()];
			}

			size_t ib;
			if (indexmap.end() == indexmap.find((*fi).B())) {
				ib = indexmap.size();
				indexmap.insert( std::map<size_t, size_t>::value_type((*fi).B(), ib ) );
				
				dstMesh->vertices().push_back( dB );
				//dstMesh->calcMinMax( dB );
			}
			else {
				ib = indexmap[(*fi).B()];
			}

			size_t ic;
			if (indexmap.end() == indexmap.find((*fi).C())) {
				ic = indexmap.size();
				indexmap.insert( std::map<size_t, size_t>::value_type((*fi).C(), ic ) );
				
				dstMesh->vertices().push_back( dC );
				//dstMesh->calcMinMax( dC );
			}
			else {
				ic = indexmap[(*fi).C()];
			}

			dstMesh->faces().push_back( CFace(ia, ib, ic) );
			dstMesh->fnormals().push_back( CFace(ia, ib, ic).getNormal( dstMesh->vertices() ) );
		}

		fi++;

		UI::STATUSBAR::printfTimed(1000, L"Wycinam zęby. Zostało: %d", srcMesh->faces().end()-fi );
	}

	dstMesh->getMaterial().FrontColor.ambient.B(0);

	zeby->setMin( dstMesh->getMin() );
	zeby->setMax( dstMesh->getMax() );
	
	//zeby->getTransform().setOrigin( ( dstMesh->getMin() + dstMesh->getMax() ) / 2);

	dstMesh->correctNormals();

	zeby->setSelfVisibility(true);

	UI::STATUSBAR::printf("Zeby ID: %d", zeby->id());

	indexmap.clear();
}




void CSzczeka::tworzMapeOkluzji2(std::shared_ptr<CMesh> mesh, CTransform tFrom, CTransform tTo)
{
	mapaOkluzji.clear();

	std::shared_ptr<CMesh> o = std::dynamic_pointer_cast<CMesh>(mesh->getCopy());
	o->setLabel("okluzja_mesh2");

	okluzja = std::make_shared<CModel3D>();
	okluzja->addChild(okluzja, o);
	okluzja->importChildrenGeometry();
	okluzja->setLabel("OKLUZJA");
	okluzja->applyTransformation(tFrom, tTo);

	okluzja->transform() = tTo;

	AP::WORKSPACE::addModel(okluzja);

	//CMesh* mesh = (CMesh*)okluzja->getData();
	for (int i = 0; i < mesh->vertices().size(); i++)
	{
		CVertex v(mesh->vertices().at(i));

		std::pair<int, int> indeks(round(10 * v.X()), round(10 * v.Y()));
		mapaOkluzji[indeks] = v.Z();
	}
}

int CSzczeka::nalezyDoPowierzchniOkluzji( CTriple<double> v )
{
	int x = round( 10 * v.X() );
	int y = round( 10 * v.Y() );
	double z = v.Z();

	std::map< std::pair<int, int>, double >::iterator point = mapaOkluzji.find( std::pair<int, int>( x, y ) );

	int result = 0;

	if ( point != mapaOkluzji.end() )
	{
		//if ( (point->second < ( z + 0.1 ) ) && (point->second > ( z - 0.1 ) ) )
		if ((point->second < (z + 0.75)) && (point->second >(z - 0.75)))
		{
			return 1;
		}
	}
	else
	{
		std::map< std::pair<int, int>, double >::iterator left = mapaOkluzji.find(std::pair<int, int>(x - 1, y));
		std::map< std::pair<int, int>, double >::iterator right = mapaOkluzji.find(std::pair<int, int>(x + 1, y));
		std::map< std::pair<int, int>, double >::iterator top = mapaOkluzji.find(std::pair<int, int>(x, y - 1));
		std::map< std::pair<int, int>, double >::iterator bottom = mapaOkluzji.find(std::pair<int, int>(x, y + 1));

		if ( (   left != mapaOkluzji.end()) ||
			 (  right != mapaOkluzji.end()) ||
			 (    top != mapaOkluzji.end()) ||
			 ( bottom != mapaOkluzji.end()) )
		{
			return 2;
		}
	}

	return 0;
}


//void CSzczeka::korygujDoOkluzji(double dd)		
//{
//	CMesh* dstMesh = (CMesh*)zeby->getData();
//
//	for (int i = 0; i < dstMesh->vnormals().size(); i++)
//	{
//		UI::STATUSBAR::printf(L"koryguje %d", i);
//		
//		CVertex *v1 = &dstMesh->vertices()[i];
//
//		CTriple<double> v(okluzja->getTransform().w2l(zeby->getTransform().l2w(*v1)));
//		
//		int x = round(10 * v.X());
//		int y = round(10 * v.Y());
//		double z = v.Z();
//
//		std::map< std::pair<int, int>, double >::iterator point = mapaOkluzji.find(std::pair<int, int>(x, y));
//
//		if (point != mapaOkluzji.end())
//		{
//			if (z > (mapaOkluzji.at(std::pair<int, int>(x, y)) + dd))
//			{
//				v.Z( mapaOkluzji.at(std::pair<int, int>(x, y)) + dd - 0.1 );
//				
//				v1->Set( zeby->getTransform().w2l( okluzja->getTransform().l2w(v) ) );
//			}
//		}
//	}
//}
//


/* 
Należy zwrócić uwagę na tę funkcję. Wydaje mi sie, ze stare podejście,
gdy punkty należące do okluzji traktowaliśmy inaczej niż inne jest już nie aktualne.
Na razie tego nie zmieniam, bo generowane szyny różniły by sie od tych które testujemy,
a nie ma na czasu na robienie ich od nowa. DO PRZEMYŚLENIA I POPRAWY
*/
void CSzczeka::rozepchajZeby( float d, bool wierzch )
{
	std::shared_ptr<CMesh> dstMesh = std::dynamic_pointer_cast<CMesh>(zeby->getData());

	dstMesh->calcVN();

	for (int i = 0; i < dstMesh->vnormals().size(); i++)
	{
		CVertex *v1 = &dstMesh->vertices()[i];

		CTriple<double> v( okluzja->getTransform().w2l( zeby->getTransform().l2w(*v1) ) );
		
		CVector3f *vn1 = &dstMesh->vnormals()[i];

		if ( wierzch && nalezyDoPowierzchniOkluzji( v ) )
		{
			if (vn1->Z() > 0)
			{
				v1->Z(v1->Z() + d); // !!!
			}
			
		}
		else
		{
			v1->Set(v1->X() + d*vn1->X(), v1->Y() + d*vn1->Y(), v1->Z() + d*vn1->Z());
		}

		AP::processEvents();
	}
}


void rozepchajZebyRegular(CMesh* mesh, float d)
{
	mesh->calcVN(); // oblicza normalne wierzchołków, jeśli ichnie było wcześniej

	for (int i = 0; i < mesh->vnormals().size(); i++)
	{
		CVertex* v1 = &mesh->vertices()[i];
		CVector3f* vn1 = &mesh->vnormals()[i];

		v1->Set(v1->x + d * vn1->x, v1->y + d * vn1->y, v1->z + d * vn1->z);
	}
}


void CSzczeka::rozepchajZebyRegular(float d)
{
	std::shared_ptr<CMesh> dstMesh = std::dynamic_pointer_cast<CMesh>(zeby->getData());

	dstMesh->calcVN();

	for (int i = 0; i < dstMesh->vnormals().size(); i++)
	{
		CVertex* v1 = &dstMesh->vertices()[i];
		CVector3f* vn1 = &dstMesh->vnormals()[i];

		v1->Set(v1->X() + d * vn1->X(), v1->Y() + d * vn1->Y(), v1->Z() + d * vn1->Z());

		AP::processEvents();
	}
}



//int znak(double d)
//{
//	if (d < 0) return -1;
//	else if (d > 0) return 1;
//	else return 0;
//}

//void CSzczeka::rozepchajZeby2(double dX, double dY, bool isWierzch)
//{
//	CMesh* dstMesh = (CMesh*)zeby->getData();
//
//	dstMesh->correctNormals();
//	dstMesh->calcVN();
//
//	//Eigen::Matrix4d
//
//	for (int i = 0; i < dstMesh->vnormals().size(); i++)
//	{
//		UI::STATUSBAR::printfTimed(1000, L"rozpycham %d", i);
//		CVertex *v1 = &dstMesh->vertices()[i];
//
//		CTriple<double> v(okluzja->getTransform().w2l(zeby->getTransform().l2w(*v1)));
//		CWektor3D *vn1 = &dstMesh->vnormals()[i];
//
//		vn1->normalize();
//
//		double nX = vn1->X();
//		double nY = vn1->Y();
//		double nZ = vn1->Z();
//
//		if (isWierzch)
//		{
//			int result = nalezyDoPowierzchniOkluzji(v);
//
//			double absN = abs(nX)+abs(nY);
//
//			double nX_XY = (absN != 0) ? (nX / absN) : 0;
//			double nY_XY = (absN != 0) ? (nY / absN) : 0;
//
//			if (result == 1) // trafiony
//			{
//				if ( nZ > 0)
//					v1->Z( v1->Z() + dY );
//			}
//			else if (result == 2) // nie trafiony ale ma przynajmniej jednego s�siada
//			{
//				double d3 = (dX + dY) / 2;
//				
//				v1->X(v1->X() + dX*nX);// nX_XY);
//				v1->Y(v1->Y() + dX*nY);// nY_XY);
//				v1->Z(v1->Z() + d3);
//			}
//			else // nie nalezy i nie ��czy sie z powierzchni� okluzji
//			{
//				//v1->X(v1->X() + d*nX_XY);
//				//v1->Y(v1->Y() + d*nY_XY);
//				//v1->Z(v1->Z());
//
//				v1->X(v1->X() + dX*nX);
//				v1->Y(v1->Y() + dX*nY);
//				v1->Z(v1->Z() + dX*nZ);
//			}
//		}
//		else
//		{
//			v1->X(v1->X() + dX*nX);
//			v1->Y(v1->Y() + dX*nY);
//			v1->Z(v1->Z() + dX*nZ);
//		}
//
//		AP::processEvents();
//	}
//}
//
//
//void CSzczeka::rozepchajZeby3(double d, double dd, CModel3D *wnetrze, bool isWierzch)
//{
//	CMesh* dstMesh = (CMesh*)zeby->getData();
//
//	dstMesh->calcVN();
//
//	for (int i = 0; i < dstMesh->vnormals().size(); i++)
//	{
//		CVertex *v1 = &dstMesh->vertices()[i];
//
//		CTriple<double> v(okluzja->getTransform().w2l(zeby->getTransform().l2w(*v1)));
//		CWektor3D *vn1 = &dstMesh->vnormals()[i];
//
//		if (isWierzch)
//		{
//			int result = nalezyDoPowierzchniOkluzji(v);
//
//			if (result == 1) // trafiony
//			{
//				if (vn1->Z() > 0)
//					v1->Z(v1->Z() + dd);
//			}
//			else if (result == 2) // nie trafiony ale ma przynajmniej jednaego s�siada
//			{
//				//double d3 = (d + dd) / 2;
//
//				//v1->Set(v1->X() + d*vn1->X(), v1->Y() + d*vn1->Y(), v1->Z() + d3);
//			}
//			else
//			{
//
//				v1->Set(v1->X() + d*vn1->X(), v1->Y() + d*vn1->Y(), v1->Z() + d*vn1->Z());
//			}
//		}
//		else
//		{
//			v1->Set(v1->X() + d*vn1->X(), v1->Y() + d*vn1->Y(), v1->Z() + d*vn1->Z());
//		}
//
//		AP::processEvents();
//	}
//}
//
//void CSzczeka::dajSzyne()
//{
//	CMesh* dstMesh = (CMesh*)zeby->getData();
//
//	zeby2 = zeby->getCopy();
//	AP::WORKSPACE::addModel(zeby2);
//
//	//zeby2->translate(CWektor3D(0.0f, 0.0f, 5.0f));
//	
//	zeby2->getTransform().setScale(1.0);
//	zeby2->getTransform().moveTheOriginToTheCenterOfRotation(false);
//
//	CMesh* mesh2 = (CMesh*)zeby2->getData();
//
//
//	for (int i = 0; i < dstMesh->vnormals().size(); i++)
//	{
//		CVertex *v1 = &dstMesh->vertices()[i];
//		CWektor3D *vn1 = &dstMesh->vnormals()[i];
//
//		v1->Set(v1->X() + 0.5f*vn1->X(), v1->Y() + 0.5f*vn1->Y(), v1->Z() + 0.5f*vn1->Z());
//
//		vn1->invert();
//
//	}
//
//	for (int i = 0; i < mesh2->vnormals().size(); i++)
//	{
//		CVertex *v2 = &mesh2->vertices()[i];
//		CWektor3D *vn2 = &mesh2->vnormals()[i];
//
//		v2->Set(v2->X() + vn2->X(), v2->Y() + vn2->Y(), v2->Z() + vn2->Z());
//	}
//
//
//	for (int i = 0; i < dstMesh->fnormals().size(); i++)
//	{
//		CWektor3D *fn = &dstMesh->fnormals()[i];
//
//		fn->invert();
//	}
//
//
//	mesh2->getMaterial().FrontColor.ambient.R(0);
//}
