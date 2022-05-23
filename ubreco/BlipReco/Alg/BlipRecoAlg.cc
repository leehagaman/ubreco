
#include "ubreco/BlipReco/Alg/BlipRecoAlg.h"

namespace blip {

  //###########################################################
  // Constructor
  //###########################################################
  BlipRecoAlg::BlipRecoAlg( fhicl::ParameterSet const& pset )
  {
    this->reconfigure(pset);
   
    // create diagnostic histograms
    if( fMakeHistos ) {
      art::ServiceHandle<art::TFileService> tfs;
      art::TFileDirectory hdir = tfs->mkdir("BlipRecoAlg");
      h_clust_nwires    = hdir.make<TH1D>("clust_nwires","Clusters (pre-cut);Wires in cluster",100,0,100);
      h_clust_timespan  = hdir.make<TH1D>("clust_timespan","Clusters (pre-cut);Time span [ticks]",100,0,100);
      for(int i=0; i<kNplanes; i++) {
        if( i == fCaloPlane ) continue;
        h_clust_overlap[i]      = hdir.make<TH1D>(Form("pl%i_clust_overlap",i),         Form("Plane %i clusters;Overlap fraction",i),101,0,1.01);
        h_clust_dt[i]           = hdir.make<TH1D>(Form("pl%i_clust_dt",i),              Form("Plane %i clusters;dT [ticks]",i),300,-15,15);
        h_clust_dtfrac[i]       = hdir.make<TH1D>(Form("pl%i_clust_dtfrac",i),          Form("Plane %i clusters;dT/#sigma",i),300,-3,3);
        h_clust_score[i]        = hdir.make<TH1D>(Form("pl%i_clust_score",i),           Form("Plane %i clusters;Match score",i),101,0,1.01);
        h_clust_picky_overlap[i]   = hdir.make<TH1D>(Form("pl%i_clust_picky_overlap",i),Form("Plane %i clusters (3 planes, intersect #Delta < 3 cm);Overlap fraction",i),101,0,1.01);
        h_clust_picky_dt[i]        = hdir.make<TH1D>(Form("pl%i_clust_picky_dt",i),     Form("Plane %i clusters (3 planes, intersect #Delta < 3 cm);dT [ticks]",i),300,-15,15);
        h_clust_picky_dtfrac[i]    = hdir.make<TH1D>(Form("pl%i_clust_picky_dtfrac",i), Form("Plane %i clusters (3 planes, intersect #Delta < 3 cm);dT/#sigma",i),300,-3,3);
        h_clust_picky_score[i]     = hdir.make<TH1D>(Form("pl%i_clust_picky_score",i),  Form("Plane %i clusters (3 planes, intersect #Delta < 3 cm);Match score",i),101,0,1.01);
        //h_clust_mScoreBest[i] = hdir.make<TH1D>(Form("pl%i_clust_score_best",i),Form("best match score for clusters on plane %i",i),101,0,1.01);
        h_nmatches[i]         = hdir.make<TH1D>(Form("pl%i_nmatches",i),                Form("number of plane%i matches to single collection cluster",i),10,0,10);
      }
    }
  
  }
  
  //--------------------------------------------------------------
  BlipRecoAlg::BlipRecoAlg( )
  {
  }
  
  //--------------------------------------------------------------  
  //Destructor
  BlipRecoAlg::~BlipRecoAlg()
  {
  }
  
  
  //###########################################################
  // Reconfigure fcl parameters
  //###########################################################
  void BlipRecoAlg::reconfigure( fhicl::ParameterSet const& pset ){
    fHitProducer        = pset.get<std::string>   ("HitProducer",       "pandora");
    fTrkProducer        = pset.get<std::string>   ("TrkProducer",       "gaushit");
    fGeantProducer      = pset.get<std::string>   ("GeantProducer",     "largeant");
    fSimDepProducer     = pset.get<std::string>   ("SimEDepProducer",   "ionization");
    fMakeHistos         = pset.get<bool>          ("MakeHistograms",    true);
    fTrueBlipMergeDist  = pset.get<float>         ("TrueBlipMergeDist", 0.3);
    fDoHitFiltering     = pset.get<bool>          ("DoHitFiltering",     true);
    fHitClustWidthFact  = pset.get<float>         ("HitClustWidthFact", 2.0);
    fHitClustWireRange  = pset.get<int>           ("HitClustWireRange", 1);
    
    fTimeOffsets          = pset.get<std::vector<float>>  ("TimeOffsets",{0.15, 0.15, 0.0});
    //fClustMatchMinOverlap = pset.get<float>         ("ClustMatchMinOverlap",0.6 );
    fClustMatchSigmaFact  = pset.get<float>         ("ClustMatchSigmaFact", 0.5 );
    fClustMatchMaxTicks   = pset.get<float>         ("ClustMatchMaxTicks",  2.0 );
    fClustMatchMinOverlap = pset.get<std::vector<float>>  ("ClustMatchMinOverlap",{0.6, 0.7});
    //fClustMatchSigmaFact  = pset.get<std::vector<float>>  ("ClustMatchSigmaFact", {0.5, 0.5});
    //fClustMatchMaxTicks   = pset.get<std::vector<float>>  ("ClustMatchMaxTicks",  {2., 2.});

    
    fCaloPlane          = pset.get<int>           ("CaloPlane",         2);
    fMaxHitTrkLength    = pset.get<float>         ("MaxHitTrkLength",   5);
    fMaxWiresInCluster  = pset.get<int>           ("MaxWiresInCluster", 7);
    fMaxClusterSpan     = pset.get<float>         ("MaxClusterSpan",    30);
    fPickyBlips         = pset.get<bool>          ("PickyBlips",        false);
    fMaxHitAmp          = pset.get<float>         ("MaxHitAmp",         200);
    fApplyTrkCylinderCut= pset.get<bool>          ("ApplyTrkCylinderCut",false);
    fCylinderRadius     = pset.get<float>         ("CylinderRadius",    15);
    fMinHitRMS          = pset.get<std::vector<float>>  ("MinHitRMS",  {-9999,-9999,-9999});
    fMaxHitRMS          = pset.get<std::vector<float>>  ("MaxHitRMS",  { 9999, 9999, 9999});
    fMinHitGOF          = pset.get<std::vector<float>>  ("MinHitGOF",  {-9999,-9999,-9999});
    fMaxHitGOF          = pset.get<std::vector<float>>  ("MaxHitGOF",  { 9999, 9999, 9999});
    fCaloAlg            = new calo::CalorimetryAlg( pset.get<fhicl::ParameterSet>("CaloAlg") );
  }



  //###########################################################
  // Main reconstruction procedure.
  //
  // This function does EVERYTHING. The resulting collections of 
  // blip::HitClusts and blip::Blips can then be retrieved after
  // this function is run.
  //###########################################################
  void BlipRecoAlg::RunBlipReco( const art::Event& evt ) {
  
    //std::cout<<"\n"
    //<<"=========== BlipRecoAlg =========================\n"
    //<<"Event "<<evt.id().event()<<" / run "<<evt.id().run()<<"\n";
  
    //=======================================
    // Reset things
    //=======================================
    blips.clear();
    hitclust.clear();
    hitinfo.clear();
    pinfo.clear();
    trueblips.clear();

  
    //=======================================
    // Get data products for this event
    //========================================
    
    // --- detector properties
    auto const* detProp = lar::providerFrom<detinfo::DetectorPropertiesService>();
  
    // -- geometry
    art::ServiceHandle<geo::Geometry> geom;
  
    // -- G4 particles
    art::Handle< std::vector<simb::MCParticle> > pHandle;
    std::vector<art::Ptr<simb::MCParticle> > plist;
    if (evt.getByLabel(fGeantProducer,pHandle))
      art::fill_ptr_vector(plist, pHandle);
  
    // -- SimEnergyDeposits
    art::Handle<std::vector<sim::SimEnergyDeposit> > sedHandle;
    std::vector<art::Ptr<sim::SimEnergyDeposit> > sedlist;
    if (evt.getByLabel(fSimDepProducer,sedHandle)) 
      art::fill_ptr_vector(sedlist, sedHandle);
  
    // -- hits (from input module)
    art::Handle< std::vector<recob::Hit> > hitHandle;
    std::vector<art::Ptr<recob::Hit> > hitlist;
    if (evt.getByLabel(fHitProducer,hitHandle))
      art::fill_ptr_vector(hitlist, hitHandle);

    // -- hits (from gaushit)
    // -- these are used in truth-matching of hits
    art::Handle< std::vector<recob::Hit> > hitHandleGH;
    std::vector<art::Ptr<recob::Hit> > hitlistGH;
    if (evt.getByLabel("gaushit",hitHandleGH))
      art::fill_ptr_vector(hitlistGH, hitHandleGH);

    // -- tracks
    art::Handle< std::vector<recob::Track> > tracklistHandle;
    std::vector<art::Ptr<recob::Track> > tracklist;
    if (evt.getByLabel(fTrkProducer,tracklistHandle))
      art::fill_ptr_vector(tracklist, tracklistHandle);
  
    // -- hit <-> track associations
    art::FindManyP<recob::Track> fmtrk(hitHandle,evt,fTrkProducer);

    // -- gaushit <-> track associations 
    art::FindManyP<recob::Track> fmtrkGH(hitHandleGH,evt,fTrkProducer);
  
    // -- gaushit <-> particle associations
    art::FindMany<simb::MCParticle,anab::BackTrackerHitMatchingData> fmhh(hitHandleGH,evt,"gaushitTruthMatch");

    
    //==================================================
    // Use G4 information to determine the "true"
    // blips in this event.
    //==================================================
    
    bool isMC = false;
    
    // Map of each hit to its gaushit index (needed if the provided
    // hit collection is some filtered subset of gaushit, in order to
    // use gaushitTruthMatch later on)
    std::map< int, int > map_gh;
    
    if( plist.size() || fmhh.isValid() ) {

      //std::cout<<"Found "<<plist.size()<<" particles from "<<fGeantProducer<<"\n";
      //std::cout<<"Found "<<sedlist.size()<<" sim energy deposits from "<<fSimDepProducer<<"\n";
      
      isMC = true;
      pinfo.resize(plist.size());
    
      for(auto& h : hitlist ) {
        if( fHitProducer == "gaushit" ) { // if input collection is gaushit, this is trivial 
          map_gh[h.key()] = h.key(); continue; }
        for (auto& gh : hitlistGH ){     // otherwise, find the matching gaushit
          if( gh->PeakTime() == h->PeakTime() && gh->Channel() == h->Channel() ) {
            map_gh[h.key()] = gh.key(); break; }
        }
      }
    
      // Loop through the MCParticles
      for(size_t i = 0; i<plist.size(); i++){
        //auto pPart = plist[i];
        //BlipUtils::FillParticleInfo( *pPart, pinfo[i], sedlist );
        BlipUtils::FillParticleInfo( *plist[i], pinfo[i], sedlist );
        pinfo[i].index = i;
      } // endloop over G4 particles
      
      // Calculate the true blips
      BlipUtils::MakeTrueBlips(pinfo, trueblips);
      BlipUtils::MergeTrueBlips(trueblips, fTrueBlipMergeDist);
    }
  


    //=======================================
    // Map track IDs to the index in the vector
    //=======================================
    //std::cout<<"Looping over tracks...\n";
    std::map<size_t,size_t> trkindexmap;
    std::map<size_t,std::vector<size_t>> trkhitMap;
    for(size_t i=0; i<tracklist.size(); i++) 
      trkindexmap[tracklist.at(i)->ID()] = i;

    

    //=======================================
    // Fill vector of hit info
    //========================================
    hitinfo.resize(hitlist.size());
    
    std::map<int,std::vector<int>> chanhitsMap;
    std::map<int,std::vector<int>> chanhitsMap_untracked;
    std::map<int,std::vector<int>> planehitsMap;
    int nhits_untracked = 0;

    //std::cout<<"Looping over the hits...\n";
    for(size_t i=0; i<hitlist.size(); i++){
      int   chan  = hitlist[i]->Channel();
      int   wire  = hitlist[i]->WireID().Wire;
      int   plane = hitlist[i]->WireID().Plane;
      int   tpc   = hitlist[i]->WireID().TPC;
      float peakT = hitlist[i]->PeakTime();
      hitinfo[i].hitid      = i;
      hitinfo[i].Hit        = hitlist[i];
      hitinfo[i].wire       = wire;
      hitinfo[i].tpc        = tpc;
      hitinfo[i].plane      = plane;
      hitinfo[i].driftTicks = peakT - detProp->GetXTicksOffset(plane,0,0) - fTimeOffsets[plane];
      hitinfo[i].qcoll      = fCaloAlg->ElectronsFromADCArea(hitlist[i]->Integral(),plane);

      if( isMC ) {
        // find G4 particle ID for leading contributor (Requires SimChannels, which are dropped by default)
        //if( blip::DoesHitHaveSimChannel(hitlist[i]) ){
        //  BlipUtils::HitTruth( hitlist[i], hitinfo[i].g4id, hitinfo[i].g4frac, hitinfo[i].g4energy, hitinfo[i].g4charge);
        //  hitinfo[i].g4ids = BlipUtils::HitTruthIds(hitlist[i]);
        //}
        // Update 5/2/22: Use gaushitTruthMatch in MicroBooNE
        hitinfo[i].g4energy = 0;
        hitinfo[i].g4charge = 0;
        int igh = map_gh[i]; 
        if( fmhh.at(igh).size() ) {
          std::vector<simb::MCParticle const*> pvec;
          std::vector<anab::BackTrackerHitMatchingData const*> btvec;
          fmhh.get(igh,pvec,btvec);
          float max = -9;
          for(size_t j=0; j<pvec.size(); j++){
            hitinfo[i].g4ids.insert(pvec.at(j)->TrackId()); 
            hitinfo[i].g4energy += btvec.at(j)->energy;
            hitinfo[i].g4charge += btvec.at(j)->numElectrons;
            if( btvec.at(j)->energy > max ) {
              max = btvec.at(j)->energy;
              hitinfo[i].g4id   = pvec.at(j)->TrackId();
              hitinfo[i].g4pdg  = pvec.at(j)->PdgCode();
              hitinfo[i].g4frac = btvec.at(j)->ideFraction;
            }
          }
        }
      }
   
      // find associated track
      if (fmtrk.isValid()){ 
        if (fmtrk.at(i).size())  hitinfo[i].trkid = trkindexmap[fmtrk.at(i)[0]->ID()];
      
        // if the hit collection didn't have associations made
        // to the tracks, try gaushit instead
      } else if (fmtrkGH.isValid()) {
        int gi = map_gh[i];
        if (fmtrkGH.at(gi).size()) hitinfo[i].trkid=trkindexmap[fmtrkGH.at(gi)[0]->ID()];
      }

      // add to the channel hit map
      chanhitsMap[chan].push_back(i);
      planehitsMap[plane].push_back(i);
      if( hitinfo[i].trkid <= 0 ) {
        chanhitsMap_untracked[chan].push_back(i);
        nhits_untracked++;
      }

    }//endloop over hits
    
    //std::cout<<"Found "<<hitlist.size()<<" hits from "<<fHitProducer<<" ("<<nhits_untracked<<" untracked)\n";
    //std::cout<<"Found "<<tracklist.size()<<" tracks from "<<fTrkProducer<<"\n";





    //=================================================================
    // Blip Reconstruction
    //================================================================
    //  
    //  Procedure
    //  [x] Look for hits that were not included in a track 
    //  [x] Filter hits based on hit width, etc
    //  [x] Merge together closely-spaced hits on same wires and adjacent wires
    //  [x] Plane-to-plane time matching
    //  [x] Wire intersection check to get XYZ
    //  [x] Create "blip" object

    // Create a series of masks that we'll update as we go along
    std::vector<bool> hitIsGood(hitlist.size(),     true);
    std::vector<bool> hitIsClustered(hitlist.size(),false);
    
    // Basic track inclusion cut: exclude hits that were tracked
    for(size_t i=0; i<hitlist.size(); i++){
      int trkid = hitinfo[i].trkid;
      if( trkid >= 0 ) {
        if( tracklist[trkid]->Length() > fMaxHitTrkLength ) hitIsGood[i] = false;
      }
    }

    // Filter based on hit properties
    if( fDoHitFiltering ) {
      for(size_t i=0; i<hitlist.size(); i++){
        if( !hitIsGood[i] ) continue;
        hitIsGood[i] = false;
        int plane = hitlist[i]->WireID().Plane;
        // goodness of fit
        if( hitlist[i]->GoodnessOfFit() < fMinHitGOF[plane] ) continue;
        if( hitlist[i]->GoodnessOfFit() > fMaxHitGOF[plane] ) continue;
        // hit width and amplitude
        if( hitlist[i]->RMS() < fMinHitRMS[plane] ) continue;
        if( hitlist[i]->RMS() > fMaxHitRMS[plane] ) continue;
        if( hitlist[i]->PeakAmplitude() > fMaxHitAmp ) continue;
        // we survived the gauntlet of cuts -- hit is good!
        hitIsGood[i] = true;
      }
    }

    
    // ---------------------------------------------------
    // Hit clustering
    // ---------------------------------------------------
    std::map<int,std::map<int,std::vector<int>>> tpc_planeclustsMap;
    //std::cout<<"Doing cluster reco\n";
    for(auto const& planehits : planehitsMap){
      for(auto const& hi : planehits.second ){
        
        // skip hits flagged as bad, or already clustered
        if( !hitIsGood[hi] || hitIsClustered[hi] ) continue; 
      
        // initialize a new cluster with this hit as seed
        blip::HitClust hc = BlipUtils::MakeHitClust(hitinfo[hi]);
        if( !hc.isValid ) continue;
        hitIsClustered[hi] = true;
       
        // see if we can add other hits to it; continue until 
        // no new hits can be lumped in with this clust
        int hitsAdded;
        do{
          hitsAdded = 0;  
          for(auto const& hj : planehits.second ) {

            if( !hitIsGood[hj] || hitIsClustered[hj] ) continue; 
            
            // skip hits outside overall cluster wire range
            int w1 = hitinfo[hj].wire - fHitClustWireRange;
            int w2 = hitinfo[hj].wire + fHitClustWireRange;
            if( w2 < hc.StartWire || w1 > hc.EndWire ) continue;
            
            // check for proximity with every other hit added
            // to this cluster so far
            for(auto const& hii : hc.HitIDs ) {
              
              if( hitinfo[hii].wire > w2 ) continue;
              if( hitinfo[hii].wire < w1 ) continue;
              
              float t1 = hitinfo[hj].driftTicks;
              float t2 = hitinfo[hii].driftTicks;
              float rms_sum = (hitlist[hj]->RMS() + hitlist[hii]->RMS());
              if( fabs(t1-t2) > fHitClustWidthFact * rms_sum ) continue;
              BlipUtils::GrowHitClust(hitinfo[hj],hc);
              hitIsClustered[hj] = true;
              hitsAdded++;
              break;
            }
          }
        } while ( hitsAdded!=0 );
        
        float span = hc.EndTime - hc.StartTime;
        if( fMakeHistos ) {
          h_clust_nwires->Fill(hc.Wires.size());
          h_clust_timespan->Fill(span);
        }
          
        // add this new cluster to the vector
        if( (int)hc.Wires.size() <= fMaxWiresInCluster && span <= fMaxClusterSpan ) {
          int idx = (int)hitclust.size();
          hc.ID = idx;
          tpc_planeclustsMap[hc.TPC][hc.Plane].push_back(idx);
          
          // go back and encode this cluster ID into the hit information
          for(auto const& hitID : hc.HitIDs) hitinfo[hitID].clustid = hc.ID;
         
          // ... and find the associated truth-blip
          if( hc.G4ID > 0 ) {
            for(size_t j=0; j< trueblips.size(); j++){
              int tbG4 = trueblips[j].LeadG4ID;
              if( tbG4 >= 0 && tbG4 == hc.G4ID ) {
                hc.EdepID = trueblips[j].ID;
                break;
              }
            }
          }
         
          // finally, add the finished cluster to the stack
          hitclust.push_back(hc);

        }
      }
    }
    //std::cout<<"Reconstructed "<<hitclust.size()<<" hit clusts\n";
   


    
    /*
    // =============================================================================
    // Plane matching and 3D blip formation
    // =============================================================================

    // --------------------------------------
    // Method 1A: Require match between calo plane (typically collection) and
    //            1 or 2 induction planes. For every hitclust on the calo plane,
    //            do the following:
    //              1. Loop over hitclusts in one of the other planes (same TPC)
    //              2. Check for wire intersections
    //              3. Find closest-matched clust and add it to the histclust group
    //              4. Repeat for remaining plane(s)
    
    for(auto const& tpcMap : tpc_planeclustsMap ) { // loop on TPCs
     
      //std::cout
      //<<"Performing cluster matching in TPC "<<tpcMap.first<<", which has "<<tpcMap.second.size()<<" planes\n";
      auto planeMap = tpcMap.second;
      if( planeMap.find(fCaloPlane) != planeMap.end() ){
        int   planeA            = fCaloPlane;
        auto  hitclusts_planeA  = planeMap[planeA];
        //std::cout<<"using plane "<<fCaloPlane<<" as reference/calo plane ("<<planeMap[planeA].size()<<" clusts)\n";
        for(auto const& i : hitclusts_planeA ) {
          auto& hcA = hitclust[i];
          
          // initiate hit-cluster group
          std::vector<blip::HitClust> hcGroup;
          hcGroup.push_back(hitclust[i]);

          // for each of the other planes, make a map of potential
          // cluster matches and for each, create a "match score"
          // that incorporates these two metrics:
          //   - # matched hits between them
          //   - average match dT-diff
          std::map<int, std::set<int>> cands;
          std::map<int, float> clust_score;
          
          // ---------------------------------------------------
          // loop over other planes
          for(auto  hitclusts_planeB : planeMap ) {
            int planeB = hitclusts_planeB.first;
            if( planeB == planeA ) continue;
          
            float best_dT =     999;
            float best_dTfrac = 999;

            // Loop over all non-matched clusts on this plane
            for(auto const& j : hitclusts_planeB.second ) {
              auto& hcB = hitclust[j];
              if( hcB.isMatched ) continue;
                
              // Calculate the cluster overlap and skip if it isn't significant
              float overlapFrac = BlipUtils::CalcHitClustsOverlap(hcA,hcB);
              h_clust_overlap[planeB] ->Fill( overlapFrac );
              if( overlapFrac <= fClustMatchMinOverlap ) continue;
              
              // Check if the two channels actually intersect
              double y,z;
              const int chA = hcA.LeadHit->Channel();
              const int chB = hcB.LeadHit->Channel();
              if( !art::ServiceHandle<geo::Geometry>()
                      ->ChannelsIntersect(chA,chB,y,z) ) continue;
            
              
              // -----------------------------------------
              // Calculate match score hit by hit
              std::set<int> hitsA = hitclust[i].HitIDs;
              std::set<int> hitsB = hitclust[j].HitIDs; 
              for(auto hitA : hitsA){
                
                float min_dt      = 999;
                float min_dt_frac = 0;

                for(auto hitB : hitsB){
                  float tA = hitinfo[hitA].driftTicks;
                  float tB = hitinfo[hitB].driftTicks;
                  float dT = tB-tA;
                  
                  
                  if( fabs(dT) < fabs(min_dt) ) {
                    float rms_A = hitlist[hitA]->RMS();
                    float rms_B = hitlist[hitB]->RMs();
                    float width = sqrt( pow(rms_A,2) + pow(rms_B,2) );
                    min_dt_frac = dT/width;
                    min_dt = dT;
                  }

                }
                  
                if( fMakeHistos && hitsA.size() == 1 && hitsB.size() == 1 ) {
                  h_hit_dtfrac[planeB]->Fill(min_dt_frac);
                  h_hit_dt[planeB]    ->Fill(min_dt);
                }
                  

                  
                  
                  //float maxWidth   = std::max(hitlist[hitA]->RMS(),hitlist[hitB]->RMS());
                  float width    = std::min(hitlist[hitA]->RMS(),hitlist[hitB]->RMS());
                  float matchTol = std::min(fHitMatchMaxTicks, fHitMatchWidthFact*width);
                  
                  // fill dT histograms if cluster size is 1
                  if( fMakeHistos ) {
                    if( hitsA.size() == 1 && hitsB.size() == 1 ) {
                      h_hit_dtfrac[planeB]->Fill(dT/width); 
                      if( fabs(dT) < fHitMatchWidthFact*width ) 
                        h_hit_dt[planeB]    ->Fill(dT); 
                    }
                  }

                  if( fabs(dT) < matchTol ) {
                    float score = std::max(1. - fabs(dT)/matchTol,0.);
                    if( score > score_hitA ) score_hitA = score;
                    cands[planeB].insert(j);
                  }//endif dT < hitMatchTol
                  
                }//endloop over hits in cluster j/B

                score_clustA += score_hitA;
              
              }//endloop over hits in cluster i/A (calo plane)
              
              // save final match score for clust i <--> clust j
              clust_score[j] = score_clustA / hitclust[i].HitIDs.size();

            }//endloop over B clusters
          }//endloop over other planes
          
          // ---------------------------------------------------
          // loop over the candidates found on each plane
          // and select the one with the largest score
          if( cands.size() ) {
            for(auto c : cands ) {
              if( fMakeHistos ) h_nmatches[c.first]->Fill(c.second.size());
              float bestScore = 0;
              int   bestID = -9;
              for(auto cid : c.second) {
                if( fMakeHistos ) h_clust_mScore[c.first]->Fill( clust_score[cid] );
                if( clust_score[cid] > bestScore ) {
                  bestScore = clust_score[cid];
                  bestID = cid;
                }
              }
              if( bestID >= 0 && bestScore >= fClustMatchMinScore ) {
                hcGroup.push_back(hitclust[bestID]);
                hitclust[bestID].isMatched = true;
                hitclust[i].isMatched = true;
                for(auto hit : hitclust[bestID].HitIDs) hitinfo[hit].ismatch = true;
                for(auto hit : hitclust[i].HitIDs) hitinfo[hit].ismatch = true;
              }
            }

            // ----------------------------------------
            // make our new blip
            blip::Blip newBlip = BlipUtils::MakeBlip(hcGroup);
            for(size_t pl=0; pl<kNplanes; pl++) {
              newBlip.matchscore[pl] = clust_score[newBlip.clustID[pl]];
            }

            // ----------------------------------------
            // if it isn't valid, forget it and move on
            if( !newBlip.isValid ) continue;

            // ----------------------------------------
            // if we are being picky...
            if( fPickyBlips ) {
              if( newBlip.NPlanes < 3           ) continue;
              if( newBlip.MaxIntersectDiff > 5  ) continue;
            }
   
            // ----------------------------------------
            // apply cylinder cut 
            for(auto& trk : tracklist ){
              if( trk->Length() < fMaxHitTrkLength ) continue;
              auto& a = trk->Vertex();
              auto& b = trk->End();
              TVector3 p1(a.X(), a.Y(), a.Z() );
              TVector3 p2(b.X(), b.Y(), b.Z() );
              // TO-DO: if this track starts or ends at a TPC boundary, 
              // we should extend p1 or p2 to outside the AV to avoid blind spots
              
              TVector3 bp = newBlip.Position;
              float d = BlipUtils::DistToLine(p1,p2,bp);
              
              
              if( d > 0 ) {
                // update closest trkdist
                if( newBlip.trkdist < 0 || d < newBlip.trkdist ) {
                  newBlip.trkdist = d;
                  newBlip.trkid = trk->ID();
                }

                // need to do some math to figure out if this is in
                // the 45 degreee "cone" relative to the start/end 
                if( !newBlip.inCylinder && d < fCylinderRadius ) {
                  float angle1 = asin( d / (p1-bp).Mag() ) * 180./3.14159;
                  float angle2 = asin( d / (p2-bp).Mag() ) * 180./3.14159;
                  ///std::cout<<"d "<<d<<"  angles "<<angle1<<"  "<<angle2<<"\n";
                  if( angle1 < 45. && angle2 < 45. ) newBlip.inCylinder = true;
                }
              }
            }//endloop over trks
            
            if( fApplyTrkCylinderCut && newBlip.inCylinder ) continue;

            // ----------------------------------------
            // if we made it this far, the blip is good!
            newBlip.ID = blips.size();
            blips.push_back(newBlip);
            
            // associate this blip with the hits and clusters within it
            for(auto hc : hcGroup )       hitclust[hc.ID].BlipID = newBlip.ID;
            for(auto h : newBlip.HitIDs ) hitinfo[h].blipid = newBlip.ID;
          
          }

        }//endloop over caloplane ("Plane A") clusters
      }//endif calo plane has clusters
    }//endloop over TPCs
  
    //std::cout<<"Reconstructed "<<blips.size()<<" 3D blips\n";
    */


    // =============================================================================
    // Plane matching and 3D blip formation
    // =============================================================================
     
    for(auto const& tpcMap : tpc_planeclustsMap ) { // loop on TPCs
     
      //std::cout
      //<<"Performing cluster matching in TPC "<<tpcMap.first<<", which has "<<tpcMap.second.size()<<" planes\n";
      auto planeMap = tpcMap.second;
      if( planeMap.find(fCaloPlane) != planeMap.end() ){
        int   planeA            = fCaloPlane;
        auto  hitclusts_planeA  = planeMap[planeA];
        //std::cout<<"using plane "<<fCaloPlane<<" as reference/calo plane ("<<planeMap[planeA].size()<<" clusts)\n";
        for(auto const& i : hitclusts_planeA ) {
          auto& hcA = hitclust[i];
          
          // initiate hit-cluster group
          std::vector<blip::HitClust> hcGroup;
          hcGroup.push_back(hcA);

          // for each of the other planes, make a map of potential matches
          std::map<int, std::set<int>> cands;
          
          std::map<int, float> map_clust_dtfrac;
          std::map<int, float> map_clust_dt;
          std::map<int, float> map_clust_overlap;
          std::map<int, float> map_clust_score;
          
          // ---------------------------------------------------
          // loop over other planes
          for(auto  hitclusts_planeB : planeMap ) {
            int planeB = hitclusts_planeB.first;
            if( planeB == planeA ) continue;
        
            float best_score    = -9;
            float best_overlap  = -9;
            float best_dT       = 999;
            float best_dTfrac   = 999;

            // Loop over all non-matched clusts on this plane
            for(auto const& j : hitclusts_planeB.second ) {
              auto& hcB = hitclust[j];
              if( hcB.isMatched ) continue;
              
              // Calculate the cluster overlap and skip if it isn't significant
              float overlapFrac = BlipUtils::CalcHitClustsOverlap(hcA,hcB);
              if( overlapFrac  > best_overlap )      best_overlap = overlapFrac;
              if( overlapFrac < fClustMatchMinOverlap [planeB] ) continue;

              // Check if the two channels actually intersect
              double y,z;
              const int chA = hcA.LeadHit->Channel();
              const int chB = hcB.LeadHit->Channel();
              if( !art::ServiceHandle<geo::Geometry>()
                      ->ChannelsIntersect(chA,chB,y,z) ) continue;
              
              // Check for a match
              float dt      = (hcB.Time - hcA.Time);
              float sigmaT  = sqrt( pow(hcA.TimeErr,2) + pow(hcB.TimeErr,2) );
             
              // Use lead hit if it's better
              //float dt2     = (hcB.LeadHitTime - hcA.LeadHitTime);
              //float sigma2  = sqrt( pow(hcA.LeadHit->RMS(),2) + pow(hcB.LeadHit->RMS(),2) );
              //if( fabs(dt2/sigma2) < fabs(dt/sigma) ) {
              //  dt     = dt2;
              //  sigmaT = sigma2;
              //}
              
              float dtfrac = dt/sigmaT;

              // score to combine information
              float score = overlapFrac * exp(-fabs(dtfrac));
              
              if( fabs(dtfrac) < fabs(best_dTfrac) ) best_dTfrac  = dtfrac;
              if( fabs(dt)     < fabs(best_dT)     ) best_dT      = dt;
              if( score        > best_score )        best_score   = score;
              
            
              
              //float matchTol = std::min(fClustMatchMaxTicks[planeB], fClustMatchSigmaFact[planeB]*sigmaT);
              float matchTol = std::min(fClustMatchMaxTicks, fClustMatchSigmaFact*sigmaT);
              if( fabs(dt) < matchTol ) { 
              
                map_clust_dt[j]       = dt;
                map_clust_dtfrac[j]   = dt/sigmaT;
                map_clust_overlap[j]  = overlapFrac;
                map_clust_score[j]    = score;
                cands[planeB].insert(j);

                //std::cout<<"  found match on plane "<<planeB<<": "<<j<<"    "<<dt<<"    "<<overlapFrac<<"\n";
              }
            
            }
            
              
            h_clust_overlap[planeB]->Fill(best_overlap);
            h_clust_dtfrac[planeB]->Fill(best_dTfrac);
            h_clust_dt[planeB]->Fill(best_dT);
            h_clust_score[planeB]->Fill(best_score);

          }//endloop over other planes
          
          // ---------------------------------------------------
          // loop over the candidates found on each plane
          // and select the one with the largest score
          //std::cout<<"Checking candidate matches to collPlane cluster "<<i<<"\n";
          if( cands.size() ) {
            
            for(auto c : cands ) {
              int plane = c.first;
              h_nmatches[plane]->Fill(c.second.size());
              //std::cout<<"  found "<<c.second.size()<<" on plane "<<c.first<<"\n";
              float bestOverlap = -999;
              int   bestID      = -9;
              for(auto cid : c.second) {
                if( map_clust_overlap[cid] > bestOverlap ) {
                  bestOverlap = map_clust_overlap[cid];
                  bestID = cid;
                }
              }
              if( bestID >= 0 ) {
                hcGroup.push_back(hitclust[bestID]);
                hitclust[bestID].isMatched = true;
                hitclust[i].isMatched = true;
                for(auto hit : hitclust[bestID].HitIDs) hitinfo[hit].ismatch = true;
                for(auto hit : hitclust[i].HitIDs) hitinfo[hit].ismatch = true;
                //std::cout<<"cands: "<<cands.size()<<"\n";
              }
            }

            // ----------------------------------------
            // make our new blip
            blip::Blip newBlip = BlipUtils::MakeBlip(hcGroup);

            // ----------------------------------------
            // if it isn't valid, forget it and move on
            if( !newBlip.isValid ) continue;

            // ----------------------------------------
            // save matching information
            //std::cout<<"Saving match info\n";
            for(auto hc : hcGroup ) {
              int pl = hc.Plane;
              newBlip.Match_dT[pl]      = map_clust_dt[hc.ID];
              newBlip.Match_dTfrac[pl]  = map_clust_dtfrac[hc.ID];
              newBlip.Match_overlap[pl] = map_clust_overlap[hc.ID];
              newBlip.Match_score[pl]   = map_clust_score[hc.ID];
            }

            // ----------------------------------------
            // if we are being picky...
            if( newBlip.NPlanes == kNplanes && newBlip.MaxIntersectDiff < 3){
            
              for(int ipl = 0; ipl < kNplanes; ipl++) {
                if( ipl == fCaloPlane ) continue;
                h_clust_picky_overlap[ipl]->Fill(newBlip.Match_overlap[ipl]);
                h_clust_picky_dtfrac[ipl] ->Fill(newBlip.Match_dTfrac[ipl]);
                h_clust_picky_dt[ipl]     ->Fill(newBlip.Match_dT[ipl]);
                h_clust_picky_score[ipl]  ->Fill(newBlip.Match_score[ipl]);
              }

            } else if ( fPickyBlips ) {
              continue;
            }
            
            // ----------------------------------------
            // apply cylinder cut 
            for(auto& trk : tracklist ){
              if( trk->Length() < fMaxHitTrkLength ) continue;
              auto& a = trk->Vertex();
              auto& b = trk->End();
              TVector3 p1(a.X(), a.Y(), a.Z() );
              TVector3 p2(b.X(), b.Y(), b.Z() );
              // TO-DO: if this track starts or ends at a TPC boundary, 
              // we should extend p1 or p2 to outside the AV to avoid blind spots
              
              TVector3 bp = newBlip.Position;
              float d = BlipUtils::DistToLine(p1,p2,bp);
              
              
              if( d > 0 ) {
                // update closest trkdist
                if( newBlip.TrkDist < 0 || d < newBlip.TrkDist ) {
                  newBlip.TrkDist = d;
                  newBlip.TrkID = trk->ID();
                }

                // need to do some math to figure out if this is in
                // the 45 degreee "cone" relative to the start/end 
                if( !newBlip.inCylinder && d < fCylinderRadius ) {
                  float angle1 = asin( d / (p1-bp).Mag() ) * 180./3.14159;
                  float angle2 = asin( d / (p2-bp).Mag() ) * 180./3.14159;
                  ///std::cout<<"d "<<d<<"  angles "<<angle1<<"  "<<angle2<<"\n";
                  if( angle1 < 45. && angle2 < 45. ) newBlip.inCylinder = true;
                }
              }
            }//endloop over trks
            
            if( fApplyTrkCylinderCut && newBlip.inCylinder ) continue;
            
            // ----------------------------------------
            // if we made it this far, the blip is good!
            newBlip.ID = blips.size();
            blips.push_back(newBlip);
            
            // associate this blip with the hits and clusters within it
            for(auto hc : hcGroup )           hitclust[hc.ID].BlipID = newBlip.ID;
            for(auto h : newBlip.HitID_set )  hitinfo[h].blipid = newBlip.ID;
          
          }

        }//endloop over caloplane ("Plane A") clusters
      }//endif calo plane has clusters
    }//endloop over TPCs
  
    //std::cout<<"Reconstructed "<<blips.size()<<" 3D blips\n";
  


    // =============================================================================
    // Calculate blip energy assuming T = T_beam (eventually can do more complex stuff
    // like associating blip with some nearby track/shower and using its tagged T0)
    //    Method 1: Assume a dE/dx = 2 MeV/cm for electrons, use that + local E-field
    //              calculate recombination.
    //    Method 2: ESTAR lookup table method ala ArgoNeuT
    // =============================================================================
    
    // Retrieve lifetime
    const lariov::UBElectronLifetimeProvider& elifetime_provider = art::ServiceHandle<lariov::UBElectronLifetimeService>()->GetProvider();
    float _electronLifetime = elifetime_provider.Lifetime() * /*convert ms->mus*/ 1e3;
    
    for(size_t i=0; i<blips.size(); i++){

      auto const* SCE = lar::providerFrom<spacecharge::SpaceChargeService>();
      float qColl = blips[i].Charge[fCaloPlane];
      float td    = blips[i].DriftTime;
      float depEl = qColl * exp( td / _electronLifetime ); 
      auto const blipPos = blips[i].Position;
      float Efield = detProp->Efield(0);
      if( SCE->EnableSimEfieldSCE() ) {
        geo::Point_t point = {double(blipPos.X()), double(blipPos.Y()), double(blipPos.Z())};
        auto const EfieldOffsets = SCE->GetEfieldOffsets(point);
        Efield *= std::hypot(1+EfieldOffsets.X(), EfieldOffsets.Y(), EfieldOffsets.Z());
      }
      
      // METHOD 1
      float dEdx = 2.0; // MeV/cm
      float recomb = BlipUtils::ModBoxRecomb(dEdx,Efield);
      blips[i].Energy = depEl * (1./recomb) * 23.6e-6;

      // METHOD 2
      //std::cout<<"Calculating ESTAR energy dep...  "<<depEl<<", "<<Efield<<"\n";
      //blips[i].EnergyESTAR = ESTAR->Interpolate(depEl, Efield); 

    }


    // ================================================
    // Save the true blip
    // ================================================
    for(size_t i=0; i<blips.size(); i++){
      auto& b = blips[i];
      float max = 0;
      for(auto clustID : b.ClustID_set ) {
        int edepid = hitclust[clustID].EdepID;
        if( edepid <= 0 ) continue;
        float E = trueblips[edepid].Energy;
        if( E > max ) {
          max = E;
          b.truth = trueblips[edepid];
        }
      }
    }
    
  
  }//End main blip reco function


  //###########################################################
  void BlipRecoAlg::PrintConfig() {
  
    printf("BlipRecoAlg Configurations\n\n");
    printf("  Input hit collection      : %s\n",          fHitProducer.c_str());
    printf("  Input trk collection      : %s\n",          fTrkProducer.c_str());
    printf("  Max wires per cluster     : %i\n",          fMaxWiresInCluster);
    printf("  Max cluster timespan      : %.1f ticks\n",    fMaxClusterSpan);
    //printf("  Max match window          : %3.1f ticks\n", fHitMatchMaxTicks);
    //printf("  Hit match RMS fact        : x%3.1f\n",      fHitMatchWidthFact);
    
    //printf("  Min cluster overlap       : %3.1f\n",       fClustMatchMinOverlap);
    printf("  Clust match sigma-factor  : %3.1f\n",       fClustMatchSigmaFact);
    printf("  Clust match max dT        : %3.1f ticks\n", fClustMatchMaxTicks);
    
    printf("  Min cluster overlap       : ");
    for(auto val : fClustMatchMinOverlap) { printf("%3.1f   ",val); } printf("\n");
    //printf("  Clust match sigma factor  : ");
    //for(auto val : fClustMatchSigmaFact) { printf("%3.1f   ",val); } printf("\n");
    //printf("  Clust match max ticks     : ");
    //for(auto val : fClustMatchMaxTicks) { printf("%3.1f   ",val); } printf("\n");
    
    //printf("  Track-cylinder radius     : %.1f cm\n",       fCylinderRadius);
    //printf("  Applying cylinder cut?    : %i\n",          fApplyTrkCylinderCut);
    //printf("  Picky blip mode?          : %i\n",        fPickyBlips);
    printf("\n");
    
  }
  
}
