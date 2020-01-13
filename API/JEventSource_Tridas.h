//
// Created by Nathan Brei on 2019-12-15.
//

#ifndef JANA2_TRIDASEVENTSOURCE_H
#define JANA2_TRIDASEVENTSOURCE_H

#include <JANA/JEventSource.h>
#include <JANA/Services/JEventGroupTracker.h>
#include <JANA/JEvent.h>

#include "DAQ/TridasEvent.h"


#include <queue>

class TridasEventSource : public JEventSource {

    JEventGroupManager m_egm;

    int m_pending_group_id;
    std::mutex m_pending_mutex;
    std::queue<std::pair<TridasEvent*, JEventGroup*>> m_pending_events;

public:

    TridasEventSource(std::string res_name, JApplication* app);


    /// SubmitAndWait provides a blocking interface for pushing groups of TridasEvents into JANA.
    /// JANA does NOT assume ownership of the events vector, nor does it clear it.
    void SubmitAndWait(std::vector<TridasEvent*>& events);


    /// GetEvent polls the queue of submitted TridasEvents and feeds them into JEvents along with a
    /// JEventGroup. A downstream EventProcessor may report the event as being finished. Once all
    /// events in the eventgroup are finished, the corresponding call to SubmitAndWait will unblock.
    void GetEvent(std::shared_ptr<JEvent> event) override;


    // This method disentagles the faWaveboardHit objects
    //bool GetObjects(std::shared_ptr<JEvent>& aEvent,JFactory* aFactory);
};


#endif //JANA2_TRIDASEVENTSOURCE_H
