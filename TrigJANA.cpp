#include "TrigJANA.h"

#include "Event_Classes.h"
#include "log.hpp"
#include "Configuration.h"
#include "PluginParameters.h"

//JANA includes
#include "JANA/JApplication.h"
#include "API/JEventSource_Tridas.h"
#include "API/GroupedEventProcessor.h"
#include "JANA/Services/JParameterManager.h"
#include "JANA/Services/JGlobalRootLock.h"

//RECONSTRUCTION includes
#include "DAQ/TridasEvent.h"
#include "addRecoFactoriesGenerators.h"

//CAlibration
#include <JANA/Calibrations/JCalibrationCCDB.h>
#include <JANA/Calibrations/JCalibrationFile.h>
#include <JANA/Calibrations/JCalibrationManager.h>

#define HAVE_CCDB 1
#include <JANA/Calibrations/JCalibrationGeneratorCCDB.h>

#include <stdlib.h>
#include <chrono>
#include <atomic>

#define USE_JANA 1

namespace tridas {
namespace tcpu {

void JANAthreadFun(JApplication *app) {
	std::cout << "TrigJANA JANAthreadFun start" << std::endl;
	app->Run();
	std::cout << "TrigJANA JANAthreadFun end" << std::endl;
}

void initialize_jana(const std::string& config_file_name, JApplication* app, TridasEventSource** evt_src) {

	std::cout << "Greetings from David and Nathan!" << std::endl;fflush(stdout);
	std::cout << "Initializing JANA" << std::endl;fflush(stdout);

	//A.C. everything must be in the config_file_name, including the plugins to be loaded!
	std::cout << "Loading options from " << config_file_name << std::endl;fflush(stdout);
	JParameterManager params;
	try {
	  std::cout<<"Before read config file"<<std::endl;fflush(stdout);
		params.ReadConfigFile(config_file_name);
	  std::cout<<"After read config file"<<std::endl;fflush(stdout);
	} catch (JException& e) {
		std::cout << "Problem loading config file '" << config_file_name << "'. Exiting." << std::endl << std::endl;
		exit(-1);
	}

	auto params_copy = new JParameterManager(params); // JApplication owns params_copy, does not own eventSources

	std::cout << "Creating JApplication " << std::endl;fflush(stdout);
	app = new JApplication(params_copy);
	japp = app;

	std::cout <<" Adding the JCalibrationGeneratorCCDB to the app"<<std::endl;
	auto calib_manager = std::make_shared<JCalibrationManager>();
	calib_manager->AddCalibrationGenerator(new JCalibrationGeneratorCCDB);
	app->ProvideService(calib_manager);
	std::cout<<"DONE"<<std::endl;

	std::cout <<"Adding the JGlobalRootLock to the app"<<std::endl;
	app->ProvideService(std::make_shared<JGlobalRootLock>());
	std::cout<<"DONE"<<std::endl;
	
	std::cout << "Adding the event source to the Japplication" << std::endl;
	*evt_src = new TridasEventSource("blocking_source", app);
	app->Add(*evt_src);
	std::cout << "DONE" << std::endl;

	std::cout << "Adding the factories Generators" << std::endl;
	addRecoFactoriesGenerators(app);
	std::cout << " Done " << std::endl;

	std::cout << "Adding the event processor the Japplication" << std::endl;
	app->Add(new GroupedEventProcessor());
	std::cout << "DONE" << std::endl;

	//Launch the thread with the JApplication and detach from it.
	std::cout << "TrigJANA starting the thread" << std::endl;

	//probably not necessary, app->Run(kFALSE) is enough
	boost::thread janaThread(JANAthreadFun, app);
	janaThread.detach();
	std::cout << "DONE and DETACHED" << std::endl;

	//Don't need - > JANA2 submit_and_wait buffers in the JEventSource.
	//A.C. without this, crashes
	while (!app->IsInitialized()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void TrigJANA(PluginArgs const& args) {
	int const id = args.id;
	unsigned plug_events = 0;
	//std::cout << args.params->ordered_begin()->first << std::endl;
	//std::cout << args.params->ordered_begin()->second.get_value<std::string>() << std::endl;

	EventCollector& evc = *args.evc;

        static size_t event_count = 0;
        static auto last_measurement = std::chrono::high_resolution_clock::now();
	static std::mutex timer_mutex;

#if USE_JANA
	//--------- Execute this if USE_JANA is !=0 ---------

	//need to pass the configuration file
	static std::once_flag is_jana_initialized;
	static JApplication * app = 0;
	static TridasEventSource* evt_src = 0;

	std::string const config_file_name = args.params->get < std::string > ("CONFIG_FILE");
	assert(config_file_name.length() > 0 && "CONFIG_FILE must be present"); //TODO: is this the right way?
	std::call_once(is_jana_initialized, &initialize_jana, config_file_name, app, &evt_src);

	/*Get the run number. It is coded in the file name of the file that is symlinked by /tmp/latest*/
	char *fRunFileName=0;
	fRunFileName=canonicalize_file_name("/tmp/latest");

	int runN=1;
	if (fRunFileName == NULL){
		std::cout<<"TrigJANA: canonicalize_file_name failed"<<std::endl;
		std::cout<<"Using run number 1"<<std::endl;
	}else{
		std::string tmpStr(fRunFileName);
		runN=atoi(tmpStr.substr(tmpStr.find_last_of("/")+1).c_str());
		free(fRunFileName);
	}

	/*We now inject the trig_events to the source*/
	std::vector<TridasEvent*> event_batch;

	for (int i = 0; i < evc.used_trig_events(); ++i) {
		TriggeredEvent& tev = *(evc.trig_event(i));
		auto event = new TridasEvent;
		event->event_number = i;
		event->time_slice = evc.ts_id();
		event->run_number = runN;
		event->should_keep = false;

		//Loop on PMT hits
		PMTHit* first_hit = reinterpret_cast<PMTHit*>(tev.sw_hit_);  //The first hit in the event
		PMTHit* last_hit = reinterpret_cast<PMTHit*>(tev.ew_hit_);   //The last hit in the event
		PMTHit* current_hit = first_hit;

		do {
			fadcHit hit;

			//Determine crate, slot, channel
			DataFrameHeader const& dfh = *dataframeheader_cast(current_hit->getRawDataStart());
			hit.crate = dfh.TowerID;
			hit.slot = dfh.EFCMID;
			hit.channel = dfh.PMTID;

			//Time
			hit.time = current_hit->get_fine_time();

			//charge
			hit.charge = current_hit->getCaliCharge();

			//samples - these may extend on more than one frame.
			auto pos = 0;
			while (pos != current_hit->length()) {
				DataFrameHeader const& dfh2 = *dataframeheader_cast(current_hit->getRawDataStart()+pos);
				pos += sizeof(DataFrameHeader);
				auto const nsamples = dfh2.NDataSamples;
				auto psamples = static_cast<uint16_t const*>(static_cast<void const*>(current_hit->getRawDataStart() + pos));
				for (auto j = 0; j < nsamples; j++) {
					hit.data.push_back(psamples[j]);
				}
				pos += getDFHPayloadSize(dfh2);
			}

			/*	At the moment (2020), TriDAS can be feed by three hardware sources:

			 - Waveboard (V1)
			 - Waveboard (V2)
			 - fa250 stream trough VTP

			 In the future, there may be more than one hit type per event, if the readout architecture is mixed.
			 Ideally, one would recognize the hit type from the hit itself, and have one JANA2 factory per hit - hits may be later processed differently.
			 One can judge this from the crate/slot/channel combination, but this is setup-dependent.
			 For the moment, this is not supported. Hence, for the moment I differentiate between waveboard and fa250 by considering that the fa250 has no samples, only time and charge.
			 */



			if (hit.data.size() == 0)
				hit.type = fadcHit_TYPE::FA250VTPMODE7;
			else
				hit.type = fadcHit_TYPE::WAVEBOARD;

			//add this hit to the TridasEvent
			event->hits.push_back(hit);

			current_hit = current_hit->next();
		} while (current_hit && current_hit != last_hit); //end loop on all the hits from this events

		//publish the event
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

#else
	//--------- Execute this if USE_JANA is 0 ---------

	for (int i = 0; i < evc.used_trig_events(); ++i) {
		TriggeredEvent& tev = *(evc.trig_event(i));
		assert(tev.nseeds_[L1TOTAL_ID] && "Triggered event with no seeds");
		tev.plugin_nseeds_[id]++;
		tev.plugin_ok_ = true;
		++plug_events;
	}

#endif

	timer_mutex.lock();
	auto latest_time = std::chrono::high_resolution_clock::now();
	event_count += evc.used_trig_events(); // I assume this is our event count
	if ((latest_time-last_measurement) >= std::chrono::seconds(1)) {
		last_measurement = latest_time;
		std::cout << "TrigJANA: Instantaneous throughput = " << event_count << " Hz" << std::endl;
		event_count = 0;
	}
	timer_mutex.unlock();
	


	evc.set_stats_for_plugin(id, plug_events);

	std::cout << "TrigJANA triggered " << evc.stats_for_plugin(id) << " of " << evc.used_trig_events() << " events in TTS " << evc.ts_id();
}

}
}
