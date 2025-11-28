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


/**
 * @brief ConcretePlugin implements the main logic for the splint maker plugin.
 * 
 * This class provides mesh processing, bridging, filling, and other operations for dental splint design.
 */
class DPVISION_DLL_API ConcretePlugin : public QObject, public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "dpVision.PluginInterface" FILE "metadata.json")
	Q_INTERFACES(PluginInterface)

	QWidget *panel, *subpanel;
	QFormLayout *moj_layout;

	bool testyAG = true;

	MojWidget* moj_widget;

	bool m_picking;
	std::shared_ptr<BiteSim> symulator;

	std::shared_ptr<CAnnotationPlane> m_cutPlane;
	std::shared_ptr<CAnnotationPlane> m_projectionPlane;

	int m_divider;
	CzekamNa waiting_for;
	std::shared_ptr<CMesh> szcz, oklu, zuch, calosc;
	std::shared_ptr<CMesh> zuch_inv;
	std::shared_ptr<CModel3D> szcz_parent;
	std::shared_ptr<CModel3D> top_model_arch;
	std::shared_ptr<CMesh> mesh_wierzch, mesh_wnetrze;

public:
    /**
     * @brief Constructor.
     */
    ConcretePlugin(void);
    ~ConcretePlugin(void) {};

	/**
	 * @brief Called when the plugin is loaded.
	 */
	virtual void onLoad() override;

	/**
	 * @brief Called when a model is indicated/selected.
	 * @param objId ID of the indicated object.
	 * @return true if handled, false otherwise.
	 */
	virtual bool onModelIndication(int objId) override;

	/**
	 * @brief Calculates occlusion between two meshes.
	 * @param szczeka Upper jaw mesh.
	 * @param zuchwa Lower jaw mesh.
	 * @param dist Distance threshold.
	 * @return Resulting mesh.
	 */
	std::shared_ptr<CMesh> liczOkluzje(std::shared_ptr<CMesh> szczeka, std::shared_ptr<CMesh> zuchwa, double dist);

	/**
	 * @brief Merges two meshes, optionally inverting one.
	 * @param wierzchZdeklem Outer mesh.
	 * @param wnetrze Inner mesh.
	 * @param invert Whether to invert the inner mesh.
	 * @return Merged mesh.
	 */
	std::shared_ptr<CMesh> scalMeshe(std::shared_ptr<CMesh> wierzchZdeklem, std::shared_ptr<CMesh> wnetrze, bool invert = false);

	void wczytaj_spreparowany_ATMDL();


	void etap00(double dist2, bool dane_z_pomiaru);
	void etap00ag(double dist2, bool dane_z_pomiaru);

	void etap01(int div);
	void etap11();
	void etap123(double dValIn, double dValOut);
    void reset_plugin();
    void wytlaczanie();
    void etap14();

	void etap14_nowy();

	void save_all();

	void showMainPanel();

	/**
	 * @brief Creates a stamp mesh from the given mesh.
	 * @param mesh Input mesh.
	 * @return Stamp mesh.
	 */
	std::shared_ptr<CMesh> stampFromMesh(std::shared_ptr<CMesh> mesh);

	/**
	 * @brief Creates an impression of a stamp on a mesh.
	 * @param wierzch Outer mesh.
	 * @param stempel Stamp mesh.
	 * @param _distMax Maximum distance for stamping.
	 */
	void zrobOdciskStempla(std::shared_ptr<CMesh> wierzch, std::shared_ptr<CMesh> stempel, double _distMax=0.0);
	
	/**
	 * @brief Creates holes in the meshes.
	 * @param wierzch Outer mesh.
	 * @param wnetrze Inner mesh.
	 * @return Pair of meshes with holes.
	 */
	std::pair< std::shared_ptr<CMesh>, std::shared_ptr<CMesh>> zrobDziury(std::shared_ptr<CMesh> wierzch, std::shared_ptr<CMesh> wnetrze);
	void zamienPrzecieciaNaDziury2(CMesh* wierzch, CMesh* stempel, double shift, CVector3d mv, std::set<INDEX_TYPE>& vertices_to_remove);

	/**
	 * @brief Bridges two meshes by connecting their boundaries.
	 * @param sz First mesh.
	 * @param ok Second mesh.
	 * @return Bridged mesh.
	 */
	std::shared_ptr<CMesh> bridging(std::shared_ptr<CMesh> sz, std::shared_ptr<CMesh> ok);

	/**
	 * @brief Fills all boundary loops of a mesh.
	 * @param test Input mesh.
	 * @return Mesh with filled boundaries.
	 */
	std::shared_ptr<CMesh> filling(std::shared_ptr<CMesh> test);

signals:
	/// Signal to update the progress bar value.
	void setProgressBarValue(int);
};
