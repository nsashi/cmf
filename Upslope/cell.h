#ifndef cell_h__
#define cell_h__
#include "../Atmosphere/Meteorology.h"
#include "../Geometry/geometry.h"
#include "../water/flux_connection.h"
#include "vegetation/StructVegetation.h"
#include "../water/WaterStorage.h"
#include "../math/StateVariable.h"
#include "Soil/RetentionCurve.h"
#include <memory>
#include <map>
#include <vector>
#include <set>
namespace cmf {
	class project;
	namespace river {
		class OpenWaterStorage;	
		class Reach;
	}
	namespace atmosphere {
		class RainCloud;

	}
    /// Contains the classes to describe the discretization of the soil continuum
	namespace upslope {
		class SoilLayer;
		class Topology;
		class Cell;
		typedef void (*connectorfunction)(cmf::upslope::Cell&,cmf::upslope::Cell&,int);
		typedef void (*internal_connector)(cmf::upslope::Cell&);
		typedef std::tr1::shared_ptr<SoilLayer> layer_ptr;
		/// A helper class to connect cells with flux_connection objects. This is generated by flux_connection classes, intended to connect cells 
		class CellConnector
		{
		private:
			connectorfunction m_connector;
			CellConnector():m_connector(0) {}
		public:
#ifndef SWIG
			CellConnector(connectorfunction connector) : m_connector(connector)			{	}
			void operator()(cmf::upslope::Cell& cell1,cmf::upslope::Cell& cell2,int start_at_layer=0) const	{
				connect(cell1,cell2,start_at_layer);
			}
#endif
			void connect(cmf::upslope::Cell& cell1,cmf::upslope::Cell& cell2,int start_at_layer=0) const {
				m_connector(cell1,cell2,start_at_layer);
			}
		};
		/// This class is the basic landscape object. It is the owner of water storages, and the upper and lower boundary conditions 
		/// of the system (rainfall, atmospheric vapor, deep groundwater)
		class Cell : public cmf::math::StateVariableOwner, public cmf::geometry::Locatable {
			Cell(const Cell& cpy);
			friend class project;
			/// @name Location
			//@{
			std::auto_ptr<Topology> m_topo;
			static int cell_count;
		public:
			cmf::upslope::Topology& get_topology();
#ifndef SWIG
			operator cmf::upslope::Topology&() {return get_topology();}
#endif
			double x,y,z;
			/// Returns the location of the cell
			cmf::geometry::point get_position() const { return cmf::geometry::point(x,y,z); }
		private:
			double m_Area;
			real m_SatDepth;
		public:
			/// Returns the area of the cell
			double get_area() const { return m_Area; }
			/// @name Saturation
			//@{
			/// Marks the saturated depth as unvalid
			void InvalidateSatDepth() {m_SatDepth=-1e20;}
			/// 
			real get_saturated_depth() ;
			void set_saturated_depth(real depth);
			//@}
		private:
			//@}
			/// @name Flux nodes of the cell
			//@{
			typedef std::vector<cmf::water::WaterStorage::ptr> storage_vector;
			typedef std::auto_ptr<cmf::atmosphere::Meteorology> meteo_pointer;
			storage_vector m_storages;
			cmf::water::WaterStorage::ptr
				m_Canopy,
				m_Snow,
				m_SurfaceWaterStorage;
			cmf::water::flux_node::ptr m_SurfaceWater;
			std::tr1::shared_ptr<cmf::atmosphere::RainCloud> m_rainfall;
			cmf::water::flux_node::ptr m_Evaporation, m_Transpiration;

			const cmf::project & m_project;
			meteo_pointer m_meteo;
			
			cmf::upslope::vegetation::Vegetation m_vegetation;
		public:
			cmf::atmosphere::Meteorology& get_meteorology() const 
			{
				return *m_meteo;
			}
			void set_meteorology(const cmf::atmosphere::Meteorology& new_meteo)
			{
				m_meteo.reset(new_meteo.copy());
			}
			std::tr1::shared_ptr<cmf::atmosphere::RainCloud> get_rainfall() const {return m_rainfall;}
			/// Returns the end point of all evaporation of this cell
 			cmf::water::flux_node::ptr get_evaporation();
 			/// Returns the end point of all transpiration of this cell
 			cmf::water::flux_node::ptr get_transpiration();
			/// returns the surface water of this cell
			cmf::water::flux_node::ptr get_surfacewater();
			void surfacewater_as_storage();
			cmf::water::WaterStorage::ptr add_storage(std::string Name,char storage_role='N',  bool isopenwater=false);
			void remove_storage(cmf::water::WaterStorage& storage);
			int storage_count() const
			{
				return int(m_storages.size());
			}
			cmf::water::WaterStorage::ptr get_storage(int index) const;
			cmf::water::WaterStorage::ptr get_canopy() const;
			cmf::water::WaterStorage::ptr get_snow() const;
			real snow_coverage() const
			{
				if (m_Snow)
					return piecewise_linear(m_Snow->get_state()/get_area(),0,0.01);
				else
					return 0.0;
			}
			bool has_wet_leaves() const
			{
				return (m_Canopy) && (m_Canopy->get_state()>1e-6*get_area());
			}
			bool has_surface_water() const
			{
				return (m_SurfaceWaterStorage) && (m_SurfaceWaterStorage->get_state()>1e-6*get_area());
			}
			cmf::upslope::vegetation::Vegetation get_vegetation() const;
			void set_vegetation(cmf::upslope::vegetation::Vegetation val) { m_vegetation = val; }

			int Id;
			const cmf::project& project() const;
			cmf::atmosphere::Weather get_weather(cmf::math::Time t) const;

		public:
			//@}
			///@name Layers
			//@{
		private:
			typedef std::vector<layer_ptr> layer_vector;
			layer_vector m_Layers;
		public:
			int layer_count() const
			{
				return int(m_Layers.size());
			}
			cmf::upslope::layer_ptr get_layer(int ndx) const;
			void add_layer(real lowerboundary,const cmf::upslope::RetentionCurve& r_curve,real saturateddepth=10);
			void remove_last_layer();
			void remove_layers();
			virtual ~Cell();
			//@}


			Cell(double x,double y,double z,double area,cmf::project & _project);
			std::string to_string();
			//@}
			void AddStateVariables(cmf::math::StateVariableVector& vector);
		};
		
		typedef std::vector<cmf::upslope::Cell*> cell_vector;
		typedef std::set<cmf::upslope::Cell*> cell_set;



	}
	
}
#endif // cell_h__
