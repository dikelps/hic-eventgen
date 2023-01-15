//...
//"tree" is the tree name

  track = new TChain("tree");
  
  track->SetBranchAddress("b", &bimp);
  track->SetBranchAddress("npart", &npart);
  track->SetBranchAddress("nTrk", &ntrk);
  track->SetBranchAddress("id", &trk_id);
  track->SetBranchAddress("pt", &trk_pt);
  track->SetBranchAddress("eta", &trkEta); 
  track->SetBranchAddress("phi", &trkPhi); 

//...
for (int itrk = 0; itrk < ntrk; itrk++)
    {
      id = trk_id[itrk];
      trk_pt[nch] = trk_pt[itrk];
      trk_eta[nch] = (trkEta[itrk]*2.0/400-1)*2.0;
          trk_phi[nch] = (trkPhi[itrk] + 0.5) * PI / 256.0;
      trk_phi[nch] -= Psi;
      if (trk_phi[nch] < 0)
        trk_phi[nch] += PI2;
      if (trk_phi[nch] > PI2)
        trk_phi[nch] -= PI2;:
      trk_id[nch] = id;
//...