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

#include "ConnectionEvent.h"
#include <string>
#include "Responder.h"
#include "Om.h"
#include "OmApp.h"
#include "ConnectionBase.h"
#include "InputPort.h"
#include "OutputPort.h"
#include "Patch.h"
#include "ClientBroadcaster.h"
#include "Port.h"
#include "PortInfo.h"
#include "Maid.h"
#include "ObjectStore.h"
#include "util/Path.h"

using std::string;
namespace Om {


//// ConnectionEvent ////


ConnectionEvent::ConnectionEvent(CountedPtr<Responder> responder, const string& src_port_path, const string& dst_port_path)
: QueuedEvent(responder),
  m_src_port_path(src_port_path),
  m_dst_port_path(dst_port_path),
  m_patch(NULL),
  m_src_port(NULL),
  m_dst_port(NULL),
  m_typed_event(NULL),
  m_error(NO_ERROR)
{
}


ConnectionEvent::~ConnectionEvent()
{
	delete m_typed_event;
}


void
ConnectionEvent::pre_process()
{
	if (m_src_port_path.parent().parent() != m_dst_port_path.parent().parent()) {
		m_error = PARENT_PATCH_DIFFERENT;
		QueuedEvent::pre_process();
		return;
	}
	
	/*m_patch = om->object_store()->find_patch(m_src_port_path.parent().parent());

	if (m_patch == NULL) {
		m_error = PORT_NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}*/
	
	Port* port1 = om->object_store()->find_port(m_src_port_path);
	Port* port2 = om->object_store()->find_port(m_dst_port_path);
	
	if (port1 == NULL || port2 == NULL) {
		m_error = PORT_NOT_FOUND;
		QueuedEvent::pre_process();
		return;
	}

	if (port1->port_info()->type() != port2->port_info()->type()) {
		m_error = TYPE_MISMATCH;
		QueuedEvent::pre_process();
		return;
	}
	
	if (port1->port_info()->is_output() && port2->port_info()->is_input()) {
		m_src_port = port1;
		m_dst_port = port2;
	} else if (port2->port_info()->is_output() && port1->port_info()->is_input()) {
		m_src_port = port2;
		m_dst_port = port1;
	} else {
		m_error = TYPE_MISMATCH;
		QueuedEvent::pre_process();
		return;
	}
	
	// Create the typed event to actually do the work
	const PortType type = port1->port_info()->type();
	if (type == AUDIO || type == CONTROL) {
		m_typed_event = new TypedConnectionEvent<sample>(m_responder,
			(OutputPort<sample>*)m_src_port, (InputPort<sample>*)m_dst_port);
	} else if (type == MIDI) {
		m_typed_event = new TypedConnectionEvent<MidiMessage>(m_responder,
			(OutputPort<MidiMessage>*)m_src_port, (InputPort<MidiMessage>*)m_dst_port);
	} else {
		m_error = TYPE_MISMATCH;
		QueuedEvent::pre_process();
		return;
	}

	m_typed_event->pre_process();
	
	QueuedEvent::pre_process();
}


void
ConnectionEvent::execute(samplecount offset)
{
	QueuedEvent::execute(offset);

	if (m_error == NO_ERROR)
		m_typed_event->execute(offset);
}


void
ConnectionEvent::post_process()
{
	if (m_error == NO_ERROR) {
		m_typed_event->post_process();
	} else {
		// FIXME: better error messages
		string msg = "Unable to make connection ";
		msg.append(m_src_port_path + " -> " + m_dst_port_path);
		m_responder->respond_error(msg);
	}
}



//// TypedConnectionEvent ////


template <typename T>
TypedConnectionEvent<T>::TypedConnectionEvent(CountedPtr<Responder> responder, OutputPort<T>* src_port, InputPort<T>* dst_port)
: QueuedEvent(responder),
  m_src_port(src_port),
  m_dst_port(dst_port),
  m_patch(NULL),
  m_process_order(NULL),
  m_connection(NULL),
  m_port_listnode(NULL),
  m_succeeded(true)
{
	assert(src_port != NULL);
	assert(dst_port != NULL);
}

template <typename T>
TypedConnectionEvent<T>::~TypedConnectionEvent()
{
	// FIXME: haaaack, prevent a double delete
	// this class is unusable by anything other than ConnectionEvent because of this
	//m_responder = NULL;
}


template <typename T>
void
TypedConnectionEvent<T>::pre_process()
{
	Node* const src_node = m_src_port->parent_node();
	Node* const dst_node = m_dst_port->parent_node();
	
	m_patch = src_node->parent_patch();

	if (src_node == NULL || dst_node == NULL) {
		m_succeeded = false;
		QueuedEvent::pre_process();
		return;
	}
	
	if (src_node->parent() != m_patch || dst_node->parent() != m_patch) {
		m_succeeded = false;
		QueuedEvent::pre_process();
		return;
	}

	m_connection = new ConnectionBase<T>(m_src_port, m_dst_port);
	m_port_listnode = new ListNode<ConnectionBase<T>*>(m_connection);
	m_patch_listnode = new ListNode<Connection*>(m_connection);
	
	dst_node->providers()->push_back(new ListNode<Node*>(src_node));
	src_node->dependants()->push_back(new ListNode<Node*>(dst_node));

	if (m_patch->process())
		m_process_order = m_patch->build_process_order();
}


template <typename T>
void
TypedConnectionEvent<T>::execute(samplecount offset)
{
	if (m_succeeded) {
		// These must be inserted here, since they're actually used by the audio thread
		m_dst_port->add_connection(m_port_listnode);
		m_patch->add_connection(m_patch_listnode);
		if (m_patch->process_order() != NULL)
			om->maid()->push(m_patch->process_order());
		m_patch->process_order(m_process_order);
	}
}


template <typename T>
void
TypedConnectionEvent<T>::post_process()
{
	if (m_succeeded) {
		assert(m_connection != NULL);
	
		m_responder->respond_ok();
	
		om->client_broadcaster()->send_connection(m_connection);
	} else {
		m_responder->respond_error("Unable to make connection.");
	}
}


} // namespace Om

