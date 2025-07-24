#include "ConcretePlugin.h"

#include "Workspace.h"
//#include "MainApplication.h"
#include "AnnotationPlane.h"
#include "AnnotationEdges.h"

#include "BiteSim.h"

//#include "HFMesh.h"
//#include "HoleFiller.h"

#include "../api/AP.h"
#include "../api/UI.h"

#include "AppSettings.h"

#include "NewEdge.h"
#include "Vector3.h"
#include "IndexedTriangle.h"
#include "KDNode.h"

#include "../gui/ProgressIndicator.h"
#include "FileConnector.h"

#include <omp.h>

//#define UseICP
//#define ExpT
//#include "dpLog.h"
#include <QDebug>

ConcretePlugin::ConcretePlugin(void)
{
	m_picking = false;

	waiting_for = CzekamNa::Nic;
	szcz = nullptr;
	oklu = nullptr;
	zuch = nullptr;
	szcz_parent = nullptr;
	top_model_arch = nullptr;
	mesh_wierzch = nullptr;
	mesh_wnetrze = nullptr;

	symulator = nullptr;

	m_workdir = L"../dane/";

	m_cutPlane = nullptr;

	//m_rzutnia = new CAnnotationPlane();
	//m_rzutnia->setSize(80.0);
	//m_rzutnia->setLabel(L"plaszczyzna rzutowania");
	//m_rzutnia->setColor(CRGBA(0.6f, 0.6f, 0.5f, 0.7f));

	m_divider = 10;

	QObject::connect(this, &ConcretePlugin::setProgressBarValue, UI::PROGRESSBAR::instance(), &ProgressIndicator::setValue);
}


ConcretePlugin::~ConcretePlugin(void)
{
	//UI::MESSAGEBOX::error( L"I'm UNLOADED" );
}


std::shared_ptr<CMesh>  ConcretePlugin::liczOkluzje(std::shared_ptr<CMesh>  szczeka, std::shared_ptr<CMesh>  zuchwa, double dist = 3.0)
{
	std::set<size_t> good;

	CMesh::KDtree *kd = &zuchwa->getKDtree(CMesh::KDtree::REBUILD);

	UI::STATUSBAR::setText("Looking for unwanted vertices. Plese wait...");
	UI::PROGRESSBAR::init(0, szczeka->vertices().size(), 0);

	int progress = 0;

#pragma omp parallel for
	for (long idx = 0; idx < szczeka->vertices().size(); idx++)
	{
		CVertex& v = szczeka->vertices()[idx];

		if (kd->is_any_in_distance_to_pt(dist,v))
			good.insert(idx);

#pragma omp atomic
		progress++;

#pragma omp critical
		{
			emit setProgressBarValue(progress);
		}

	}

	zuchwa->removeKDtree();
	UI::PROGRESSBAR::hide();

	std::shared_ptr<CMesh> okluzja = std::dynamic_pointer_cast<CMesh>(szczeka->getCopy());

	UI::STATUSBAR::setText("Removing unwanted faces. Plese wait...");

	std::vector<CFace>& faces = okluzja->faces();
	std::vector<bool> toErase(faces.size(), false);

	UI::PROGRESSBAR::init(0, faces.size(), 0);
	progress = 0;

#pragma omp parallel for
	for (int idx = 0; idx < faces.size(); ++idx) {
		CFace& f = faces[idx];

		if ((good.find(f.A()) == good.end()) || (good.find(f.B()) == good.end()) || (good.find(f.C()) == good.end())) {
			toErase[idx] = true;
		}

#pragma omp atomic
		++progress;

#pragma omp critical
		{
			emit setProgressBarValue(progress);
		}
	}

	// Usunięcie elementów w sposób sekwencyjny
	// auto it = std::remove_if(faces.begin(), faces.end(),
	// 	[&toErase, idx = 0](const CFace&) mutable {
	// 		return toErase[idx++];
	// 	});
	// faces.erase(it, faces.end());

    size_t idx = 0;
    auto it = std::remove_if(faces.begin(), faces.end(),
        [&toErase, &idx](const CFace&) {
            return toErase[idx++];
        });
    faces.erase(it, faces.end());


	okluzja->removeUnusedVertices();

	UI::PROGRESSBAR::hide();

	UI::STATUSBAR::setText("Ready!");

	return okluzja;
}

std::shared_ptr<CMesh> meshWithKeyword(QString keyword, CObject::Children kids)
{
	for (auto& kid : kids) {
		if (kid.second->hasKeyword(keyword) && kid.second->hasType(CObject::Type::MESH))
			return std::dynamic_pointer_cast<CMesh>(kid.second);

		if (kid.second->hasChildren()) {
			std::shared_ptr<CMesh> tmp =
				meshWithKeyword(keyword, std::dynamic_pointer_cast<CObject>(kid.second)->children());
			if (tmp != nullptr)
				return tmp;
		}
	}
	return nullptr;
}


std::shared_ptr<CBaseObject> somethingWithKeyword(QString keyword, std::shared_ptr<CBaseObject> root)
{
	if (root->hasKeyword(keyword))
		return root;
	else {
		if (root->hasCategory(CBaseObject::OBJECT)) {
			auto r = std::dynamic_pointer_cast<CObject>(root);
			for (auto kid : r->children()) {
				std::shared_ptr<CBaseObject> obj = somethingWithKeyword(keyword, kid.second);
				if (obj) return obj;
			}
			for (auto kid : r->annotations()) {
				std::shared_ptr<CBaseObject> obj = somethingWithKeyword(keyword, kid.second);
				if (obj) return obj;
			}
		}
		else if (root->hasCategory(CBaseObject::ANNOTATION)) {
			auto r = std::dynamic_pointer_cast<CAnnotation>(root);
			for (auto kid : r->annotations()) {
				std::shared_ptr<CBaseObject> obj = somethingWithKeyword(keyword, kid.second);
				if (obj) return obj;
			}
		}
	}
	return nullptr;
}


std::shared_ptr<CMesh> meshWithKeywordInLabel(QString keyword, CObject::Children kids)
{
	for (auto& kid : kids) {
		if (kid.second->getLabel().contains(keyword, Qt::CaseInsensitive) && kid.second->hasType(CObject::Type::MESH))
			return std::dynamic_pointer_cast<CMesh>(kid.second);

		if (kid.second->hasChildren()) {
			std::shared_ptr<CMesh> tmp = meshWithKeywordInLabel(keyword, std::dynamic_pointer_cast<CObject>(kid.second)->children());
			if (tmp != nullptr)
				return tmp;
		}
	}
	return nullptr;
}


void ConcretePlugin::wczytaj_spreparowany_ATMDL()
{
	QString fileName = QDir::toNativeSeparators(
		QFileDialog::getOpenFileName( 0, QString::fromUtf8("Wybierz plik szczeki"), AppSettings::mainSettings()->value("recentFile").toString(), CFileConnector::getLoadExts())
	);

	if (!fileName.isEmpty() && QFileInfo(fileName).exists()) {
		std::shared_ptr<CModel3D> obj = AP::WORKSPACE::loadModel(fileName);
		
		if (obj == nullptr) { //ERROR
			UI::MESSAGEBOX::error(QString::fromUtf8("Nie udało sie otworzyć pliku %1").arg(fileName), QString::fromUtf8("Błąd odczytu pliku"));
			return;
		}

		if (obj->hasChildren()) {
			AppSettings::mainSettings()->setValue("recentFile", fileName);
		}

		qInfo() << QString("Licznik referencji do obj: %1").arg(obj.use_count());

		UI::updateAllViews();

		

		m_cutPlane = std::dynamic_pointer_cast<CAnnotationPlane>( somethingWithKeyword("cut_plane", obj) );

		szcz = meshWithKeyword("upper", obj->children());

		if (szcz == nullptr) {
			szcz = meshWithKeywordInLabel("upper", obj->children());
		}

		if (szcz == nullptr) {
			szcz = meshWithKeywordInLabel("szczeka", obj->children());
		}

		if (szcz) {
			moj_widget->wybor_siatek->ustawSzczeke(szcz->getLabel());
			szcz_parent = std::dynamic_pointer_cast<CModel3D>(szcz->getParentPtr());
		}

		zuch = meshWithKeyword("lower", obj->children());

		if (zuch == nullptr) {
			zuch = meshWithKeywordInLabel("lower", obj->children());
		}

		if (zuch == nullptr) {
			zuch = meshWithKeywordInLabel("zuchwa", obj->children());
		}

		if (zuch) {
			moj_widget->wybor_siatek->ustawZuchwe(zuch->getLabel());
		}

		oklu = meshWithKeyword("occlusion", obj->children());

		if (oklu == nullptr) {
			oklu = meshWithKeywordInLabel("occlusion", obj->children());
		}


		qInfo() << QString("Licznik referencji do oklu: %1").arg(oklu.use_count());
		//return;

		if (oklu) {
			moj_widget->wybor_siatek->ustawOkluzje(oklu->getLabel());

			if (szcz && zuch && oklu->hasKeyword("inv_jaw")) {
				Eigen::Matrix4d Ms = CBaseObject::getGlobalTransformationMatrix(szcz);
				Eigen::Matrix4d Mz = CBaseObject::getGlobalTransformationMatrix(zuch);
				Eigen::Matrix4d M = (Mz * Ms.inverse()).inverse();


				std::shared_ptr<CBaseObject> oklu_parent = oklu->getParentPtr();

				AP::OBJECT::removeChild(oklu_parent, oklu);

				std::shared_ptr<CModel3D> new_oklu_parent = std::make_shared<CModel3D>();
				new_oklu_parent->addChild(new_oklu_parent, oklu);
				new_oklu_parent->setTransform(M);

				AP::OBJECT::addChild(oklu_parent, new_oklu_parent);


			}
		}

		std::shared_ptr<CBaseObject> sz = somethingWithKeyword("new_upper", obj);
		if (sz && sz->hasType(CObject::MESH)) {
			szcz = std::dynamic_pointer_cast<CMesh>(sz);
			moj_widget->wybor_siatek->ustawSzczeke(szcz->getLabel());
		}

		if (szcz && zuch) {
			moj_widget->wybor_siatek->enableLiczOklu(true);
		}

		if (szcz && oklu) {
			moj_widget->etap11->setEnabled(true);
			moj_widget->przytnij_szczene->setEnabled(true);
		}

		m_workdir = QFileInfo(fileName).absolutePath().toStdWString();
	}
}

void ConcretePlugin::etap00(double dist2, bool dane_z_pomiaru)
{
	std::shared_ptr<CMesh> sz1 = std::dynamic_pointer_cast<CMesh>(szcz->getCopy());
	std::shared_ptr<CMesh> zu1 = std::dynamic_pointer_cast<CMesh>(zuch->getCopy());

	Eigen::Matrix4d mSz = CBaseObject::getGlobalTransformationMatrix(szcz);
	Eigen::Matrix4d mZu = CBaseObject::getGlobalTransformationMatrix(zuch);
	
	CTransform tSz(mSz);
	CTransform tZu(mZu);

	if (dane_z_pomiaru)
	{
		zu1->applyTransformation(tZu, tSz);
	}

	CTransform invT = (Eigen::Matrix4d) (mZu.inverse() * mSz);

	QCursor c = moj_widget->cursor();
	c.setShape(Qt::CursorShape::WaitCursor);
	moj_widget->setCursor(c);

	oklu = liczOkluzje(sz1, zu1, dist2);
	oklu->setLabel(QString("okluzja: %1mm").arg(dist2));
	oklu->calcFN();

	std::shared_ptr<CBaseObject> zP = zuch->getParentPtr();
	std::shared_ptr<CBaseObject> sP = szcz->getParentPtr();

	if (dane_z_pomiaru)
	{
		std::shared_ptr<CModel3D> obj = std::make_shared<CModel3D>();
		obj->addChild(obj, oklu);
		obj->importChildrenGeometry();
		obj->setLabel("inv");
		obj->setTransform(invT);
		if (sP)
		{
			AP::OBJECT::addChild(sP, obj);
		}
		else
		{
			AP::WORKSPACE::addModel(obj);
		}
	}
	else
	{
		if (zP) {
			AP::OBJECT::addChild(zP, oklu);
		}
		else {
			auto obj = std::make_shared<CModel3D>();
			obj->addChild(obj, oklu);
			obj->importChildrenGeometry();
			obj->setLabel("ROBOCZE");
			obj->setTransform(mZu);
			AP::WORKSPACE::addModel(obj);
		}
	}

	c.setShape(Qt::CursorShape::ArrowCursor);
	moj_widget->setCursor(c);

	UI::updateAllViews();
}


void ConcretePlugin::etap01(int div)
{
	if (oklu == nullptr || szcz == nullptr)
	{
		UI::MESSAGEBOX::error("Najpierw wybierz siatki do przetwarzania.");
		return;
	}

	moj_widget->etap11->setDisabled(true);
	moj_widget->przytnij_szczene->setHidden(true); // >setDisabled(true);
	moj_widget->wybor_siatek->setHidden(true); // > setDisabled(true);

	std::shared_ptr<CModel3D> top_model = szcz_parent;
	while (top_model->getParent() != nullptr)
	{
		top_model = std::dynamic_pointer_cast<CModel3D>(top_model->getParentPtr());
	}

	top_model_arch = top_model;
	AP::WORKSPACE::removeModel(top_model);

	QCursor c = moj_widget->cursor();
	c.setShape(Qt::CursorShape::WaitCursor);
	moj_widget->setCursor(c);

	m_divider = div;

	symulator = std::make_shared<BiteSim>();
	symulator->m_divider = m_divider;

	symulator->szczeka_inicjuj2(szcz);

	symulator->szczeka_obj->transform() = CBaseObject::getGlobalTransformationMatrix(szcz);
	symulator->szczeka_obj->applyTransform();

	Eigen::Matrix4d mSz = CBaseObject::getGlobalTransformationMatrix(szcz);
	Eigen::Matrix4d mOk = CBaseObject::getGlobalTransformationMatrix(oklu);
	
	//std::cout << mSz << endl;
	//std::cout << mOk << endl;

	symulator->szczeka_tworzMapeOkluzji2(oklu, mOk, mSz);
	
	//symulator->szczeka_okluzja->transform() = szcz->getGlobalTransformationMatrix();
	//symulator->szczeka_okluzja->applyTransform();

	//AP::mainApp().settings->value("plugins/nowaSzyna/cutPlane");

	CBoundingBox bb = symulator->szczeka_obj->getBoundingBox();
	CBoundingBox bbOk = symulator->szczeka_okluzja->getBoundingBox();

	if (m_cutPlane == nullptr) {
		m_cutPlane = std::make_shared<CAnnotationPlane>();
		m_cutPlane->setSize(80.0);
		m_cutPlane->setLabel(L"plaszczyzna ciecia");
		m_cutPlane->setColor(CRGBA(1.0f, 0.8f, 0.3f, 0.7f));

		m_cutPlane->setCenter(bb.getMidpoint());


		//CVector3d n1 = CVector3d(bb.getMidpoint(), bbOk.getMidpoint()).getNormalized();

		//CVector3d n2 = CVector3d::ZAxis();

		//if ((bb.dX() <= bb.dY()) && (bb.dX() <= bb.dZ())) {
		//	n2 = CVector3d::XAxis();
		//}
		//else if ((bb.dY() <= bb.dX()) && (bb.dY() <= bb.dZ())) {
		//	n2 = CVector3d::YAxis();
		//}

		//double il = n1.dotProduct(n2);

		//if (il < 0) n2 = -n2;

		CVector3d n2 = std::dynamic_pointer_cast<CMesh>(symulator->szczeka_okluzja->getData())->getMainNormalVector();
		n2.x = 0.0;
		n2.y = 0.0;

		m_cutPlane->setNormal(n2);
	}
	//Eigen::Matrix4d M = szcz->getGlobalTransformationMatrix().inverse();
	//CAnnotationPlane* pp = new CAnnotationPlane(m_cutPlane->get_transformed(M));
	//pp->setSize(m_cutPlane->getSize());
	//AP::WORKSPACE::addObject(pp);
	//CModel3D* pp_par = (CModel3D*)pp->getParent();
	//pp_par->transform() = szcz->getGlobalTransformationMatrix();

	//m_rzutnia->setCenter(m_cutPlane->getCenter());
	//m_rzutnia->setNormal(m_cutPlane->getNormal());

	AP::MODEL::addAnnotation(symulator->szczeka_obj, m_cutPlane);
	UI::updateAllViews();

	AP::WORKSPACE::setCurrentModel(NO_CURRENT_MODEL);


	moj_widget->widget_info = new WidgetInfo({ QString::fromUtf8("Aby odciąć nadmiarowe fragmenty siatki ustaw płaszczyznę cięcia we właściwej pozycji względem szczęki i kliknij \"Dalej\"") });

	moj_layout->addRow(moj_widget->widget_info);

	QObject::connect(moj_widget->widget_info->btStart, &QPushButton::clicked, [&]() { etap11(); });

	c.setShape(Qt::CursorShape::ArrowCursor);
	moj_widget->setCursor(c);
}


//QMatrix4x4 rotation_matrix_from_vectors(CVector3d _n1, CVector3d _n2) {
//	// Normalize vectors
//	CVector3d n1 = _n1.getNormalized();
//	CVector3d n2 = _n2.getNormalized();
//
//	// Angle of rotation (dot product)
//	double cosTheta = n1.dotProduct(n2);
//	double theta = acos(cosTheta);
//
//	// Check if vectors are anti-parallel
//	if (std::abs(cosTheta + 1.0) < 1e-6) {
//		// Find an orthogonal vector for the axis of rotation
//		CVector3d ortho = n1.crossProduct(CVector3d(1, 0, 0));
//		if (ortho.length() < 1e-6) { // If n1 is parallel to (1,0,0), use a different orthogonal vector
//			ortho = n1.crossProduct(CVector3d(0, 1, 0));
//		}
//		ortho = ortho.getNormalized();
//
//		// 180 degree rotation matrix around orthogonal vector
//		QMatrix4x4 R;
//		double x = ortho.x;
//		double y = ortho.y;
//		double z = ortho.z;
//		R(0, 0) = -1 + 2 * x * x;
//		R(0, 1) = 2 * x * y;
//		R(0, 2) = 2 * x * z;
//		R(1, 0) = 2 * x * y;
//		R(1, 1) = -1 + 2 * y * y;
//		R(1, 2) = 2 * y * z;
//		R(2, 0) = 2 * x * z;
//		R(2, 1) = 2 * y * z;
//		R(2, 2) = -1 + 2 * z * z;
//		R(3, 3) = 1;
//
//		return R;
//	}
//
//	// Axis of rotation (cross product)
//	CVector3d k = n1.crossProduct(n2);
//
//	// Rodrigues' rotation formula components
//	QMatrix4x4 K;
//	K.setRow(0, QVector4D(0, -k.z, k.y, 0));
//	K.setRow(1, QVector4D(k.z, 0, -k.x, 0));
//	K.setRow(2, QVector4D(-k.y, k.x, 0, 0));
//	K.setRow(3, QVector4D(0, 0, 0, 1));
//
//	// Identity matrix
//	QMatrix4x4 I;
//	I.setToIdentity();
//
//	// Compute R using Rodrigues' rotation formula
//	QMatrix4x4 R = I + K * sin(theta) + K * K * (1 - cos(theta));
//
//	return R;
//}

QMatrix4x4 planeToTransform(CVector3d norm) {
	QVector3D planeNormal(norm[0], norm[1], norm[2] );
	planeNormal.normalize();

	// kostka w której będzie wykonywane rzutowanie
	// ma być tak ustawiona, żeby oś Z była
	// mniej więcej zgodna z wektorem normalnym płaszczyzny
	// cięcia, dlatego bierzemy wektor osi Z
	QVector3D objectZAxis(0.0f, 0.0f, 1.0f); 

	// Obliczamy kąt i oś rotacji
	float cosTheta = QVector3D::dotProduct(objectZAxis, planeNormal);
	float angle = qAcos(cosTheta);

	QVector3D rotationAxis = QVector3D::crossProduct(objectZAxis, planeNormal);

	// Obsługa przypadku równoległego/antyrównoległego
	if (rotationAxis.length() < 1e-6f) {
		if (cosTheta < 0.0f) {
			// Obrót o 180 stopni
			rotationAxis = QVector3D(1.0f, 0.0f, 0.0f);
			angle = M_PI;
		}
		else {
			qInfo() << "Wektory są już wyrównane." << Qt::endl;
			return QMatrix4x4();
		}
	}
	else {
		rotationAxis.normalize();
	}

	// Tworzymy quaternion rotacji
	QQuaternion rotationQuat = QQuaternion::fromAxisAndAngle(rotationAxis, qRadiansToDegrees(angle));
	QMatrix4x4 rotationMatrix;
	rotationMatrix.rotate(rotationQuat);

	return rotationMatrix;
}

void ConcretePlugin::etap11()
{
	moj_widget->widget_info->setDisabled(true);

	m_cutPlane->setSelfVisibility(false);
	UI::updateAllViews();

	QCursor c = moj_widget->cursor();
	c.setShape(Qt::CursorShape::WaitCursor);
	moj_widget->setCursor(c);

	symulator->szczeka_wytnijZebyNEW(m_cutPlane);
	AP::WORKSPACE::addModel(symulator->szczeka_zeby);

	symulator->wnetrze = new CSiateczka1(80, 80, 30, symulator->m_divider); // arguments: sizeX, sizeY, depth, divider

	AP::WORKSPACE::addModel(symulator->wnetrze->obj);

	m_plaszczyzna_rzutowania = std::make_shared<CAnnotationPlane>(CPoint3d(0,0,0), CVector3d(0,0,1));
	m_plaszczyzna_rzutowania->setSize(80);

	AP::OBJECT::addChild(symulator->wnetrze->obj, m_plaszczyzna_rzutowania);
	
	symulator->wnetrze->obj->transform().fromQMatrix4x4(planeToTransform(m_cutPlane->getNormal()));
	symulator->wnetrze->obj->transform().translate(CVector3d(CPoint3d(0),m_cutPlane->getCenter()));
	
	UI::updateAllViews();

	moj_widget->etap12 = new WidgetEtap12();
	moj_layout->addRow(moj_widget->etap12);

	QObject::connect(moj_widget->etap12->btStart, &QPushButton::clicked, [&]() { etap12(moj_widget->etap12->insideDist->value()); });

	c.setShape(Qt::CursorShape::ArrowCursor);
	moj_widget->setCursor(c);
}

void ConcretePlugin::etap12(double dVal)
{
	moj_widget->etap12->setDisabled(true);

	QCursor c = moj_widget->cursor();
	c.setShape(Qt::CursorShape::WaitCursor);
	moj_widget->setCursor(c);

	//m_rzutnia_copy = new CAnnotationPlane(*m_rzutnia);

	qInfo() << "etap12 -> symulator->generujWnetrzeNEW()";

	symulator->create_inner_surface(symulator->wnetrze, dVal);
	mesh_wnetrze = std::dynamic_pointer_cast<CMesh>(symulator->wnetrze->obj->getChild());


	UI::DOCK::WORKSPACE::update();

	moj_widget->etap13 = new WidgetEtap13();
	moj_layout->addRow(moj_widget->etap13);

	//QObject::connect(moj_widget->etap13->btStart, &QPushButton::clicked, [&]() { etap13(moj_widget->etap13->outsideDist->value()); });
	QObject::connect(moj_widget->etap13->btStart2, &QPushButton::clicked, [&]() { etap13_v2(moj_widget->etap13->outsideDist->value()); });
	//QObject::connect(moj_widget->etap13->btStart3, &QPushButton::clicked, [&]() { odcisk_stepla(); });

	c.setShape(Qt::CursorShape::ArrowCursor);
	moj_widget->setCursor(c);
}

//
//void ConcretePlugin::odcisk_stepla()
//{
//	qInfo() << "odcisk stempla";
//	std::list<std::shared_ptr<CBaseObject>> sel = AP::WORKSPACE::SELECTION::getObjList({ CObject::Type::MESH });
//
//	if (sel.size() > 0)
//	{
//		std::list<std::shared_ptr<CBaseObject>>::iterator isel = sel.begin();
//
//		//std::shared_ptr<CMesh>  wierzch1 = ((CMesh*)(*isel))->getCopy();
//		//wierzch1->setParent(nullptr);
//		//isel++;
//		std::shared_ptr<CMesh> stempel = std::dynamic_pointer_cast<CMesh>(*isel);
//		
//		symulator->wierzch->zrobWyciskZuchwy_v1(std::dynamic_pointer_cast<CModel3D>(stempel->getParentPtr()), 0.0, 1);
//
//		symulator->wierzch->klejDziury3();
//		symulator->wierzch->usunNadmiaroweScianki();
//
//	}
//}
//

void ConcretePlugin::etap13_v2(double dVal)
{
	//moj_widget->etap13->setDisabled(true);

	QCursor c = moj_widget->cursor();
	c.setShape(Qt::CursorShape::WaitCursor);
	moj_widget->setCursor(c);

	qInfo() << "etap13 - wersja 2";

	std::shared_ptr<CMesh> zu = std::dynamic_pointer_cast<CMesh>(zuch->getCopy());

	Eigen::Matrix4d ms = CBaseObject::getGlobalTransformationMatrix(szcz_parent);
	Eigen::Matrix4d mz = CBaseObject::getGlobalTransformationMatrix(zuch);
	
	//UWAGA to moze byc potrzebny wybór ms lub mz w zależności od położenia źródłowej żuchwy
	Eigen::Matrix4d mm = ms;// mz;

	AP::WORKSPACE::addObject(zu);
	((CModel3D*)zu->getParent())->transform() = mm;

	symulator->create_outer_surface(dVal, zu);
	
	mesh_wierzch = std::dynamic_pointer_cast<CMesh>(symulator->wierzch->obj->getChild());

	UI::DOCK::WORKSPACE::update();

	AP::OBJECT::removeChild(symulator->wnetrze->obj, m_plaszczyzna_rzutowania);

	m_cutPlane->setSelfVisibility(true);
	UI::updateAllViews();

	moj_widget->etap14 = new WidgetInfo({ QString::fromUtf8("Możesz teraz jeszcze skorygować ustawienie płaszczyzny cięcia. Następnie kliknij przycisk \"Dotnj\".") });
	moj_widget->etap14->btStart->setText("Dotnij");

	moj_layout->addRow(moj_widget->etap14);

	QObject::connect(moj_widget->etap14->btStart, &QPushButton::clicked, [&]() { etap14(); });

	c.setShape(Qt::CursorShape::ArrowCursor);
	moj_widget->setCursor(c);
}

//
//void ConcretePlugin::etap13(double dVal)
//{
//	moj_widget->etap13->setDisabled(true);
//
//	QCursor c = moj_widget->cursor();
//	c.setShape(Qt::CursorShape::WaitCursor);
//	moj_widget->setCursor(c);
//
//	qInfo() << "etap13 - symulator->generujWierzchOdRazu2()";
//
//	std::shared_ptr<CMesh> zu = std::dynamic_pointer_cast<CMesh>(zuch->getCopy());
//
//	Eigen::Matrix4d ms = CBaseObject::getGlobalTransformationMatrix(szcz_parent);
//	Eigen::Matrix4d mz = CBaseObject::getGlobalTransformationMatrix(zuch);
//
//	//UWAGA to moze byc potrzebny wybór ms lub mz w zależności od położenia źródłowej żuchwy
//	Eigen::Matrix4d mm = ms;// mz;
//
//	AP::WORKSPACE::addObject(zu);
//	((CModel3D*)zu->getParent())->transform() = mm;
//
//	//symulator->generujWierzchOdRazu3(dVal, zu);
//	symulator->generujWierzchOdRazu3_CSiateczka1(dVal, zu);
//
//	mesh_wierzch = std::dynamic_pointer_cast<CMesh>(symulator->wierzch->obj->getChild());
//
//	UI::DOCK::WORKSPACE::update();
//
//	AP::OBJECT::removeChild(symulator->wnetrze->obj, m_plaszczyzna_rzutowania);
//
//	m_cutPlane->setSelfVisibility(true);
//	UI::updateAllViews();
//
//	moj_widget->etap14 = new WidgetInfo({ QString::fromUtf8("Możesz teraz jeszcze skorygować ustawienie płaszczyzny cięcia. Następnie kliknij przycisk \"Dotnj\".") });
//	moj_widget->etap14->btStart->setText("Dotnij");
//
//	moj_layout->addRow(moj_widget->etap14);
//
//	QObject::connect(moj_widget->etap14->btStart, &QPushButton::clicked, [&]() { etap14(); });
//
//	c.setShape(Qt::CursorShape::ArrowCursor);
//	moj_widget->setCursor(c);
//}
//

#include "AnnotationPoints.h"

void decapitation(std::shared_ptr<CBaseObject> victim, std::shared_ptr<CAnnotationPlane> guillotine)
{
	if ((nullptr == victim) || (nullptr == guillotine)) return;

	CPlane p = *guillotine;

	CPoint3d p1 = p.m_center;
	CPoint3d p2 = p1 + p.m_normal;

	Eigen::Matrix4d T0 = CBaseObject::getGlobalTransformationMatrix(guillotine);

	p1 = T0 * p1; // do wsp. workspace
	p2 = T0 * p2;

	Eigen::Matrix4d T1 = CBaseObject::getGlobalTransformationMatrix(victim);
	Eigen::Matrix4d T1inv = T1.inverse();

	p1 = T1inv * p1; // do wsp. szczeka_obj
	p2 = T1inv * p2;

	p.m_center = p1;
	p.m_normal = CVector3d(p1, p2).getNormalized();


	if ((victim->hasType(CObject::CLOUD)) || (victim->hasType(CObject::ORDEREDCLOUD)))
	{
		std::shared_ptr<CPointCloud> cloud = std::dynamic_pointer_cast<CPointCloud>(victim);
		std::shared_ptr<CPointCloud> rest = std::make_shared<CPointCloud>();

		cloud->cutPlane(p, rest);

		AP::OBJECT::addChild(victim, rest);
	}
	else if (victim->hasType(CObject::MESH))
	{
		std::shared_ptr<CMesh> mesh = std::dynamic_pointer_cast<CMesh>(victim);
		
		if (mesh->hasFaces())
		{
			std::shared_ptr<CAnnotationPoints> pts = std::make_shared<CAnnotationPoints>();
			std::shared_ptr<CMesh> krawedz = std::make_shared<CMesh>();  //nullptr;
			std::shared_ptr<CMesh> reszta = std::make_shared<CMesh>();

			mesh->cutPlane(p, reszta, krawedz, pts);

			AP::OBJECT::addChild(victim, pts);
			AP::OBJECT::addChild(victim, krawedz);
			AP::OBJECT::addChild(victim, reszta);
		}
		else
		{
			std::shared_ptr<CPointCloud> rest = std::make_shared<CPointCloud>();

			std::dynamic_pointer_cast<CPointCloud>(victim)->cutPlane(*guillotine, rest); //DLACZEGO NIE p ????

			AP::OBJECT::addChild(victim, rest);
		}
	}
	UI::updateAllViews();

}


void ConcretePlugin::etap14()
{
	moj_widget->etap14->setDisabled(true);

	QCursor c = moj_widget->cursor();
	c.setShape(Qt::CursorShape::WaitCursor);
	moj_widget->setCursor(c);

	symulator->wierzch->obj->applyTransform();

	std::shared_ptr<CMesh> wierzch = std::dynamic_pointer_cast<CMesh>(symulator->wierzch->obj->getChild());

	decapitation(wierzch, m_cutPlane);

	//----------------------------------------------------------------

	symulator->wnetrze->obj->applyTransform();

	std::shared_ptr<CMesh> wnetrze = std::dynamic_pointer_cast<CMesh>(symulator->wnetrze->obj->getChild());

	decapitation(wnetrze, m_cutPlane);

	//----------------------------------------------------------------

	//((CModel3D*)symulator->mBD->getParent())->applyTransform();

	//std::shared_ptr<CMesh>  _mBD = symulator->mBD;

	//decapitation(_mBD, m_cutPlane);

	//----------------------------------------------------------------

	//((CModel3D*)symulator->mZD->getParent())->applyTransform();

	//std::shared_ptr<CMesh>  _mZD = symulator->mZD;

	//decapitation(_mZD, m_cutPlane);

	//----------------------------------------------------------------

	AP::OBJECT::removeChild(symulator->szczeka_obj, m_cutPlane);

	UI::DOCK::WORKSPACE::update();


	moj_widget->info_exchange = new WidgetInfo({
	QString::fromUtf8("W tym miejscu powinno sie dać podmienić siatkę reprezentująca wierzch na inną..."),
	QString::fromUtf8("Zaznacz checkbox przy siatce (Mesh) która chcesz wstawić do projektu i kliknij:"),
		});
	moj_widget->info_exchange->btStart->setText("EXCHANGE");

	moj_layout->addRow(moj_widget->info_exchange);

	QObject::connect(moj_widget->info_exchange->btStart, &QPushButton::clicked, [&]() {
		go_to_exchange();
		});


	moj_widget->etap15 = new WidgetInfo({
		QString::fromUtf8("Program podejmie próbę połączenia powierzchni wewnętrznej i zewnętrznej szyny, tworząc \"wodoszczelną\" siatkę."), 
		QString::fromUtf8("Ten etap jest jeszcze w fazie testowej i nie zawsze efekt jest zadowalający."),
		QString::fromUtf8("Może być potrzebne użycie zewnętrznego oprogramowania w celu uzyskania poprawnej siatki.")
		});
	moj_widget->etap15->btStart->setText("Połącz w szynę");

	moj_layout->addRow(moj_widget->etap15);

	QObject::connect(moj_widget->etap15->btStart, &QPushButton::clicked, [&]() { etap_dekiel_cien(); });

	c.setShape(Qt::CursorShape::ArrowCursor);
	moj_widget->setCursor(c);
}


void ConcretePlugin::go_to_exchange()
{
	std::list<std::shared_ptr<CBaseObject>> sel = AP::WORKSPACE::SELECTION::getObjList({CBaseObject::Type::MESH}, nullptr);

	if (!sel.empty()) {
		std::shared_ptr<CMesh> m = std::dynamic_pointer_cast<CMesh>(*sel.begin());


		CObject::Children kids;

		for (const auto& kid : AP::WORKSPACE::instance()->children())
		{
			kids[kid.first] = kid.second;
		}

		std::shared_ptr<CMesh> tmp = meshWithKeywordInLabel(QString("wierzch_mesh"), kids);
		if (tmp != nullptr)
		{
			qInfo() << "Znaleziono: " << tmp->getLabel();
			
			m->setLabel("wierzch_mesh");
			tmp->setLabel("old_wrzch_msh");

			std::shared_ptr<CBaseObject> parentM = m->getParentPtr();

			AP::OBJECT::removeChild(parentM, m);

			std::shared_ptr<CBaseObject> parent = tmp->getParentPtr();

			AP::OBJECT::removeChild(parent, tmp);

			AP::OBJECT::addChild(parent, m);
			AP::OBJECT::addChild(parentM, tmp);

			//lbl->setText(m->getLabel());
		}
	}
}

//#include "FileConnector.h"

void save_mesh(std::shared_ptr<CMesh>  m, QString label, QString path)
{
	Eigen::Matrix4d M = CBaseObject::getGlobalTransformationMatrix(m);
	std::shared_ptr<CMesh> mesh = std::dynamic_pointer_cast<CMesh>(m->getCopy());

	CTransform t0, t1(M);
	mesh->applyTransformation(t1, t0);

	mesh->setLabel(label);

	mesh->removeAllChilds();

	auto obj = std::make_shared<CModel3D>();

	obj->setLabel(label);

	obj->addChild(obj, mesh);

	obj->save(path);

	//delete szczeka_obj;
}

void ConcretePlugin::go_to_multisaver()
{
	//qInfo() << CFileConnector::getSaveExts();

	QFileInfo fi(AppSettings::mainSettings()->value("recentFile").toString());

	QString init_path = QString("%1\\calosc.obj").arg(fi.absoluteDir().absolutePath());

	QString splint_path = UI::FILECHOOSER::getSaveFileName(QString("Wybierz plik zapisu szyny"), init_path, "OBJ File (*.obj)"); // CFileConnector::getSaveExts());


	qInfo() << splint_path;

	QFileInfo splint_info(splint_path);
	QDir dir = splint_info.absoluteDir();
	qInfo() << splint_info.completeSuffix();
	qInfo() << dir.absolutePath();

	//CParser *parser = CFileConnector::getSaveParser(splint_path);

	//if (parser == nullptr) qInfo() << "brak parsera !!!";

	CObject::Children kids;

	for (const auto& kid : AP::WORKSPACE::instance()->children())
	{
		kids[kid.first] = kid.second;
	}

	std::shared_ptr<CMesh>  tmp = meshWithKeywordInLabel("szyna_mesh", kids);
	if (tmp != nullptr)
	{
		qInfo() << "Znaleziono: " << tmp->getLabel();
		save_mesh(tmp, "calosc", splint_path);
		AppSettings::mainSettings()->setValue("recentFile", splint_path);
	}

	tmp = meshWithKeywordInLabel(QString("wierzch_mesh"), kids);
	if (tmp != nullptr)
	{
		qInfo() << "Znaleziono: " << tmp->getLabel();
		save_mesh(tmp, "wierzch", QString("%1\\%2.%3").arg(dir.absolutePath()).arg("wierzch").arg(splint_info.completeSuffix()));
	}

	tmp = meshWithKeywordInLabel(QString("wierzchBD_mesh"), kids);
	if (tmp != nullptr)
	{
		qInfo() << "Znaleziono: " << tmp->getLabel();
		save_mesh(tmp, "wierzch_bez_okluzji", QString("%1\\%2.%3").arg(dir.absolutePath()).arg("wierzch_bez_okluzji").arg(splint_info.completeSuffix()));
	}

	tmp = meshWithKeywordInLabel(QString("wierzchZD_mesh"), kids);
	if (tmp != nullptr)
	{
		qInfo() << "Znaleziono: " << tmp->getLabel();
		save_mesh(tmp, "wierzch_dziurawy", QString("%1\\%2.%3").arg(dir.absolutePath()).arg("wierzch_dziurawy").arg(splint_info.completeSuffix()));
	}

	tmp = meshWithKeywordInLabel(QString("wnetrze_mesh"), kids);
	if (tmp != nullptr)
	{
		qInfo() << "Znaleziono: " << tmp->getLabel();
		save_mesh(tmp, "wnetrze", QString("%1\\%2.%3").arg(dir.absolutePath()).arg("wnetrze").arg(splint_info.completeSuffix()));
	}

	tmp = meshWithKeywordInLabel(QString("rozepch1"), kids);
	if (tmp != nullptr)
	{
		qInfo() << "Znaleziono: " << tmp->getLabel();
		save_mesh(tmp, tmp->getParent()->getLabel(), QString("%1\\%2.%3").arg(dir.absolutePath()).arg(tmp->getParent()->getLabel()).arg(splint_info.completeSuffix()));
	}

	tmp = meshWithKeywordInLabel(QString("rozepch2"), kids);
	if (tmp != nullptr)
	{
		qInfo() << "Znaleziono: " << tmp->getLabel();
		save_mesh(tmp, tmp->getParent()->getLabel(), QString("%1\\%2.%3").arg(dir.absolutePath()).arg(tmp->getParent()->getLabel()).arg(splint_info.completeSuffix()));
	}

	tmp = meshWithKeywordInLabel(QString("testowa zuchwa"), kids);
	if (tmp != nullptr)
	{
		qInfo() << "Znaleziono: " << tmp->getLabel();
		save_mesh(tmp, "zuchwa", QString("%1\\%2.%3").arg(dir.absolutePath()).arg("zuchwa").arg(splint_info.completeSuffix()));
	}

	tmp = meshWithKeywordInLabel(QString("szczeka_mesh"), kids);
	if (tmp != nullptr)
	{
		qInfo() << "Znaleziono: " << tmp->getLabel();
		save_mesh(tmp, "szczeka", QString("%1\\%2.%3").arg(dir.absolutePath()).arg("szczeka").arg(splint_info.completeSuffix()));
	}

	tmp = meshWithKeywordInLabel(QString("okluzja_mesh"), kids);
	if (tmp != nullptr)
	{
		qInfo() << "Znaleziono: " << tmp->getLabel();
		save_mesh(tmp, "okluzja", QString("%1\\%2.%3").arg(dir.absolutePath()).arg("okluzja").arg(splint_info.completeSuffix()));
	}

	//delete parser;

	qInfo() << "Zapisano.";
	UI::STATUSBAR::setText(QString::fromUtf8("ZAPISANO - sprawdź czy wsystko jest zanim skasujesz"));
}

void ConcretePlugin::etap_dekiel_cien()
{
	moj_widget->etap15->setDisabled(true);
	
	QCursor c = moj_widget->cursor();
	c.setShape(Qt::CursorShape::WaitCursor);
	moj_widget->setCursor(c);

	std::shared_ptr<CMesh> wierzch = std::dynamic_pointer_cast<CMesh>(symulator->wierzch->obj->getChild());
	std::shared_ptr<CMesh> wnetrze = std::dynamic_pointer_cast<CMesh>(symulator->wnetrze->obj->getChild());
	//CVector3d ray = -m_rzutnia_copy->getNormal();
	CVector3d ray = -m_cutPlane->getNormal();

	calosc = dekiel_cien(wierzch, wnetrze, ray);
	


	UI::STATUSBAR::setText(L"Prawdopodobnie gotowe !");

	moj_widget->info_zapis = new WidgetInfo({
		QString::fromUtf8("W tym miejscu mozesz sobie zapisać ważniejsze siatki."),
		QString::fromUtf8("coś tu więcej napiszę jesli to będzie działało..."),
		});
	moj_widget->info_zapis->btStart->setText("ZAPISZ");

	moj_layout->addRow(moj_widget->info_zapis);

	QObject::connect(moj_widget->info_zapis->btStart, &QPushButton::clicked, [&]() {
		go_to_multisaver();
		});


	moj_widget->info_koncowe = new WidgetInfo({
		QString::fromUtf8("Zrobione."),
		QString::fromUtf8("Szyna powinna być widoczna w oknie po lewej pod nazwą \"całość\"."),
		QString::fromUtf8("Niestety aby znów skorzystać z wtyczki musisz uruchomić program ponownie."),
		QString::fromUtf8("It's not a bug, it's the feature..."),
		});
	moj_widget->info_koncowe->btStart->setText("KONIEC");

	moj_layout->addRow(moj_widget->info_koncowe);

	QObject::connect(moj_widget->info_koncowe->btStart, &QPushButton::clicked, [&]() {
		qInfo() << "TEST 1";
		etap_zapisz_wynik(); 
	});

	c.setShape(Qt::CursorShape::ArrowCursor);
	moj_widget->setCursor(c);
}

void ConcretePlugin::etap_zapisz_wynik()
{
	qInfo() << "TEST 2";

	if (szcz_parent != nullptr)
	{
		if (calosc->getParent() != nullptr)
			AP::OBJECT::removeChild(calosc->getParentPtr(), calosc);
		
		calosc->removeAllChilds();

		calosc->setLabel("szyna");

		
		if (szcz->hasKeyword("new_upper")) {
			szcz_parent = std::dynamic_pointer_cast<CModel3D>(szcz->getParentPtr());
		}
		else {
			CTransform nullT;
			CTransform szT(CBaseObject::getGlobalTransformationMatrix(szcz).inverse());

			calosc->applyTransformation(szT, nullT);
			mesh_wierzch->applyTransformation(szT, nullT);
			mesh_wnetrze->applyTransformation(szT, nullT);
		}

		AP::OBJECT::addChild(szcz_parent, calosc);

		/*********************************************************************/

		if (mesh_wierzch->getParent() != nullptr)
			AP::OBJECT::removeChild(mesh_wierzch->getParentPtr(), mesh_wierzch);

		mesh_wierzch->removeAllChilds();

		mesh_wierzch->setLabel("wierzch");


		AP::OBJECT::addChild(szcz_parent, mesh_wierzch);

		/*********************************************************************/

		if (mesh_wnetrze->getParent() != nullptr)
			AP::OBJECT::removeChild(mesh_wnetrze->getParentPtr(), mesh_wnetrze);

		mesh_wnetrze->removeAllChilds();

		mesh_wnetrze->setLabel("wnetrze");


		AP::OBJECT::addChild(szcz_parent, mesh_wnetrze);

	}

	std::shared_ptr<CModel3D> top_model = top_model_arch;

	CWorkspace::Children w = AP::WORKSPACE::instance()->children();

	for (auto &p : w)
	{
		if (p.second != top_model)
		{
			AP::WORKSPACE::removeModel(p.second);
			p.second = nullptr;
		}
	}

	AP::WORKSPACE::addModel(top_model);

	waiting_for = CzekamNa::Nic;
	szcz = nullptr;
	oklu = nullptr;
	zuch = nullptr;
	szcz_parent = nullptr;
	top_model_arch = nullptr;
	mesh_wierzch = nullptr;
	mesh_wnetrze = nullptr;
	calosc = nullptr;

	UI::PLUGINPANEL::clear(m_ID);
	showMainPanel();
}

void ConcretePlugin::showMainPanel()
{
	panel = UI::PLUGINPANEL::instance(m_ID);
	panel->layout()->setContentsMargins(0, 0, 0, 0);

	moj_widget = new MojWidget();

	panel->layout()->addWidget(moj_widget);
	moj_layout = (QFormLayout*)moj_widget->layout();

	//((QGridLayout*)panel->layout())->addWidget(subpanel, 0, 0);

	moj_widget->wybor_siatek = new WidgetWyborSiatek();

	QObject::connect(moj_widget->wybor_siatek->btAtmdl, &QPushButton::clicked, [&]() { wczytaj_spreparowany_ATMDL(); });
	QObject::connect(moj_widget->wybor_siatek->btSzcz, &QPushButton::clicked, [&]() { waiting_for = CzekamNa::Sczeke; });
	QObject::connect(moj_widget->wybor_siatek->btZuch, &QPushButton::clicked, [&]() { waiting_for = CzekamNa::Zuchwe; });
	QObject::connect(moj_widget->wybor_siatek->btOklu, &QPushButton::clicked, [&]() { waiting_for = CzekamNa::Okluzje; });


	QObject::connect(moj_widget->wybor_siatek->btLiczOklu, &QPushButton::clicked, [&]() {
		if (szcz && zuch) {
			bool dane_z_pomiaru = moj_widget->wybor_siatek->zPomiaru->isChecked();

			etap00(moj_widget->wybor_siatek->okluDist->value(), dane_z_pomiaru);

			moj_widget->wybor_siatek->ustawOkluzje(oklu->getLabel());

			if (szcz && oklu) {
				moj_widget->etap11->setEnabled(true);
				moj_widget->przytnij_szczene->setEnabled(true);
			}
		}
		});

	moj_layout->addRow(moj_widget->wybor_siatek);

	moj_widget->przytnij_szczene = new WidgetPrzytnijSzczeke();
	QObject::connect(moj_widget->przytnij_szczene->btStart, &QPushButton::clicked, [&]() {
		if (szcz && oklu) {
			QCursor c = moj_widget->cursor();
			c.setShape(Qt::CursorShape::WaitCursor);
			moj_widget->setCursor(c);

			CTransform tSz(CBaseObject::getGlobalTransformationMatrix(szcz));
			std::shared_ptr<CMesh> sz1 = std::dynamic_pointer_cast<CMesh>(szcz->getCopy());
			std::shared_ptr<CMesh> sz2 = liczOkluzje(sz1, oklu, moj_widget->przytnij_szczene->insideDist->value());
			sz2->setLabel(QString("szczeka po przycieciu"));
			sz2->calcFN();

			//CModel3D* obj2 = new CModel3D();
			//obj2->addChild(sz2);
			//obj2->importChildrenGeometry();
			//obj2->setLabel("SZCZEKA");
			//obj2->setTransform(tSz);
			////obj2->calcVN();
			//AP::WORKSPACE::addModel(obj2);

			AP::OBJECT::addChild(szcz_parent, sz2);

			if (sz2) {
				szcz = sz2;
				moj_widget->wybor_siatek->ustawSzczeke(szcz->getLabel());
			}


			if (szcz && oklu) {
				moj_widget->etap11->setEnabled(true);
			}

			c.setShape(Qt::CursorShape::ArrowCursor);
			moj_widget->setCursor(c);
		}
		});

	moj_layout->addRow(moj_widget->przytnij_szczene);
	moj_widget->przytnij_szczene->setDisabled(true);

	moj_widget->etap11 = new WidgetEtap1();

	QObject::connect(moj_widget->etap11->btStart, &QPushButton::clicked, [&]() { etap01(moj_widget->etap11->meshDivider->value()); });

	moj_layout->addRow(moj_widget->etap11);

	moj_widget->etap11->setDisabled(true);
}

void ConcretePlugin::onLoad()
{
	UI::PLUGINPANEL::create( m_ID, "Projekt szyny" );

	showMainPanel();
}

void ConcretePlugin::onUnload()
{
	//delete symulator;
}


void ConcretePlugin::run(void)
{
}

#include "AnnotationPoint.h"
#include "AnnotationPoints.h"
#include "AnnotationPath.h"
#include "AnnotationPath.h"
#include "AnnotationVPath.h"


void ConcretePlugin::znajdzWierzcholkiBrzegowe(std::shared_ptr<CMesh>  mesh, std::set<unsigned int>& boundaryVertices)
{
	CMesh::Edges bound;
	mesh->findBoundaryEdges(bound);

	boundaryVertices.clear();

	for (auto e : bound)
	{
		boundaryVertices.insert(e.first);
		boundaryVertices.insert(e.second);
	}
}

std::shared_ptr<CMesh> ConcretePlugin::zrzutujNaPlaszczyzne(std::shared_ptr<CMesh> mesh, CPlane &cutPlane, CVector3d ray)
{
	std::set<unsigned int> boundaryVertices;

	znajdzWierzcholkiBrzegowe(mesh, boundaryVertices);

	std::shared_ptr<CMesh> rzut = std::dynamic_pointer_cast<CMesh>(mesh->getCopy());
	for (unsigned int i=0; i<rzut->vertices().size(); i++)
	{
		if (boundaryVertices.find(i) == boundaryVertices.end())
		{
			CVertex& v = rzut->vertices()[i];
			CPoint3d rzut;

			if (cutPlane.rayIntersection(v, ray, 30, rzut))
				v.Set(rzut);
		}
	}
	return rzut;
}

std::shared_ptr<CModel3D> ConcretePlugin::dodajMeshDoProjektu(std::shared_ptr<CMesh>  mesh, QString label)
{
	auto nowyModel = std::make_shared<CModel3D>();
	nowyModel->addChild(nowyModel, mesh);
	nowyModel->setMin(mesh->getMin());
	nowyModel->setMax(mesh->getMax());

	nowyModel->setLabel(label);

	AP::WORKSPACE::addModel(nowyModel);

	return nowyModel;
}


std::shared_ptr<CMesh>  ConcretePlugin::scalMeshe(std::shared_ptr<CMesh>  mesh1, std::shared_ptr<CMesh>  mesh2, bool invert)
{
	std::shared_ptr<CMesh> calosc = std::dynamic_pointer_cast<CMesh>(mesh1->getCopy());

	unsigned int offset = calosc->vertices().size();

	for (auto& v : mesh2->vertices())
	{
		calosc->vertices().push_back(v);
	}


	if (invert)
	{
		for (auto& f1 : mesh2->faces())
		{
			CFace f(offset + f1.C(), offset + f1.B(), offset + f1.A());
			calosc->faces().push_back(f);
		}
	}
	else
	{
		for (auto& f1 : mesh2->faces())
		{
			CFace f(offset + f1.A(), offset + f1.B(), offset + f1.C());
			calosc->faces().push_back(f);
		}
	}

	calosc->removeDuplicateVertices();

	return calosc;
}


int ConcretePlugin::usunWadliweScianki(std::shared_ptr<CMesh>  mesh)
{
	std::set<unsigned int> anormalfaces;

	for (int j = 0; j < mesh->faces().size(); j++)
	{
		CFace& f = mesh->faces()[j];
		if ((f.A() == f.B()) || (f.B() == f.C()) || (f.C() == f.A()))
		{
			//UI::MESSAGEBOX::error(L"scianka z dwoma jednakowymi wierzchołkami", mesh->getLabel());

			anormalfaces.insert(j);
		}
	}

	for (std::set<unsigned int>::reverse_iterator rit = anormalfaces.rbegin(); rit != anormalfaces.rend(); rit++)
	{
		mesh->removeFace(*rit);
	}

	int result = anormalfaces.size();
	anormalfaces.clear();

	return result;
}

void ConcretePlugin::wytnijDekiel2(std::shared_ptr<CMesh>  rzutWierzchu, std::shared_ptr<CMesh>  wnetrze)
{
	double dist = m_divider / 2.0;// 0.05;

	std::set<unsigned int> boundaryVertices1;
	znajdzWierzcholkiBrzegowe(rzutWierzchu, boundaryVertices1);

	std::set<unsigned int> boundaryVertices2;
	znajdzWierzcholkiBrzegowe(wnetrze, boundaryVertices2);

	std::set<unsigned int> verticesToRemove;
	std::set<unsigned int> doNotTouch;

	//CVector3d ray = -m_rzutnia_copy->getNormal();
	CVector3d ray = -m_cutPlane->getNormal();

	for (auto i : boundaryVertices2)
	{
		CVertex& v = wnetrze->vertices()[i];

		int idx = rzutWierzchu->getKDtree(CPointCloud::KDtree::PRESERVE).closest_to_pt(v);

		if (idx >= 0) 
		{
			doNotTouch.insert(idx);
			wnetrze->vertices()[i] = rzutWierzchu->vertices()[idx];
		}
	}

	for (unsigned int i = 0; i < wnetrze->vertices().size(); i++)
	{
		UI::STATUSBAR::printfTimed(500, L"wycinanie dekla, pęla2. i = %d", i);
		CVertex v = wnetrze->vertices()[i];
		CPoint3d rzut = v;

		bool brzeg = boundaryVertices2.find(i) != boundaryVertices2.end();

		if (!brzeg)
		{
			if ( m_cutPlane->rayIntersection(v, ray, 30, rzut) )
				v = rzut;
		}

		int idx = rzutWierzchu->getKDtree(CPointCloud::KDtree::PRESERVE).closest_to_pt(v, dist);

		if (idx >= 0)
		{
			if (doNotTouch.find(idx) == doNotTouch.end())
			{
				verticesToRemove.insert(idx);
			}
		}
	}

	for (long j = rzutWierzchu->faces().size() - 1; j >= 0; j--)
	{
		CFace f = rzutWierzchu->faces()[j];

		if ((verticesToRemove.find(f.A()) != verticesToRemove.end()) ||
			(verticesToRemove.find(f.B()) != verticesToRemove.end()) ||
			(verticesToRemove.find(f.C()) != verticesToRemove.end()))
		{
			rzutWierzchu->removeFace(j);
		}
	}

	rzutWierzchu->removeUnusedVertices();


}



std::shared_ptr<CMesh>  ConcretePlugin::dekiel_cien( std::shared_ptr<CMesh> wierzch, std::shared_ptr<CMesh> wnetrze, CVector3d ray )
{
	usunWadliweScianki(wnetrze);
	usunWadliweScianki(wierzch);

	wierzch->removeDuplicateVertices();
	wnetrze->removeDuplicateVertices();


	std::shared_ptr<CMesh> rzutWierzchu = zrzutujNaPlaszczyzne( wierzch, *m_cutPlane, ray);
	dodajMeshDoProjektu(rzutWierzchu, "dekiel z wierzchu");


//	std::shared_ptr<CMesh>  rzutWnetrza = zrzutujNaPłaszczyznę(wnetrze, m_cutPlane, ray);
//	dodajMeshDoProjektu(rzutWnetrza, L"dekiel z wnetrza");

	//rzutWierzchu->removeDuplicateVertices();
	//rzutWnetrza->removeDuplicateVertices();
	
	usunWadliweScianki(rzutWierzchu);
//	usunWadliweScianki(rzutWnetrza);


	wytnijDekiel2(rzutWierzchu, wnetrze);

	rzutWierzchu->removeDuplicateVertices();
	wnetrze->removeDuplicateVertices();

	usunWadliweScianki(wnetrze);
	usunWadliweScianki(rzutWierzchu);

	std::shared_ptr<CMesh>  wierzchZdeklem = scalMeshe(wierzch, rzutWierzchu, true);
	dodajMeshDoProjektu(wierzchZdeklem, "wierzch z deklem");


	std::shared_ptr<CMesh>  calosc = scalMeshe(wierzchZdeklem, wnetrze);
	dodajMeshDoProjektu(calosc, QString::fromUtf8("całość"));
	calosc->setLabel("szyna_mesh");

	//AP::WORKSPACE::addObject(calosc);

	usunWadliweScianki(calosc);


	////////////////////////////////////////////////////////////////////


	MapOfNewEdges allEdges;

	createE2Fmap(calosc, allEdges);

	std::set<INDEX_TYPE> facesToRemove;

	for (const auto& e : allEdges)
	{
		if (e.second.size() > 2)
		{
			for (auto idxf : e.second)
			{
				CFace f = calosc->faces()[idxf];

				MapOfNewEdges::iterator i1 = allEdges.find(f.A(), f.B());
				MapOfNewEdges::iterator i2 = allEdges.find(f.B(), f.C());
				MapOfNewEdges::iterator i3 = allEdges.find(f.C(), f.A());

				if ((i1->second.size() < 2) || (i2->second.size() < 2) || (i3->second.size() < 2))
				{
					facesToRemove.insert(idxf);
				}
			}
		}
	}

	for (long j = calosc->faces().size() - 1; j >= 0; j--)
	{
		if (facesToRemove.find(j) != facesToRemove.end())
		{
			calosc->removeFace(j);
		}
	}

	calosc->removeUnusedVertices();

	return calosc;
}


void ConcretePlugin::createE2Fmap(std::shared_ptr<CMesh> mesh, MapOfNewEdges& allEdges)
{
	for (unsigned int j = 0; j < mesh->faces().size(); j++)
	{
		CFace f = mesh->faces()[j];

		MapOfNewEdges::iterator it = allEdges.find(f.A(), f.B());
		if (it == allEdges.end())
		{
			allEdges[NewEdge::first_type(f.A(), f.B())].insert(j);
		}
		else
		{
			it->second.insert(j);
		}

		it = allEdges.find(f.B(), f.C());
		if (it == allEdges.end())
		{
			allEdges[NewEdge::first_type(f.B(), f.C())].insert(j);
		}
		else
		{
			it->second.insert(j);
		}

		it = allEdges.find(f.C(), f.A());
		if (it == allEdges.end())
		{
			allEdges[NewEdge::first_type(f.C(), f.A())].insert(j);
		}
		else
		{
			it->second.insert(j);
		}
	}
}


bool ConcretePlugin::onModelIndication(int objId)
{
	if (objId == NO_CURRENT_MODEL)
	{
		waiting_for = CzekamNa::Nic;
		UI::STATUSBAR::setText("MESH SELECTION CANCELED");
		return true;
	}

	if (waiting_for != 0)
	{
		std::shared_ptr<CBaseObject> obj = AP::WORKSPACE::findId(objId);

		if (obj && obj->hasType(CBaseObject::Type::MESH)) {
			if (waiting_for == CzekamNa::Sczeke)
			{
				//UI::MESSAGEBOX::information("I'M YOUR NEW \"SZCZEKA\"");
				szcz = std::dynamic_pointer_cast<CMesh>(obj);

				moj_widget->wybor_siatek->ustawSzczeke(szcz->getLabel());
				
				szcz_parent = std::dynamic_pointer_cast<CModel3D>(szcz->getParentPtr());

				if (szcz && oklu) {
					moj_widget->etap11->setEnabled(true);
				}

				waiting_for = CzekamNa::Nic;
			}
			else if (waiting_for == CzekamNa::Okluzje)
			{
				//UI::MESSAGEBOX::information("I AM YOUR \"OKLUZJA\"");
				oklu = std::dynamic_pointer_cast<CMesh>(obj);

				moj_widget->wybor_siatek->ustawOkluzje(oklu->getLabel());

				if (szcz && oklu) {
					moj_widget->etap11->setEnabled(true);
				}

				waiting_for = CzekamNa::Nic;
			}
			else if (waiting_for == CzekamNa::Zuchwe)
			{
				//UI::MESSAGEBOX::information("I AM YOUR NEW \"ZUCHWA\"");
				zuch = std::dynamic_pointer_cast<CMesh>(obj);
				
				moj_widget->wybor_siatek->ustawZuchwe(zuch->getLabel());

				waiting_for = CzekamNa::Nic;
			}

			if (szcz && zuch) {
				moj_widget->wybor_siatek->enableLiczOklu(true);
			}
		}
		else
		{
			UI::MESSAGEBOX::information("SELECT A MESH, PLEASE");
		}
	}
	else
	{
		UI::STATUSBAR::setText("NOTHING TO DO");
	}
	return true;
}