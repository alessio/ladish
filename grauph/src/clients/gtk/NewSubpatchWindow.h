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


#ifndef NEWSUBPATCHWINDOW_H
#define NEWSUBPATCHWINDOW_H

#include "PluginModel.h"
#include <libglademm/xml.h>
#include <gtkmm.h>


namespace OmGtk {
	
class PatchController;


/** 'New Subpatch' window.
 *
 * Loaded by glade as a derived object.
 *
 * \ingroup OmGtk
 */
class NewSubpatchWindow : public Gtk::Window
{
public:
	NewSubpatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);

	void patch_controller(PatchController* pc);

	void set_next_module_location(int x, int y)
		{ m_new_module_x = x; m_new_module_y = y; }
	
private:
	void name_changed();
	void ok_clicked();
	void cancel_clicked();

	PatchController* m_patch_controller;
	
	int m_new_module_x;
	int m_new_module_y;
	
	Gtk::Entry*      m_name_entry;
	Gtk::Label*      m_message_label;
	Gtk::SpinButton* m_poly_spinbutton;
	Gtk::Button*     m_ok_button;
	Gtk::Button*     m_cancel_button;
};
 

} // namespace OmGtk

#endif // NEWSUBPATCHWINDOW_H
