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

#ifndef GTKOBJECTCONTROLLER_H
#define GTKOBJECTCONTROLLER_H

#include <string>
#include <gtkmm.h>

#include "ObjectModel.h"
#include "ObjectController.h"


using std::string;

using namespace LibOmClient;

namespace OmGtk {

class Controller;


/** Controller for an Om object.
 *
 * Management of the model and view are this object's responsibility.
 * The view may not be created (ie in the case of patches which have never
 * been shown and all their children).
 *
 * \ingroup OmGtk
 */
class GtkObjectController : public LibOmClient::ObjectController
{
public:
	GtkObjectController(ObjectModel* model);
	virtual ~GtkObjectController() {}

	/** Destroy object.
	 *
	 * Object must be safe to delete immediately following the return of
	 * this call.
	 */
	virtual void destroy() = 0;

	virtual void add_to_store() = 0;
	virtual void remove_from_store() = 0;

	virtual void metadata_update(const string& key, const string& value);

	/** Rename object */
	virtual void set_path(const Path& new_path)
	{ m_model->set_path(new_path); }
	
	const Path& path() const { return m_model->path(); }

	ObjectModel* model() const { return m_model; }

protected:
	ObjectModel* m_model;      ///< Model for this object
};


} // namespace OmGtk

#endif // GTKOBJECTCONTROLLER_H
