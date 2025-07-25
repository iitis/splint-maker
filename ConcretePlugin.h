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
	std::shared_ptr<CAnnotationPlane> m_plaszczyzna_rzutowania;

	int m_divider;
	CzekamNa waiting_for;
	std::shared_ptr<CMesh> szcz, oklu, zuch, calosc;
	std::shared_ptr<CMesh> zuch_inv;
	std::shared_ptr<CModel3D> szcz_parent;
	std::shared_ptr<CModel3D> top_model_arch;
	std::shared_ptr<CMesh> mesh_wierzch, mesh_wnetrze;

public:
    ConcretePlugin(void);
    ~ConcretePlugin(void) {};

	virtual void onLoad() override;
	virtual bool onModelIndication(int objId) override;

	std::shared_ptr<CMesh> liczOkluzje(std::shared_ptr<CMesh> szczeka, std::shared_ptr<CMesh> zuchwa, double dist);

	std::shared_ptr<CMesh> scalMeshe(std::shared_ptr<CMesh> wierzchZdeklem, std::shared_ptr<CMesh> wnetrze, bool invert = false);

	void wczytaj_spreparowany_ATMDL();

	void etap00(double dist2, bool dane_z_pomiaru);
	void etap01(int div);
	void etap11();
    void etap123(double dValIn, double dValOut);
    void wytlaczanie();
    void etap14();

	void save_all();

	void showMainPanel();


	std::shared_ptr<CMesh> stampFromMesh(std::shared_ptr<CMesh> mesh);

	void zrobOdciskStempla(std::shared_ptr<CMesh> wierzch, std::shared_ptr<CMesh> stempel, double _distMax=0.0);
	
	std::pair< std::shared_ptr<CMesh>, std::shared_ptr<CMesh>> zrobDziury(std::shared_ptr<CMesh> wierzch, std::shared_ptr<CMesh> wnetrze);

	std::shared_ptr<CMesh> bridging(std::shared_ptr<CMesh> sz, std::shared_ptr<CMesh> ok);

	std::shared_ptr<CMesh> filling(std::shared_ptr<CMesh> test);

signals:
	void setProgressBarValue(int);
};

