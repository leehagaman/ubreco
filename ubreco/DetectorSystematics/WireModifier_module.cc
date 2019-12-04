////////////////////////////////////////////////////////////////////////
// Class:       WireModifier
// Plugin Type: producer (art v3_01_02)
// File:        WireModifier_module.cc
//
// Generated at Thu Aug 22 12:19:37 2019 by Wesley Ketchum using cetskelgen
// from cetlib version v3_05_01.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include <memory>

#include "lardataobj/RawData/RawDigit.h"
#include "lardataobj/RecoBase/Wire.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/Simulation/SimEnergyDeposit.h"
#include "larcoreobj/SimpleTypesAndConstants/PhysicalConstants.h"

#include "lardata/Utilities/AssociationUtil.h"

#include "TFile.h"
#include "TSpline.h"

namespace sys {
  class WireModifier;
}


class sys::WireModifier : public art::EDProducer {
public:
  explicit WireModifier(fhicl::ParameterSet const& p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  WireModifier(WireModifier const&) = delete;
  WireModifier(WireModifier&&) = delete;
  WireModifier& operator=(WireModifier const&) = delete;
  WireModifier& operator=(WireModifier&&) = delete;

  // Required functions.
  void produce(art::Event& e) override;

private:

  art::InputTag fWireInputTag;
  art::InputTag fSimEdepShiftedInputTag;
  art::InputTag fSimEdepOrigInputTag;
  art::InputTag fHitInputTag;
  bool          fMakeRawDigitAssn;
  bool          fCopyRawDigitAssn;

  std::string fSplinesFileName;
  std::vector<std::string> fSplineNames_Charge_X;
  std::vector<std::string> fSplineNames_Sigma_X;
  
  std::vector<TSpline3*> fTSplines_Charge_X;
  std::vector<TSpline3*> fTSplines_Sigma_X;

  bool fUseCollectiveEdepsForScales; //alternative is to use energy weighted scales per edep
  bool fApplyXScale;
  bool fApplyYScale;
  bool fApplyZScale;
  bool fApplyYZScale;
  bool fApplyAngleScales;
  bool fApplydEdXScale;

  //useful math things
  //static constexpr double ONE_OVER_SQRT_2PI = 1./std::sqrt(2*util::pi());
  double GAUSSIAN(double t, double mean,double sigma,double a=1.0){
    return ( (a/sigma /std::sqrt(2*util::pi()) * std::exp( -1.*(t-mean)*(t-mean)*0.5/sigma/sigma) ));
  }

  static constexpr double A_w = 3.33328;
  static constexpr double C_U = 338.140;
  static constexpr double C_V = 2732.53;
  static constexpr double C_Y = 4799.19;
  static constexpr double A_t = 18.2148;
  static constexpr double C_t = 818.351;
  static constexpr double SIN_SIXTY = std::sqrt(3)/2;
  static constexpr double COS_SIXTY = 0.5;


  typedef std::pair<unsigned int,unsigned int> ROI_Key_t;
  std::map< ROI_Key_t,std::vector<size_t> > fROIMatchedEdepMap;
  std::map< ROI_Key_t,std::vector<size_t> > fROIMatchedHitMap;
  
  typedef struct ROIProperties{
    ROI_Key_t key;
    unsigned int plane;
    float begin;
    float end;
    float total_q;
    float center;   //charge weighted center of ROI
    float sigma;    //charge weighted RMS of ROI
  } ROIProperties_t;

  typedef std::pair<ROI_Key_t, unsigned int> SubROI_Key_t;
  
  typedef struct SubROIProperties{
    SubROI_Key_t key;
    unsigned int plane;
    float total_q;
    float center;
    float sigma;
  } SubROIProperties_t;

  typedef struct ScaleValues{
    double r_Q;
    double r_sigma;
  } ScaleValues_t;

  typedef struct TruthProperties{
    float x;
    float x_rms;
    float x_rms_noWeight;
    float tick;
    float tick_rms;
    float tick_rms_noWeight;
    float total_energy;
    float x_min;
    float x_max;
    float tick_min;
    float tick_max;

    float y;
    float z;
    double dxdr;
    double dydr;
    double dzdr;
    double dedr;

    ScaleValues_t scales_avg[3];

  } TruthProperties_t;


  ROIProperties_t CalcROIProperties(recob::Wire::RegionsOfInterest_t::datarange_t const&);

  std::vector< std::pair<unsigned int, unsigned int> > GetTargetROIs(sim::SimEnergyDeposit const&);
  std::vector< std::pair<unsigned int, unsigned int> > GetHitTargetROIs(recob::Hit const&);
  
  void FillROIMatchedEdepMap(std::vector<sim::SimEnergyDeposit> const&, std::vector<recob::Wire> const&);
  void FillROIMatchedHitMap(std::vector<recob::Hit> const&, std::vector<recob::Wire> const&);

  std::vector<SubROIProperties_t> CalcSubROIProperties(ROIProperties_t const&, std::vector<const recob::Hit*> const&);

  std::map<SubROI_Key_t, std::vector<const sim::SimEnergyDeposit*>> MatchEdepsToSubROIs(std::vector<SubROIProperties_t> const&, std::vector<const sim::SimEnergyDeposit*> const&);

  TruthProperties_t CalcPropertiesFromEdeps(std::vector<const sim::SimEnergyDeposit*> const&);

  ScaleValues_t GetScaleValues(TruthProperties_t const&,ROIProperties_t const&);
  ScaleValues_t GetScaleValues(TruthProperties_t const&,unsigned int const&);

  void ModifyROI(std::vector<float> &,
		 ROIProperties_t const &, 
		 std::vector<SubROIProperties_t> const&,
		 std::map<SubROI_Key_t, ScaleValues_t> const&);

};


sys::WireModifier::ROIProperties_t 
sys::WireModifier::CalcROIProperties(recob::Wire::RegionsOfInterest_t::datarange_t const& roi)
{
  ROIProperties_t roi_vals;
  roi_vals.begin = roi.begin_index();
  roi_vals.end = roi.end_index();

  roi_vals.center=0;
  roi_vals.total_q=0;
  roi_vals.sigma=0;

  auto const& roi_data = roi.data();
  for(size_t i_t = 0; i_t<roi_data.size(); ++i_t){
    roi_vals.center += roi_data[i_t]*(i_t+roi_vals.begin);
    roi_vals.total_q += roi_data[i_t];
  }
  roi_vals.center = roi_vals.center/roi_vals.total_q;

  for(size_t i_t = 0; i_t<roi_data.size(); ++i_t) {
    roi_vals.sigma += roi_data[i_t]*(i_t+roi_vals.begin-roi_vals.center)*(i_t+roi_vals.begin-roi_vals.center);
  }
  roi_vals.sigma = std::sqrt(roi_vals.sigma/roi_vals.total_q);

  return roi_vals;
}

std::vector< std::pair<unsigned int, unsigned int> > 
sys::WireModifier::GetTargetROIs(sim::SimEnergyDeposit const& shifted_edep)
{
  //channel number, time tick
  std::vector< std::pair<unsigned int,unsigned int> > target_roi_vec;

  int edep_U_wire = std::round( A_w*(-SIN_SIXTY*shifted_edep.Y() + COS_SIXTY*shifted_edep.Z()) + C_U );
  int edep_V_wire = std::round( A_w*( SIN_SIXTY*shifted_edep.Y() + COS_SIXTY*shifted_edep.Z()) + C_V );
  int edep_Y_wire = std::round( A_w*shifted_edep.Z() + C_Y );
  int edep_tick   = std::round( A_t*shifted_edep.X() + C_t );

  if (edep_tick<0 || edep_tick>=6400)
    return target_roi_vec;

  if(edep_U_wire>=0 && edep_U_wire<2400)
    target_roi_vec.emplace_back((unsigned int)edep_U_wire,(unsigned int)edep_tick);

  if(edep_V_wire>=2400 && edep_V_wire<4800)
    target_roi_vec.emplace_back((unsigned int)edep_V_wire,(unsigned int)edep_tick);

  if(edep_Y_wire>=4800 && edep_Y_wire<8256)
    target_roi_vec.emplace_back((unsigned int)edep_Y_wire,(unsigned int)edep_tick);

  return target_roi_vec;
}

std::vector< std::pair<unsigned int, unsigned int> > 
sys::WireModifier::GetHitTargetROIs(recob::Hit const& hit)
{
  //vector of pairs of channel number, time tick
  std::vector< std::pair<unsigned int,unsigned int> > target_roi_vec;
  
  int hit_wire = hit.Channel();
  int hit_tick = int(round(hit.PeakTime()));
  
  if ( hit_tick<0 || hit_tick>=6400 )
    return target_roi_vec;

  target_roi_vec.emplace_back((unsigned int)hit_wire, (unsigned int)hit_tick);
  
  return target_roi_vec;
}

void sys::WireModifier::FillROIMatchedEdepMap(std::vector<sim::SimEnergyDeposit> const& edepVec,
					      std::vector<recob::Wire> const& wireVec)
{
  fROIMatchedEdepMap.clear();

  std::unordered_map<unsigned int,unsigned int> wireChannelMap;
  for(size_t i_w=0; i_w<wireVec.size(); ++i_w)
    wireChannelMap[wireVec[i_w].Channel()] = i_w;

  for(size_t i_e=0; i_e<edepVec.size(); ++i_e){

    std::vector< std::pair<unsigned int, unsigned int> > target_rois = GetTargetROIs(edepVec[i_e]); 

    for( auto const& target_roi : target_rois){

      //std::cout << "Matched edeps to channel " << target_roi.first << " time tick " << target_roi.second << std::endl;

      auto const& target_wire = wireVec.at(wireChannelMap[target_roi.first]);

      //if(target_roi.first!=target_wire.Channel())
      //throw std::runtime_error("ERROR! Channel ordering doesn't match wire ordering.");

      //std::cout << "\tGot wire " << target_wire.Channel() << std::endl;

      //std::cout << "\tWire has " << target_wire.SignalROI().n_ranges() << std::endl;

      if(target_wire.SignalROI().n_ranges()==0) continue;
      if(target_wire.SignalROI().is_void(target_roi.second)) continue;

      auto range_number = target_wire.SignalROI().find_range_iterator(target_roi.second) - target_wire.SignalROI().begin_range();

      fROIMatchedEdepMap[std::make_pair(target_wire.Channel(),range_number)].push_back(i_e);

    }//end loop over target rois

  }//end loop over all edeps

}

void sys::WireModifier::FillROIMatchedHitMap(std::vector<recob::Hit> const& hitVec,
					     std::vector<recob::Wire> const& wireVec)
{
  fROIMatchedHitMap.clear();
  
  std::unordered_map<unsigned int,unsigned int> wireChannelMap;
  for(size_t i_w=0; i_w<wireVec.size(); ++i_w)
    wireChannelMap[wireVec[i_w].Channel()] = i_w;
  
  for(size_t i_h=0; i_h<hitVec.size(); ++i_h){
    
    std::vector< std::pair<unsigned int, unsigned int> > target_rois = GetHitTargetROIs(hitVec[i_h]);
    
    for( auto const& target_roi : target_rois){
      
      auto const& target_wire = wireVec.at(wireChannelMap[target_roi.first]);
      
      if(target_wire.SignalROI().n_ranges()==0) continue;
      if(target_wire.SignalROI().is_void(target_roi.second)) continue;

      auto range_number = target_wire.SignalROI().find_range_iterator(target_roi.second) - target_wire.SignalROI().begin_range();
      
      fROIMatchedHitMap[std::make_pair(target_wire.Channel(),range_number)].push_back(i_h);
      
    }//end loop over target rois
    
  }//end loop over all hits
 
}

std::vector<sys::WireModifier::SubROIProperties_t>
sys::WireModifier::CalcSubROIProperties(sys::WireModifier::ROIProperties_t const& roi_properties, std::vector<const recob::Hit*> const& hitPtrVec) {

  std::vector<SubROIProperties_t> subroi_properties_vec;
  SubROIProperties_t subroi_properties;
  subroi_properties.plane = roi_properties.plane;

  // if this ROI doesn't contain any hits, define SubROI based on ROI properities
  if ( hitPtrVec.size() == 0 ) {
    subroi_properties.key     = std::make_pair( roi_properties.key, 0 );
    subroi_properties.total_q = roi_properties.total_q;
    subroi_properties.center  = roi_properties.center;
    subroi_properties.sigma   = roi_properties.sigma;
    subroi_properties_vec.push_back(subroi_properties);
  }

  // otherwise, define SubROIs based on hits
  else {
    for ( unsigned int i_h=0; i_h < hitPtrVec.size(); i_h++ ) {
      auto hit_ptr = hitPtrVec[i_h];
      subroi_properties.key     = std::make_pair( roi_properties.key, i_h );
      subroi_properties.total_q = hit_ptr->Integral();
      subroi_properties.center  = hit_ptr->PeakTime();
      subroi_properties.sigma   = hit_ptr->RMS();
      subroi_properties_vec.push_back(subroi_properties);
    } //end loop over hits
  }

  return subroi_properties_vec;

}

std::map<sys::WireModifier::SubROI_Key_t, std::vector<const sim::SimEnergyDeposit*>>
sys::WireModifier::MatchEdepsToSubROIs(std::vector<sys::WireModifier::SubROIProperties_t> const& subROIPropVec,
				       std::vector<const sim::SimEnergyDeposit*> const& edepPtrVec) {

  // for each TrackID, which EDeps are associated with it? keys are TrackIDs
  std::map<int, std::vector<const sim::SimEnergyDeposit*>> TrackIDMatchedEDepMap;
  // total energy of EDeps matched to the ROI (not strictly necessary, but useful for understanding/development)
  double total_energy = 0.;
  // loop over edeps, fill TrackIDMatchedEDepMap and calculate total energy
  for ( auto edep_ptr : edepPtrVec ) {
    TrackIDMatchedEDepMap[edep_ptr->TrackID()].push_back(edep_ptr);
    total_energy += edep_ptr->E();
  }
  // calculate EDep properties by TrackID
  std::map<int, sys::WireModifier::TruthProperties_t> TrackIDMatchedPropertyMap;
  for ( auto const& track_edeps : TrackIDMatchedEDepMap ) { TrackIDMatchedPropertyMap[track_edeps.first] = CalcPropertiesFromEdeps(track_edeps.second); }

  // for each EDep, which subROI(s) (if any) is it plausibly matched to? based on whether the EDep's projected tick is within +/-1sigma of the subROI center
  std::map<unsigned int, std::vector<unsigned int>> EDepMatchedSubROIMap;   // keys are indexes of edepPtrVec, values are vectors of indexes of subROIPropVec
  // for each TrackID, which subROI(s) (if any) are its EDeps matched to? based on whether any EDeps with that TrackID are within +/-1sigma of the subROI center
  std::map<int, std::unordered_set<unsigned int>> TrackIDMatchedSubROIMap;  // keys are TrackIDs, values are sets of indexes of subROIPropVec
  // for each subROI, which EDep(s) (if any) are matched to it? based on minimum distance matching
  std::map<unsigned int, std::vector<unsigned int>> SubROIMatchedEDepMap;   // keys are indexes of subROIPropVec, values are vectors of indexes of edepPtrVec
  // for each subROI, which TrackID(s) (if any) are matched? for each TrackID, how much energy is matched? based on minimum distance matching
  std::map<unsigned int, std::map<int, double>> SubROIMatchedTrackEnergyMap; // keys are indexes of subROIPropVec, values are maps of TrackIDs to matched energy (in MeV)


  // loop over EDeps
  for ( unsigned int i_e=0; i_e < edepPtrVec.size(); i_e++ ) {
    
    // get EDep properties
    auto edep_ptr  = edepPtrVec[i_e];
    auto edep_tick = A_t * edep_ptr->X() + C_t;

    // loop over subROIs  
    unsigned int closest_hit = 0;
    float min_dist = 100000.;
    for ( unsigned int i_h=0; i_h < subROIPropVec.size(); i_h++ ) {
      auto subroi_prop = subROIPropVec[i_h];
      if ( edep_tick > subroi_prop.center-subroi_prop.sigma && edep_tick < subroi_prop.center+subroi_prop.sigma ) {
	EDepMatchedSubROIMap[i_e].push_back(i_h);
	TrackIDMatchedSubROIMap[edep_ptr->TrackID()].emplace(i_h);
      }
      float hit_dist = std::abs( edep_tick - subroi_prop.center ) / subroi_prop.sigma;
      if ( hit_dist < min_dist ) {
	closest_hit = i_h;
	min_dist = hit_dist;
      }
    } // end loop over subROIs

    // if EDep is less than 2.5 units away from closest subROI, assign it to that subROI
    if ( min_dist < 2.5 ) {
      auto i_h = closest_hit;
      SubROIMatchedEDepMap[i_h].push_back(i_e);
      SubROIMatchedTrackEnergyMap[i_h][edep_ptr->TrackID()] += edep_ptr->E();
    }
    
  } // end loop over EDeps

  // convert to desired format (possibly a better way to do this...?)
  std::map<SubROI_Key_t, std::vector<const sim::SimEnergyDeposit*>> ReturnMap;
  for ( auto it_h = SubROIMatchedEDepMap.begin(); it_h != SubROIMatchedEDepMap.end(); it_h++ ) {
    auto key = subROIPropVec[it_h->first].key;
    for ( auto i_e : it_h->second ) {
      ReturnMap[key].push_back(edepPtrVec[i_e]);
    }
  }

  //if ( verbose == 0 ) return ReturnMap;

  /*
  // ** INITIAL INFORMATIONAL COUTS **
  std::cout << "Initial info..." << std::endl;
  // print out ROI properties
  //std::cout << "Channel " << roi_properties.key.first << ", signal ROI " << roi_properties.key.second << " (plane " << roi_properties.plane << ")" << std::endl;
  //std::cout << "  ROI bounds: (" << roi_properties.begin << ", " << roi_properties.end << ")" << std::endl;
  //std::cout << "  ROI center: " << roi_properties.center<< std::endl;
  //std::cout << "  ROI sigma:  " << roi_properties.sigma << std::endl;
  //std::cout << "  ROI charge: " << roi_properties.total_q << std::endl;
  
  // print out hit properties
  std::cout << "  Number of hits: " << subROIPropVec.size() << std::endl;
  float total_hit_charge = 0.;
  for ( auto subroi_prop : subROIPropVec ) total_hit_charge += subroi_prop.total_q;
  std::cout << "  Total hit charge: " << total_hit_charge << std::endl;
  for ( unsigned int i_h=0; i_h < subROIPropVec.size(); i_h++ ) { 
    auto subroi_prop = subROIPropVec[i_h];
    std::cout << "    For hit #" << i_h << ":" << std::endl;
    std::cout << "      Hit center: " << subroi_prop.center  << std::endl;
    std::cout << "      Hit width:  " << subroi_prop.sigma   << std::endl;
    std::cout << "      Hit charge: " << subroi_prop.total_q << std::endl;
  }

  // print out global edep properties
  auto edep_col_properties = CalcPropertiesFromEdeps(edepPtrVec);
  //std::cout << "  Global EDep center: " << edep_col_properties.tick << std::endl;
  //std::cout << "  Global EDep width:  " << edep_col_properties.tick_rms << " (with charge-weighted RMS)" << std::endl;
  //std::cout << "  Global EDep width:  " << edep_col_properties.tick_rms_noWeight << " (with non-charge-weighted RMS)" << std::endl;
  std::cout << "  Number of contributing EDeps: " << edepPtrVec.size() << std::endl;
  std::cout << "  Number of contributing TrackIDs: " << TrackIDMatchedEDepMap.size() << std::endl;
  std::cout << "  Total energy: " << total_energy << "  MeV" << std::endl;

  // print out EDep properties by TrackID...
  // for each TrackID, print center, width, and total energy
  int trk_ctr = 0;
  for ( auto trk_edep_pair : TrackIDMatchedEDepMap ) {
    std::cout << "    For track #" << trk_ctr << ":" << std::endl;
    std::cout << "      TrackID:   " << trk_edep_pair.first << std::endl;
    std::cout << "      Num EDeps: " << trk_edep_pair.second.size() << std::endl;
    std::cout << "      PDG code:  " << trk_edep_pair.second[0]->PdgCode() << std::endl;
    edep_col_properties = CalcPropertiesFromEdeps(trk_edep_pair.second);
    std::cout << "      TrackID EDep center:    " << edep_col_properties.tick << std::endl;
    //std::cout << "      TrackID EDep width:     " << edep_col_properties.tick_rms << " (with charge-weighted RMS)" << std::endl;
    //std::cout << "      TrackID EDep width:     " << edep_col_properties.tick_rms_noWeight << " (with non-charge-weighted RMS)" << std::endl; 
    std::cout << "      TrackID EDep min tick:  " << edep_col_properties.tick_min << std::endl;
    std::cout << "      TrackID EDep max tick:  " << edep_col_properties.tick_max << std::endl;
    std::cout << "      TrackID total energy:   " << edep_col_properties.total_energy << " MeV" << std::endl;
    trk_ctr++;
  }
  */

  /*
  // print out information on assigned EDeps
  std::cout << "  Info for assigned EDeps..." << std::endl;
  //for ( auto hit_trk_pair : HitMatchedTrackEnergyMap ) {
  for ( unsigned int i_h=0; i_h < subROIPropVec.size(); i_h++ ) {
    std::cout << "  Hit #" << std::setw(4) << i_h << ":";
    for ( auto trk_edep_pair : TrackIDMatchedEDepMap ) {
      std::cout << std::setw(10);
      auto trk = trk_edep_pair.first;
      auto it_trk = SubROIMatchedTrackEnergyMap[i_h].find(trk);
      if ( it_trk == SubROIMatchedTrackEnergyMap[i_h].end() ) std::cout << "--";
      else std::cout << it_trk->second;
    }
    std::cout << std::endl;
  }
  */
  
  // by construction, no unassiged EDeps after first pass
  /*
  // print out information about unassigned EDeps
  std::cout << "  Number of unassigned EDeps: " << UnassignedEDeps.size() << std::endl;
  for ( auto i_e : UnassignedEDeps ) {

    // get EDep properties
    auto edep_ptr  = edepPtrVec[i_e];
    auto edep_tick = A_t * edep_ptr->X() + C_t;

    std::cout << "    For EDep #" << i_e << ":" << std::endl;
    std::cout << "      TrackID:         " << edep_ptr->TrackID() << std::endl;
    std::cout << "      EDep tick:       " << edep_tick << std::endl;
    std::cout << "      EDep energy:     " << edep_ptr->E() << " MeV" << std::endl;
    std::cout << "      Matched to hits: ";
    if ( EDepMatchedHitMap[i_e].size() == 0 ) std::cout << "none" << std::endl;
    else { 
      for ( auto i_h : EDepMatchedHitMap[i_e] ) std::cout << i_h << " ";
      std::cout << std::endl;
    }
    std::cout << "      TrackID matched to " << TrackIDMatchedHitMap[edep_ptr->TrackID()].size() << " hits..." << std::endl;
    for ( auto i_h : TrackIDMatchedHitMap[edep_ptr->TrackID()] ) {
      std::cout << "        Hit #" << i_h << " has " << HitMatchedTrackEnergyMap[i_h][edep_ptr->TrackID()] << " MeV of the TrackID's energy" << std::endl;
    }
  }
  */

  return ReturnMap;

}

sys::WireModifier::TruthProperties_t 
sys::WireModifier::CalcPropertiesFromEdeps(std::vector<const sim::SimEnergyDeposit*> const& edepPtrVec){
  
  
  //split the edeps by TrackID
  std::map< int, std::vector<const sim::SimEnergyDeposit*> > edepptrs_by_trkid;
  std::map< int, double > energy_per_trkid;
  for(auto edep_ptr : edepPtrVec){
    edepptrs_by_trkid[edep_ptr->TrackID()].push_back(edep_ptr);
    energy_per_trkid[edep_ptr->TrackID()]+=edep_ptr->E();
  }
  
  int trkid_max=-1;
  double energy_max=-1;
  for(auto e_p_id : energy_per_trkid){
    //std::cout << "\t" << e_p_id.first << " :" << e_p_id.second << std::endl;
    if(e_p_id.second > energy_max){
      trkid_max = e_p_id.first;
      energy_max = e_p_id.second;
    }      
  }
  
  //std::cout << "TrkID with max energy (" << energy_max << ") is " << trkid_max << std::endl;
  auto edepPtrVecMaxE = edepptrs_by_trkid[trkid_max];
  
  //first, let's loop over all edeps and get an average weight scale...
  TruthProperties_t edep_props;
  double total_energy_all = 0.0;//,dx=0,dy=0,dz=0;
  
  ScaleValues_t scales_e_weighted[3];
  for(size_t i_p=0; i_p<3; ++i_p){
    scales_e_weighted[i_p].r_Q=0.0;
    scales_e_weighted[i_p].r_sigma=0.0;
  }
  for(auto edep_ptr : edepPtrVec){
    
    if (edep_ptr->StepLength()==0) continue;
    
    edep_props.x=edep_ptr->X();
    edep_props.y=edep_ptr->X();
    edep_props.z=edep_ptr->Z();
    
    edep_props.dxdr=(edep_ptr->EndX()-edep_ptr->StartX())/edep_ptr->StepLength();
    edep_props.dydr=(edep_ptr->EndY()-edep_ptr->StartY())/edep_ptr->StepLength();
    edep_props.dzdr=(edep_ptr->EndZ()-edep_ptr->StartZ())/edep_ptr->StepLength();
    
    edep_props.dedr = edep_ptr->E()/edep_ptr->StepLength();
    total_energy_all += edep_ptr->E();
    
    //std::cout << "\t\t" << edep_ptr->E() << " " << edep_ptr->X() << " ";

    for(size_t i_p=0; i_p<3; ++i_p){
      auto scales = GetScaleValues(edep_props,i_p);
      scales_e_weighted[i_p].r_Q += edep_ptr->E()*scales.r_Q;
      scales_e_weighted[i_p].r_sigma += edep_ptr->E()*scales.r_sigma;

      //std::cout << scales.r_Q << " " << scales.r_sigma << " ";

    }
    //std::cout << std::endl;

  }
  
  //std::cout << "\tAvg:" << total_energy_all << " \t ";
  for(size_t i_p=0; i_p<3; ++i_p){
    scales_e_weighted[i_p].r_Q = scales_e_weighted[i_p].r_Q/total_energy_all;
    scales_e_weighted[i_p].r_sigma = scales_e_weighted[i_p].r_sigma/total_energy_all;

    //std::cout<< scales_e_weighted[i_p].r_Q << " " <<scales_e_weighted[i_p].r_sigma << " ";
  }
  //std::cout << std::endl;
  
  TruthProperties_t edep_col_properties;
  
  //copy in the scales that were calculated before
  for(size_t i_p=0; i_p<3; ++i_p){
    edep_col_properties.scales_avg[i_p].r_Q = scales_e_weighted[i_p].r_Q;
    edep_col_properties.scales_avg[i_p].r_sigma = scales_e_weighted[i_p].r_sigma;
  }
  
  
  //do calculations here
  edep_col_properties.x = 0.;
  edep_col_properties.x_rms = 0.;
  edep_col_properties.x_rms_noWeight = 0.;
  edep_col_properties.x_min = 300.;
  edep_col_properties.x_max = -50.;
  
  edep_col_properties.y = 0.;
  edep_col_properties.z = 0.;
  edep_col_properties.dxdr = 0.;
  edep_col_properties.dydr = 0.;
  edep_col_properties.dzdr = 0.;
  
  double total_energy = 0.0;//,dx=0,dy=0,dz=0;
  for(auto edep_ptr : edepPtrVecMaxE){
    
    /*
      std::cout << "\t\t" << edep_ptr->E() << " " << edep_ptr->X() << " " << edep_ptr->Y() << " " << edep_ptr->Z()
      << " " << edep_ptr->EndX()-edep_ptr->StartX() << " " << edep_ptr->EndY()-edep_ptr->StartY() << " " << edep_ptr->EndZ()-edep_ptr->StartZ()
      << " " << (edep_ptr->EndX()-edep_ptr->StartX())/(edep_ptr->EndY()-edep_ptr->StartY())
      << " " << (edep_ptr->EndX()-edep_ptr->StartX())/(edep_ptr->EndZ()-edep_ptr->StartZ())
      << " " << (edep_ptr->EndY()-edep_ptr->StartY())/(edep_ptr->EndZ()-edep_ptr->StartZ())
      << " " << (edep_ptr->EndX()-edep_ptr->StartX())/edep_ptr->StepLength()
      << " " << (edep_ptr->EndY()-edep_ptr->StartY())/edep_ptr->StepLength()
      << " " << (edep_ptr->EndZ()-edep_ptr->StartZ())/edep_ptr->StepLength()
      << std::endl;
    */
    
    edep_col_properties.x += edep_ptr->X()*edep_ptr->E();
    if ( edep_ptr->X() < edep_col_properties.x_min ) edep_col_properties.x_min = edep_ptr->X();
    if ( edep_ptr->X() > edep_col_properties.x_max ) edep_col_properties.x_max = edep_ptr->X();
    total_energy += edep_ptr->E();
    
    edep_col_properties.y += edep_ptr->Y()*edep_ptr->E();
    edep_col_properties.z += edep_ptr->Z()*edep_ptr->E();
    
    
    if (edep_ptr->StepLength()==0) continue;
    
    /*
      dx = edep_ptr->EndX()-edep_ptr->StartX();
      dy = edep_ptr->EndY()-edep_ptr->StartY();
      dz = edep_ptr->EndZ()-edep_ptr->StartZ();
      
      edep_col_properties.dxdy += edep_ptr->E()*(edep_ptr->EndX()-edep_ptr->StartX())/(edep_ptr->EndY()-edep_ptr->StartY());
      edep_col_properties.dxdz += edep_ptr->E()*(edep_ptr->EndX()-edep_ptr->StartX())/(edep_ptr->EndZ()-edep_ptr->StartZ());
      edep_col_properties.dydz += edep_ptr->E()*(edep_ptr->EndY()-edep_ptr->StartY())/(edep_ptr->EndZ()-edep_ptr->StartZ());
    */
    
    edep_col_properties.dxdr += edep_ptr->E()*(edep_ptr->EndX()-edep_ptr->StartX())/edep_ptr->StepLength();
    edep_col_properties.dydr += edep_ptr->E()*(edep_ptr->EndY()-edep_ptr->StartY())/edep_ptr->StepLength();
    edep_col_properties.dzdr += edep_ptr->E()*(edep_ptr->EndZ()-edep_ptr->StartZ())/edep_ptr->StepLength();
    
  }
  
  if(total_energy>0.0){
    edep_col_properties.x = edep_col_properties.x/total_energy;
    
    edep_col_properties.y = edep_col_properties.y/total_energy;
    edep_col_properties.z = edep_col_properties.z/total_energy;

    /*
      edep_col_properties.dxdy = edep_col_properties.dxdy/total_energy;
      edep_col_properties.dxdz = edep_col_properties.dxdz/total_energy;
      edep_col_properties.dydz = edep_col_properties.dydz/total_energy;
    */
    edep_col_properties.dxdr = edep_col_properties.dxdr/total_energy;
    edep_col_properties.dydr = edep_col_properties.dydr/total_energy;
    edep_col_properties.dzdr = edep_col_properties.dzdr/total_energy;
    
  }
  
  for(auto edep_ptr : edepPtrVecMaxE) {
    edep_col_properties.x_rms += (edep_ptr->X()-edep_col_properties.x)*(edep_ptr->X()-edep_col_properties.x)*edep_ptr->E();
    edep_col_properties.x_rms_noWeight += (edep_ptr->X()-edep_col_properties.x)*(edep_ptr->X()-edep_col_properties.x);
  }
  
  edep_col_properties.x_rms_noWeight = std::sqrt(edep_col_properties.x_rms_noWeight);
  
  if(total_energy>0.0)
    edep_col_properties.x_rms = std::sqrt(edep_col_properties.x_rms/total_energy);
  
  // convert x-related proerties to ticks
  edep_col_properties.tick = A_t*edep_col_properties.x + C_t;
  edep_col_properties.tick_rms = A_t*edep_col_properties.x_rms;
  edep_col_properties.tick_rms_noWeight = A_t*edep_col_properties.x_rms_noWeight;
  edep_col_properties.tick_min = A_t*edep_col_properties.x_min + C_t;
  edep_col_properties.tick_max = A_t*edep_col_properties.x_max + C_t;
  
  edep_col_properties.total_energy = total_energy;
  
  /*
    std::cout << "Edep Props: " << std::endl;
    std::cout << "\tnedep=" << edepPtrVecMaxE.size() << std::endl;
    std::cout << "\te=" << edep_col_properties.total_energy << std::endl;
    std::cout << "\tx=" << edep_col_properties.x << std::endl;
    std::cout << "\ty=" << edep_col_properties.y << std::endl;
    std::cout << "\tz=" << edep_col_properties.z << std::endl;
    
    std::cout << "\tdxdy=" << edep_col_properties.dxdy << std::endl;
    std::cout << "\tdxdz=" << edep_col_properties.dxdz << std::endl;
    std::cout << "\tdydz=" << edep_col_properties.dydz << std::endl;
    
    std::cout << "\tdxdr=" << edep_col_properties.dxdr << std::endl;
    std::cout << "\tdydr=" << edep_col_properties.dydr << std::endl;
    std::cout << "\tdzdr=" << edep_col_properties.dzdr << std::endl;
  */
  
  
  
  return edep_col_properties;
}



sys::WireModifier::ScaleValues_t
sys::WireModifier::GetScaleValues(sys::WireModifier::TruthProperties_t const& truth_props, sys::WireModifier::ROIProperties_t const& roi_vals)
{
  return GetScaleValues(truth_props,roi_vals.plane);
}
sys::WireModifier::ScaleValues_t
sys::WireModifier::GetScaleValues(sys::WireModifier::TruthProperties_t const& truth_props, unsigned int const& plane)
{
  ScaleValues_t scales;
  
  scales.r_Q=1.;
  scales.r_sigma=1.;

  //get scales here
  if(plane==0){
    if(fApplyXScale){
      scales.r_Q *= fTSplines_Charge_X[0]->Eval(truth_props.x);
      scales.r_sigma *= fTSplines_Sigma_X[0]->Eval(truth_props.x);
    }
    if(fApplyYScale){    
    }
    if(fApplyZScale){    
    }
    if(fApplyYZScale){    
    }
    if(fApplyAngleScales){    
    }
    if(fApplydEdXScale){    
    }
  }
  else if(plane==1){
    if(fApplyXScale){
      scales.r_Q *= fTSplines_Charge_X[1]->Eval(truth_props.x);
      scales.r_sigma *= fTSplines_Sigma_X[1]->Eval(truth_props.x);
    }
    if(fApplyYScale){    
    }
    if(fApplyZScale){    
    }
    if(fApplyYZScale){    
    }
    if(fApplyAngleScales){    
    }
    if(fApplydEdXScale){    
    }
  }
  else if(plane==2){
    if(fApplyXScale){
      scales.r_Q *= fTSplines_Charge_X[2]->Eval(truth_props.x);
      scales.r_sigma *= fTSplines_Sigma_X[2]->Eval(truth_props.x);
    }
    if(fApplyYScale){    
    }
    if(fApplyZScale){    
    }
    if(fApplyYZScale){    
    }
    if(fApplyAngleScales){    
    }
    if(fApplydEdXScale){    
    }
  }
  
  //std::cout << "For plane=" << roi_vals.plane << " and x=" << truth_props.x
  //	    << " scales are " << scales.r_Q << " and " << scales.r_sigma << std::endl;
  
  return scales;
}

void sys::WireModifier::ModifyROI(std::vector<float> & roi_data,
				  sys::WireModifier::ROIProperties_t const& roi_prop,
				  std::vector<sys::WireModifier::SubROIProperties_t> const& subROIPropVec, 
				  std::map<sys::WireModifier::SubROI_Key_t, sys::WireModifier::ScaleValues_t> const& subROIScaleMap)
{
  
  bool verbose=false;
  //if(roi_data.size()>100) verbose=true;
  
  double q_orig = 0.;
  double q_mod = 0.;
  double scale_ratio = 1.;
  
  for(size_t i_t=0; i_t < roi_data.size(); i_t++) {
    
    // reset q_orig, q_mod, scale_ratio
    q_orig = 0.;
    q_mod = 0.;
    scale_ratio = 1.;
    
    // calculate q_orig, q_mod for this tick
    for ( auto const& subroi_prop : subROIPropVec ) {
      auto scale_vals = subROIScaleMap.find(subroi_prop.key)->second;
      q_orig += GAUSSIAN( i_t+roi_prop.begin,
			  subroi_prop.center,
			  subroi_prop.sigma,
			  subroi_prop.total_q );
      q_mod  += GAUSSIAN( i_t+roi_prop.begin,
			  subroi_prop.center,
			  scale_vals.r_sigma * subroi_prop.sigma, 
			  scale_vals.r_Q     * subroi_prop.total_q );
      
    }
    
    if(isnan(q_orig)) {
      std::cout << "WARNING: obtained q_orig = NaN... setting to zero" << std::endl;
      q_orig = 0.;
    }
    if(isnan(q_mod)) {
      std::cout << "WARNING: obtained q_mod = NaN... setting to zero" << std::endl;
      q_mod = 0.;
    }
    
    scale_ratio = q_mod / q_orig;
    
    if(isnan(scale_ratio)) {
      std::cout << "WARNING: obtained scale_ratio = " << q_mod << " / " << q_orig << " = NaN... setting to 1" << std::endl;
      scale_ratio = 1.;
    }
    if(isinf(scale_ratio)) {
      std::cout << "WARNING: obtained scale_ratio = " << q_mod << " / " << q_orig << " = inf... setting to 1" << std::endl;
      scale_ratio = 1.0;
    }
    
    roi_data[i_t] = scale_ratio * roi_data[i_t];
    
    if(verbose)
      std::cout << "\t tick " << i_t << ":"
		<< " data=" << roi_data[i_t]
		<< ", q_orig=" << q_orig
		<< ", q_mod=" << q_mod
		<< ", ratio=" << scale_ratio
		<< std::endl;
  }
  
  return;
  
}

sys::WireModifier::WireModifier(fhicl::ParameterSet const& p)
  : EDProducer{p},
  fWireInputTag(p.get<art::InputTag>("WireInputTag")),
  fSimEdepShiftedInputTag(p.get<art::InputTag>("SimEdepShiftedInputTag")),
  fSimEdepOrigInputTag(p.get<art::InputTag>("SimEdepOrigInputTag")),
  fHitInputTag(p.get<art::InputTag>("HitInputTag")),
  fMakeRawDigitAssn(p.get<bool>("MakeRawDigitAssn",true)),
  fCopyRawDigitAssn(p.get<bool>("CopyRawDigitAssn",false)),
  fSplinesFileName(p.get<std::string>("SplinesFileName")),
  fSplineNames_Charge_X(p.get< std::vector<std::string> >("SplineNames_Charge_X")),
  fSplineNames_Sigma_X(p.get< std::vector<std::string> >("SplineNames_Sigma_X")),
  fUseCollectiveEdepsForScales(p.get<bool>("UseCollectiveEdepsForScales",false)),
  fApplyXScale(p.get<bool>("ApplyXScale",true)),
  fApplyYScale(p.get<bool>("ApplyYScale",false)),
  fApplyZScale(p.get<bool>("ApplyZScale",false)),
  fApplyYZScale(p.get<bool>("ApplyYZScale",true)),
  fApplyAngleScales(p.get<bool>("ApplyAngleScales",true)),
  fApplydEdXScale(p.get<bool>("ApplydEdXScale",true))
				 
{
  produces< std::vector< recob::Wire > >();
    
  if(fMakeRawDigitAssn)
    produces< art::Assns<raw::RawDigit,recob::Wire> >();


  fSplinesFileName = std::string(std::getenv("UBOONEDATA_DIR"))+"/systematics/det_sys/"+fSplinesFileName;
  std::cout << "Spline file is " << fSplinesFileName;

  TFile f_splines(fSplinesFileName.c_str(),"r");

  fTSplines_Charge_X.resize(fSplineNames_Charge_X.size());
  for(size_t i_s=0; i_s<fSplineNames_Charge_X.size(); ++i_s)
    f_splines.GetObject(fSplineNames_Charge_X[i_s].c_str(),fTSplines_Charge_X[i_s]);
  
  fTSplines_Sigma_X.resize(fSplineNames_Sigma_X.size());
  for(size_t i_s=0; i_s<fSplineNames_Sigma_X.size(); ++i_s)
    f_splines.GetObject(fSplineNames_Sigma_X[i_s].c_str(),fTSplines_Sigma_X[i_s]);

  //ifApplyYZ, don't applyY and applyZ separately
  if(fApplyYZScale){
    fApplyYScale=false;
    fApplyZScale=false;
  }

}

void sys::WireModifier::produce(art::Event& e)
{

  //get wires
  art::Handle< std::vector<recob::Wire> > wireHandle;
  e.getByLabel(fWireInputTag, wireHandle);
  auto const& wireVec(*wireHandle);

  //get association to rawdigit
  art::FindManyP<raw::RawDigit> digit_assn(wireHandle,e,fWireInputTag);

  //get sim edeps
  art::Handle< std::vector<sim::SimEnergyDeposit> > edepShiftedHandle;
  e.getByLabel(fSimEdepShiftedInputTag,edepShiftedHandle);
  auto const& edepShiftedVec(*edepShiftedHandle);
  
  art::Handle< std::vector<sim::SimEnergyDeposit> > edepOrigHandle;
  e.getByLabel(fSimEdepOrigInputTag,edepOrigHandle);
  auto const& edepOrigVec(*edepOrigHandle);

  // get hits
  art::Handle< std::vector<recob::Hit> > hitHandle;
  e.getByLabel(fHitInputTag, hitHandle);
  auto const& hitVec(*hitHandle);

  //output new wires and new associations
  std::unique_ptr< std::vector<recob::Wire> > new_wires(new std::vector<recob::Wire>());
  std::unique_ptr< art::Assns<raw::RawDigit,recob::Wire> > new_digit_assn(new art::Assns<raw::RawDigit,recob::Wire>());

  //first fill our roi to edep map
  FillROIMatchedEdepMap(edepShiftedVec,wireVec);
  // and fill our roi to hit map
  FillROIMatchedHitMap(hitVec,wireVec);

  //loop through the wires and rois per wire...
  for(size_t i_w=0; i_w<wireVec.size(); ++i_w){

    auto const& wire = wireVec[i_w];

    //make a new roi list
    recob::Wire::RegionsOfInterest_t new_rois;
    new_rois.resize(wire.SignalROI().size());

    unsigned int my_plane=wire.View();

    for(size_t i_r=0; i_r<wire.SignalROI().get_ranges().size(); ++i_r){

      auto const& range = wire.SignalROI().get_ranges()[i_r];
      ROI_Key_t roi_key(wire.Channel(),i_r);

      //get the matching edeps
      auto it_map = fROIMatchedEdepMap.find(roi_key);
      if(it_map==fROIMatchedEdepMap.end()){
	new_rois.add_range(range.begin_index(),range.data());
	continue;
      }
      std::vector<size_t> matchedEdepIdxVec = it_map->second;      
      if(matchedEdepIdxVec.size()==0){
	new_rois.add_range(range.begin_index(),range.data());
	continue;
      }
      std::vector<const sim::SimEnergyDeposit*> matchedEdepPtrVec;
      std::vector<const sim::SimEnergyDeposit*> matchedShiftedEdepPtrVec;
      for(auto i_e : matchedEdepIdxVec) {
	matchedEdepPtrVec.push_back(&edepOrigVec[i_e]);
	matchedShiftedEdepPtrVec.push_back(&edepShiftedVec[i_e]);
      }


      // get the matching hits
      std::vector<const recob::Hit*> matchedHitPtrVec;
      auto it_hit_map = fROIMatchedHitMap.find(roi_key);
      if( it_hit_map != fROIMatchedHitMap.end() ) {
	for( auto i_h : it_hit_map->second ) {
	  matchedHitPtrVec.push_back(&hitVec[i_h]);
	}
      }

      //calc roi properties
      auto roi_properties = CalcROIProperties(range);
      roi_properties.key   = roi_key;
      roi_properties.plane = my_plane;

      std::cout << "DOING WIRE ROI (wire=" << wire.Channel() << ", roi_idx=" << i_r
		<< ", roi_begin=" << roi_properties.begin << ", roi_size=" << roi_properties.end-roi_properties.begin << ")" << std::endl;
      std::cout << "  Have " << matchedEdepPtrVec.size() << " matching Edeps" << std::endl;
      std::cout << "  Have " << matchedHitPtrVec.size() << " matching hits" << std::endl;

      // get the subROIs
      auto subROIPropVec = CalcSubROIProperties(roi_properties, matchedHitPtrVec);
      std::cout << "  Have " << subROIPropVec.size() << " subROIs" << std::endl;

      // get the edeps per subROI
      auto SubROIMatchedShiftedEdepMap = MatchEdepsToSubROIs(subROIPropVec, matchedShiftedEdepPtrVec);
      // convert from shifted edep pointers to original edep pointers
      std::map<SubROI_Key_t, std::vector<const sim::SimEnergyDeposit*>> SubROIMatchedEdepMap;
      for ( auto const& key_edepPtrVec_pair : SubROIMatchedShiftedEdepMap ) {
	auto key = key_edepPtrVec_pair.first;
	for ( auto const& shifted_edep_ptr : key_edepPtrVec_pair.second ) {
	  for ( unsigned int i_e=0; i_e < matchedShiftedEdepPtrVec.size(); i_e++ ) {
	    if ( shifted_edep_ptr == matchedShiftedEdepPtrVec[i_e] ) {
	      SubROIMatchedEdepMap[key].push_back(matchedEdepPtrVec[i_e]);
	      break;
	    }
	  }
	}
      } // end conversion
      for ( auto const& pair : SubROIMatchedEdepMap ) std::cout << "  For subROI #" << pair.first.second << ", have " 
								<< pair.second.size() << " matching Edeps" << std::endl;

      //get the scaling values
      //std::map<SubROI_Key_t, TruthProperties_t> SubROIMatchedTruthMap;
      std::map<SubROI_Key_t, ScaleValues_t>     SubROIMatchedScalesMap;
      for ( auto const& subroi_prop : subROIPropVec ) {
	ScaleValues_t scale_vals;
	auto key = subroi_prop.key;
	auto key_it =  SubROIMatchedEdepMap.find(key);
	if ( key_it == SubROIMatchedEdepMap.end() ){
	  scale_vals.r_Q     = 1.;
	  scale_vals.r_sigma = 1.;
	}
	else {
	  auto truth_vals = CalcPropertiesFromEdeps(key_it->second);
	  if(fUseCollectiveEdepsForScales) //use bulk properties of edeps to determine scale
	    scale_vals = GetScaleValues(truth_vals, roi_properties);
	  else //use the energy-weighted average scale values per edep
	    scale_vals = truth_vals.scales_avg[roi_properties.plane];
	}
	SubROIMatchedScalesMap[key] = scale_vals;
      }
      for ( auto const& key_scale_pair : SubROIMatchedScalesMap ) {
        std::cout << "  For subroi #" << key_scale_pair.first.second << ", have "
                    << "scale factors r_Q = " << key_scale_pair.second.r_Q << " and r_sigma = " << key_scale_pair.second.r_sigma << std::endl;
      }

      //get modified ROI given scales
      std::vector<float> modified_data(range.data());
      ModifyROI(modified_data, roi_properties, subROIPropVec, SubROIMatchedScalesMap);
      
      new_rois.add_range(roi_properties.begin,modified_data);
      
    }//end loop over rois
    
    //make our new wire object
    new_wires->emplace_back(new_rois,wire.Channel(),wire.View());

    
    //get the associated rawdigit
    if(fCopyRawDigitAssn){
      auto const& rd_ptrs = digit_assn.at(i_w);
      for(auto const& rd_ptr : rd_ptrs)
	util::CreateAssn(*this,e,*new_wires,rd_ptr,*new_digit_assn,new_wires->size()-1);
    }

  }//end loop over wires

  e.put(std::move(new_wires));

  if(fMakeRawDigitAssn)
    e.put(std::move(new_digit_assn));
  
  //get a list of ptrs to the matched edeps for a wire
}
DEFINE_ART_MODULE(sys::WireModifier)

