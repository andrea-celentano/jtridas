#include "GroupedEventProcessor.h"
#include "DAQ/faWaveboardHit.h"

#include <vector>

void GroupedEventProcessor::Process(const std::shared_ptr<const JEvent>& event) {


	auto tridas_event = event->GetSingle<TridasEvent>();
	tridas_event->should_keep = true;

	auto group = event->GetSingle<JEventGroup>();

	std::vector <const faWaveboardHit*> hits;
	event->Get(hits);


	for (auto hit : hits){
		std::cout<<1.*hit->m_channel.crate<<" "<<1.*hit->m_channel.slot<<" "<<1.*hit->m_channel.channel<<" "<<1.*hit->charge<<std::endl;
	}


	// Sequentially, process each event and report when a group finishes
	std::lock_guard < std::mutex > lock(m_mutex);

	//LOG << "Processed group #" << group->GetGroupId() << ", event #" << event->GetEventNumber() << LOG_END;

	bool finishes_group = group->FinishEvent();
	if (finishes_group) {
		LOG << "Processed last element in group " << group->GetGroupId() << LOG_END;
	}
}

