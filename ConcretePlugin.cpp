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

//#include "interfaces/IProgressListener.h"

#include <memory>

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

	moj_widget->etap123 = new WidgetEtap123();
	moj_layout->addRow(moj_widget->etap123);

	QObject::connect(moj_widget->etap123->btStart23, &QPushButton::clicked, [&]() { etap123(moj_widget->etap123->insideDist->value(), moj_widget->etap123->outsideDist->value()); });

	c.setShape(Qt::CursorShape::ArrowCursor);
	moj_widget->setCursor(c);
}

void ConcretePlugin::etap123(double dValIn, double dValOut)
{
	moj_widget->etap11->setDisabled(true);
	moj_widget->etap123->setDisabled(true);

	QCursor c = moj_widget->cursor();
	c.setShape(Qt::CursorShape::WaitCursor);
	moj_widget->setCursor(c);

	qInfo() << "etap12 -> symulator->generujWnetrzeNEW()";

	symulator->create_inner_surface(symulator->wnetrze, dValIn);
	mesh_wnetrze = std::dynamic_pointer_cast<CMesh>(symulator->wnetrze->obj->getChild());


	qInfo() << "etap13 - wersja 2";

	std::shared_ptr<CMesh> zu = std::dynamic_pointer_cast<CMesh>(zuch->getCopy());

	Eigen::Matrix4d ms = CBaseObject::getGlobalTransformationMatrix(szcz_parent);
	Eigen::Matrix4d mz = CBaseObject::getGlobalTransformationMatrix(zuch);
	
	//UWAGA to moze byc potrzebny wybór ms lub mz w zależności od położenia źródłowej żuchwy
	Eigen::Matrix4d mm = ms;// mz;

	AP::WORKSPACE::addObject(zu);
	((CModel3D*)zu->getParent())->transform() = mm;

	symulator->create_outer_surface(dValOut, zu);
	
	mesh_wierzch = std::dynamic_pointer_cast<CMesh>(symulator->wierzch->obj->getChild());

	UI::DOCK::WORKSPACE::update();

	AP::OBJECT::removeChild(symulator->wnetrze->obj, m_plaszczyzna_rzutowania);

	// m_cutPlane->setSelfVisibility(true);
	// UI::updateAllViews();

	// moj_widget->etap14 = new WidgetInfo({ QString::fromUtf8("Możesz teraz jeszcze skorygować ustawienie płaszczyzny cięcia. Następnie kliknij przycisk \"Dotnj\".") });
	// moj_widget->etap14->btStart->setText("Dotnij");

	// moj_layout->addRow(moj_widget->etap14);

	// QObject::connect(moj_widget->etap14->btStart, &QPushButton::clicked, [&]() { etap14(); });

	etap14();


	UI::STATUSBAR::setText(L"Prawdopodobnie gotowe !");

	moj_widget->info_zapis = new WidgetInfo({
		QString::fromUtf8("W tym miejscu mozesz sobie zapisać ważniejsze siatki."),
		// QString::fromUtf8("coś tu więcej napiszę jesli to będzie działało..."),
		});
	moj_widget->info_zapis->btStart->setText("ZAPISZ");

	moj_layout->addRow(moj_widget->info_zapis);

	QObject::connect(moj_widget->info_zapis->btStart, &QPushButton::clicked, [&]() {
		go_to_multisaver();
		});


	moj_widget->info_koncowe = new WidgetInfo({
		QString::fromUtf8("Zrobione."),
		// QString::fromUtf8("Szyna powinna być widoczna w oknie po lewej pod nazwą \"całość\"."),
		QString::fromUtf8("Niestety aby znów skorzystać z wtyczki musisz uruchomić program ponownie."),
		QString::fromUtf8("It's not a bug, it's the feature..."),
		});
	moj_widget->info_koncowe->btStart->setText("KONIEC");

	moj_layout->addRow(moj_widget->info_koncowe);

	QObject::connect(moj_widget->info_koncowe->btStart, &QPushButton::clicked, [&]() { wytlaczanie(); });


	c.setShape(Qt::CursorShape::ArrowCursor);
	moj_widget->setCursor(c);
}

void ConcretePlugin::wytlaczanie()
{
	CObject::Children kids;

	moj_widget->info_koncowe->setDisabled(true);

	for (const auto& kid : CWorkspace::instance()->children())
	{
		kids[kid.first] = kid.second;
	}

	std::shared_ptr<CMesh> sz, ok, zu, wi, wn;

	auto tmp = meshWithKeywordInLabel(QString("wierzch_mesh"), kids);
	if (tmp) {
		qInfo() << "Znaleziono: " << tmp->getLabel();
		Eigen::Matrix4d M = CBaseObject::getGlobalTransformationMatrix(tmp);
		CTransform t0, t1(M);
		tmp->applyTransformation(t1, t0);
		wi = tmp;
		wi->setLabel("wierzch");
	}

	tmp = meshWithKeywordInLabel(QString("wnetrze_mesh"), kids);
	if (tmp) {
		qInfo() << "Znaleziono: " << tmp->getLabel();
		Eigen::Matrix4d M = CBaseObject::getGlobalTransformationMatrix(tmp);
		CTransform t0, t1(M);
		tmp->applyTransformation(t1, t0);
		wn = tmp;
		wn->setLabel("wnetrze");
	}

	tmp = meshWithKeywordInLabel(QString("testowa zuchwa"), kids);
	if (tmp) {
		qInfo() << "Znaleziono: " << tmp->getLabel();
		Eigen::Matrix4d M = CBaseObject::getGlobalTransformationMatrix(tmp);
		CTransform t0, t1(M);
		tmp->applyTransformation(t1, t0);
		zu = tmp;
		zu->setLabel("zuchwa");
	}

	tmp = meshWithKeywordInLabel(QString("szczeka_mesh"), kids);
	if (tmp)
	{
		qInfo() << "Znaleziono: " << tmp->getLabel();
		Eigen::Matrix4d M = CBaseObject::getGlobalTransformationMatrix(tmp);
		CTransform t0, t1(M);
		tmp->applyTransformation(t1, t0);
		sz = tmp;
		sz->setLabel("szczeka");
	}

	tmp = meshWithKeywordInLabel(QString("okluzja_mesh"), kids);
	if (tmp)
	{
		qInfo() << "Znaleziono: " << tmp->getLabel();
		Eigen::Matrix4d M = CBaseObject::getGlobalTransformationMatrix(tmp);
		CTransform t0, t1(M);
		tmp->applyTransformation(t1, t0);
		ok = tmp;
		ok->setLabel("okluzja");
	}

	AP::WORKSPACE::removeAllModels();

	CWorkspace::instance()->_objectAdd(sz); sz->getParentPtr()->setLabel(sz->getLabel());
	CWorkspace::instance()->_objectAdd(ok); ok->getParentPtr()->setLabel(ok->getLabel());
	CWorkspace::instance()->_objectAdd(zu); zu->getParentPtr()->setLabel(zu->getLabel());
	CWorkspace::instance()->_objectAdd(wi); wi->getParentPtr()->setLabel(wi->getLabel());
	CWorkspace::instance()->_objectAdd(wn); wn->getParentPtr()->setLabel(wn->getLabel());
	
	UI::DOCK::WORKSPACE::update();

	qInfo() << "budowa stempla";

	auto stmp = stempelOnMesh(ok);
	

	qInfo() << "odcisk stempla";

	zrobOdciskStempla(wi, stmp, 0.0);

	qInfo() << "odcisk zuchwy";

	zrobOdciskStempla(wi, zu, 0.2);

	qInfo() << "dziury";
	auto [wi2, wn2] = zrobDziury(wi, wn);

	qInfo() << "bridging";
	if (auto bridged = bridging(wi2, wn2)) {
		qInfo() << "filling";
		if (auto filled = filling(bridged)) {
			filled->setLabel("szyna");
			CWorkspace::instance()->_objectAdd(filled);
			filled->getParentPtr()->setLabel("szyna");

			UI::DOCK::WORKSPACE::update();

			qInfo() << "looks good!!!";
		}
	}
	qInfo() << "that's all folks...";
}



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
	//moj_widget->etap14->setDisabled(true);

	//QCursor c = moj_widget->cursor();
	//c.setShape(Qt::CursorShape::WaitCursor);
	//moj_widget->setCursor(c);

	qInfo() << "TEST4";


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


	// moj_widget->info_exchange = new WidgetInfo({
	// QString::fromUtf8("W tym miejscu powinno sie dać podmienić siatkę reprezentująca wierzch na inną..."),
	// QString::fromUtf8("Zaznacz checkbox przy siatce (Mesh) która chcesz wstawić do projektu i kliknij:"),
	// 	});
	// moj_widget->info_exchange->btStart->setText("EXCHANGE");

	// moj_layout->addRow(moj_widget->info_exchange);

	// QObject::connect(moj_widget->info_exchange->btStart, &QPushButton::clicked, [&]() {
	// 	go_to_exchange();
	// 	});


	// moj_widget->etap15 = new WidgetInfo({
	// 	QString::fromUtf8("Program podejmie próbę połączenia powierzchni wewnętrznej i zewnętrznej szyny, tworząc \"wodoszczelną\" siatkę."), 
	// 	QString::fromUtf8("Ten etap jest jeszcze w fazie testowej i nie zawsze efekt jest zadowalający."),
	// 	QString::fromUtf8("Może być potrzebne użycie zewnętrznego oprogramowania w celu uzyskania poprawnej siatki.")
	// 	});
	// moj_widget->etap15->btStart->setText("Połącz w szynę");

	// moj_layout->addRow(moj_widget->etap15);

	// QObject::connect(moj_widget->etap15->btStart, &QPushButton::clicked, [&]() { etap_dekiel_cien(); });

	// c.setShape(Qt::CursorShape::ArrowCursor);
	// moj_widget->setCursor(c);
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

	QString init_path = QString("%1/calosc.obj").arg(fi.absoluteDir().absolutePath());

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

	// std::shared_ptr<CMesh>  tmp = meshWithKeywordInLabel("szyna_mesh", kids);
	// if (tmp != nullptr)
	// {
	// 	qInfo() << "Znaleziono: " << tmp->getLabel();
	// 	save_mesh(tmp, "calosc", splint_path);
	// 	AppSettings::mainSettings()->setValue("recentFile", splint_path);
	// }

	auto tmp = meshWithKeywordInLabel(QString("wierzch_mesh"), kids);
	if (tmp != nullptr)
	{
		qInfo() << "Znaleziono: " << tmp->getLabel();
		save_mesh(tmp, "wierzch", QString("%1/%2.%3").arg(dir.absolutePath()).arg("wierzch").arg(splint_info.completeSuffix()));
	}

	// tmp = meshWithKeywordInLabel(QString("wierzchBD_mesh"), kids);
	// if (tmp != nullptr)
	// {
	// 	qInfo() << "Znaleziono: " << tmp->getLabel();
	// 	save_mesh(tmp, "wierzch_bez_okluzji", QString("%1/%2.%3").arg(dir.absolutePath()).arg("wierzch_bez_okluzji").arg(splint_info.completeSuffix()));
	// }

	// tmp = meshWithKeywordInLabel(QString("wierzchZD_mesh"), kids);
	// if (tmp != nullptr)
	// {
	// 	qInfo() << "Znaleziono: " << tmp->getLabel();
	// 	save_mesh(tmp, "wierzch_dziurawy", QString("%1/%2.%3").arg(dir.absolutePath()).arg("wierzch_dziurawy").arg(splint_info.completeSuffix()));
	// }

	tmp = meshWithKeywordInLabel(QString("wnetrze_mesh"), kids);
	if (tmp != nullptr)
	{
		qInfo() << "Znaleziono: " << tmp->getLabel();
		save_mesh(tmp, "wnetrze", QString("%1/%2.%3").arg(dir.absolutePath()).arg("wnetrze").arg(splint_info.completeSuffix()));
	}

	tmp = meshWithKeywordInLabel(QString("testowa zuchwa"), kids);
	if (tmp != nullptr)
	{
		qInfo() << "Znaleziono: " << tmp->getLabel();
		save_mesh(tmp, "zuchwa", QString("%1/%2.%3").arg(dir.absolutePath()).arg("zuchwa").arg(splint_info.completeSuffix()));
	}

	tmp = meshWithKeywordInLabel(QString("szczeka_mesh"), kids);
	if (tmp != nullptr)
	{
		qInfo() << "Znaleziono: " << tmp->getLabel();
		save_mesh(tmp, "szczeka", QString("%1/%2.%3").arg(dir.absolutePath()).arg("szczeka").arg(splint_info.completeSuffix()));
	}

	tmp = meshWithKeywordInLabel(QString("okluzja_mesh"), kids);
	if (tmp != nullptr)
	{
		qInfo() << "Znaleziono: " << tmp->getLabel();
		save_mesh(tmp, "okluzja", QString("%1/%2.%3").arg(dir.absolutePath()).arg("okluzja").arg(splint_info.completeSuffix()));
	}

	//delete parser;

	qInfo() << "Zapisano.";
	UI::STATUSBAR::setText(QString::fromUtf8("ZAPISANO - sprawdź czy wsystko jest zanim skasujesz"));
}

// void ConcretePlugin::etap_dekiel_cien()
// {
// 	moj_widget->etap15->setDisabled(true);
	
// 	QCursor c = moj_widget->cursor();
// 	c.setShape(Qt::CursorShape::WaitCursor);
// 	moj_widget->setCursor(c);

// 	std::shared_ptr<CMesh> wierzch = std::dynamic_pointer_cast<CMesh>(symulator->wierzch->obj->getChild());
// 	std::shared_ptr<CMesh> wnetrze = std::dynamic_pointer_cast<CMesh>(symulator->wnetrze->obj->getChild());
// 	//CVector3d ray = -m_rzutnia_copy->getNormal();
// 	CVector3d ray = -m_cutPlane->getNormal();

// 	calosc = dekiel_cien(wierzch, wnetrze, ray);
	


// 	UI::STATUSBAR::setText(L"Prawdopodobnie gotowe !");

// 	moj_widget->info_zapis = new WidgetInfo({
// 		QString::fromUtf8("W tym miejscu mozesz sobie zapisać ważniejsze siatki."),
// 		QString::fromUtf8("coś tu więcej napiszę jesli to będzie działało..."),
// 		});
// 	moj_widget->info_zapis->btStart->setText("ZAPISZ");

// 	moj_layout->addRow(moj_widget->info_zapis);

// 	QObject::connect(moj_widget->info_zapis->btStart, &QPushButton::clicked, [&]() {
// 		go_to_multisaver();
// 		});


// 	moj_widget->info_koncowe = new WidgetInfo({
// 		QString::fromUtf8("Zrobione."),
// 		QString::fromUtf8("Szyna powinna być widoczna w oknie po lewej pod nazwą \"całość\"."),
// 		QString::fromUtf8("Niestety aby znów skorzystać z wtyczki musisz uruchomić program ponownie."),
// 		QString::fromUtf8("It's not a bug, it's the feature..."),
// 		});
// 	moj_widget->info_koncowe->btStart->setText("KONIEC");

// 	moj_layout->addRow(moj_widget->info_koncowe);

// 	QObject::connect(moj_widget->info_koncowe->btStart, &QPushButton::clicked, [&]() {
// 		qInfo() << "TEST 1";
// 		etap_zapisz_wynik(); 
// 	});

// 	c.setShape(Qt::CursorShape::ArrowCursor);
// 	moj_widget->setCursor(c);
// }

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




void computeFullChamferDistanceTransform(const VoxelGrid& grid, std::vector<float>& distanceGrid)
{
	const int dimX = grid.dimX, dimY = grid.dimY, dimZ = grid.dimZ;
	distanceGrid.resize(dimX * dimY * dimZ, std::numeric_limits<float>::max());

	// 1. Inicjalizacja: voxele zaj�te (materia�) = 0.0, reszta = INF
//#pragma omp parallel for
	for (int z = 0; z < dimZ; ++z)
		for (int y = 0; y < dimY; ++y)
			for (int x = 0; x < dimX; ++x)
			{
				if (grid.at(x, y, z) == 1)
					distanceGrid[x + y * dimX + z * dimX * dimY] = 0.0f;
			}

	// 2. Przygotuj 26-kierunkow� mask�
	std::vector<std::tuple<int, int, int, float>> offsets;
	offsets.reserve(26);

	for (int dz = -1; dz <= 1; ++dz)
		for (int dy = -1; dy <= 1; ++dy)
			for (int dx = -1; dx <= 1; ++dx)
				if (!(dx == 0 && dy == 0 && dz == 0))
				{
					float cost = std::sqrt(dx * dx + dy * dy + dz * dz);
					offsets.emplace_back(dx, dy, dz, cost);
				}

	// 3. Forward pass
//#pragma omp parallel for
	for (int z = 0; z < dimZ; ++z)
		for (int y = 0; y < dimY; ++y)
			for (int x = 0; x < dimX; ++x)
			{
				int idx = x + y * dimX + z * dimX * dimY;
				float& currentDist = distanceGrid[idx];

				for (const auto& [dx, dy, dz, cost] : offsets)
				{
					int nx = x + dx, ny = y + dy, nz = z + dz;
					if (nx >= 0 && ny >= 0 && nz >= 0 && nx < dimX && ny < dimY && nz < dimZ)
					{
						int nidx = nx + ny * dimX + nz * dimX * dimY;
						float newDist = distanceGrid[nidx] + cost;
						if (newDist < currentDist)
							currentDist = newDist;
					}
				}
			}

	// 4. Backward pass
//#pragma omp parallel for
	for (int z = dimZ - 1; z >= 0; --z)
		for (int y = dimY - 1; y >= 0; --y)
			for (int x = dimX - 1; x >= 0; --x)
			{
				int idx = x + y * dimX + z * dimX * dimY;
				float& currentDist = distanceGrid[idx];

				for (const auto& [dx, dy, dz, cost] : offsets)
				{
					int nx = x + dx, ny = y + dy, nz = z + dz;
					if (nx >= 0 && ny >= 0 && nz >= 0 && nx < dimX && ny < dimY && nz < dimZ)
					{
						int nidx = nx + ny * dimX + nz * dimX * dimY;
						float newDist = distanceGrid[nidx] + cost;
						if (newDist < currentDist)
							currentDist = newDist;
					}
				}
			}
}




void applyDilation(VoxelGrid& grid, const std::vector<float>& distanceGrid, float dilationRadius)
{
	int dimX = grid.dimX, dimY = grid.dimY, dimZ = grid.dimZ;

#pragma omp parallel for
	for (int z = 0; z < dimZ; ++z) {
		for (int y = 0; y < dimY; ++y) {
			for (int x = 0; x < dimX; ++x) {
				int idx = x + y * dimX + z * dimX * dimY;
				if (distanceGrid[idx] <= dilationRadius / grid.voxelSize) {
					grid.at(x, y, z) = 1;  // zaznacz jako cz�� rozszerzonej siatki
				}
			}
		}
	}
}

void applyErosion(VoxelGrid& grid, const std::vector<float>& distanceGrid, float erosionRadius)
{
	int dimX = grid.dimX, dimY = grid.dimY, dimZ = grid.dimZ;

#pragma omp parallel for
	for (int z = 0; z < dimZ; ++z) {
		for (int y = 0; y < dimY; ++y) {
			for (int x = 0; x < dimX; ++x) {
				int idx = x + y * dimX + z * dimX * dimY;

				// je�li dany woksel NALE�Y do obiektu,
				// ale jest zbyt blisko kraw�dzi � usu� go
				if (grid.at(x, y, z) == 1 &&
					distanceGrid[idx] < erosionRadius / grid.voxelSize)
				{
					grid.at(x, y, z) = 0;
				}
			}
		}
	}
}



void dylatacja(const VoxelGrid& input, VoxelGrid& output, double val)
{
	std::vector<float> distanceGrid(input.dimX * input.dimY * input.dimZ, std::numeric_limits<float>::max());
	computeFullChamferDistanceTransform(input, distanceGrid);
	output = input;
	applyDilation(output, distanceGrid, val);
}




void erozja(const VoxelGrid& input, VoxelGrid& output, double val)
{
	VoxelGrid inverted;
	inverted.allocateLike(input);
	inverted.negate(input);

	std::vector<float> distanceGrid(input.dimX * input.dimY * input.dimZ, std::numeric_limits<float>::max());
	computeFullChamferDistanceTransform(inverted, distanceGrid);
	output = input;
	applyErosion(output, distanceGrid, val);
}


void applyMedianFilter3D(const VoxelGrid& input, VoxelGrid& output)
{
	output = input;  // kopiujemy metadane i dane

#pragma omp parallel for
	for (int z = 1; z < input.dimZ - 1; ++z) {
		for (int y = 1; y < input.dimY - 1; ++y) {
			for (int x = 1; x < input.dimX - 1; ++x) {
				std::vector<uint8_t> neighborhood;

				for (int dz = -1; dz <= 1; ++dz)
					for (int dy = -1; dy <= 1; ++dy)
						for (int dx = -1; dx <= 1; ++dx)
							neighborhood.push_back(input.at(x + dx, y + dy, z + dz));

				std::nth_element(neighborhood.begin(), neighborhood.begin() + 13, neighborhood.end());
				uint8_t median = neighborhood[13];  // mediana w 3x3x3 = 14-ty element

				output.at(x, y, z) = median;
			}
		}
	}
}



void createStempel(const VoxelGrid& okluzja, VoxelGrid& stempel)
{
	//auto prgs = IProgressListener::getDefault();

	stempel = okluzja;

	// tworz� bry�� - wype�niam wn�trze wokselami
	stempel.fillUp();

	UI::PROGRESSBAR::init(0, 100, 0);
	UI::PROGRESSBAR::setText("Budowa stempla");

	// kopia klocka do p�niejszego wyciskania
	auto kopia = stempel;

	// teraz zape�niam ca�y �uk wzd�u� Z wokselami - gubi� kszta�t z�b�w
	stempel.fillDown();

	UI::PROGRESSBAR::setValue(10);

	// pogrubiona kopia w celu zaklejenia przestrzeni mi�dzyz�bowych
	VoxelGrid dilatedKopia;
	dylatacja(kopia, dilatedKopia, 3.0);

	UI::PROGRESSBAR::setValue(20);

	VoxelGrid imprint;
	imprint.allocateLike(stempel);

	// tu robi� odcisk tej pogrubionej kopii
	imprint.diff_of(stempel, dilatedKopia);


	UI::PROGRESSBAR::setValue(30);

	// je�li zosta�y zadziory, to pr�buj� je usun��
	VoxelGrid tmp_imprint;
	erozja(imprint, tmp_imprint, 2.0);

	UI::PROGRESSBAR::setValue(40);

	// i ponownie rozepcha� odcisk by da�o si� w nim odbi� oryginalne z�by
	dylatacja(tmp_imprint, imprint, 6.0);

	UI::PROGRESSBAR::setValue(50);

	// odciskam oryginalne z�by
	imprint.diff_in_place(kopia);

	UI::PROGRESSBAR::setValue(60);

	VoxelGrid filtered;
	applyMedianFilter3D(imprint, filtered);

	UI::PROGRESSBAR::setValue(70);

	erozja(filtered, tmp_imprint, 0.3);
	dylatacja(tmp_imprint, imprint, 2.0);

	// odciskam oryginalne z�by
	imprint.diff_in_place(kopia);

	UI::PROGRESSBAR::setValue(80);

	// vox_stempel wci�� zawiera odbicie �uku z�bowego, gdzie dla ka�dego Z woksel==1
	// troch� go odchudzam po x i y
	VoxelGrid tmp_stempel;
	erozja(stempel, tmp_stempel, 1.0);

	UI::PROGRESSBAR::setValue(90);

	// i bior� cz�� wsp�ln� z szerokim wyciskiem, by usun�� niepotrzebn� grubo��
	stempel.intersection_of(tmp_stempel, imprint);

	// teraz vox_stempel zawiera odcisk z�b�w

	UI::PROGRESSBAR::hide();
}



struct Vec3 {
	float x, y, z;

	Vec3 operator-(const Vec3& other) const {
		return Vec3{ x - other.x, y - other.y, z - other.z };
	}

	float dot(const Vec3& other) const {
		return x * other.x + y * other.y + z * other.z;
	}

	Vec3 cross(const Vec3& other) const {
		return Vec3{
			y * other.z - z * other.y,
			z * other.x - x * other.z,
			x * other.y - y * other.x
		};
	}

	float norm() const {
		return std::sqrt(dot(*this));
	}
};


float distancePointToTriangle(
	float px, float py, float pz,
	const CVertex& v0, const CVertex& v1, const CVertex& v2)
{
	Vec3 p{ px, py, pz };
	Vec3 a{ v0.x, v0.y, v0.z };
	Vec3 b{ v1.x, v1.y, v1.z };
	Vec3 c{ v2.x, v2.y, v2.z };

	// edge vectors
	Vec3 ab = b - a;
	Vec3 ac = c - a;
	Vec3 ap = p - a;

	float d1 = ab.dot(ap);
	float d2 = ac.dot(ap);

	if (d1 <= 0 && d2 <= 0) return (p - a).norm(); // barycentric region outside A

	Vec3 bp = p - b;
	float d3 = ab.dot(bp);
	float d4 = ac.dot(bp);
	if (d3 >= 0 && d4 <= d3) return (p - b).norm(); // outside B

	float vc = d1 * d4 - d3 * d2;
	if (vc <= 0 && d1 >= 0 && d3 <= 0) {
		float v = d1 / (d1 - d3);
		Vec3 proj = Vec3{ a.x + v * ab.x, a.y + v * ab.y, a.z + v * ab.z };
		return (p - proj).norm(); // on edge AB
	}

	Vec3 cp = p - c;
	float d5 = ab.dot(cp);
	float d6 = ac.dot(cp);
	if (d6 >= 0 && d5 <= d6) return (p - c).norm(); // outside C

	float vb = d5 * d2 - d1 * d6;
	if (vb <= 0 && d2 >= 0 && d6 <= 0) {
		float w = d2 / (d2 - d6);
		Vec3 proj = Vec3{ a.x + w * ac.x, a.y + w * ac.y, a.z + w * ac.z };
		return (p - proj).norm(); // on edge AC
	}

	float va = d3 * d6 - d5 * d4;
	if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) {
		float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
		Vec3 edge = Vec3{ c.x - b.x, c.y - b.y, c.z - b.z };
		Vec3 proj = Vec3{ b.x + w * edge.x, b.y + w * edge.y, b.z + w * edge.z };
		return (p - proj).norm(); // on edge BC
	}

	// inside face region
	Vec3 n = ab.cross(ac);
	n = Vec3{ n.x / n.norm(), n.y / n.norm(), n.z / n.norm() }; // unit normal
	float dist = std::abs((p - a).dot(n));
	return dist;
}



VoxelGrid rasterizeMeshToVoxels(
	const std::vector<CVertex>& vertices,
	const std::vector<CFace>& faces,
	float voxelSize)
{
	// 1. znajd� bounding box
	float minX = std::numeric_limits<float>::max(), minY = minX, minZ = minX;
	float maxX = -minX, maxY = -minX, maxZ = -minX;

	for (const auto& v : vertices) {
		minX = std::min(minX, v.x); maxX = std::max(maxX, v.x);
		minY = std::min(minY, v.y); maxY = std::max(maxY, v.y);
		minZ = std::min(minZ, v.z); maxZ = std::max(maxZ, v.z);
	}

	// Mo�na doda� zapas
	float padding = 2.0; // voxelSize * 2;
	minX -= padding; minY -= padding; minZ -= 10.0;
	maxX += padding; maxY += padding; maxZ += padding;

	int dimX = std::ceil((maxX - minX) / voxelSize);
	int dimY = std::ceil((maxY - minY) / voxelSize);
	int dimZ = std::ceil((maxZ - minZ) / voxelSize);

	VoxelGrid grid;
	grid.voxelSize = voxelSize;
	grid.dimX = dimX;
	grid.dimY = dimY;
	grid.dimZ = dimZ;
	grid.minX = minX;
	grid.minY = minY;
	grid.minZ = minZ;



	grid.data.resize(dimX * dimY * dimZ, 0);

	// 2. rasteryzacja tr�jk�t�w
	for (const auto& face : faces) {
		const CVertex& v0 = vertices[face.A()];
		const CVertex& v1 = vertices[face.B()];
		const CVertex& v2 = vertices[face.C()];

		// oszacuj bounding box tr�jk�ta w voxelach
		float triMinX = std::min({ v0.x, v1.x, v2.x });
		float triMinY = std::min({ v0.y, v1.y, v2.y });
		float triMinZ = std::min({ v0.z, v1.z, v2.z });
		float triMaxX = std::max({ v0.x, v1.x, v2.x });
		float triMaxY = std::max({ v0.y, v1.y, v2.y });
		float triMaxZ = std::max({ v0.z, v1.z, v2.z });

		int startX = std::max(0, (int)((triMinX - minX) / voxelSize));
		int endX = std::min(dimX - 1, (int)((triMaxX - minX) / voxelSize));
		int startY = std::max(0, (int)((triMinY - minY) / voxelSize));
		int endY = std::min(dimY - 1, (int)((triMaxY - minY) / voxelSize));
		int startZ = std::max(0, (int)((triMinZ - minZ) / voxelSize));
		int endZ = std::min(dimZ - 1, (int)((triMaxZ - minZ) / voxelSize));

		// dla ka�dego voxela w bounding boxie sprawd�, czy tr�jk�t go przecina
		for (int z = startZ; z <= endZ; ++z) {
			for (int y = startY; y <= endY; ++y) {
				for (int x = startX; x <= endX; ++x) {
					// oblicz �rodek voxela
					float cx = minX + (x + 0.5f) * voxelSize;
					float cy = minY + (y + 0.5f) * voxelSize;
					float cz = minZ + (z + 0.5f) * voxelSize;

					// sprawd� odleg�o�� od �rodka voxela do tr�jk�ta
					float dist = distancePointToTriangle(cx, cy, cz, v0, v1, v2);

					if (dist <= voxelSize * 0.866f) {  // przek�tna sze�cianu / 2
						grid.at(x, y, z) = 1;
					}
				}
			}
		}
	}

	return grid;
}

#include "Volumetric.h"


std::shared_ptr<Volumetric> createVolumetric(VoxelGrid& voxel_grid)
{
	auto vol = Volumetric::create(voxel_grid.dimZ, voxel_grid.dimY, voxel_grid.dimX);

	Volumetric::VoxelType maks = 0;

	for (int l = 0; l < voxel_grid.dimZ; l++)
		for (int r = 0; r < voxel_grid.dimY; r++)
			for (int c = 0; c < voxel_grid.dimX; c++) {
				Volumetric::VoxelType val = 1000 * voxel_grid.at(c, r, l);
				vol->at(l, r, c) = val;
				if (maks < val) maks = val;
			}

	for (int i = 0; i < vol->metadata.size(); i++)
	{
		auto& md = vol->metadata[i];

		md.image_position_patient[0] = voxel_grid.minX; // += delta[0];
		md.image_position_patient[1] = voxel_grid.minY; // += delta[1];
		md.image_position_patient[2] = voxel_grid.minZ + voxel_grid.voxelSize * i;

		md.slice_location = md.image_position_patient[2];

		md.pixel_spacing[0] = voxel_grid.voxelSize;
		md.pixel_spacing[1] = voxel_grid.voxelSize;
		md.slice_distance = voxel_grid.voxelSize;
		md.slice_thickness = voxel_grid.voxelSize;
	}

	vol->adjustMinMax();
	vol->adjustMinMaxColor(maks + 1);

	return vol;
}



std::shared_ptr<CMesh> ConcretePlugin::stempelOnMesh(std::shared_ptr<CMesh> mesh) {
	if (mesh)
	{
		// rasteryzacja -> wokselowy odpowiednik siatki
		vox_okluzja = rasterizeMeshToVoxels(mesh->vertices(), mesh->faces(), 0.1);

		createStempel(vox_okluzja, vox_stempel);

		auto vol = createVolumetric(vox_stempel);

		//AP::WORKSPACE::addObject(vol);

		std::shared_ptr<CMesh> stempel = nullptr;
		if (stempel = vol->marching_cube(1)) {
			stempel->setLabel("stempel");
			if (AP::WORKSPACE::addObject(stempel)) {
				stempel->getParentPtr()->setLabel("stempel");
			}
		}

		UI::DOCK::WORKSPACE::update();
		UI::updateAllViews();

		return stempel;
	}
	return nullptr;
}


void ConcretePlugin::onStempelButton() {
	auto sel = AP::WORKSPACE::SELECTION::getObjList({ CObject::Type::MESH });

	if (sel.size() > 0)
	{
		auto isel = sel.begin();

		auto mesh = std::dynamic_pointer_cast<CMesh>(*isel);

		stempelOnMesh(mesh);
	}
}

//--------------------------------------------------------------------


#include "KDNode2.h"


void ConcretePlugin::zrobOdciskStempla(std::shared_ptr<CMesh> wierzch, std::shared_ptr<CMesh> stempel, double _distMax)
{
	//	unsigned long t1 = GetTickCount();

	KDNode2* tree = KDNode2::build(stempel.get(), 10240);
	KDNode2::HitMap hmap;

	//rzutnia->calcVN();

	//CPoint3d mid = rzutnia->getCenterOfWeight();

	int last_i = 0;
	for (int i = 0; i < wierzch->vertices().size(); i++) {
		//		UI::STATUSBAR::printfTimed(1000, L"Tworz� wycisk stempla. Wertex %d z %d.", i, wierzch->vertices().size());

		CVertex p0 = wierzch->vertices()[i];

		KDNode2* ptr = tree;

		bool found = false;

		CVector3d mv(0.0, 0.0, -1.0);

		double shift = 2.0;
		CPoint3d p0mv = p0 + CVector3d(0.0, 0.0, shift);

		hmap.clear();
		bool hit = tree->hit(stempel.get(), p0mv, mv, hmap);

		if (hit) {
			//qInfo() << "hmap size = " << hmap.size();

			double dist = (*hmap.begin()).second.first;
			CPoint3d p1 = (*hmap.begin()).second.second;
			int idx = (*hmap.begin()).first;

			for (auto h : hmap) {
				double dd = h.second.first;

				if (dd < dist) {
					dist = dd;
					p1 = h.second.second;
					idx = h.first;
				}
			}

			if (dist <= shift + _distMax)
			{
				if (last_i + 1000 < i) {
					qInfo() << "i = " << i << " dist = " << dist;
					last_i = i;
				}

				double dz = p1.z - wierzch->vertices()[i].z;

				if (dz > 0)
					wierzch->vertices()[i].z = p1.z + 0.1;
				//else
				//	wierzch->vertices()[i].z += 0.1;
			}
		}

	}


	//	UI::STATUSBAR::printf(L"Wycisk stempla gotowy. Czas wykonania: %d ms", GetTickCount() - t1);

	//	UI::updateAllViews();
}



void ConcretePlugin::onOdciskStemplaButton(double distMax)
{
	qInfo() << "odcisk stempla";
	auto sel = AP::WORKSPACE::SELECTION::getObjList({ CObject::Type::MESH });

	if (sel.size() > 0)
	{
		auto isel = sel.begin();

		auto wierzch = std::dynamic_pointer_cast<CMesh>((*isel)->getCopy());
		wierzch->setParent(nullptr);
		isel++;
		auto stempel = std::dynamic_pointer_cast<CMesh>((*isel)->getCopy());
		stempel->setParent(nullptr);

		zrobOdciskStempla(wierzch, stempel, distMax);

		AP::WORKSPACE::addObject(wierzch);
	}
}



//-----------------------------------------------------------------------------------





#include "CollisionDetector.h"
#include "AnnotationSetOfFaces.h"
#include "triangulacja.h"

void przeciecia(std::shared_ptr<CMesh> mesh1, std::shared_ptr<CMesh> mesh2) {

	std::map<INDEX_TYPE, std::set<INDEX_TYPE>*> crossed;

	if (CollisionDetector::getIntersectionOfMeshWithMesh3d(mesh1, mesh2, crossed))
	{
		//std::set<INDEX_TYPE> s1, s2;

		//for (auto pair : crossed)
		//{
		//	s1.insert(pair.first);
		//	s2.insert(pair.second->begin(), pair.second->end());
		//}

		auto ed = std::make_shared<CAnnotationEdges>();

		SplitTriangles2(mesh1, mesh2, crossed, *ed);

		AP::OBJECT::addChild(mesh1, ed);

		UI::STATUSBAR::setText("Ready. You can see sugested faces.");
	}
	else
	{
		UI::STATUSBAR::setText("No intersections found");
	}



}



#include <omp.h>
#include <mutex>

void zamienPrzecieciaNaDziury2(CMesh* wierzch, CMesh* stempel, double shift, CVector3d mv, std::set<INDEX_TYPE>& vertices_to_remove)
{
	std::mutex mtx;

	//	unsigned long t1 = GetTickCount();

	KDNode2* tree = KDNode2::build(stempel, 5000);

	int last_i = 0;


#pragma omp parallel
	{
		std::set<INDEX_TYPE> local_set;

#pragma omp for
		for (int i = 0; i < wierzch->vertices().size(); i++) {

			CVertex p0 = wierzch->vertices()[i];

			// punkt pocz�tkowy dla wyszukiwania
			CPoint3d p0mv = p0 + CVector3d(0.0, 0.0, shift);

			KDNode2::HitMap hmap;
			bool hit = tree->hit(stempel, p0mv, mv, hmap);

			if (hit) {
				double dist = (*hmap.begin()).second.first;
				CPoint3d p1 = (*hmap.begin()).second.second;
				int idx = (*hmap.begin()).first;

				if (hmap.size() > 1) {
					for (auto h : hmap) {
						double dd = h.second.first;

						if (dd < dist) {
							dist = dd;
							p1 = h.second.second;
							idx = h.first;
						}
					}
				}

				//if (shift >= 0)
				//{
				if (dist > abs(shift))
				{
					// punkt nale�y do przeciecia
					local_set.insert(i);

					if (last_i + 1000 < i) {
						qInfo() << "i = " << i << " dist = " << dist;
						last_i = i;
					}
				}
				//}
				//else
				//{
				//	if (dist > abs(shift))
				//	{
				//		// punkt nale�y do przeciecia
				//		local_set.insert(i);

				//		if (last_i + 1000 < i) {
				//			qInfo() << "i = " << i << " dist = " << dist;
				//			last_i = i;
				//		}
				//	}
				//}
			}

		}

		// Po zako�czeniu pracy w�tku, scal wyniki
		std::lock_guard<std::mutex> lock(mtx);
		vertices_to_remove.insert(local_set.begin(), local_set.end());

	}
}



void ConcretePlugin::onZrobDziuryButton()
{
	qInfo() << "robimy dziury, Hej!!!";
	auto sel = AP::WORKSPACE::SELECTION::getObjList({ CObject::Type::MESH });

	if (sel.size() > 0)
	{
		auto isel = sel.begin();

		auto wierzch = std::dynamic_pointer_cast<CMesh>(*isel);
		auto wierzch_nowy = std::dynamic_pointer_cast<CMesh>(wierzch->getCopy());
		wierzch_nowy->setParent(nullptr);
		wierzch_nowy->removeDuplicateVertices();

		isel++;

		auto wnetrze = std::dynamic_pointer_cast<CMesh>(*isel);
		auto wnetrze_nowe = std::dynamic_pointer_cast<CMesh>(wnetrze->getCopy());
		wnetrze_nowe->setParent(nullptr);
		wnetrze_nowe->removeDuplicateVertices();


		przeciecia(wierzch_nowy, wnetrze_nowe);



		std::set<INDEX_TYPE> vertices_to_remove;
		std::vector<INDEX_TYPE> faces_to_remove;



		qInfo() << "--- dziury w wierzchu...";
		zamienPrzecieciaNaDziury2(wierzch_nowy.get(), wnetrze.get(), 10.0, CVector3d(0.0, 0.0, -1.0), vertices_to_remove);

		qInfo() << QString("--- wierzcho�k�w do usuni�cia jest: %1").arg(vertices_to_remove.size());

		qInfo() << "--- usuwanie nadmiarowych scianek...";

		for (INDEX_TYPE j = 0; j < wierzch_nowy->faces().size(); j++) {
			CFace& f = wierzch_nowy->faces()[j];

			if ((vertices_to_remove.find(f.A()) != vertices_to_remove.end()) ||
				(vertices_to_remove.find(f.B()) != vertices_to_remove.end()) ||
				(vertices_to_remove.find(f.C()) != vertices_to_remove.end()))
			{
				faces_to_remove.push_back(j);
			}
		}

		qInfo() << QString("--- znaleziono: %1").arg(faces_to_remove.size());

		qInfo() << QString("--- lb. scianek przed usuwaniem: %1").arg(wierzch_nowy->faces().size());
		qInfo() << QString("--- lb. wierzcholkow przed usuwaniem: %1").arg(wierzch_nowy->vertices().size());

		// na wszelki wypadek, �eby miec pewno��, �e indeksy s� malej�co
		std::sort(faces_to_remove.begin(), faces_to_remove.end(), std::greater<INDEX_TYPE>());

		for (INDEX_TYPE j : faces_to_remove) {
			wierzch_nowy->removeFace(j);
		}

		qInfo() << QString("--- lb. scianek po usuwaniu: %1").arg(wierzch_nowy->faces().size());


		wierzch_nowy->removeUnusedVertices();

		qInfo() << QString("--- lb. wierzcholkow po usuwaniu: %1").arg(wierzch_nowy->vertices().size());

		wierzch_nowy->setLabel("wierzch_nowy");
		AP::WORKSPACE::addObject(wierzch_nowy);






		vertices_to_remove.clear();
		faces_to_remove.clear();



		qInfo() << "--- dziury we wnetrzu...";
		zamienPrzecieciaNaDziury2(wnetrze_nowe.get(), wierzch.get(), -10.0, CVector3d(0.0, 0.0, 1.0), vertices_to_remove);

		qInfo() << QString("--- wierzcho�k�w do usuni�cia jest: %1").arg(vertices_to_remove.size());

		qInfo() << "--- usuwanie nadmiarowych scianek...";

		for (INDEX_TYPE j = 0; j < wnetrze_nowe->faces().size(); j++) {
			CFace& f = wnetrze_nowe->faces()[j];

			if ((vertices_to_remove.find(f.A()) != vertices_to_remove.end()) ||
				(vertices_to_remove.find(f.B()) != vertices_to_remove.end()) ||
				(vertices_to_remove.find(f.C()) != vertices_to_remove.end()))
			{
				faces_to_remove.push_back(j);
			}
		}

		qInfo() << QString("--- znaleziono: %1").arg(faces_to_remove.size());

		qInfo() << QString("--- lb. scianek przed usuwaniem: %1").arg(wnetrze_nowe->faces().size());
		qInfo() << QString("--- lb. wierzcholkow przed usuwaniem: %1").arg(wnetrze_nowe->vertices().size());

		// na wszelki wypadek, �eby miec pewno��, �e indeksy s� malej�co
		std::sort(faces_to_remove.begin(), faces_to_remove.end(), std::greater<INDEX_TYPE>());

		for (INDEX_TYPE j : faces_to_remove) {
			wnetrze_nowe->removeFace(j);
		}

		qInfo() << QString("--- lb. scianek po usuwaniu: %1").arg(wnetrze_nowe->faces().size());

		faces_to_remove.clear();

		wnetrze_nowe->removeUnusedVertices();

		qInfo() << QString("--- lb. wierzcholkow po usuwaniu: %1").arg(wnetrze_nowe->vertices().size());

		wnetrze_nowe->setLabel("wnetrze_nowe");
		AP::WORKSPACE::addObject(wnetrze_nowe);
	}
}


std::pair< std::shared_ptr<CMesh>, std::shared_ptr<CMesh>> ConcretePlugin::zrobDziury(std::shared_ptr<CMesh> wierzch, std::shared_ptr<CMesh> wnetrze)
{
	qInfo() << "robimy dziury, Hej!!!";

	if (wierzch && wnetrze)
	{
		auto wierzch_nowy = std::dynamic_pointer_cast<CMesh>(wierzch->getCopy());
		wierzch_nowy->setParent(nullptr);
		wierzch_nowy->removeDuplicateVertices();

		auto wnetrze_nowe = std::dynamic_pointer_cast<CMesh>(wnetrze->getCopy());
		wnetrze_nowe->setParent(nullptr);
		wnetrze_nowe->removeDuplicateVertices();

		przeciecia(wierzch_nowy, wnetrze_nowe);

		std::set<INDEX_TYPE> vertices_to_remove;
		std::vector<INDEX_TYPE> faces_to_remove;

		qInfo() << "--- dziury w wierzchu...";
		zamienPrzecieciaNaDziury2(wierzch_nowy.get(), wnetrze.get(), 10.0, CVector3d(0.0, 0.0, -1.0), vertices_to_remove);

		qInfo() << QString("--- wierzcho�k�w do usuni�cia jest: %1").arg(vertices_to_remove.size());

		qInfo() << "--- usuwanie nadmiarowych scianek...";

		for (INDEX_TYPE j = 0; j < wierzch_nowy->faces().size(); j++) {
			CFace& f = wierzch_nowy->faces()[j];

			if ((vertices_to_remove.find(f.A()) != vertices_to_remove.end()) ||
				(vertices_to_remove.find(f.B()) != vertices_to_remove.end()) ||
				(vertices_to_remove.find(f.C()) != vertices_to_remove.end()))
			{
				faces_to_remove.push_back(j);
			}
		}

		qInfo() << QString("--- znaleziono: %1").arg(faces_to_remove.size());

		qInfo() << QString("--- lb. scianek przed usuwaniem: %1").arg(wierzch_nowy->faces().size());
		qInfo() << QString("--- lb. wierzcholkow przed usuwaniem: %1").arg(wierzch_nowy->vertices().size());

		// na wszelki wypadek, �eby miec pewno��, �e indeksy s� malej�co
		std::sort(faces_to_remove.begin(), faces_to_remove.end(), std::greater<INDEX_TYPE>());

		for (INDEX_TYPE j : faces_to_remove) {
			wierzch_nowy->removeFace(j);
		}

		qInfo() << QString("--- lb. scianek po usuwaniu: %1").arg(wierzch_nowy->faces().size());


		wierzch_nowy->removeUnusedVertices();

		qInfo() << QString("--- lb. wierzcholkow po usuwaniu: %1").arg(wierzch_nowy->vertices().size());

		wierzch_nowy->setLabel("wierzch_nowy");
		//AP::WORKSPACE::addObject(wierzch_nowy);


		vertices_to_remove.clear();
		faces_to_remove.clear();

		qInfo() << "--- dziury we wnetrzu...";
		zamienPrzecieciaNaDziury2(wnetrze_nowe.get(), wierzch.get(), -10.0, CVector3d(0.0, 0.0, 1.0), vertices_to_remove);

		qInfo() << QString("--- wierzcho�k�w do usuni�cia jest: %1").arg(vertices_to_remove.size());

		qInfo() << "--- usuwanie nadmiarowych scianek...";

		for (INDEX_TYPE j = 0; j < wnetrze_nowe->faces().size(); j++) {
			CFace& f = wnetrze_nowe->faces()[j];

			if ((vertices_to_remove.find(f.A()) != vertices_to_remove.end()) ||
				(vertices_to_remove.find(f.B()) != vertices_to_remove.end()) ||
				(vertices_to_remove.find(f.C()) != vertices_to_remove.end()))
			{
				faces_to_remove.push_back(j);
			}
		}

		qInfo() << QString("--- znaleziono: %1").arg(faces_to_remove.size());

		qInfo() << QString("--- lb. scianek przed usuwaniem: %1").arg(wnetrze_nowe->faces().size());
		qInfo() << QString("--- lb. wierzcholkow przed usuwaniem: %1").arg(wnetrze_nowe->vertices().size());

		// na wszelki wypadek, �eby miec pewno��, �e indeksy s� malej�co
		std::sort(faces_to_remove.begin(), faces_to_remove.end(), std::greater<INDEX_TYPE>());

		for (INDEX_TYPE j : faces_to_remove) {
			wnetrze_nowe->removeFace(j);
		}

		qInfo() << QString("--- lb. scianek po usuwaniu: %1").arg(wnetrze_nowe->faces().size());

		faces_to_remove.clear();

		wnetrze_nowe->removeUnusedVertices();

		qInfo() << QString("--- lb. wierzcholkow po usuwaniu: %1").arg(wnetrze_nowe->vertices().size());

		wnetrze_nowe->setLabel("wnetrze_nowe");
		//AP::WORKSPACE::addObject(wnetrze_nowe);

		return { wierzch_nowy, wnetrze_nowe };
	}
	return { nullptr, nullptr };
}

//------------------------------------------------------------------------------------------------------------------


// Funkcja znajduje skierowane krawędzie brzegowe (czyli takie, które występują tylko raz w danym kierunku)
void findBoundaryEdgesWithDirection(const std::vector<CFace>& faces,
	std::vector<CEdge>& boundaryEdges,
	std::unordered_map<INDEX_TYPE, std::vector<INDEX_TYPE>>& boundaryGraph)
{
	using Edge = std::pair<INDEX_TYPE, INDEX_TYPE>;
	std::unordered_map<Edge, INDEX_TYPE, PairHash> directedEdgeCount;

	for (const CFace& f : faces) {
		directedEdgeCount[{f.A(), f.B()}]++;
		directedEdgeCount[{f.B(), f.C()}]++;
		directedEdgeCount[{f.C(), f.A()}]++;
	}

	boundaryEdges.clear();
	boundaryGraph.clear();

	for (const auto& [e, count] : directedEdgeCount) {
		Edge reversed = { e.second, e.first };
		if (count == 1 && directedEdgeCount.find(reversed) == directedEdgeCount.end()) {
			boundaryEdges.emplace_back(e.first, e.second);
			boundaryGraph[e.first].push_back(e.second);
		}
	}
}


#include <unordered_set>


std::vector<std::vector<INDEX_TYPE>> findBoundaryLoops(
	const std::unordered_map<INDEX_TYPE, std::vector<INDEX_TYPE>>& boundaryGraph)
{
	std::unordered_set<std::pair<INDEX_TYPE, INDEX_TYPE>, PairHash> visitedEdges;
	std::vector<std::vector<INDEX_TYPE>> loops;

	for (const auto& [start, neighbors] : boundaryGraph) {
		for (INDEX_TYPE next : neighbors) {
			std::pair<INDEX_TYPE, INDEX_TYPE> edge = { start, next };

			if (visitedEdges.find(edge) != visitedEdges.end())
				continue;

			std::vector<INDEX_TYPE> loop;
			INDEX_TYPE current = start;
			INDEX_TYPE prev = -1;

			loop.push_back(current);

			while (true) {
				const auto it = boundaryGraph.find(current);
				if (it == boundaryGraph.end())
					break;

				const auto& nextList = it->second;

				INDEX_TYPE foundNext = -1;
				for (INDEX_TYPE candidate : nextList) {
					std::pair<INDEX_TYPE, INDEX_TYPE> e = { current, candidate };

					if (visitedEdges.find(e) == visitedEdges.end()) {
						foundNext = candidate;
						visitedEdges.insert(e);
						break;
					}
				}

				if (foundNext == -1)
					break;

				current = foundNext;
				loop.push_back(current);

				if (current == start) {
					if (loop.size() >= 3)
						loops.push_back(loop);
					break;
				}
			}
		}
	}

	return loops;
}



float squaredDistance(const CVertex& a, const CVertex& b) {
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	float dz = a.z - b.z;
	return dx * dx + dy * dy + dz * dz;
}


// Nowa funkcja mostkująca: pary punktów z jednej pętli -> jeden z drugiej
std::pair<std::vector<CVertex>, std::vector<CFace>> BridgingByMidpointMatching(
	const std::vector<CVertex>& V1, const std::vector<unsigned int>& IS_V1,
	const std::vector<CVertex>& V2, const std::vector<unsigned int>& IS_V2,
	float maxDistance = 2.0f)
{
	std::vector<CVertex> outVertices;
	std::vector<CFace> outFaces;

	const float maxDistSquared = maxDistance * maxDistance;

	for (size_t i = 0; i < IS_V1.size(); ++i) {
		size_t i2 = (i + 1) % IS_V1.size();

		const CVertex& a = V1[IS_V1[i]];
		const CVertex& b = V1[IS_V1[i2]];

		// Środek odcinka a-b
		CVertex mid;
		mid.x = 0.5f * (a.x + b.x);
		mid.y = 0.5f * (a.y + b.y);
		mid.z = 0.5f * (a.z + b.z);

		// Znajdź najbliższy punkt z drugiej siatki
		float minDist2 = std::numeric_limits<float>::max();
		int bestIdx = -1;
		for (unsigned int j : IS_V2) {
			//if (used_in_V2.find(j) != used_in_V2.end())
			//    continue;

			float d2 = squaredDistance(mid, V2[j]);
			if (d2 < minDist2) {
				minDist2 = d2;
				bestIdx = j;
			}
		}

		if (bestIdx != -1 && minDist2 < maxDistSquared) {
			// Dodajemy trójkąt (a, best, b)
			int idxA = static_cast<int>(outVertices.size());
			outVertices.push_back(a);
			outVertices.push_back(V2[bestIdx]);
			outVertices.push_back(b);
			outFaces.emplace_back(CFace(idxA, idxA + 1, idxA + 2));

			//used_in_V2.insert(bestIdx);
		}
	}

	return { outVertices, outFaces };
}



std::shared_ptr<CMesh> scalSiatki(std::vector<std::shared_ptr<CMesh>> siatki)
{
	auto result = std::make_shared<CMesh>();

	for (auto m : siatki) {

		for (auto f : m->faces()) {
			int idx = result->vertices().size();

			result->vertices().push_back(m->vertices()[f.A()]);
			result->vertices().push_back(m->vertices()[f.B()]);
			result->vertices().push_back(m->vertices()[f.C()]);

			result->faces().push_back(CFace(idx, idx + 1, idx + 2));
		}
	}

	result->removeDuplicateVertices(0.01);

	return result;
}


std::shared_ptr<CMesh> ConcretePlugin::bridging(std::shared_ptr<CMesh> wierzch, std::shared_ptr<CMesh> wnetrze) {
	if (wierzch && wnetrze)
	{
		wierzch->removeDuplicateVertices();
		wnetrze->removeDuplicateVertices();

		std::pair< std::vector<CVertex>, std::vector<CFace> > result;

		std::vector<CEdge> sz_ed;
		std::unordered_map<INDEX_TYPE, std::vector<INDEX_TYPE>> sz_bGraph;

		findBoundaryEdgesWithDirection(wierzch->faces(), sz_ed, sz_bGraph);
		std::vector<std::vector<INDEX_TYPE>> sz_loops = findBoundaryLoops(sz_bGraph);

		auto sz_ae = std::make_shared<CAnnotationEdges>();

		std::vector<INDEX_TYPE> IS_V;

		for (auto e : sz_ed) {
			IS_V.push_back(e.first);
			sz_ae->addEdge(wierzch->vertices()[e.first], wierzch->vertices()[e.second]);
		}


		std::vector<CEdge> ok_ed;
		std::unordered_map<INDEX_TYPE, std::vector<INDEX_TYPE>> ok_bGraph;

		findBoundaryEdgesWithDirection(wnetrze->faces(), ok_ed, ok_bGraph);
		std::vector<std::vector<INDEX_TYPE>> ok_loops = findBoundaryLoops(ok_bGraph);

		auto ok_ae = std::make_shared<CAnnotationEdges>();

		std::vector<INDEX_TYPE> IS_VZ;

		for (auto e : ok_ed) {
			IS_VZ.push_back(e.first);
			ok_ae->addEdge(wnetrze->vertices()[e.first], wnetrze->vertices()[e.second]);
		}

		std::vector<std::shared_ptr<CMesh>> siatki;

		for (auto l = 0; l < sz_loops.size(); l++) {

			qInfo() << QString("loop: %1, size: %2").arg(l).arg(sz_loops[l].size());

			result = BridgingByMidpointMatching(wierzch->vertices(), sz_loops[l], wnetrze->vertices(), IS_VZ, 8.0);

			qInfo() << QString("---- vertices: %1, faces: %2").arg(result.first.size()).arg(result.second.size());

			auto mostki = std::make_shared<CMesh>();

			mostki->vertices() = result.first;
			mostki->faces() = result.second;

			mostki->removeDuplicateVertices();

			siatki.push_back(mostki);
		}

		auto mesh1 = scalSiatki(siatki);

		auto test = scalSiatki({ wierzch, wnetrze, mesh1 });

		return test;
	}

	return nullptr;
}


//----------------------------------------------------------------------------




//// Funkcja znajduje skierowane krawędzie brzegowe (czyli takie, które występują tylko raz w danym kierunku)
//void findBoundaryEdgesWithDirection(const std::vector<CFace>& faces,
//	std::vector<CEdge>& boundaryEdges,
//	std::unordered_map<INDEX_TYPE, std::vector<INDEX_TYPE>>& boundaryGraph)
//{
//	using Edge = std::pair<INDEX_TYPE, INDEX_TYPE>;
//	std::unordered_map<Edge, INDEX_TYPE, PairHash> directedEdgeCount;
//
//	for (const CFace& f : faces) {
//		directedEdgeCount[{f.A(), f.B()}]++;
//		directedEdgeCount[{f.B(), f.C()}]++;
//		directedEdgeCount[{f.C(), f.A()}]++;
//	}
//
//	boundaryEdges.clear();
//	boundaryGraph.clear();
//
//	for (const auto& [e, count] : directedEdgeCount) {
//		Edge reversed = { e.second, e.first };
//		if (count == 1 && directedEdgeCount.find(reversed) == directedEdgeCount.end()) {
//			boundaryEdges.emplace_back(e.first, e.second);
//			boundaryGraph[e.first].push_back(e.second);
//		}
//	}
//}

#include <queue>

std::vector<INDEX_TYPE> findShortestCycleFrom(INDEX_TYPE start, const std::unordered_map<INDEX_TYPE, std::vector<INDEX_TYPE>>& graph)
{
	std::queue<std::vector<INDEX_TYPE>> q;
	std::unordered_set<INDEX_TYPE> visited;

	q.push({ start });

	while (!q.empty()) {
		auto path = q.front(); q.pop();
		INDEX_TYPE current = path.back();

		auto it = graph.find(current);
		if (it == graph.end())
			return {};
		//continue; // brak sąsiadów – pomiń

	//for (INDEX_TYPE neighbor : graph.at(current)) {
		for (INDEX_TYPE neighbor : it->second) {
			if (neighbor == start && path.size() >= 3) {
				path.push_back(start);
				return path; // domknięty cykl
			}

			// unikamy powtórzeń
			if (std::find(path.begin(), path.end(), neighbor) == path.end()) {
				auto newPath = path;
				newPath.push_back(neighbor);
				q.push(newPath);
			}
		}
	}

	return {}; // brak cyklu
}





std::vector<std::vector<INDEX_TYPE>> findBoundaryLoopsFromEdges(const std::vector<CEdge>& boundaryEdges)
{
	std::unordered_map<INDEX_TYPE, std::vector<INDEX_TYPE>> graph; // , back_graph;
	std::unordered_set<INDEX_TYPE> vertices_remains;
	for (const CEdge& edge : boundaryEdges) {
		graph[edge.first].push_back(edge.second);

		//        back_graph[edge.second].push_back(edge.first);

		vertices_remains.insert(edge.first);
		vertices_remains.insert(edge.second);
	}


	std::vector<std::vector<INDEX_TYPE>> loops;

	auto current = vertices_remains.begin();
	while (current != vertices_remains.end()) {

		std::vector<INDEX_TYPE> path = findShortestCycleFrom(*current, graph);

		if (path.empty()) {
			vertices_remains.erase(*current);
		}
		else {
			loops.push_back(path);

			for (auto p : path) {
				vertices_remains.erase(p);
			}
		}

		current = vertices_remains.begin();
	}

	return loops;
}



#include "AnnotationEdges.h"
#include "../api/AP.h"

// Funkcja zamyka wszystkie wykryte pętle brzegowe tworząc wachlarze trójkątów wokół centroidu
std::pair<std::vector<CVertex>, std::vector<CFace>> FillBoundaryLoops(const std::vector<CVertex>& vertices, const std::vector<CFace>& faces)
{
	// PLEASE NOTE: INDEX_TYPE is currently defined as uint32_t

	std::vector<CEdge> boundaryEdges;
	std::unordered_map<INDEX_TYPE, std::vector<INDEX_TYPE>> boundaryGraph;

	findBoundaryEdgesWithDirection(faces, boundaryEdges, boundaryGraph);



	//    CAnnotationEdges* ok_ae = new CAnnotationEdges();

	std::vector<INDEX_TYPE> IS_VZ;

	for (auto e : boundaryEdges) {
		IS_VZ.push_back(e.first);
		//        ok_ae->addEdge(vertices[e.first], vertices[e.second]);
	}

	//    AP::WORKSPACE::addObject(ok_ae);


	auto loops = findBoundaryLoopsFromEdges(boundaryEdges);
	qDebug() << "Znaleziono pętli:" << loops.size();

	//auto loops = filterDisjointEdgeLoops(rawLoops);
	//qDebug() << "Po filtracji:" << loops.size();

	std::vector<CVertex> outVertices;
	std::vector<CFace> outFaces;

	int cnt = 0;
	for (const auto& loop : loops) {
		qInfo() << QString("loop: %1, size: %2").arg(cnt).arg(loop.size());
		cnt++;

		if (loop.size() < 3) {
			qInfo() << "WADLIWA PETLA";
			continue;
		}
		else if (loop.size() > 500) {
			qInfo() << "ZA DUZA PETLA";
			continue;
		}
		else if (loop.size() == 3) {
			CVertex a = vertices[loop[2]];
			CVertex b = vertices[loop[1]];
			CVertex c = vertices[loop[0]];

			int aIdx = static_cast<int>(outVertices.size());
			int bIdx = aIdx + 1;
			int cIdx = aIdx + 2;

			outVertices.push_back(a);
			outVertices.push_back(b);
			outVertices.push_back(c);

			outFaces.emplace_back(CFace(aIdx, bIdx, cIdx));
		}
		else {
			// Oblicz centroid
			CVertex center = { 0, 0, 0 };
			for (int idx : loop) {
				center += vertices[idx];
			}
			float inv = 1.0f / loop.size();
			center *= inv;

			// Dodaj centroid do wynikowej listy i zapamiętaj jego indeks
			int centerIndex = static_cast<int>(outVertices.size());
			outVertices.push_back(center);

			// Dodaj trójkąty wachlarzowe z istniejących punktów pętli
			for (size_t i = 0; i < loop.size(); ++i) {
				CVertex a = vertices[loop[i]];
				CVertex b = vertices[loop[(i + 1) % loop.size()]];
				int aIdx = static_cast<int>(outVertices.size());
				int bIdx = aIdx + 1;
				outVertices.push_back(a);
				outVertices.push_back(b);
				outFaces.emplace_back(CFace(aIdx, centerIndex, bIdx));
			}
		}
	}

	return { outVertices, outFaces };
}







std::shared_ptr<CMesh> ConcretePlugin::filling(std::shared_ptr<CMesh> test)
{
	if (test)
	{
		test->removeDuplicateVertices();

		auto new_faces = FillBoundaryLoops(test->vertices(), test->faces());

		auto nowe = std::make_shared<CMesh>();

		nowe->vertices() = new_faces.first;
		nowe->faces() = new_faces.second;

		nowe->removeDuplicateVertices();

		auto full = scalSiatki({ test, nowe });

		return full;
	}

	return nullptr;
}
