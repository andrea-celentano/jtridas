#include "GroupedEventProcessor.h"
#include "DAQ/faWaveboardHit.h"
#include "Trigger/TriggerDecision.h"
#include <vector>

void GroupedEventProcessor::Process(const std::shared_ptr<const JEvent>& event) {


	auto tridas_event = event->GetSingle<TridasEvent>();
	auto group = event->GetSingle<JEventGroup>();

	std::vector <const TriggerDecision*> triggers;
	event->GetAll(triggers);

	// Here we communicate back to TriDAS the results of the trigger
	// decisions via the TridasEvent object. The should_keep member
	// is set to true if any decision object indicates it is positive.
	// We also communicate all individual trigger decisions via the 
	// pluin_nseeds_[] public array member (see e-mail from Carmello
	// on 8/27/2020). n.b. This will write to the triggerWords member of
	// the TridasEvent object. Class defined in:
	// streamingReco/src/libraries/DAQ/TridasEvent.h
	//
	// The values are then copied from that to the TriDAS TriggeredEvent
	// object (class defined in tridas-core/DAQ/TCPU/inc/EventClasses.h).
	// The actual copy is done in jtridas/TrigJANA.cpp.

	for (auto trigger : triggers){
		auto trigger_word = (trigger->GetID())<<16;
		if (trigger->GetDecision() == true){
			tridas_event->should_keep = true;
			trigger_word |= 0x01;
		}
		tridas_event->triggerWords.push_back( trigger_word ); // communicate trigger ID and decision to TriDAS
	}


	// Sequentially, process each event and report when a group finishes
	std::lock_guard < std::mutex > lock(m_mutex);

	//LOG << "Processed group #" << group->GetGroupId() << ", event #" << event->GetEventNumber() << LOG_END;

	bool finishes_group = group->FinishEvent();
	if (finishes_group) {
		LOG << "Processed last element in group " << group->GetGroupId() << LOG_END;
	}
}

