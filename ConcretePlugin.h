#pragma once

#include <set>
#include <list>

#include "Vector3.h"
#include "Point3.h"
#include "NewEdge.h"

class CAnnotationPlane;
class CAnnotationPoint;
class BiteSim;
class CModel3D;
class CMesh;
class CAnnotationEdges;
class CAnnotationPath;
class CAnnotationVPath;
class CPlane;
#include "dll_global.h"
#include "PluginInterface.h"

#include <QtWidgets>

#include "VoxelGrid.h"


#include "moje_widgety.h"

typedef enum { Nic, Sczeke, Zuchwe, Okluzje } CzekamNa;

struct PairHash {
	std::size_t operator()(const std::pair<unsigned int, unsigned int>& p) const {
		return std::hash<unsigned int>()(p.first) ^ (std::hash<unsigned int>()(p.second) << 1);
	}
};


class DPVISION_DLL_API ConcretePlugin : public QObject, public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "dpVision.PluginInterface" FILE "metadata.json")
	Q_INTERFACES(PluginInterface)

	QWidget *panel, *subpanel;
	QFormLayout *moj_layout;

	MojWidget* moj_widget;

	bool m_picking;
	std::shared_ptr<BiteSim> symulator;

	std::shared_ptr<CAnnotationPlane> m_cutPlane;
	std::shared_ptr<CAnnotationPlane> m_plaszczyzna_rzutowania; // , * m_rzutnia_copy;

	std::shared_ptr<CAnnotationPlane> m_c1, m_c2;

	std::shared_ptr<CAnnotationEdges> m_pts1, m_pts2;

	std::wstring m_workdir;

	//std::wstring szFileName, okFileName;

	int m_divider;
	CzekamNa waiting_for;
	std::shared_ptr<CMesh> szcz, oklu, zuch, calosc;
	std::shared_ptr<CMesh> zuch_inv;
	std::shared_ptr<CModel3D> szcz_parent;
	std::shared_ptr<CModel3D> top_model_arch;
	std::shared_ptr<CMesh> mesh_wierzch, mesh_wnetrze;

	VoxelGrid vox_szyna, vox_okluzja, vox_wynik, vox_stempel;

public:
    ConcretePlugin(void);
    ~ConcretePlugin(void);

	std::shared_ptr<CMesh> liczOkluzje(std::shared_ptr<CMesh> szczeka, std::shared_ptr<CMesh> zuchwa, double dist);

	void run(void);

	void pickSlot(int objId, CAnnotationPoint& pt) {};

	void znajdzWierzcholkiBrzegowe(std::shared_ptr<CMesh> mesh, std::set<unsigned int>& boundaryVertices);

	std::shared_ptr<CMesh> zrzutujNaPlaszczyzne(std::shared_ptr<CMesh> mesh, CPlane &cutPlane, CVector3d ray);

	std::shared_ptr<CModel3D> dodajMeshDoProjektu(std::shared_ptr<CMesh> mesh, QString label);

	std::shared_ptr<CMesh> scalMeshe(std::shared_ptr<CMesh> wierzchZdeklem, std::shared_ptr<CMesh> wnetrze, bool invert = false);

	void wczytaj_spreparowany_ATMDL();

	void etap00(double dist2, bool dane_z_pomiaru);
	void etap01(int div);
	void etap11();
    void etap123(double dValIn, double dValOut);
    void wytlaczanie();
    // void etap12(double dVal);
    // void etap13_v2(double dVal);
    void etap14();

	void go_to_exchange();

	// void etap_dekiel_cien();

	void save_all();

	void go_to_multisaver();

	void etap_zapisz_wynik();

	void showMainPanel();

	void onLoad();
	void onUnload();
	int usunWadliweScianki(std::shared_ptr<CMesh> mesh);
	void wytnijDekiel2(std::shared_ptr<CMesh> rzutWierzchu, std::shared_ptr<CMesh> rzutWnetrza);
	std::shared_ptr<CMesh> dekiel_cien(std::shared_ptr<CMesh> wierzch, std::shared_ptr<CMesh> wnetrze, CVector3d ray);
	void createE2Fmap(std::shared_ptr<CMesh> mesh, MapOfNewEdges& allEdges);
	virtual bool onModelIndication(int objId) override;

	std::shared_ptr<CMesh> stampFromMesh(std::shared_ptr<CMesh> mesh);

	void onStempelButton();


	void zrobOdciskStempla(std::shared_ptr<CMesh> wierzch, std::shared_ptr<CMesh> stempel, double _distMax);
	void onOdciskStemplaButton(double distMax = 0.0);


	void onZrobDziuryButton();

	std::pair< std::shared_ptr<CMesh>, std::shared_ptr<CMesh>> zrobDziury(std::shared_ptr<CMesh> wierzch, std::shared_ptr<CMesh> wnetrze);

	std::shared_ptr<CMesh> bridging(std::shared_ptr<CMesh> sz, std::shared_ptr<CMesh> ok);

	std::shared_ptr<CMesh> filling(std::shared_ptr<CMesh> test);

signals:
	void setProgressBarValue(int);
};
