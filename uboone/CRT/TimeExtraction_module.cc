////////////////////////////////////////////////////////////////////////
// Class:       TimeExtraction
// Module Type: analyzer
// File:        TimeExtraction_module.cc
// Description: Module for extracting TPC/CRT events GPS timestamp.
// Generated at Fri Jun 23 02:48:04 2017 by David Lorca Galindo using artmod
// from cetpkgsupport v1_11_00.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include <artdaq-core/Data/Fragment.hh>

#include "art/Framework/Services/Optional/TFileService.h"

#include "ubooneobj/CRT/CRTHit.hh"
#include "ubooneobj/CRT/CRTTrack.hh"
#include "uboone/CRT/CRTAuxFunctions.hh"
#include "ubooneobj/RawData/DAQHeaderTimeUBooNE.h"

#include "TTree.h"

#include "TH1F.h"
#include "TH2F.h"
#include "TH3S.h"
#include "TProfile.h"
#include "TF1.h"
#include "TDatime.h"
#include <iostream>
#include <stdio.h>
#include <sstream>
#include <vector>
#include <map>
#include <utility>

namespace crt {
  class TimeExtraction;
}

class crt::TimeExtraction : public art::EDAnalyzer {
public:
  explicit TimeExtraction(fhicl::ParameterSet const & p);
  // The destructor generated by the compiler is fine for classes
  // without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  TimeExtraction(TimeExtraction const &) = delete;
  TimeExtraction(TimeExtraction &&) = delete;
  TimeExtraction & operator = (TimeExtraction const &) = delete;
  TimeExtraction & operator = (TimeExtraction &&) = delete;

  // Required functions.
  void analyze(art::Event const & e) override;

  // Selected optional functions.
  void beginJob() override;
  void endJob() override;

private:

  // Declare member data here.

  int fEvtNum; //Number of current event
  uint32_t frunNum;                //Run Number taken from event
  uint32_t fsubRunNum;             //Subrun Number taken from event

  std::string  data_label_;
  int file_type_;
  std::string  data_label_DAQHeader_;
};


crt::TimeExtraction::TimeExtraction(fhicl::ParameterSet const & p)
  : EDAnalyzer(p),
    data_label_(p.get<std::string>("data_label")),
    file_type_(p.get<int>("file_type")),
    data_label_DAQHeader_(p.get<std::string>("data_label_DAQHeader_"))
{}

void crt::TimeExtraction::analyze(art::Event const & evt)
{

  
 
    frunNum    = evt.run();
    fsubRunNum = evt.subRun();
    fEvtNum = evt.event();
    
    art::Timestamp evtTime = evt.time();
    
    auto evt_time_sec = evtTime.timeHigh();  
    auto evt_time_nsec = evtTime.timeLow();  
    std::cout.precision(19);
    
    std::cout<< "Run:  "<<frunNum << "   subRun: " <<fsubRunNum<<std::endl;                          
    std::cout<<"event: "<<fEvtNum <<std::endl;                          
    std::cout<<"Timestamp_sec: (Trigger)  "<<evt_time_sec<< "   " <<std::endl;                          
    std::cout<<"Timestamp_nsec: (Trigger)  "<<evt_time_nsec<< "   " <<std::endl;                          
    std::cout<<"                 " <<std::endl;                          
    
    //for TPC
    if(file_type_==0){
      //get DAQ Header                                     
      art::Handle< raw::DAQHeaderTimeUBooNE > rawHandle_DAQHeader;
      evt.getByLabel(data_label_DAQHeader_, rawHandle_DAQHeader);
      
      //check to make sure the data we asked for is valid
      if(!rawHandle_DAQHeader.isValid()){
	std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()
		  << ", event " << evt.event() << " has zero" 
		  << " DAQHeaderTimeUBooNE  " << " in with label " << data_label_DAQHeader_ << std::endl;
	std::cout<<"                 " <<std::endl;                          
	return;
      }
      
      raw::DAQHeaderTimeUBooNE const& my_DAQHeader(*rawHandle_DAQHeader);
      art::Timestamp evtTimeGPS = my_DAQHeader.gps_time();
      double evt_timeGPS_sec = evtTimeGPS.timeHigh();
      double evt_timeGPS_nsec = evtTimeGPS.timeLow();
      
      std::cout<<"GPS:: timestamp_sec: (Trigger sec)  "<<evt_timeGPS_sec<< "   " <<std::endl;
      std::cout<<"GPS:: timestamp_nsec: (Trigger nsec)  "<<evt_timeGPS_nsec<< "   " <<std::endl;
      std::cout<<"                 " <<std::endl;                          
    }
    //for TPC
    
    
    
    //for crt
    if(file_type_==1){
      art::Handle< std::vector<crt::CRTHit> > rawHandle;
      evt.getByLabel(data_label_, rawHandle); //
      
      //check to make sure the data we asked for is valid                                                                                                      
      if(!rawHandle.isValid()){
	std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()
		  << ", event " << evt.event() << " has zero"
		  << " CRTHits " << " in module " << data_label_ << std::endl;
	std::cout << std::endl;
	return;
      }
      
      //get better access to the data               
      std::vector<crt::CRTHit> const& CRTHitCollection(*rawHandle);
      std::cout<<"                 " <<std::endl;                          
      std::cout<<"CRTHits in this event::  "<<CRTHitCollection.size() <<std::endl;
      std::cout<<"                 " <<std::endl;                    
      
      std::vector<int>::size_type end = CRTHitCollection.size() - 1;
      
      crt::CRTHit CRTHitevent_first = CRTHitCollection[0];
      std::cout<< "CRTHitevent_first::  "<<std::endl;                          
      std::cout<< "second::ts0_s  "<<CRTHitevent_first.ts0_s<<std::endl;                          
      std::cout<< "nano_second::ts0_ns  "<<CRTHitevent_first.ts0_ns<<std::endl;                          
      std::cout<< "nano_second::ts1_ns  "<<CRTHitevent_first.ts1_ns<<std::endl;                          
      std::cout<<"                 " <<std::endl;                          
      
      crt::CRTHit CRTHitevent_last = CRTHitCollection[end];
      std::cout<< "CRTHitevent_last::  "<<std::endl;                          
      std::cout<< "second::ts0_s  "<<CRTHitevent_last.ts0_s<<std::endl;                          
      std::cout<< "nano_second::ts0_ns  "<<CRTHitevent_last.ts0_ns<<std::endl;                          
      std::cout<< "nano_second::ts1_ns  "<<CRTHitevent_last.ts1_ns<<std::endl;                          
      std::cout<<"                 " <<std::endl;                          
      
      //for(std::vector<int>::size_type i = 0; i != CRTHitCollection.size(); i++) {//A 
      
      // }//A
    }
    //for crt


}

void crt::TimeExtraction::beginJob()
{
  // Implementation of optional member function here.
}

void crt::TimeExtraction::endJob()
{
  // Implementation of optional member function here.
}

DEFINE_ART_MODULE(crt::TimeExtraction)
