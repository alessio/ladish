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

#ifndef MIDINOTENODE_H
#define MIDINOTENODE_H

#include <string>
#include "InternalNode.h"
#include "util/types.h"

using std::string;

namespace Om {

class MidiMessage;
template <typename T> class InputPort;
template <typename T> class OutputPort;


/** MIDI note input node.
 *
 * For pitched instruments like keyboard, etc.
 *
 * \ingroup engine
 */
class MidiNoteNode : public InternalNode
{
public:
	MidiNoteNode(const string& path, size_t poly, Patch* parent, samplerate srate, size_t buffer_size);
	~MidiNoteNode();

	void run(size_t nframes);
	
	void note_on(uchar note_num, uchar velocity, samplecount offset);
	void note_off(uchar note_num, samplecount offset);
	void all_notes_off(samplecount offset);

	void sustain_on();
	void sustain_off(samplecount offset);

private:
	
	/** Key, one for each key on the keyboard */
	struct Key {
		enum State { OFF, ON_ASSIGNED, ON_UNASSIGNED };
		Key() : state(OFF), voice(0), time(0) {}
		State state; size_t voice; samplecount time;
	};

	/** Voice, one of these always exists for each voice */
	struct Voice {
		enum State { FREE, ACTIVE, HOLDING };
		Voice() : state(FREE), note(0) {}
		State state; uchar note; samplecount time;
	};

	float note_to_freq(int num);
	void free_voice(size_t voice, samplecount offset);

	Voice* m_voices;
	Key    m_keys[128];
	bool   m_sustain;   ///< Whether or not hold pedal is depressed
	
	InputPort<MidiMessage>* m_midi_in_port;
	OutputPort<sample>*     m_freq_port;
	OutputPort<sample>*     m_vel_port;
	OutputPort<sample>*     m_gate_port;
	OutputPort<sample>*     m_trig_port;
};


} // namespace Om

#endif // MIDINOTENODE_H
