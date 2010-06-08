

// Copyright 2010 by Philipp Kraft
// This file is part of cmf.
//
//   cmf is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 2 of the License, or
//   (at your option) any later version.
//
//   cmf is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with cmf.  If not, see <http://www.gnu.org/licenses/>.
//   
#ifndef algorithm_h__
#define algorithm_h__
#include "cell.h"
#include "../math/num_array.h"
#include "../atmosphere/meteorology.h"

#include "../geometry/geometry.h"
namespace cmf {
	namespace upslope {
		typedef const cell_vector&  cells_ref;


		/// Finds the nearest cell for a position
		cmf::upslope::Cell* find_cell(cells_ref,cmf::geometry::point p,double max_dist=1e20);
		/// Gets the outer cells of a network. Unstable! Might create an infinite loop
		cmf::upslope::cell_vector get_boundary_cells(cmf::upslope::cells_ref cells);
		/// Gets all connections of a bunch of cells
		cmf::water::connection_set get_connections(cells_ref cells);
		/// Connects all adjacent cells with a flux connection, implementing the CellConnector protocol
		void connect_cells_with_flux(cells_ref cells, const cmf::upslope::CellConnector& connect,int start_at_layer=0);
		/// Fills the sinks in a network. Unstable! Might create an infinite loop
		int fill_sinks(cells_ref cells,double min_difference=0.001);
		/// Returns the total area of a cell collection
		double area(cells_ref cells);
		

		cmf::geometry::point_vector cell_positions(cmf::upslope::cells_ref cells);
		cmf::geometry::point_vector cell_flux_directions(cmf::upslope::cells_ref cells,cmf::math::Time);
		
		/// A class to structure cells in a project using their main outlets.
		class subcatchment
		{
		public:
			/// The outlet cell of the subcatchment
			const cmf::upslope::Cell& pourpoint;
			/// Cells which are pourpoints for higher subcatchment
			cmf::upslope::cell_vector inflowcells;
			/// The cells of the subcatchment
			cmf::upslope::cell_vector cells;

			/// Creates a subcatchment from a pourpoint cell
			/// @param pourpoint The pourpoint (outlet of the subcatchment)
			/// @param area_threshold Minimum contributing area size to form a subcatchment. 
			/// When area_threshold > area(cells), all upslope cells of pourpoint are used.
			subcatchment(cmf::upslope::Cell& pourpoint,double area_threshold=1e308);

		};


	}
	
}
#endif // algorithm_h__
