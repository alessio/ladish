/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef OMPORT_H
#define OMPORT_H

#include <cassert>
#include <string>
#include <flowcanvas/Port.h>

namespace LibOmClient { class PortModel; }
using namespace LibOmClient;
using namespace LibFlowCanvas;
using std::string; using std::list;

namespace OmGtk {
	
class FlowCanvas;
class PatchController;
class PatchWindow;
class OmModule;


/** A Port on an OmModule.
 * 
 * \ingroup OmGtk
 */
class OmPort : public LibFlowCanvas::Port
{
public:
	OmPort(OmModule* module, PortModel* pm);

	virtual ~OmPort() {}

	//void set_name(const string& n);
	
	PortModel* model() const { return m_port_model; }
	
private:
	PortModel*          m_port_model;
};


} // namespace OmGtk

#endif // OMPORT_H
