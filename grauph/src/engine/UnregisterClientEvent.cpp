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

#include "UnregisterClientEvent.h"
#include "Responder.h"
#include "Om.h"
#include "OmApp.h"
#include "ClientBroadcaster.h"
#include "interface/ClientInterface.h"

namespace Om {


UnregisterClientEvent::UnregisterClientEvent(CountedPtr<Responder> responder, ClientKey key)
: QueuedEvent(responder)
, _key(key)
{
}


void
UnregisterClientEvent::post_process()
{
	if (om->client_broadcaster()->unregister_client(_key))
		m_responder->respond_ok();
	else
		m_responder->respond_error("Unable to unregister client");
}


} // namespace Om

