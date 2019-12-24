#include "TrigJANA.h"

#include "Event_Classes.h"
#include "log.hpp"
#include "Configuration.h"
#include "PluginParameters.h"

//JANA includes
#include "JANA/JApplication.h"
#include "API/JEventSource_Tridas.h"
#include "API/GroupedEventProcessor.h"

#include "DAQ/TridasEvent.h"
#include "addRecoFactoriesGenerators.h"

namespace tridas {
namespace tcpu {

void JANAthreadFun(JApplication *app) {
	std::cout << "TrigJANA JANAthreadFun start" << std::endl;
	app->Run();
	std::cout << "TrigJANA JANAthreadFun end" << std::endl;
}

void TrigJANA(PluginArgs const& args) {
	int const id = args.id;
	unsigned plug_events = 0;
	//std::cout << args.params->ordered_begin()->first << std::endl;
	//std::cout << args.params->ordered_begin()->second.get_value<std::string>() << std::endl;

	//std::string const config_file_name = args.params->get < std::string > ("CONFIG_FILE");
	//assert(scale_factor.size() > 0 && "CONFIG_FILE must be present");

	//need to pass the configuration file
	static thread_local int isFirst = 1;
	static thread_local JApplication * app = new JApplication();
	static thread_local TridasEventSource* evt_src = new TridasEventSource("blocking_source", app);

	if (isFirst) {
		std::cout << "TrigJANA isFirst being called" << std::endl;
		isFirst = 0;
		std::cout << "Adding the event source to the Japplication" << std::endl;
		app->Add(evt_src);
		std::cout << "DONE" << std::endl;

		std::cout << "Adding the factories Generators"<<std::endl;
		addRecoFactoriesGenerators(app);
		std::cout << " Done "<<std::endl;


		std::cout << "Adding the event processor the Japplication" << std::endl;
		app->Add(new GroupedEventProcessor());
		std::cout << "DONE" << std::endl;

		//Launch the thread with the JApplication and detach from it.
		std::cout << "TrigJANA starting the thread" << std::endl;
		boost::thread janaThread(JANAthreadFun, app);
		janaThread.detach();
		std::cout << "DONE and DETACHED" << std::endl;
		while (!app->IsInitialized()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	/*We now inject the trig_events to the source*/
	EventCollector& evc = *args.evc;;
	std::vector<TridasEvent*> event_batch;
	for (int i = 0; i < evc.used_trig_events(); ++i) {
		TriggeredEvent& tev = *(evc.trig_event(i));
		auto event = new TridasEvent;
		event->event_number = i;
		event->run_number = 22;
		event->should_keep = false;

		//Loop on PMT hits
		PMTHit* first_hit = reinterpret_cast<PMTHit*>(tev.sw_hit_);  //The first hit in the event
		PMTHit* last_hit = reinterpret_cast<PMTHit*>(tev.ew_hit_);   //The last hit in the event
		PMTHit* current_hit=first_hit;
		do { //loop on all the hits from this event
			fadcHit hit;

			size_t absID = current_hit->getAbsPMTID();

			//Determine crate, slot, channel
			auto geo = *(args.geom);
			hit.crate = absID / (geo.pmts_per_floor * geo.floors_per_tower);
			hit.slot = (absID - hit.crate * (geo.pmts_per_floor * geo.floors_per_tower)) / geo.pmts_per_floor + geo.floor_offset;
			hit.channel = (absID - hit.crate * (geo.pmts_per_floor * geo.floors_per_tower)) % geo.pmts_per_floor;

			//Time
			hit.time = current_hit->get_fine_time();

			//charge
			hit.charge = current_hit->getCaliCharge();

			event->hits.push_back(hit);

			current_hit = current_hit->next();
		} while (current_hit && current_hit != last_hit); //end loop on all the hits from this events
		event_batch.push_back(event);
	}
	//Here is the call to the function that forces JANA to process all the events - this is a blocking function.
	evt_src->SubmitAndWait(event_batch);

	// for each event, get the result and flag the TriDAS event
	for (int i = 0; i < evc.used_trig_events(); ++i) {
		TriggeredEvent& tev = *(evc.trig_event(i));
		assert(tev.nseeds_[L1TOTAL_ID] && "Triggered event with no seeds");
		if (event_batch[i]->should_keep) {
			tev.plugin_nseeds_[id]++;
			tev.plugin_ok_ = true;
			++plug_events;
		}
		delete event_batch[i];
	}

	event_batch.clear();
	evc.set_stats_for_plugin(id, plug_events);

	std::cout << "TrigJANA triggered " << evc.stats_for_plugin(id) << " of " << evc.used_trig_events() << " events in TTS " << evc.ts_id();
}

}
}
