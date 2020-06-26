////////////////////////////////////////////////////////////////////////
// Class:       DecayFinder
// Plugin Type: analyzer module
// File:        DecayFinder.h
//
// Generated by Wouter Van De Pontseele
// June 25, 2020
////////////////////////////////////////////////////////////////////////

#ifndef DECAYFINDER_H
#define DECAYFINDER_H

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/SpacePoint.h"

#include "larcore/Geometry/Geometry.h"
#include "nusimdata/SimulationBase/MCParticle.h"
#include "nusimdata/SimulationBase/MCTruth.h"

#include "TTree.h"

class DecayFinder;

class DecayFinder : public art::EDAnalyzer
{
public:
    explicit DecayFinder(fhicl::ParameterSet const &p);
    // The compiler-generated destructor is fine for non-base
    // classes without bare pointers or other resource use.

    // Plugins should not be copied or assigned.
    DecayFinder(DecayFinder const &) = delete;
    DecayFinder(DecayFinder &&) = delete;
    DecayFinder &operator=(DecayFinder const &) = delete;
    DecayFinder &operator=(DecayFinder &&) = delete;

    // Required functions.
    void analyze(art::Event const &e) override;
    void reconfigure(fhicl::ParameterSet const &p);
    void clearEvent();

    /**
     *  @brief  Collect and fill the MC based decay information.
     *
     *  @param  e Art event
     */
    void FillTrueDecay(art::Event const &e);

    /**
     *  @brief  Find reconstructed hits
     *
     *  @return 1 if succesfully, 0 if not.
     */
    bool FindRecoHits(art::Event const &e);

    /**
     *  @brief  Returns if point is inside a fiducial volume
     *
     *  @param fiducial volume tolerance: -x,+x,-y,+y,-z,+z
     *  @return 1 if succesfully, 0 if not.
     */
    bool IsContained(float x, float y, float z, const std::vector<float> &borders) const;

    void endSubRun(const art::SubRun &subrun);

private:
    typedef art::Handle< std::vector<recob::Hit> > HitHandle;
    typedef std::vector< art::Ptr<recob::Hit> > HitVector;
    // Fields needed for the analyser
    std::string m_hit_producer;
    bool m_isData;

    //// Tree for every event
    TTree *fEventTree;
    uint fRun, fSubrun, fEvent;

    // MC info
    float fDecayEnergy;
    float fDecayTime;
    int fDecayType;

    // Reco info
    int fNumHits;
    std::vector<float> fHitCharge;
    std::vector<float> fHitAmplitude;
    std::vector<float> fHitTime;
    std::vector<uint> fHitPlane;
    std::vector<uint> fHitWire;
};

void DecayFinder::reconfigure(fhicl::ParameterSet const &p)
{
    m_hit_producer = p.get<std::string>("hit_producer", "gaushit");
    m_isData = p.get<bool>("is_data", false);
}

DecayFinder::DecayFinder(fhicl::ParameterSet const &p)
    : EDAnalyzer(p)
{
    art::ServiceHandle<art::TFileService> tfs;
    this->reconfigure(p);

    //// Check if things are set up properly:
    std::cout << std::endl;
    std::cout << "[DecayFinder constructor] Checking set-up" << std::endl;
    std::cout << "[DecayFinder constructor] hit_producer: " << m_hit_producer << std::endl;
    std::cout << "[DecayFinder constructor] is_data: " << m_isData << std::endl;

    //// Tree for every event
    fEventTree = tfs->make<TTree>("Event", "Event Tree");
    fEventTree->Branch("event", &fEvent, "event/i");
    fEventTree->Branch("run", &fRun, "run/i");
    fEventTree->Branch("subrun", &fSubrun, "subrun/i");

    if (!m_isData)
    {
        fEventTree->Branch("sim_decay_energy", &fDecayEnergy, "sim_decay_energy/F");
        fEventTree->Branch("sim_decay_time", &fDecayTime, "sim_decay_time/F");
        fEventTree->Branch("sim_decay_type", &fDecayType, "sim_decay_type/i");
    }

    fEventTree->Branch("reco_num_hits", &fNumHits, "reco_num_hits/i");
    fEventTree->Branch("reco_hit_charge", "std::vector< float >", &fHitCharge);
    fEventTree->Branch("reco_hit_amplitude", "std::vector< float >", &fHitAmplitude);
    fEventTree->Branch("reco_hit_time", "std::vector< float >", &fHitTime);
    fEventTree->Branch("reco_hit_plane", &fHitPlane, "reco_hit_plane/i");
    fEventTree->Branch("reco_hit_wire", &fHitWire, "reco_hit_wire/i");
}

void DecayFinder::clearEvent()
{
    // MC info
    fDecayEnergy = 0;
    fDecayTime = 0;
    fDecayType = 0;

    // Reco info
    fNumHits = 0;
    fHitCharge.clear();
    fHitAmplitude.clear();
    fHitTime.clear();
    fHitPlane.clear();
    fHitWire.clear();
}

DEFINE_ART_MODULE(DecayFinder)
#endif // DECAYFINDER_H
