#include "GroupedEventProcessor.h"
#include "DAQ/faWaveboardHit.h"
#include "Trigger/TriggerDecision.h"
#include <vector>

void GroupedEventProcessor::Process(const std::shared_ptr<const JEvent>& event) {


	auto tridas_event = event->GetSingle<TridasEvent>();
	auto group = event->GetSingle<JEventGroup>();

	std::vector <const TriggerDecision*> triggers;
	event->GetAll(triggers);

	std::vector <const faWaveboardHit*> hits;
	event->Get(hits);

	std::cout<<"EVENT"<<std::endl;fflush(stdout);


	for (auto trigger : triggers){
		std::cout<<trigger->GetDescription()<<std::endl;
		if (trigger->GetDecision() == true) 	tridas_event->should_keep = true;
	}

	std::cout<<"END-EVENT"<<std::endl;fflush(stdout);

	// Sequentially, process each event and report when a group finishes
	std::lock_guard < std::mutex > lock(m_mutex);

	//LOG << "Processed group #" << group->GetGroupId() << ", event #" << event->GetEventNumber() << LOG_END;

	bool finishes_group = group->FinishEvent();
	if (finishes_group) {
		LOG << "Processed last element in group " << group->GetGroupId() << LOG_END;
	}
}

