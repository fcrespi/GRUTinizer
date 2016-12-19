#include "TRuntimeObjects.h"
#include "e441.h"

static const double m_target = mg24.GetMass();

// ----------------------------------------------------------------------
// extern "C" is needed to prevent name mangling.
// The function signature must be exactly as shown here,
//   or else bad things will happen.
extern "C"
void MakeHistograms(TRuntimeObjects& obj) {
  LoadCuts();
  LoadRaytraceParams(3,1,1);

  TCagra* cagra = obj.GetDetector<TCagra>();
  TGrandRaiden* gr = obj.GetDetector<TGrandRaiden>();

  if (gr) {
    MakeGrandRaidenHistograms(obj,*gr);
  }
  if (cagra) {
    MakeCAGRAHistograms(obj,*cagra);
  }
  if (cagra && gr) {
    MakeCoincidenceHistograms(obj, *cagra, *gr);
  }

}
// ----------------------------------------------------------------------

void PoleZeroHistos(TRuntimeObjects& obj, TCagraHit& core_hit, string local_dirname = "") {
  auto flags = core_hit.GetFlags();
  if (TANLEvent::PileUpFlag(flags) || TANLEvent::PileUpOnlyFlag(flags)) { return; }

  int detector = core_hit.GetDetnum();
  string chan = core_hit.GetLeaf() + std::to_string(core_hit.GetSegnum());

  Double_t prerise = core_hit.GetPreRise()/TANLEvent::GetShapingTime();
  stream.str("");  stream << "Prerise[Q]_" << detector << "_" << chan;
  obj.FillHistogram(local_dirname, stream.str(),2000,0,10000,core_hit.GetCharge(),1250,6000,8500,prerise);
  for (auto& seg_hit : core_hit) {
    string seg_chan = seg_hit.GetLeaf() + std::to_string(seg_hit.GetSegnum());
    stream.str("");  stream << "Prerise[Q]_" << detector << "_" << seg_chan;
    obj.FillHistogram(local_dirname, stream.str(),2000,-10000,0,seg_hit.GetCharge(),1250,6000,8500,prerise);
  }
  // stream.str("");  stream << "Q[Prerise]_" << detector << "_" << chan;
  // obj.FillHistogram(local_dirname, stream.str(),1250,6000,8500,prerise,3000,0,6000,core_hit.GetCharge());
  // for (auto& seg : core_hit) {
  //   string seg_chan = seg_hit.GetLeaf() + std::to_string(seg_hit.GetSegnum());
  //   stream.str("");  stream << "Q[Prerise]Seg_" << detector << "_" << seg_chan;
  //   obj.FillHistogram(local_dirname, stream.str(),1250,6000,8500,prerise,3000,0,6000,seg.GetCharge());
  // }

  stream.str("");  stream << "Prerise[E_pzcor_basesample]_" << detector << "_" << chan;
  obj.FillHistogram(local_dirname, stream.str(),3000,0,6000,core_hit.GetCorrectedEnergy(core_hit.GetBaseSample()),1250,6000,8500,prerise);
  stream.str("");  stream << "Prerise[E_pzcor_constant]_" << detector << "_" << chan;
  obj.FillHistogram(local_dirname, stream.str(),3000,-2000,4000,core_hit.GetCorrectedEnergy(),1250,6000,8500,prerise);


  stream.str(""); stream << "E_pzcor_constant" << detector << "_" << chan;
  auto pzchan = core_hit.GetCorrectedEnergy();
  obj.FillHistogram(local_dirname,stream.str(),4000,0,12000,pzchan);
  for (auto& seg_hit : core_hit) {
    string seg_chan = seg_hit.GetLeaf() + std::to_string(seg_hit.GetSegnum());
    stream.str(""); stream << "E_pzcor_constant" << detector << "_" << seg_chan;
    obj.FillHistogram(local_dirname,stream.str(),4000,0,12000,seg_hit.GetCorrectedEnergy());
  }

}

void PileUp (TRuntimeObjects& obj, TCagraHit& core_hit) {
  dirname = "PileUp";

  auto flags = core_hit.GetFlags();
  auto pileup = TANLEvent::PileUpFlag(flags);
  if (pileup) {
    obj.FillHistogram(dirname,"Summary",10,1,10,3);
    PoleZeroHistos(obj,core_hit,dirname);
  } else {
    obj.FillHistogram(dirname,"Summary",10,1,10,7);
    PoleZeroHistos(obj,core_hit,"NoPileUp");
  }
}


void MakeCAGRAHistograms(TRuntimeObjects& obj, TCagra& cagra) {

  for (auto& core_hit : cagra) {

    //PileUp(obj,core_hit);

    int detector = core_hit.GetDetnum();
    char core_leaf = core_hit.GetLeaf();
    string chan = core_leaf + std::to_string(core_hit.GetSegnum());
    size_t crystal_id = TCagra::GetCrystalId(detector,core_leaf);

    // cagra core energy summary
    obj.FillHistogram("CrystalEnergySummary",
                      6000,-2000,22000,core_hit.GetCorrectedEnergy(),
                      49,0,49,crystal_id);
    obj.FillHistogram("EgammaSum",7000,0,21000,core_hit.GetCorrectedEnergy());


    static ULong_t first_ts = 0;
    if (first_ts <= 1e6){  first_ts = core_hit.Timestamp(); std::cout << "Timestamp: " << first_ts << "\n" << std::endl; }
    else {
      // cagra core time summary
      obj.FillHistogram("CrystalTimeSummary",
                        1000,0,4000,(core_hit.Timestamp()-first_ts)*10/1.0e9, // in seconds - 1 bin = 4 seconds
                        49,0,49,crystal_id);

      obj.FillHistogram("NumEvents","cagra_hits_time",1000,0,8000,(core_hit.Timestamp()-first_ts)*10/1.0e9);
    }


    auto position = core_hit.GetPosition(pos::core_only);
    obj.FillHistogram("ArrayHits",
                      180,0,180,position.Theta()*180/TMath::Pi(),
                      360,-180,180,position.Phi()*180/TMath::Pi());


    PoleZeroHistos(obj,core_hit,"PoleZero");


    // central contact signals
    // name = "Det_" + std::to_string(detector) + "_" + chan;
    // obj.FillHistogram("CAGRA_Raw", name,2000,0,0,core_hit.GetCharge());

    if (core_hit.GetChannel() == 4 && core_hit.GetSystem()=='Y') {
      name = "BGO_" + std::to_string(detector) + "_" + chan;
      obj.FillHistogram("BGO", name,2000,0,0,core_hit.GetCharge());
    }


    // // segment (side channel) signals
    // for (auto& segment : core_hit) {
    //   string seg_chan = segment.GetLeaf() + std::to_string(segment.GetSegnum());
    //   stream.str(""); stream << "Det_" << detector << "_" << seg_chan;
    //   obj.FillHistogram("CAGRA_Raw",stream.str(),2000,0,0,segment.GetCharge());
    // }


    // same but for calibrated energies
    // stream.str("");
    // stream << "Det_" << detector << "_" << chan;
    // obj.FillHistogram("CAGRA_Calibrated",stream.str(),2000,0,10000,core_hit.GetEnergy());
    // for (auto& segment : core_hit) {
    //   string seg_chan = segment.GetLeaf() + std::to_string(segment.GetSegnum());
    //   stream.str(""); stream << "Det_" << detector << "_" << seg_chan;
    //   obj.FillHistogram("CAGRA_Calibrated",stream.str(),2000,0,10000,segment.GetEnergy());
    // }


  } // end loop over cagra hits

}
void MakeGrandRaidenHistograms(TRuntimeObjects& obj, TGrandRaiden& gr) {

  std::function<void(std::string)> fp_corrections;

  for (auto& hit : gr) {

    auto& rcnp = hit.GR();



    if (rcnp.GR_MYRIAD(0) != BAD_NUM) {
      obj.FillHistogram("Timing","MyriadTimestamp",10000,1e9,5e12,hit.GetTimestamp());
    }

    static ULong_t prev_ts = 0;
    if (prev_ts) {
      obj.FillHistogram("Timing","GR_EventPeriod",5000,100,50000,hit.GetTimestamp()-prev_ts);
    }
    prev_ts = hit.GetTimestamp();

    auto rf = rcnp.GR_RF(0);
    if (rf != BAD_NUM) {
      obj.FillHistogram("GR","GR_RF",1000,0,0,rf);
    }


    // X, A, Y, B, RF, DE1, DE2
    // X[A],X[Y],X[B],X[RF],DE1[X],DE2[X]
    // Y[A],Y[B],Y[RF],DE1[Y],DE2[Y]
    // A[B], A[RF], DE1[A], DE2[A]
    // B[RF], DE1[B], DE2[B]
    // DE1[RF], DE2[RF], DE1[DE2]

    obj.FillHistogram("GR","RayID",64,-16,48, rcnp.GR_RAYID(0));
    if (rcnp.GR_RAYID(0) == 0) { // if track reconstruction successful
      obj.FillHistogram("GR","X",1200,-600,600, rcnp.GR_X(0));
      obj.FillHistogram("GR","X_cal",1000,0,20, rcnp.GR_X(0)*0.01074+6.872);
      obj.FillHistogram("GR","Y",200,-100,100, rcnp.GR_Y(0));
      obj.FillHistogram("GR","A",100,-1,1, rcnp.GR_TH(0)); // need to learn
      obj.FillHistogram("GR","B",100,-1,1, rcnp.GR_PH(0)); // from hist.def
      obj.FillHistogram("GR","A[X]",1200,-600,600,rcnp.GR_X(0),1000,-1,1,rcnp.GR_TH(0));
      obj.FillHistogram("GR","Y[A]",600,-0.15,0.15,rcnp.GR_TH(0),500,-50,50,rcnp.GR_Y(0));
      obj.FillHistogram("GR","Y[B]",500,-0.1,0.1,rcnp.GR_PH(0),500,-50,50,rcnp.GR_Y(0));


      dirname = "GR_new";
      obj.FillHistogram(dirname,"X[A]",300,-0.15,0.15,rcnp.GR_TH(0),1200,-600,600,rcnp.GR_X(0));
      obj.FillHistogram(dirname,"X[Y]",200,-100,100,rcnp.GR_Y(0),1200,-600,600,rcnp.GR_X(0));
      obj.FillHistogram(dirname,"X[B]",250,-0.1,0.1,rcnp.GR_PH(0),1200,-600,600,rcnp.GR_X(0));
      //obj.FillHistogram(dirname,"X[RF]",500,0,0,rcnp.GR_RF(0),1200,-600,600,rcnp.GR_X(0));
      obj.FillHistogram(dirname,"RF[A]",1000,-1,1,rcnp.GR_TH(0),500,0,0,rcnp.GR_RF(0));

      auto rf_Acor = rcnp.GR_RF(0)-(-1914.5*rcnp.GR_TH(0));
      obj.FillHistogram(dirname,"RF_Acor[A]",1000,-1,1,rcnp.GR_TH(0),500,0,0,rf_Acor);
      obj.FillHistogram(dirname,"RF[X]",1200,-600,600,rcnp.GR_X(0),500,0,0,rcnp.GR_RF(0));
      obj.FillHistogram(dirname,"RF_Acor[X]",1200,-600,600,rcnp.GR_X(0),500,0,0,rf_Acor);
      auto rf_Acor_Xcor = rf_Acor - (0.17205*rcnp.GR_X(0));
      obj.FillHistogram(dirname,"RF_Acor_Xcor[X]",1200,-600,600,rcnp.GR_X(0),500,0,0,rf_Acor_Xcor);
      obj.FillHistogram(dirname,"DE1[X]",1200,-600,600,rcnp.GR_X(0),2000,0,2000, hit.GetMeanPlastE1());
      obj.FillHistogram(dirname,"DE2[X]",1200,-600,600,rcnp.GR_X(0),2000,0,2000, hit.GetMeanPlastE2());
      obj.FillHistogram(dirname,"DE3[X]",1200,-600,600,rcnp.GR_X(0),2000,0,2000, hit.GetMeanPlastE3());

      obj.FillHistogram(dirname,"Y[A]",300,-0.15,0.15,rcnp.GR_TH(0),200,-100,100,rcnp.GR_Y(0));
      obj.FillHistogram(dirname,"Y[B]",250,-0.1,0.1,rcnp.GR_PH(0),200,-100,100,rcnp.GR_Y(0));
      obj.FillHistogram(dirname,"Y[X]",1200,-600,600,rcnp.GR_X(0),200,-100,100,rcnp.GR_Y(0));
      obj.FillHistogram(dirname,"Y[X]",1200,-600,600,rcnp.GR_X(0),200,-100,100,rcnp.GR_Y(0));
      obj.FillHistogram(dirname,"Y[X]",1200,-600,600,rcnp.GR_X(0),200,-100,100,rcnp.GR_Y(0));
      obj.FillHistogram(dirname,"Y[X]",1200,-600,600,rcnp.GR_X(0),200,-100,100,rcnp.GR_Y(0));

      obj.FillHistogram(dirname,"A[RF]",500,700,1200,rf,1000,-1,1, rcnp.GR_TH(0));


      auto ycor = rcnp.GR_Y(0)+892.46*rcnp.GR_PH(0);
      obj.FillHistogram(dirname,"Y[B]cor",250,-0.1,0.1,rcnp.GR_PH(0),200,-100,100,ycor);
      if (ycor<11 && ycor >-8) {
        obj.FillHistogram("GR","A[X]_gateYcor",1200,-600,600,rcnp.GR_X(0),1000,-1,1,rcnp.GR_TH(0));
        obj.FillHistogram(dirname,"Y[B]cor",250,-0.1,0.1,rcnp.GR_PH(0),200,-100,100,ycor);

      }

      obj.FillHistogram(dirname,"DE1[RF]",1000,0,0,rf,2000,0,2000, hit.GetMeanPlastE1());
      obj.FillHistogram(dirname,"DE2[RF]",1000,0,0,rf,2000,0,2000, hit.GetMeanPlastE2());
      obj.FillHistogram(dirname,"DE3[RF]",1000,0,0,rf,2000,0,2000, hit.GetMeanPlastE3());
      
      obj.FillHistogram(dirname,"DE1[RF_Acor_Xcor]",500,0,0,rf_Acor_Xcor,1000,0,2000, hit.GetMeanPlastE1());
      obj.FillHistogram(dirname,"DE2[RF_Acor_Xcor]",500,0,0,rf_Acor_Xcor,1000,0,2000, hit.GetMeanPlastE2());
      obj.FillHistogram(dirname,"DE1[dE2]",2000,0,2000, hit.GetMeanPlastE2(),2000,0,2000, hit.GetMeanPlastE1());
      obj.FillHistogram(dirname,"DE2[DE3]",2000,0,2000, hit.GetMeanPlastE2(),2000,0,2000, hit.GetMeanPlastE3());
      
      obj.FillHistogram(dirname,"dE1[A]",1000,-1,1, rcnp.GR_TH(0),2000,0,2000, hit.GetMeanPlastE1());
      obj.FillHistogram(dirname,"dE2[A]",1000,-1,1, rcnp.GR_TH(0),2000,0,2000, hit.GetMeanPlastE2());
      obj.FillHistogram(dirname,"dE3[A]",1000,-1,1, rcnp.GR_TH(0),2000,0,2000, hit.GetMeanPlastE3());


      // raytracing
      double A=0,B=0;
      std::tie(A,B) = hit.Raytrace();
      obj.FillHistogram(dirname,"B[A]",500,0,0,A,500,0,0,B);

      // for (auto& cut : xcuts) {
      //   if (x < cut+xwidth && x >= cut) {
      //     stream.str(""); stream << "_x[" << cut << "," << cut+xwidth << ")";
      //     obj.FillHistogram(dirname,"B[A]"+stream.str(),500,0,0,A,500,0,0,B);
      //   }
      // }


      auto ejectile = hit.GetEjectileVector();
      // missing mass
      ejectile.SetMag(hit.GetMomentum());
      obj.FillHistogram("MissingMass","ThetaLab",300,0,0,ejectile.Theta());
      obj.FillHistogram("MissingMass","PhiLab",300,0,0,ejectile.Phi());
      if (ejectile.Theta() < 0.01) {
        obj.FillHistogram("MissingMass","PhiLab_10mrad",300,0,0,ejectile.Phi());
      }
      auto p_ejectile = ejectile.Mag();
      auto e_ejectile = TMath::Sqrt(p_ejectile*p_ejectile+m_projectile*m_projectile);
      auto ke_ejectile = e_ejectile - m_projectile;
      double theta_cm=0,Ex=0,J_L=0;

      std::tie(theta_cm,Ex,J_L) = kine_2b(m_projectile,m_target,m_projectile,m_target,ke_projectile,ejectile.Theta(), ke_ejectile);


      obj.FillHistogram("MissingMass","ThetaCM",300,0,0,theta_cm);
      obj.FillHistogram("MissingMass","ReconstructedEx",1024,0,0,Ex);
      if (ejectile.Theta() < 0.01) {
        obj.FillHistogram("MissingMass","ReconstructedEx_0_10_mrad",1024,0,0,Ex);
      }
      obj.FillHistogram("MissingMass","KE_ejectile",1024,0,0,ke_ejectile);

      obj.FillHistogram("MissingMass","KE_projectile",1024,0,0,ke_projectile);
      obj.FillHistogram("MissingMass","ejectile_theta",1000,-.1,.1,ejectile.Theta());


      // LaBr3 prompt analysis
      dirname = "LaBr3_prompt";
      for (auto const& labr_hit : hit.GetLaBr()) {
        int channum = labr_hit.channel;
        if ((labr_hit.qtc_le >= -3000) && (labr_hit.qtc_le <= -2000)) { // prompt
          stream.str(""); stream << "X_LaBrE_" << channum;
          obj.FillHistogram(dirname, stream.str(),300,-600,600,rcnp.GR_X(0),2500,0,20000,labr_hit.GetEnergy());
          stream.str(""); stream << "X_LaBrE_sum";
          obj.FillHistogram(dirname, stream.str(),300,-600,600,rcnp.GR_X(0),2500,0,20000,labr_hit.GetEnergy());

          // old
          obj.FillHistogram(dirname,"GR_X",1200,-600,600, rcnp.GR_X(0));
          obj.FillHistogram(dirname,"GR_Y",200,-100,100, rcnp.GR_Y(0));
          obj.FillHistogram(dirname,"GR_Theta",100,-1,1, rcnp.GR_TH(0));
          obj.FillHistogram(dirname,"GR_Phi",100,-1,1, rcnp.GR_PH(0));
          obj.FillHistogram(dirname,"X_TH",1200,-600,600,rcnp.GR_X(0),1000,-1,1,rcnp.GR_TH(0));

          obj.FillHistogram(dirname,"GR_Theta_Phi",100,-1,1, rcnp.GR_TH(0),100,-1,1, rcnp.GR_PH(0));
          obj.FillHistogram(dirname,"GR_X_Y",1200,-600,600, rcnp.GR_X(0),200,-100,100, rcnp.GR_Y(0));


          stream.str(""); stream << "LaBrE_LEGate" << channum;
          obj.FillHistogram(dirname,stream.str(), 10000, -5000, 15000, labr_hit.GetEnergy());

          stream.str(""); stream << "X_LaBrE" << channum;
          obj.FillHistogram(dirname,stream.str(),1200,-600,600,rcnp.GR_X(0),2500,0,20000,labr_hit.GetEnergy());


        }
        else if ((labr_hit.qtc_le >= -4000) && (labr_hit.qtc_le <= -3000)) { // random
          stream.str(""); stream << "rand_X_LaBrE_" << channum;
          obj.FillHistogram(dirname, stream.str(),300,-600,600,rcnp.GR_X(0),2500,0,20000,labr_hit.GetEnergy());
          stream.str(""); stream << "rand_X_LaBrE_sum";
          obj.FillHistogram(dirname, stream.str(),300,-600,600,rcnp.GR_X(0),2500,0,20000,labr_hit.GetEnergy());
        }

        stream.str(""); stream << "LaBrLeading" << channum;
        obj.FillHistogram("GR", stream.str(), 10000,-40000, 40000, labr_hit.qtc_le);

        stream.str(""); stream << "LaBr" << channum << "_LE[LaBr_E]";
        obj.FillHistogram("GR", stream.str(), 1000, -5000, 15000, labr_hit.GetEnergy(), 1000,-40000, 40000, labr_hit.qtc_le);


        stream.str(""); stream << "LaBrE" << channum;
        obj.FillHistogram("GR", stream.str(), 10000, -5000, 15000, labr_hit.GetEnergy());
      } // end labr3 analysis


    } // end rayid == 0 (good reconstruction)

    if (rcnp.GR_ADC()) {
      auto& adc = *rcnp.GR_ADC();
      for (int i=0; i<6; i++) {
        stream.str(""); stream << "GR_ADC" << i;
        obj.FillHistogram("GR",stream.str().c_str(), 1000,0,2000, adc[i]);
      }
    }







  }

}

void MakeCoincidenceHistograms(TRuntimeObjects& obj, TCagra& cagra, TGrandRaiden& gr) {


  for (auto& hit : gr) {

    auto& rcnp = hit.GR();

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (rcnp.GR_RAYID(0) != 0) { return; } // HARD CUT TO ONLY TAKE EVENTS WHICH HAD A GOOD GR RECONSTRUCTION !!!!!//
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    auto grtime = hit.GetTimestamp();
    auto rf = rcnp.GR_RF(0);
    auto x = rcnp.GR_X(0);
    //auto Ex = x*0.01074+6.872; // calibrated for 12C @ commissioning on 04.10.16

    // coincidence rate
    static ULong_t first_timestamp = grtime;
    if (first_timestamp) {
      auto rate = (grtime-first_timestamp)/1e8;
      //cout << grtime << " " << first_timestamp << endl;
      obj.FillHistogram("Coincident","Rate",3000,0,30000, rate);
    }

    auto ycor = rcnp.GR_Y(0)+892.46*rcnp.GR_PH(0);
//    auto a = rcnp.GR_TH(0);


    // coincidence time difference
    for (auto& core_hit : cagra) {

      int detector = core_hit.GetDetnum();
      char core_leaf = core_hit.GetLeaf();
      string chan = core_leaf + std::to_string(core_hit.GetSegnum());
      size_t crystal_id = TCagra::GetCrystalId(detector,core_leaf);
      bool bgo_hit = false;

      auto cagratime = core_hit.Timestamp();
      auto tdiff = cagratime-grtime;

      obj.FillHistogram("Coincident","Diff_CAGRA_GR", 1000,-500,1500,cagratime-grtime);

      stream.str("");
      stream << "TimeDiff_" << detector << "_" << chan;
      obj.FillHistogram("Coincident",stream.str().c_str(),1000,-500,1500,tdiff);


      // stream.str(""); stream << "Egam[tdiff]_" <<detector << "_" << chan;
      // obj.FillHistogram(stream.str(),5000,0,10000,core_hit.GetCorrectedEnergy(),1000,-500,1500,tdiff);


      // doppler reconstruction

      dirname = "ParticleGamma";
      auto Ecm = core_hit.GetDoppler(beta,pos::core_only);
      auto ejectile = hit.GetEjectileVector();
      auto Ecm_particle = core_hit.GetDoppler(beta,pos::core_only,ejectile);
      auto Elab = core_hit.GetCorrectedEnergy();

      obj.FillHistogram(dirname,"Ecm_particle[Elab]",
                        2500,0,10000,Elab,
                        2500,0,10000,Ecm_particle);
      obj.FillHistogram(dirname,"EdopplerSummary",
                        6000,-2000,22000,Ecm,
                        49,0,49,crystal_id);
      obj.FillHistogram(dirname,"EdopplerSum",7000,0,21000,Ecm);
      obj.FillHistogram(dirname,"EdopplerParticleSummary",
                        6000,-2000,22000,Ecm_particle,
                        49,0,49,crystal_id);
      obj.FillHistogram(dirname,"EdopplerParticleSum",7000,0,21000,Ecm_particle);


      obj.FillHistogram(dirname,"Y[B]cor",250,-0.1,0.1,rcnp.GR_PH(0),200,-100,100,ycor);
      //obj.FillHistogram(dirname,"Y[B]cor",250,-0.1,0.1,rcnp.GR_PH(0),200,-100,100,ycor);

      if (ycor<11 && ycor >-8) { // maybe not needed
        obj.FillHistogram(dirname,"Diff_CAGRA_GR", 1000,-500,1500,cagratime-grtime);
        obj.FillHistogram(dirname,"A[X]_gateYcor",1200,-600,600,rcnp.GR_X(0),1000,-1,1,rcnp.GR_TH(0));
        //stream.str(""); stream << "TimeDiff_" << detector << "_" << chan;
        obj.FillHistogram(dirname,stream.str().c_str(),1000,-500,1500,tdiff);

        obj.FillHistogram(dirname+"_gates","A[X]_gateYcor_midgateAX", 300,-600,600,rcnp.GR_X(0),1000,-1,1,rcnp.GR_TH(0));
        obj.FillHistogram(dirname+"_gates","Diff_CAGRA_GR_midgateAX", 1000,-500,1500,tdiff);

        // gate on prompt event timing peak
        if (tdiff>=252 && tdiff<=260) {
          int detector = core_hit.GetDetnum();
          char core_leaf = core_hit.GetLeaf();
          string chan = core_leaf + std::to_string(core_hit.GetSegnum());
          size_t crystal_id = TCagra::GetCrystalId(detector,core_leaf);

          // cagra core energy summary
          obj.FillHistogram("CrystalEnergySummaryPrompt",
                            6000,-2000,22000,core_hit.GetCorrectedEnergy(),
                            49,0,49,crystal_id);

          // cagra core energy summary vs. rcnp.GR_X(0)
          obj.FillHistogram("CrystalEnergySummaryPrompt_vs_X",
                            300,-600,600,rcnp.GR_X(0),
                            6000,-2000,22000,core_hit.GetCorrectedEnergy());
          obj.FillHistogram("EgammaSumPrompt",7000,0,21000,core_hit.GetCorrectedEnergy());

          obj.FillHistogram(dirname,"EdopplerSummaryPrompt",
                            6000,-2000,22000,Ecm,
                            49,0,49,crystal_id);
          obj.FillHistogram(dirname,"EdopplerSum",7000,0,21000,Ecm);
          obj.FillHistogram(dirname,"EdopplerParticleSum",7000,0,21000,Ecm_particle);
          obj.FillHistogram(dirname,"EdopplerParticleSummaryPrompt",
                            6000,-2000,22000,Ecm_particle,
                            49,0,49,crystal_id);
          obj.FillHistogram(dirname,"EdopplerParticleSumPrompt",7000,0,21000,Ecm_particle);
          // cagra core energy summary vs. rcnp.GR_X(0)
          obj.FillHistogram("EdopplerParticle_vs_X",
                            300,-600,600,rcnp.GR_X(0),
                            6000,-2000,22000,Ecm_particle);

          for (auto& cagra_hit : cagra) {
            if (cagra_hit.GetDetnum() == detector && cagra_hit.GetSystem() == 'Y' && cagra_hit.GetChannel() == 4) {
              auto bgotdiff = core_hit.Timestamp() - cagra_hit.Timestamp();
              obj.FillHistogram(dirname,"BGOTime_diff",5000,0,0, bgotdiff);
              if (bgotdiff < 10 && bgotdiff > -10) {
                bgo_hit = true;
                break;
                //obj.FillHistogram(dirname,"BGOTime_diff_accepted",1000,0,0,bgotdiff);
              }

            }
          }

          if (core_hit.GetSystem()=='Y') {
            if (bgo_hit) {
              obj.FillHistogram(dirname,"BGO_counts",10,0,10,3);
            } else {
              obj.FillHistogram(dirname,"BGO_counts",10,0,10,7);
              obj.FillHistogram("EdopplerParticleBGOveto_vs_X",
                                300,-600,600,rcnp.GR_X(0),
                                6000,-2000,22000,Ecm_particle);
              obj.FillHistogram(dirname,"EdopplerParticleSumBGOveto",7000,0,21000,Ecm_particle);
            }
          }




          bgo_hit = false;


          obj.FillHistogram("EgammaSumPrompt",7000,0,21000,core_hit.GetCorrectedEnergy());

          if (rcnp.GR_X(0) > -370 && rcnp.GR_X(0) < -270) {
            obj.FillHistogram("Doppler_15MeV","EdopplerSum",7000,0,21000,Ecm);
            obj.FillHistogram("Doppler_15MeV","EdopplerParticleSum",7000,0,21000,Ecm_particle);
            obj.FillHistogram("Doppler_15MeV","EdopplerParticleSummaryPrompt",
                              6000,-2000,22000,Ecm_particle,
                              49,0,49,crystal_id);
          }


          //obj.FillHistogram(dirname,"Momentum",1000,2500,2700,hit.GetMomentum());
          obj.FillHistogram(dirname,"Momentum",1000,2400,2800,hit.GetMomentum());
          auto p_gamma = core_hit.GetMomentumVector(pos::core_only);
          auto p_invariant = hit.ReconstructInvariant(p_gamma);
          auto m_invariant = m_projectile + Li6Ex;
          auto e_invariant = TMath::Sqrt(p_invariant.Mag()*p_invariant.Mag() + m_invariant*m_invariant);
          auto ke_invariant = e_invariant - m_invariant;
          obj.FillHistogram(dirname,"KE_invariant",1024,500,600,ke_invariant);
          obj.FillHistogram(dirname,"p_invariant",1024,2400,2800,p_invariant.Mag());

          double theta_cm=0,Ex=0,J_L=0;
          std::tie(theta_cm,Ex,J_L) = kine_2b(m_projectile,m_target,m_invariant,m_target,ke_projectile,p_invariant.Theta(), ke_invariant);

          if (Ecm_particle>=3400 && Ecm_particle<=3800) {
            obj.FillHistogram("6LiEx_gate","ThetaCM",300,0,0.15,theta_cm);
            obj.FillHistogram("6LiEx_gate","ReconstructedEx",200,0,100,Ex);
            if (ejectile.Theta() < 0.01) {
              obj.FillHistogram("6LiEx_gate","ReconstructedEx_0_10_mrad",200,0,100,Ex);
            }
            obj.FillHistogram("6LiEx_gate","KE_projectile",200,500,600,ke_projectile);
            obj.FillHistogram("6LiEx_gate","ejectile_theta",1000,-.1,.1,ejectile.Theta());
          } else if (Ecm_particle>=3900 && Ecm_particle <=4300) {
            obj.FillHistogram("6LiEx_sideband","ThetaCM",300,0,0.15,theta_cm);
            obj.FillHistogram("6LiEx_sideband","ReconstructedEx",200,0,100,Ex);
            if (ejectile.Theta() < 0.01) {
              obj.FillHistogram("6LiEx_sideband","ReconstructedEx_0_10_mrad",200,0,100,Ex);
            }
            obj.FillHistogram("6LiEx_sideband","KE_projectile",1024,500,600,ke_projectile);
            obj.FillHistogram("6LiEx_sideband","ejectile_theta",1000,-.1,.1,ejectile.Theta());
          }
          obj.FillHistogram("InvariantMissingMass","ThetaCM",300,0,0.15,theta_cm);
          obj.FillHistogram("InvariantMissingMass","ReconstructedEx",200,0,100,Ex);
          if (ejectile.Theta() < 0.01) {
            obj.FillHistogram("InvariantMissingMass","ReconstructedEx_0_10_mrad",200,0,100,Ex);
          }
          obj.FillHistogram("InvariantMissingMass","KE_projectile",1024,500,600,ke_projectile);




        }

        // gate on random events side band of timing peak
        if (tdiff>=212 && tdiff<=240) {
          int detector = core_hit.GetDetnum();
          char core_leaf = core_hit.GetLeaf();
          string chan = core_leaf + std::to_string(core_hit.GetSegnum());
          size_t crystal_id = TCagra::GetCrystalId(detector,core_leaf);

          // cagra core energy summary
          obj.FillHistogram("Summary","rand_CrystalEnergySummaryPrompt",
                            6000,-2000,22000,core_hit.GetCorrectedEnergy(),
                            49,0,49,crystal_id);

          // cagra core energy summary vs. rcnp.GR_X(0)
          obj.FillHistogram("Summary","rand_CrystalEnergySummaryPrompt_vs_X",
                            300,-600,600,rcnp.GR_X(0),
                            6000,-2000,22000,core_hit.GetCorrectedEnergy());

        }
      }
    } // end cagra analysis
  }
}

void LoadCuts() {
  // Example of how to load a cut once.
  // statically define the cut in e441.h
  // if(!your_fav_cut) {
  //   TPreserveGDirectory Preserve;
  //   TFile fcut("./cuts/newHe3cut.root");
  //   your_fav_cut = (TCutG*)fcut.Get("_cut0");
  //   std::cout << "Loaded he3 gate." << std::endl;
  // }
}

void LoadRaytraceParams(size_t xdeg, size_t adeg, size_t ydeg) {
  static bool once = false;
  if (once) { return; }
  once = true;
  std::cout << "Loading Raytrace Parameters. " << std::endl;

  // sieve slit transformation coefficients
  // output from sieveslit.py
  TGrandRaidenHit::SetRaytraceParams(
    { // a fit parameters
      -16.46324544230171227,
        -0.02122094932284203,
        -0.00000175692614590,
        -0.00000000019248113,
        355.31589759280973340
    },
    { // b fit parameters
      -1.69063228313189517,
        5.37742950170189715,
        -41.03604322025192630,
        -14.04205837959593950,
        0.00859637765853833,
        0.00173614493084734,
        -0.23865171220755754,
        0.04761148936398286,
        0.00002267665926495,
        -0.00001638172446673,
        -0.00163527983664722,
        0.00038621985050345,
        0.00000000314766529,
        -0.00000002363445651,
        -0.00000167708819323,
        0.00000058166479827
        },
    xdeg, // polynomial degree in fit for x
    adeg,
    ydeg
    );
}
