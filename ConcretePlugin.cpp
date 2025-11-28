#include "ConcretePlugin.h"

#include "Workspace.h"

#include "AnnotationPlane.h"
#include "AnnotationEdges.h"

#include "BiteSim.h"

#include "../api/AP.h"
#include "../api/UI.h"

#include "AppSettings.h"

#include "NewEdge.h"
#include "Vector3.h"
#include "IndexedTriangle.h"
#include "KDNode.h"

#include "FileConnector.h"

#include "../gui/ProgressIndicator.h"
//#include "interfaces/IProgressListener.h"

#include <memory>
#include <omp.h>

#include <QDebug>
//#include "dpLog.h"

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

	m_cutPlane = nullptr;
	m_projectionPlane = nullptr;

	m_divider = 10;

	QObject::connect(this, &ConcretePlugin::setProgressBarValue, UI::PROGRESSBAR::instance(), &ProgressIndicator::setValue);
}



std::shared_ptr<CMesh>  ConcretePlugin::liczOkluzje(std::shared_ptr<CMesh>  szczeka, std::shared_ptr<CMesh>  zuchwa, double dist = 3.0)
{
	std::set<size_t> good;

	// ZAKŁADAMY ŻE JUŻ SĄ WE WSPÓLNYM UKŁADZIE WSPÓŁZĘDNYCH
	// 
	//auto szczeka = std::dynamic_pointer_cast<CMesh>(szczeka1->getCopy());
	//auto zuchwa = std::dynamic_pointer_cast<CMesh>(zuchwa1->getCopy());

	//auto _sM = CBaseObject::getGlobalTransformationMatrix(szczeka1);
	//auto _zM = CBaseObject::getGlobalTransformationMatrix(zuchwa1);

	//auto _sT = CTransform(_sM);
	//auto _zT = CTransform(_zM);
	//auto nullT = CTransform();

	////transformacja szczeki do ukladu zuchwy
	//szczeka->applyTransformation(_sT, nullT);
	////transformacja zuchwy do ukladu zuchwy (tożsama)
	//zuchwa->applyTransformation(_zT, nullT);

	CMesh::KDtree *kd = &zuchwa->getKDtree(CMesh::KDtree::REBUILD);

	UI::STATUSBAR::setText("Looking for unwanted vertices. Please wait...");
	UI::PROGRESSBAR::init(0, szczeka->vertices().size(), 0);

	int progress = 0;

#pragma omp parallel for
for (long idx = 0; idx < szczeka->vertices().size(); idx++)
{
    CVertex& v = szczeka->vertices()[idx];
    
    if (kd->is_any_in_distance_to_pt(dist,v))
    {
        #pragma omp critical
        {
            good.insert(idx);
        }
    }

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
		QFileDialog::getOpenFileName( 0, QString::fromUtf8("Select upper jaw file"), AppSettings::mainSettings()->value("recentFile").toString(), CFileConnector::getLoadExts())
	);

	if (!fileName.isEmpty() && QFileInfo(fileName).exists()) {
		std::shared_ptr<CModel3D> obj = AP::WORKSPACE::loadModel(fileName);
		
		if (obj == nullptr) { //ERROR
			UI::MESSAGEBOX::error(QString::fromUtf8("Failed to open file %1").arg(fileName), QString::fromUtf8("File read error"));
			return;
		}

		if (obj->hasChildren()) {
			AppSettings::mainSettings()->setValue("recentFile", fileName);
		}

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


std::pair<Eigen::Matrix4d, Eigen::Matrix4d> getTransformationMatrices(std::shared_ptr<CBaseObject> obj, std::string stop)
{
	Eigen::Matrix4d rootToStop = Eigen::Matrix4d::Identity();
	Eigen::Matrix4d stopToObj = Eigen::Matrix4d::Identity();

	if (!obj) return { rootToStop, stopToObj };

	QString stopQ = QString::fromStdString(stop);

	// Zbierz węzły od obiektu w górę aż do (ale NIE włączając) stop
	std::vector<std::shared_ptr<CBaseObject>> below;
	std::shared_ptr<CBaseObject> cur = obj;
	while (cur && cur->getLabel().compare(stopQ, Qt::CaseInsensitive) != 0) {
		below.push_back(cur);
		cur = cur->getParentPtr();
	}

	// 'cur' jest teraz albo węzłem o label==stop, albo nullptr
	// Zbierz węzły od stop w górę (włącznie)
	std::vector<std::shared_ptr<CBaseObject>> above;
	while (cur) {
		above.push_back(cur);
		cur = cur->getParentPtr();
	}

	// stop -> obj: mnożymy od dołu (obj) do góry (bez stop)
	for (size_t i = 0; i < below.size(); ++i) {
		auto n = below[i];
		if (n->hasTransformation()) {
			stopToObj = n->getTransformationMatrix() * stopToObj;
		}
	}

	// root -> stop: mnożymy od stop w górę, ale kolejność mnożenia daje finalnie root..stop
	for (size_t i = 0; i < above.size(); ++i) {
		auto n = above[i];
		if (n->hasTransformation()) {
			rootToStop = n->getTransformationMatrix() * rootToStop;
		}
	}

	return { rootToStop, stopToObj };
}

std::pair<Eigen::Matrix4d, Eigen::Matrix4d> makeReversedOrderMatrices(
	const Eigen::Matrix4d& rootToStop,
	const Eigen::Matrix4d& stopToObj)
{
	Eigen::Matrix4d rootToStop1 = rootToStop;
	Eigen::Matrix4d stopToObj1 = stopToObj;

	// Spróbuj odwrócić stopToObj; sprawdź numeryczną odwracalność
	const double eps = 1e-12;
	Eigen::FullPivLU<Eigen::Matrix4d> lu(stopToObj);
	if (!lu.isInvertible()) {
		// fallback: nie da się odwrócić - zwracamy oryginały (bez transformacji)
		return { rootToStop1, stopToObj1 };
	}

	Eigen::Matrix4d stopToObjInv = stopToObj.inverse();

	// koniugacja: rootToStop1 = S^{-1} * R * S
	rootToStop1 = stopToObjInv * rootToStop * stopToObj;

	return { rootToStop1, stopToObj1 };
}

void ConcretePlugin::etap00ag(double dist2, bool dane_z_pomiaru)
{
	QCursor c = moj_widget->cursor();
	c.setShape(Qt::CursorShape::WaitCursor);
	moj_widget->setCursor(c);

	std::shared_ptr<CMesh> sz1 = std::dynamic_pointer_cast<CMesh>(szcz->getCopy());
	std::shared_ptr<CMesh> zu1 = std::dynamic_pointer_cast<CMesh>(zuch->getCopy());

	Eigen::Matrix4d mSz = CBaseObject::getGlobalTransformationMatrix(szcz);
	auto [mZuR, mZu] = getTransformationMatrices(zuch, "transformation");

	auto [mZuR1, mZu1] = makeReversedOrderMatrices(mZuR, mZu);

	CTransform nullT;
	CTransform tSz(mSz);
	CTransform tZu(mZu);
	CTransform tZuR1(mZuR1);
	CTransform tZuR(mZuR);
	CTransform tZuRinv(mZuR.inverse());

	auto nowymodelSz = std::make_shared<CModel3D>();
	nowymodelSz->setLabel("Szczeka_global");
	nowymodelSz->setTransform(tSz);
	AP::WORKSPACE::addObject(nowymodelSz);
	AP::OBJECT::addChild(nowymodelSz, sz1);
	nowymodelSz->applyTransform();


	auto nowymodelZu = std::make_shared<CModel3D>();
	nowymodelZu->setLabel("Zuchwa_global");
	nowymodelZu->setTransform(tZu);
	AP::WORKSPACE::addObject(nowymodelZu);
	AP::OBJECT::addChild(nowymodelZu, zu1);
	nowymodelZu->applyTransform();

	auto nowymodelZu2 = std::make_shared<CModel3D>();
	nowymodelZu2->setLabel("*transformation");
	nowymodelZu2->setTransform(tZuR);
	AP::OBJECT::addChild(nowymodelSz, nowymodelZu2);
	//AP::OBJECT::addChild(nowymodelZu2, zu1);



	oklu = liczOkluzje(sz1, zu1, dist2);

	oklu->setLabel(QString("okluzja: %1mm").arg(dist2));
	oklu->calcFN();

	AP::OBJECT::addChild(nowymodelZu2, oklu);

	auto nowymodelZu3 = std::make_shared<CModel3D>();
	nowymodelZu3->setLabel("*transformation_inv");
	nowymodelZu3->setTransform(tZuR);
	AP::OBJECT::addChild(nowymodelSz, nowymodelZu3);
	AP::OBJECT::addChild(nowymodelZu3, zu1);
	//AP::OBJECT::addChild(nowymodelZu3, oklu);
	nowymodelZu3->applyTransform();
	nowymodelZu3->setTransform(tZuRinv);


	AP::WORKSPACE::removeModel(nowymodelZu);



	szcz = sz1;
	szcz_parent = std::dynamic_pointer_cast<CModel3D>(szcz->getParentPtr());
	zuch = zu1;

	moj_widget->wybor_siatek->ustawSzczeke(szcz->getLabel());
	moj_widget->wybor_siatek->ustawZuchwe(zuch->getLabel());
	moj_widget->wybor_siatek->ustawOkluzje(oklu->getLabel());

	//ustawiam pion w osi Z
	nowymodelSz->transform().rotateAroundAxisDeg(CVector3d::XAxis(), 90.0);


	c.setShape(Qt::CursorShape::ArrowCursor);
	moj_widget->setCursor(c);

	UI::updateAllViews();
}


void ConcretePlugin::etap01(int div)
{
	if (oklu == nullptr || szcz == nullptr)
	{
		UI::MESSAGEBOX::error("First, select the meshes to be processed.");
		return;
	}

	moj_layout->removeRow(moj_widget->etap11);
	moj_layout->removeRow(moj_widget->przytnij_szczene);
	moj_layout->removeRow(moj_widget->wybor_siatek);
	
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
	
	symulator->szczeka_tworzMapeOkluzji2(oklu, mOk, mSz);
	
	CBoundingBox bb = symulator->szczeka_obj->getBoundingBox();
	CBoundingBox bbOk = symulator->szczeka_okluzja->getBoundingBox();

	if (m_cutPlane == nullptr) {
		m_cutPlane = std::make_shared<CAnnotationPlane>();
		m_cutPlane->setSize(80.0);
		m_cutPlane->setLabel(L"cutting plane");
		m_cutPlane->setColor(CRGBA(1.0f, 0.8f, 0.3f, 0.7f));

		m_cutPlane->setCenter(bb.getMidpoint());

		CVector3d n2 = oklu->getMainNormalVector(true);
		//CVector3d n2 = std::dynamic_pointer_cast<CMesh>(symulator->szczeka_okluzja->getData())->getMainNormalVector(true);
		n2.x = 0.0;
		n2.y = 0.0;

		m_cutPlane->setNormal(n2);

		if (testyAG) {
			//tymczasowe - testowe ustawienie płaszczyzny ciecia
			m_cutPlane->setCenter(CPoint3d(-4.7, -54.5, -39.0));
			m_cutPlane->setNormal(CVector3d(-0.030508, 0.347240, -0.937280).getNormalized());

			//m_projectionPlane = std::make_shared<CAnnotationPlane>(m_cutPlane->getCenter(), m_cutPlane->getNormal());
			//m_projectionPlane->setCenter(CPoint3d(-4.715613, -54.49725, -37.67131));
			//m_projectionPlane->setNormal(CVector3d(-0.026998, 0.485099, -0.874042).getNormalized());

			//m_projectionPlane->setSize(80);
			//m_projectionPlane->setLabel(L"projection plane");
			//AP::MODEL::addAnnotation(symulator->szczeka_obj, m_projectionPlane);
		}
	}

	AP::MODEL::addAnnotation(symulator->szczeka_obj, m_cutPlane);
	UI::updateAllViews();

	AP::WORKSPACE::setCurrentModel(NO_CURRENT_MODEL);


	moj_widget->widget_info = new WidgetInfo({ QString::fromUtf8("To cut off excess pieces of mesh, set the cutting plane in the correct position relative to the jaw and click ‘Next’.") });

	moj_layout->addRow(moj_widget->widget_info);

	QObject::connect(moj_widget->widget_info->btStart, &QPushButton::clicked, [&]() { etap11(); });

	c.setShape(Qt::CursorShape::ArrowCursor);
	moj_widget->setCursor(c);
}


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
	//moj_widget->widget_info->setDisabled(true);
	moj_layout->removeRow(moj_widget->widget_info);

	m_cutPlane->setSelfVisibility(false);
	UI::updateAllViews();

	QCursor c = moj_widget->cursor();
	c.setShape(Qt::CursorShape::WaitCursor);
	moj_widget->setCursor(c);

	symulator->szczeka_wytnijZebyNEW(m_cutPlane);
	AP::WORKSPACE::addModel(symulator->szczeka_zeby);

	symulator->wnetrze = new CSiateczka1(80, 80, 30, symulator->m_divider); // arguments: sizeX, sizeY, depth, divider

	AP::WORKSPACE::addModel(symulator->wnetrze->obj);

	if (m_projectionPlane == nullptr) {
		m_projectionPlane = std::make_shared<CAnnotationPlane>(m_cutPlane->getCenter(), m_cutPlane->getNormal());
		m_projectionPlane->setSize(80);

		if (testyAG) {
			//tymczasowe - testowe ustawienie płaszczyzny projekcji
			m_projectionPlane->setCenter(CPoint3d(10.0, 0.0, -20.0));
		}

		AP::OBJECT::addChild(symulator->wnetrze->obj, m_projectionPlane);
	}
	
	symulator->wnetrze->obj->transform().fromQMatrix4x4(planeToTransform(m_projectionPlane->getNormal()));
	symulator->wnetrze->obj->transform().translate(CVector3d(CPoint3d(0),m_cutPlane->getCenter()));
	
	UI::updateAllViews();

	moj_widget->etap123 = new WidgetEtap123();
	moj_layout->addRow(moj_widget->etap123);

	QObject::connect(moj_widget->etap123->btStart23, &QPushButton::clicked, [&]() {
		etap123(moj_widget->etap123->insideDist->value(), moj_widget->etap123->outsideDist->value()); 
	});

	c.setShape(Qt::CursorShape::ArrowCursor);
	moj_widget->setCursor(c);
}





void ConcretePlugin::etap123(double dValIn, double dValOut)
{
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

	AP::OBJECT::removeChild(symulator->wnetrze->obj, m_projectionPlane);

	// selektywne usuwanie zerowych ścianek z wnętrza
	// (wierzch już ma usunięte skrajne ścianki dzięki usunSkrajneScianki())
	qInfo() << "Selektywne usuwanie zerowych scianek z wnetrza...";
	symulator->usunNiepasujaceZeroweSciankiWnetrza();

	etap14_nowy();

	wytlaczanie();

	c.setShape(Qt::CursorShape::ArrowCursor);
	moj_widget->setCursor(c);

	moj_widget->info_zapis = new WidgetInfo({
		QString::fromUtf8("You can save all the important meshes."),
		});
	moj_widget->info_zapis->btStart->setText("LET'S DO IT");

	moj_layout->addRow(moj_widget->info_zapis);

	QObject::connect(moj_widget->info_zapis->btStart, &QPushButton::clicked, [&]() { save_all(); });


	moj_widget->info_koncowe = new WidgetInfo({
		QString::fromUtf8("Remove all models and reset plugin."),
		});
	moj_widget->info_koncowe->btStart->setText("RESET");

	moj_layout->addRow(moj_widget->info_koncowe);

	QObject::connect(moj_widget->info_koncowe->btStart, &QPushButton::clicked, [&]() { reset_plugin(); });

}



void ConcretePlugin::reset_plugin() 
{
	AP::WORKSPACE::removeAllModels();

	waiting_for = CzekamNa::Nic;
	szcz = nullptr;
	oklu = nullptr;
	zuch = nullptr;
	szcz_parent = nullptr;
	top_model_arch = nullptr;
	mesh_wierzch = nullptr;
	mesh_wnetrze = nullptr;

	symulator = nullptr;

	m_cutPlane = nullptr;
	m_projectionPlane = nullptr;

	m_divider = 10;

	UI::PLUGINPANEL::clear(m_ID);
	showMainPanel();
}


void ConcretePlugin::wytlaczanie()
{
	CObject::Children kids;

	for (const auto& kid : CWorkspace::instance()->children())
	{
		kids[kid.first] = kid.second;
	}

	std::shared_ptr<CMesh> sz, ok, zu, wi, wn;

	auto tmp = meshWithKeywordInLabel(QString("wierzch_mesh"), kids);
	if (tmp) {
		qInfo() << "Found: " << tmp->getLabel();
		Eigen::Matrix4d M = CBaseObject::getGlobalTransformationMatrix(tmp);
		CTransform t0, t1(M);
		tmp->applyTransformation(t1, t0);
		wi = tmp;
		wi->setLabel("wierzch");
		wi->removeAllChilds();
	}

	tmp = meshWithKeywordInLabel(QString("wnetrze_mesh"), kids);
	if (tmp) {
		qInfo() << "Found: " << tmp->getLabel();
		Eigen::Matrix4d M = CBaseObject::getGlobalTransformationMatrix(tmp);
		CTransform t0, t1(M);
		tmp->applyTransformation(t1, t0);
		wn = tmp;
		wn->setLabel("wnetrze");
		wn->removeAllChilds();
	}

	tmp = meshWithKeywordInLabel(QString("testowa zuchwa"), kids);
	if (tmp) {
		qInfo() << "Found: " << tmp->getLabel();
		Eigen::Matrix4d M = CBaseObject::getGlobalTransformationMatrix(tmp);
		CTransform t0, t1(M);
		tmp->applyTransformation(t1, t0);
		zu = tmp;
		zu->setLabel("zuchwa");
	}

	tmp = meshWithKeywordInLabel(QString("szczeka_mesh"), kids);
	if (tmp)
	{
		qInfo() << "Found: " << tmp->getLabel();
		Eigen::Matrix4d M = CBaseObject::getGlobalTransformationMatrix(tmp);
		CTransform t0, t1(M);
		tmp->applyTransformation(t1, t0);
		sz = tmp;
		sz->setLabel("szczeka");
	}

	tmp = meshWithKeywordInLabel(QString("okluzja_mesh"), kids);
	if (tmp)
	{
		qInfo() << "Found: " << tmp->getLabel();
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

	UI::STATUSBAR::setText("Creation of an occlusal stamp");

	auto stmp = stampFromMesh(ok);

	UI::STATUSBAR::setText("Impressing an occlusal stamp on the outer surface of the splint");
	qInfo() << "Impressing an occlusal stamp on the outer surface of the splint";

	zrobOdciskStempla(wi, stmp, 0.0);

	UI::STATUSBAR::setText("Imprinting the lower jaw on the outer surface of the splint");
	qInfo() << "Imprinting the lower jaw on the outer surface of the splint";

	zrobOdciskStempla(wi, zu, 0.2);

	UI::STATUSBAR::setText("Making holes at intersections");
	qInfo() << "Making holes at intersections" << endl;

	//return;


	auto [wi2, wn2] = zrobDziury(wi, wn);


	UI::STATUSBAR::setText("Bridging the outer and inner surfaces");

	if (auto bridged = bridging(wi2, wn2)) {

		UI::STATUSBAR::setText("Filling holes in the surface");

		if (auto filled = filling(bridged)) {
			filled->setLabel("szyna");
			CWorkspace::instance()->_objectAdd(filled);
			filled->getParentPtr()->setLabel("szyna");

			UI::DOCK::WORKSPACE::update();

			UI::STATUSBAR::setText("The splint is ready");
		}
		else {
			UI::STATUSBAR::setText("Error when filling holes");
		}
	}
	else {
		UI::STATUSBAR::setText("Error when bridging surfaces");
	}
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
	symulator->wierzch->obj->applyTransform();

	std::shared_ptr<CMesh> wierzch = std::dynamic_pointer_cast<CMesh>(symulator->wierzch->obj->getChild());

	decapitation(wierzch, m_cutPlane);

	//----------------------------------------------------------------

	symulator->wnetrze->obj->applyTransform();

	std::shared_ptr<CMesh> wnetrze = std::dynamic_pointer_cast<CMesh>(symulator->wnetrze->obj->getChild());

	decapitation(wnetrze, m_cutPlane);

	//----------------------------------------------------------------

	AP::OBJECT::removeChild(symulator->szczeka_obj, m_cutPlane);

	UI::DOCK::WORKSPACE::update();
}


void ConcretePlugin::etap14_nowy()
{
	//symulator->wierzch->obj->applyTransform();

	//std::shared_ptr<CMesh> wierzch = std::dynamic_pointer_cast<CMesh>(symulator->wierzch->obj->getChild());

	//decapitation(wierzch, m_cutPlane);

	////----------------------------------------------------------------

	//symulator->wnetrze->obj->applyTransform();

	//std::shared_ptr<CMesh> wnetrze = std::dynamic_pointer_cast<CMesh>(symulator->wnetrze->obj->getChild());

	//decapitation(wnetrze, m_cutPlane);

	//----------------------------------------------------------------

	AP::OBJECT::removeChild(symulator->szczeka_obj, m_cutPlane);

	UI::DOCK::WORKSPACE::update();
}


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




void ConcretePlugin::save_all()
{
	QFileInfo fi(AppSettings::mainSettings()->value("recentFile").toString());

	QString init_path = QString("%1/szyna.obj").arg(fi.absoluteDir().absolutePath());

	QString splint_path = UI::FILECHOOSER::getSaveFileName(QString("Wybierz plik zapisu szyny"), init_path, "OBJ File (*.obj)");

	qInfo() << splint_path;

	QFileInfo splint_info(splint_path);
	QDir dir = splint_info.absoluteDir();

	qInfo() << splint_info.completeSuffix();
	qInfo() << dir.absolutePath();

	CObject::Children kids;

	for (const auto& kid : AP::WORKSPACE::instance()->children())
	{
		kids[kid.first] = kid.second;
	}

	auto tmp = meshWithKeywordInLabel(QString("wierzch"), kids);
	if (tmp != nullptr)
	{
		qInfo() << "Found: " << tmp->getLabel();
		save_mesh(tmp, "wierzch", QString("%1/%2.%3").arg(dir.absolutePath()).arg("wierzch").arg(splint_info.completeSuffix()));
	}

	tmp = meshWithKeywordInLabel(QString("wnetrze"), kids);
	if (tmp != nullptr)
	{
		qInfo() << "Found: " << tmp->getLabel();
		save_mesh(tmp, "wnetrze", QString("%1/%2.%3").arg(dir.absolutePath()).arg("wnetrze").arg(splint_info.completeSuffix()));
	}

	tmp = meshWithKeywordInLabel(QString("zuchwa"), kids);
	if (tmp != nullptr)
	{
		qInfo() << "Found: " << tmp->getLabel();
		save_mesh(tmp, "zuchwa", QString("%1/%2.%3").arg(dir.absolutePath()).arg("zuchwa").arg(splint_info.completeSuffix()));
	}

	tmp = meshWithKeywordInLabel(QString("szczeka"), kids);
	if (tmp != nullptr)
	{
		qInfo() << "Found: " << tmp->getLabel();
		save_mesh(tmp, "szczeka", QString("%1/%2.%3").arg(dir.absolutePath()).arg("szczeka").arg(splint_info.completeSuffix()));
	}

	tmp = meshWithKeywordInLabel(QString("okluzja"), kids);
	if (tmp != nullptr)
	{
		qInfo() << "Found: " << tmp->getLabel();
		save_mesh(tmp, "okluzja", QString("%1/%2.%3").arg(dir.absolutePath()).arg("okluzja").arg(splint_info.completeSuffix()));
	}

	tmp = meshWithKeywordInLabel(QString("stempel"), kids);
	if (tmp != nullptr)
	{
		qInfo() << "Found: " << tmp->getLabel();
		save_mesh(tmp, "stempel", QString("%1/%2.%3").arg(dir.absolutePath()).arg("stempel").arg(splint_info.completeSuffix()));
	}

	tmp = meshWithKeywordInLabel(QString("szyna"), kids);
	if (tmp != nullptr)
	{
		qInfo() << "Found: " << tmp->getLabel();
		save_mesh(tmp, "szyna", QString("%1/%2.%3").arg(dir.absolutePath()).arg("szyna").arg(splint_info.completeSuffix()));
	}

	UI::STATUSBAR::setText(QString::fromUtf8("ZAPISANO"));
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

	QObject::connect(moj_widget->wybor_siatek->btLiczOkluAg, &QPushButton::clicked, [&]() {
		if (szcz && zuch) {
			bool dane_z_pomiaru = moj_widget->wybor_siatek->zPomiaru->isChecked();

			etap00ag(moj_widget->wybor_siatek->okluDist->value(), dane_z_pomiaru);

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

	moj_widget->etap11 = new WidgetGestoscSiatki();

	QObject::connect(moj_widget->etap11->btStart, &QPushButton::clicked, [&]() { etap01(moj_widget->etap11->meshDivider->value()); });

	moj_layout->addRow(moj_widget->etap11);

	moj_widget->etap11->setDisabled(true);
}

void ConcretePlugin::onLoad()
{
	UI::PLUGINPANEL::create( m_ID, "Splint Maker" );

	showMainPanel();
}


#include "AnnotationPoint.h"
#include "AnnotationPoints.h"
#include "AnnotationPath.h"
#include "AnnotationPath.h"
#include "AnnotationVPath.h"


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

	qInfo() << "Tworzenie stempla...";	

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



std::shared_ptr<CMesh> ConcretePlugin::stampFromMesh(std::shared_ptr<CMesh> mesh) {
	if (mesh)
	{
		// rasteryzacja -> wokselowy odpowiednik siatki
		VoxelGrid vox_okluzja = rasterizeMeshToVoxels(mesh->vertices(), mesh->faces(), 0.1);
		VoxelGrid vox_stempel;

		qInfo() << "Rasteryzacja zako�czona. X:" << vox_okluzja.dimX << " Y:" << vox_okluzja.dimY << " Z:" << vox_okluzja.dimZ;
		createStempel(vox_okluzja, vox_stempel);

		auto vol = createVolumetric(vox_stempel);

		//AP::WORKSPACE::addObject(vol);

		if (auto stempel = vol->marching_cube(1)) {
			stempel->setLabel("stempel");
			//if (AP::WORKSPACE::addObject(stempel)) {
			//	stempel->getParentPtr()->setLabel("stempel");
			//	UI::DOCK::WORKSPACE::update();
			//	UI::updateAllViews();
			//}

			return stempel;
		}
	}
	return nullptr;
}


//--------------------------------------------------------------------


#include "KDNode2.h"


void ConcretePlugin::zrobOdciskStempla(std::shared_ptr<CMesh> wierzch, std::shared_ptr<CMesh> stempel, double _distMax)
{
	//	unsigned long t1 = GetTickCount();

	qInfo() << "Odcisk stempla, tworze KDTree. Stempel: " << stempel->faces().size() << " scianek.";

	KDNode2* tree = KDNode2::build(stempel.get(), 10240);
	KDNode2::HitMap hmap;

	//rzutnia->calcVN();

	//CPoint3d mid = rzutnia->getCenterOfWeight();

	UI::PROGRESSBAR::init(0, wierzch->vertices().size(), 0);
	UI::PROGRESSBAR::setText("Imprinting:");
	
	qInfo() << "Imprinting " << wierzch->getLabel() << " with " << stempel->getLabel();

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
					//qInfo() << "i = " << i << " dist = " << dist;
					UI::PROGRESSBAR::setValue(i);
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

	UI::PROGRESSBAR::hide();

	//	UI::STATUSBAR::printf(L"Wycisk stempla gotowy. Czas wykonania: %d ms", GetTickCount() - t1);

	//	UI::updateAllViews();
}




//-----------------------------------------------------------------------------------





#include "CollisionDetector.h"
#include "AnnotationSetOfFaces.h"
#include "triangulacja.h"

//void przeciecia(std::shared_ptr<CMesh> mesh1, std::shared_ptr<CMesh> mesh2) {
//
//	std::map<INDEX_TYPE, std::set<INDEX_TYPE>*> crossed;
//
//	if (CollisionDetector::getIntersectionOfMeshWithMesh3d(mesh1, mesh2, crossed))
//	{
//		auto ed = std::make_shared<CAnnotationEdges>();
//
//		SplitTriangles2(mesh1, mesh2, crossed, *ed);
//
//		AP::OBJECT::addChild(mesh1, ed);
//
//		UI::STATUSBAR::setText("Ready. You can see sugested faces.");
//	}
//	else
//	{
//		UI::STATUSBAR::setText("No intersections found");
//	}
//
//
//
//}

void przeciecia(std::shared_ptr<CMesh> mesh1, std::shared_ptr<CMesh> mesh2) {
	qInfo() << "=== PRZECIECIA: START ===";
	qInfo() << "Mesh1: faces=" << mesh1->faces().size() << ", vertices=" << mesh1->vertices().size();
	qInfo() << "Mesh2: faces=" << mesh2->faces().size() << ", vertices=" << mesh2->vertices().size();

	std::map<INDEX_TYPE, std::set<INDEX_TYPE>*> crossed;

	qInfo() << "Wykrywanie kolizji...";
	UI::PROGRESSBAR::init(0, 100, 0);
	UI::PROGRESSBAR::setText("Detecting intersections...");
	UI::PROGRESSBAR::setValue(10);

	if (CollisionDetector::getIntersectionOfMeshWithMesh3d(mesh1, mesh2, crossed))
	{
		qInfo() << "Znaleziono przeciec:" << crossed.size();

		UI::PROGRESSBAR::setValue(50);
		UI::PROGRESSBAR::setText("Splitting triangles...");

		auto ed = std::make_shared<CAnnotationEdges>();

		qInfo() << "Rozdzielanie trojkatow...";
		SplitTriangles2(mesh1, mesh2, crossed, *ed);

		qInfo() << "Dodawanie krawedzi...";
		AP::OBJECT::addChild(mesh1, ed);

		UI::PROGRESSBAR::setValue(100);
		UI::STATUSBAR::setText("Ready. You can see suggested faces.");
	}
	else
	{
		qInfo() << "Nie znaleziono przeciec";
		UI::PROGRESSBAR::hide();
		UI::STATUSBAR::setText("No intersections found");
	}

	UI::PROGRESSBAR::hide();
	qInfo() << "=== PRZECIECIA: END ===";
}



#include <omp.h>
#include <mutex>

//void zamienPrzecieciaNaDziury2(CMesh* wierzch, CMesh* stempel, double shift, CVector3d mv, std::set<INDEX_TYPE>& vertices_to_remove)
//{
//	std::mutex mtx;
//
//	KDNode2* tree = KDNode2::build(stempel, 5000);
//
//	int last_i = 0;
//
//
//#pragma omp parallel
//	{
//		std::set<INDEX_TYPE> local_set;
//
//#pragma omp for
//		for (int i = 0; i < wierzch->vertices().size(); i++) {
//
//			CVertex p0 = wierzch->vertices()[i];
//
//			// punkt pocz�tkowy dla wyszukiwania
//			CPoint3d p0mv = p0 + CVector3d(0.0, 0.0, shift);
//
//			KDNode2::HitMap hmap;
//			bool hit = tree->hit(stempel, p0mv, mv, hmap);
//
//			if (hit) {
//				double dist = (*hmap.begin()).second.first;
//				CPoint3d p1 = (*hmap.begin()).second.second;
//				int idx = (*hmap.begin()).first;
//
//				if (hmap.size() > 1) {
//					for (auto h : hmap) {
//						double dd = h.second.first;
//
//						if (dd < dist) {
//							dist = dd;
//							p1 = h.second.second;
//							idx = h.first;
//						}
//					}
//				}
//
//				if (dist > abs(shift))
//				{
//					// punkt nale�y do przeciecia
//					local_set.insert(i);
//
//					if (last_i + 1000 < i) {
//						qInfo() << "i = " << i << " dist = " << dist;
//						last_i = i;
//					}
//				}
//			}
//
//		}
//
//		// Po zako�czeniu pracy w�tku, scal wyniki
//		std::lock_guard<std::mutex> lock(mtx);
//		vertices_to_remove.insert(local_set.begin(), local_set.end());
//
//	}
//}

void zamienPrzecieciaNaDziury2(CMesh* wierzch, CMesh* stempel, double shift, CVector3d mv, std::set<INDEX_TYPE>& vertices_to_remove)
{
	qInfo() << "=== ZAMIEN PRZECIECIA NA DZIURY ===";
	qInfo() << "Wierzch vertices:" << wierzch->vertices().size();
	qInfo() << "Stempel faces:" << stempel->faces().size();
	qInfo() << "Shift:" << shift << ", mv:" << mv.x << "," << mv.y << "," << mv.z;

	std::mutex mtx;

	qInfo() << "Budowanie KD-tree...";
	KDNode2* tree = KDNode2::build(stempel, 5000);
	qInfo() << "KD-tree zbudowane";

	UI::PROGRESSBAR::init(0, wierzch->vertices().size(), 0);
	UI::PROGRESSBAR::setText("Finding vertices to remove...");

	std::atomic<int> progress(0);

#pragma omp parallel
	{
		std::set<INDEX_TYPE> local_set;

#pragma omp for
		for (int i = 0; i < wierzch->vertices().size(); i++) {
			CVertex p0 = wierzch->vertices()[i];
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

				if (dist > abs(shift)) {
					local_set.insert(i);
				}
			}

			// Progress bar update (co 1000 wierzchołków)
			int local_progress = ++progress;
			if (local_progress % 1000 == 0) {
#pragma omp critical
				{
					UI::PROGRESSBAR::setValue(local_progress);
				}
			}
		}

		// Scal wyniki
		std::lock_guard<std::mutex> lock(mtx);
		vertices_to_remove.insert(local_set.begin(), local_set.end());
	}

	UI::PROGRESSBAR::hide();
	qInfo() << "Znaleziono wierzcholkow do usuniecia:" << vertices_to_remove.size();
	qInfo() << "=================================";
}

std::pair<std::shared_ptr<CMesh>, std::shared_ptr<CMesh>> ConcretePlugin::zrobDziury(std::shared_ptr<CMesh> wierzch, std::shared_ptr<CMesh> wnetrze)
{
	qInfo() << "=== ZROB DZIURY: START ===";
	qInfo() << "Wierzch: faces=" << wierzch->faces().size() << ", vertices=" << wierzch->vertices().size();
	qInfo() << "Wnetrze: faces=" << wnetrze->faces().size() << ", vertices=" << wnetrze->vertices().size();

	UI::PROGRESSBAR::init(0, 100, 0);
	UI::PROGRESSBAR::setText("Making holes:");

	if (wierzch && wnetrze)
	{
		qInfo() << "Kopiowanie wierzchu...";
		auto wierzch_nowy = std::dynamic_pointer_cast<CMesh>(wierzch->getCopy());
		wierzch_nowy->setParent(nullptr);
		wierzch_nowy->removeDuplicateVertices();

		UI::PROGRESSBAR::setValue(10);

		qInfo() << "Kopiowanie wnetrza...";
		auto wnetrze_nowe = std::dynamic_pointer_cast<CMesh>(wnetrze->getCopy());
		wnetrze_nowe->setParent(nullptr);
		wnetrze_nowe->removeDuplicateVertices();

		UI::PROGRESSBAR::setValue(20);

		qInfo() << "--- PRZECIECIA START ---";
		przeciecia(wierzch_nowy, wnetrze_nowe);
		qInfo() << "--- PRZECIECIA END ---";

		UI::PROGRESSBAR::init(0, 30, 0);
		UI::PROGRESSBAR::setText("Making holes:");

		std::set<INDEX_TYPE> vertices_to_remove;
		std::vector<INDEX_TYPE> faces_to_remove;

		qInfo() << "--- dziury w wierzchu...";
		zamienPrzecieciaNaDziury2(wierzch_nowy.get(), wnetrze.get(), 10.0, CVector3d(0.0, 0.0, -1.0), vertices_to_remove);

		UI::PROGRESSBAR::setValue(40);


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

		UI::PROGRESSBAR::setValue(50);

		qInfo() << QString("--- znaleziono: %1").arg(faces_to_remove.size());

		qInfo() << QString("--- lb. scianek przed usuwaniem: %1").arg(wierzch_nowy->faces().size());
		qInfo() << QString("--- lb. wierzcholkow przed usuwaniem: %1").arg(wierzch_nowy->vertices().size());

		// na wszelki wypadek, �eby miec pewno��, �e indeksy s� malej�co
		std::sort(faces_to_remove.begin(), faces_to_remove.end(), std::greater<INDEX_TYPE>());

		for (INDEX_TYPE j : faces_to_remove) {
			wierzch_nowy->removeFace(j);
		}

		UI::PROGRESSBAR::setValue(60);

		qInfo() << QString("--- lb. scianek po usuwaniu: %1").arg(wierzch_nowy->faces().size());


		wierzch_nowy->removeUnusedVertices();

		qInfo() << QString("--- lb. wierzcholkow po usuwaniu: %1").arg(wierzch_nowy->vertices().size());

		wierzch_nowy->setLabel("wierzch_nowy");
		//AP::WORKSPACE::addObject(wierzch_nowy);

		UI::PROGRESSBAR::setValue(70);


		vertices_to_remove.clear();
		faces_to_remove.clear();

		qInfo() << "--- dziury we wnetrzu...";
		zamienPrzecieciaNaDziury2(wnetrze_nowe.get(), wierzch.get(), -10.0, CVector3d(0.0, 0.0, 1.0), vertices_to_remove);


		UI::PROGRESSBAR::setValue(80);

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

		UI::PROGRESSBAR::setValue(85);

		qInfo() << QString("--- znaleziono: %1").arg(faces_to_remove.size());

		qInfo() << QString("--- lb. scianek przed usuwaniem: %1").arg(wnetrze_nowe->faces().size());
		qInfo() << QString("--- lb. wierzcholkow przed usuwaniem: %1").arg(wnetrze_nowe->vertices().size());

		// na wszelki wypadek, �eby miec pewno��, �e indeksy s� malej�co
		std::sort(faces_to_remove.begin(), faces_to_remove.end(), std::greater<INDEX_TYPE>());

		for (INDEX_TYPE j : faces_to_remove) {
			wnetrze_nowe->removeFace(j);
		}

		UI::PROGRESSBAR::setValue(90);

		qInfo() << QString("--- lb. scianek po usuwaniu: %1").arg(wnetrze_nowe->faces().size());

		faces_to_remove.clear();

		wnetrze_nowe->removeUnusedVertices();

		qInfo() << QString("--- lb. wierzcholkow po usuwaniu: %1").arg(wnetrze_nowe->vertices().size());

		wnetrze_nowe->setLabel("wnetrze_nowe");
		//AP::WORKSPACE::addObject(wnetrze_nowe);

		UI::PROGRESSBAR::hide();

		qInfo() << "=== ZROB DZIURY: END ===";
		return { wierzch_nowy, wnetrze_nowe };
	}

	UI::PROGRESSBAR::hide();

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

	UI::PROGRESSBAR::init(0, 100, 0);
	UI::PROGRESSBAR::setText("Bridging:");

	if (wierzch && wnetrze)
	{
		wierzch->removeDuplicateVertices();
		wnetrze->removeDuplicateVertices();

		UI::PROGRESSBAR::setValue(10);

		std::pair< std::vector<CVertex>, std::vector<CFace> > result;

		std::vector<CEdge> sz_ed;
		std::unordered_map<INDEX_TYPE, std::vector<INDEX_TYPE>> sz_bGraph;

		findBoundaryEdgesWithDirection(wierzch->faces(), sz_ed, sz_bGraph);

		UI::PROGRESSBAR::setValue(30);

		std::vector<std::vector<INDEX_TYPE>> sz_loops = findBoundaryLoops(sz_bGraph);

		UI::PROGRESSBAR::setValue(50);

		auto sz_ae = std::make_shared<CAnnotationEdges>();

		std::vector<INDEX_TYPE> IS_V;

		for (auto e : sz_ed) {
			IS_V.push_back(e.first);
			sz_ae->addEdge(wierzch->vertices()[e.first], wierzch->vertices()[e.second]);
		}


		std::vector<CEdge> ok_ed;
		std::unordered_map<INDEX_TYPE, std::vector<INDEX_TYPE>> ok_bGraph;

		UI::PROGRESSBAR::setValue(55);

		findBoundaryEdgesWithDirection(wnetrze->faces(), ok_ed, ok_bGraph);
		std::vector<std::vector<INDEX_TYPE>> ok_loops = findBoundaryLoops(ok_bGraph);

		UI::PROGRESSBAR::setValue(65);

		auto ok_ae = std::make_shared<CAnnotationEdges>();

		std::vector<INDEX_TYPE> IS_VZ;

		for (auto e : ok_ed) {
			IS_VZ.push_back(e.first);
			ok_ae->addEdge(wnetrze->vertices()[e.first], wnetrze->vertices()[e.second]);
		}

		std::vector<std::shared_ptr<CMesh>> siatki;

		UI::PROGRESSBAR::setValue(70);

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

		UI::PROGRESSBAR::setValue(90);

		auto mesh1 = scalSiatki(siatki);

		auto test = scalSiatki({ wierzch, wnetrze, mesh1 });

		UI::PROGRESSBAR::hide();

		return test;
	}

	UI::PROGRESSBAR::hide();

	return nullptr;
}


//----------------------------------------------------------------------------


#include <queue>

std::vector<INDEX_TYPE> findShortestCycleFrom_BAK(INDEX_TYPE start, const std::unordered_map<INDEX_TYPE, std::vector<INDEX_TYPE>>& graph)
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

std::vector<INDEX_TYPE> findShortestCycleFrom(INDEX_TYPE start, const std::unordered_map<INDEX_TYPE, std::vector<INDEX_TYPE>>& graph)
{
	std::queue<std::pair<INDEX_TYPE, INDEX_TYPE>> q; // (current, parent)
	std::unordered_map<INDEX_TYPE, INDEX_TYPE> parent;
	std::unordered_set<INDEX_TYPE> visited;

	q.push({ start, (INDEX_TYPE)-1 });
	visited.insert(start);

	while (!q.empty()) {
		auto [current, prev] = q.front();
		q.pop();

		auto it = graph.find(current);
		if (it == graph.end())
			continue;

		for (INDEX_TYPE neighbor : it->second) {
			// Znaleziono cykl
			if (neighbor == start && parent.size() >= 2) {
				std::vector<INDEX_TYPE> path;
				path.push_back(start);
				INDEX_TYPE node = current;
				while (node != start) {
					path.push_back(node);
					node = parent[node];
				}
				path.push_back(start);
				std::reverse(path.begin(), path.end());
				return path;
			}

			// Pomiń rodzica (unikamy natychmiastowego powrotu)
			if (neighbor == prev)
				continue;

			if (visited.find(neighbor) == visited.end()) {
				visited.insert(neighbor);
				parent[neighbor] = current;
				q.push({ neighbor, current });
			}
		}
	}

	return {}; // brak cyklu
}


std::vector<std::vector<INDEX_TYPE>> findBoundaryLoopsFromEdges_BAK(const std::vector<CEdge>& boundaryEdges)
{
	std::unordered_map<INDEX_TYPE, std::vector<INDEX_TYPE>> graph;
	std::unordered_set<INDEX_TYPE> vertices_remains;
	for (const CEdge& edge : boundaryEdges) {
		graph[edge.first].push_back(edge.second);

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


std::vector<std::vector<INDEX_TYPE>> findBoundaryLoopsFromEdges(const std::vector<CEdge>& boundaryEdges)
{
	std::unordered_map<INDEX_TYPE, std::vector<INDEX_TYPE>> graph;
	std::unordered_set<INDEX_TYPE> vertices_remains;

	// Budowa grafu z rezerwacją pamięci
	graph.reserve(boundaryEdges.size() * 2);
	vertices_remains.reserve(boundaryEdges.size() * 2);

	for (const CEdge& edge : boundaryEdges) {
		graph[edge.first].push_back(edge.second);
		vertices_remains.insert(edge.first);
		vertices_remains.insert(edge.second);
	}

	std::vector<std::vector<INDEX_TYPE>> loops;
	loops.reserve(vertices_remains.size() / 10); // estymacja liczby pętli

	while (!vertices_remains.empty()) {
		INDEX_TYPE current = *vertices_remains.begin();
		std::vector<INDEX_TYPE> path = findShortestCycleFrom(current, graph);

		if (path.empty()) {
			vertices_remains.erase(current);
		}
		else {
			loops.push_back(std::move(path));

			// Usuwanie wszystkich wierzchołków z pętli naraz
			for (INDEX_TYPE p : loops.back()) {
				vertices_remains.erase(p);
			}
		}
	}

	return loops;
}


#include "AnnotationEdges.h"
#include "../api/AP.h"

// Funkcja zamyka wszystkie wykryte pętle brzegowe tworząc wachlarze trójkątów wokół centroidu
std::pair<std::vector<CVertex>, std::vector<CFace>> FillBoundaryLoops(const std::vector<CVertex>& vertices, const std::vector<CFace>& faces)
{
	// PLEASE NOTE: INDEX_TYPE is currently defined as uint32_t

	UI::PROGRESSBAR::init(0,100,0);
	UI::PROGRESSBAR::setText("Filling:");


	std::vector<CEdge> boundaryEdges;
	std::unordered_map<INDEX_TYPE, std::vector<INDEX_TYPE>> boundaryGraph;

	qInfo() << "Znajdowanie krawedzi brzegowych...";

	findBoundaryEdgesWithDirection(faces, boundaryEdges, boundaryGraph);

	qInfo() << "Znaleziono krawedzi brzegowych:" << boundaryEdges.size();

	UI::PROGRESSBAR::setValue(10);


	std::vector<INDEX_TYPE> IS_VZ;

	for (auto e : boundaryEdges) {
		IS_VZ.push_back(e.first);
	}

	UI::PROGRESSBAR::setValue(15);


	auto loops = findBoundaryLoopsFromEdges(boundaryEdges);
	qDebug() << "Znaleziono pętli:" << loops.size();

	UI::PROGRESSBAR::setValue(25);

	std::vector<CVertex> outVertices;
	std::vector<CFace> outFaces;

	UI::PROGRESSBAR::init(0, 25+loops.size(), 25);

	int cnt = 0;
	for (const auto& loop : loops) {
		UI::PROGRESSBAR::setValue(25+cnt);

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

	UI::PROGRESSBAR::hide();

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
