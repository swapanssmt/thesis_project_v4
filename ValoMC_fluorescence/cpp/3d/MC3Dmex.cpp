
#include <string>
#define _USE_MATH_DEFINES
#define VALOMC_MEX
#include <cmath>
#include <limits>
#include <inttypes.h>
#include <string>
#include <vector>

#include "mex.h"
#include "Array.hpp"
#include "ArrayMEX.hpp"
#include "MC3D.hpp"
#include "../versionstring.h"

#include "matrix.h"

// Compiling (from MATLAB prompt):
//   mex MC3Dmex.cpp
//
// To compile with OpenMP (multithread) support (from MATLAB prompt):
//   mex -DUSE_OMP MC3Dmex.cpp CFLAGS="$CFLAGS -fopenmp" LDFLAGS="$LDFLAGS -fopenmp"
// Do not use OpenMP version if the MATLAB does not support the compiler used

time_t starting_time;


#ifdef _MAKE_CTRL_C_POSSIBLE_
extern "C" bool utIsInterruptPending();
#endif

void finalchecks(int csum, int Nphoton) {
  if (csum != Nphoton)
  {
    mexPrintf("WARNING: RUN WAS ABORTED OR PARALLEL COMPUTING ENVIRONMENT IS NOT WORKING CORRECTLY. \n");
    // destroy progress bar
    mexEvalString("delete(mcwaitbar);");
  }
}

void finalchecks_destroy_bar(int csum, int Nphoton) {
   finalchecks(csum, Nphoton);
}


bool Progress_with_bar(double perc){
  //  printf("  %d %%\r", perc);
  mxArray *result;
  result=mexGetVariable("base", "abort_photonMC");
  if(result != NULL) {
    if(mxIsLogicalScalarTrue(result)) {
      mxDestroyArray(result);
      return false;
    }
  }
  time_t now;
  time(&now);
  double timedifference = difftime(now,starting_time);
  
  #ifdef _MAKE_CTRL_C_POSSIBLE_
  if(utIsInterruptPending()) {
      mxDestroyArray(result);
      return false;
  }
  #endif

  char matlabstring[5012];
  
  if(timedifference > 0) {
    
    double remainingtime = (100.0-perc)/(perc/timedifference);
    double hours = floor(remainingtime/(60*60));
    double minutes = floor((remainingtime - hours*60*60)/60);
    double seconds = (remainingtime - hours*60*60 - minutes*60);    
    
    sprintf(&matlabstring[0], "waitbar(%f,mcwaitbar,'%i hours %i minutes and %i seconds left');\n", perc / 100.0, (int) hours, (int) minutes, (int) ceil(seconds)); 
  //  mexPrintf("%s",matlabstring);
  } else {
     sprintf(&matlabstring[0],  "waitbar(0, mcwaitbar,'Estimating the time left');\n");    
  }

  mexEvalString(matlabstring);
  
  fflush(stdout);
  
  if(result != NULL) mxDestroyArray(result);
  
  return true;
}

bool Progress(double perc){
  mexPrintf("  %f %%\r", perc);

  return true;
}

void mexFunction(int nlhs, mxArray **plhs, int nrhs, const mxArray **prhs)
{
  mexPrintf("                 ValoMC-3D\n");
  char infobuf[5012];
  version_string(infobuf);
  mexPrintf("%s",infobuf);
  
  if ((nrhs != 19) || ((nlhs != 8) && (nlhs != 9)))
  {
    mexPrintf("nrhs %i nlhs %i", nrhs, nlhs);
    mexErrMsgTxt("Syntax:\n [vsol, bsol, ebsol, R_vsol, R_bsol, R_ebsol, simulationtime, rnseed, [HN]] = MC3Dmex(H, HN, BH, r, BCType, BCIntensity, BCLightDirectionType, BCLNormal, BCn, mua, mus, g, n, f, phase0, Nphoton, ang_discr_centroid, disablepbar, rnseed)\n");
  }
  mexPrintf("Initializing MC3D...\n");
  
  // Parse input
  Array<int_fast64_t> H, HN, BH;
  Array<double> r, ang_discr_centroid, mua, mus, g, n, phase0;
  Array<char> BCType, BCLightDirectionType;
  Array<double> BCLNormal, BCn, f, BCIntensity;
  Array<int_fast64_t> Nphoton;
  //Array<int_fast64_t> NBin3Dtheta;
  //Array<int_fast64_t> NBin3Dphi;
  Array<double> GaussianSigma;
  Array<int_fast64_t> disable_pbar;
  Array<uint_fast64_t> rndseed;

  Convert_mxArray(prhs[0], H);
  Convert_mxArray(prhs[1], HN);
  Convert_mxArray(prhs[2], BH);
  Convert_mxArray(prhs[3], r);
  Convert_mxArray(prhs[4], BCType);
  Convert_mxArray(prhs[5], BCIntensity);    // [AL]: New array for light source intensity 
  Convert_mxArray(prhs[6], BCLightDirectionType); // [AL]: New array, determines if lightsource given relative to normal or not
  Convert_mxArray(prhs[7], BCLNormal);
  Convert_mxArray(prhs[8], BCn);
  Convert_mxArray(prhs[9], mua);
  Convert_mxArray(prhs[10], mus);
  Convert_mxArray(prhs[11], g);
  Convert_mxArray(prhs[12], n);
  Convert_mxArray(prhs[13], f);
  Convert_mxArray(prhs[14], phase0);
  Convert_mxArray(prhs[15], Nphoton);
  Convert_mxArray(prhs[16], ang_discr_centroid);
  //Convert_mxArray(prhs[16], NBin3Dtheta);
  //Convert_mxArray(prhs[17], NBin3Dphi);
  Convert_mxArray(prhs[17], disable_pbar);
  Convert_mxArray(prhs[18], rndseed);

//  Convert_mxArray(prhs[15], GaussianSigma); 

  // Set parameters to MC
  MC3D MC;
  MC.H = H;
  MC.HN = HN;
  MC.BH = BH;
  MC.r = r;
  MC.BCType = BCType;
  MC.BCIntensity = BCIntensity; // [AL]
  MC.BCLightDirectionType = BCLightDirectionType; // [AL]
  MC.BCLNormal = BCLNormal;
  MC.BCn = BCn;
  MC.mua = mua;
  MC.mus = mus;
  MC.g = g;
  MC.n = n;
  MC.f = f[0];
  MC.Nphoton = Nphoton[0];
  MC.ang_discr_centroid = ang_discr_centroid;
  //MC.NBin3Dtheta = NBin3Dtheta[0];
  //MC.NBin3Dphi = NBin3Dphi[0];
  MC.phase0 = phase0[0];
  //MC.GaussianSigma = GaussianSigma;
  //make negative phase0 positive

  if(MC.phase0 < 0) {
    MC.phase0 += 2*M_PI*ceil(-MC.phase0 / (2*M_PI));
  }
  if(rndseed[1]) {
     MC.seed = (unsigned long) rndseed[0];
  } else {
     MC.seed = (unsigned long) time(NULL);
  }
  // Initialize
  try {
    MC.ErrorChecks();
    MC.Init();
  } catch(mcerror e) {
    std::string message = "Error in initializing MC3D: " + std::string(errorstring(e)) + "\n"; 
    mexErrMsgTxt(message.c_str());
    return;
  }
  
  time(&starting_time);

  // Compute
  if(disable_pbar[0] == 0) {
     mexPrintf("Computing... \n");
    // Create a wait bar
     mexEvalString("assignin('base','abort_photonMC', false);");
     mexEvalString("mcwaitbar = waitbar(0,'Please wait..', 'name', 'Running simulation', 'CreateCancelBtn','abort_photonMC=true;');");

     MC.MonteCarlo(Progress_with_bar, finalchecks_destroy_bar);
     mexPrintf("...done\n");
     printf("\n"); fflush(stdout);
  } else {
     mexPrintf("Computing... \n");
     MC.MonteCarlo(Progress, finalchecks);

     mexPrintf("...done\n");
     printf("\n"); fflush(stdout);
  }

  time_t now;

  // Show lossage
  if(MC.loss) mexPrintf(" %ld photons lost during computation!\n", MC.loss);

  // Copy solution from MC to output
  Array<double> vsolr, vsoli, bsolr, bsoli;
  Array<double> dbsolr, dbsoli; // [AL]
  Array<double> R_vsolr, R_vsoli, R_bsolr, R_bsoli;
  Array<double> R_dbsolr, R_dbsoli; // [AL]
  
  Convert_mxArray(&plhs[0], vsolr, vsoli, MC.ER.Nx, MC.ER.Ny);
  Convert_mxArray(&plhs[1], bsolr, bsoli, MC.EBR.Nx, MC.EBR.Ny);
  Convert_mxArray(&plhs[2], dbsolr, dbsoli, MC.DEBR.Nx, MC.DEBR.Ny);

  // ****************modify************************
  Convert_mxArray(&plhs[3], R_vsolr, R_vsoli, MC.R_ER.Nx, MC.R_ER.Ny);
  Convert_mxArray(&plhs[4], R_bsolr, R_bsoli, MC.R_EBR.Nx, MC.R_EBR.Ny);
  Convert_mxArray(&plhs[5], R_dbsolr, R_dbsoli, MC.R_DEBR.Nx, MC.R_DEBR.Ny);
  // ****************************************************

  plhs[6]=mxCreateDoubleMatrix(1,1,mxREAL); // [AL]
  time(&now);

  *mxGetPr(plhs[6])=(double) difftime(now,starting_time);

  long ii;
  for(ii = 0; ii < MC.ER.N; ii++){
    vsolr[ii] = MC.ER[ii];
    vsoli[ii] = MC.EI[ii];
  }
  for(ii = 0; ii < MC.EBR.N; ii++){
    bsolr[ii] = MC.EBR[ii];
    bsoli[ii] = MC.EBI[ii];
  }
  for(ii = 0; ii < MC.DEBR.N; ii++){
    dbsolr[ii] = MC.DEBR[ii];
    dbsoli[ii] = MC.DEBI[ii];
  }

  //*********************modify**************************
  for(ii = 0; ii < MC.R_ER.N; ii++){
    R_vsolr[ii] = MC.R_ER[ii];
    R_vsoli[ii] = MC.R_EI[ii];
  }
  for(ii = 0; ii < MC.R_EBR.N; ii++){
    R_bsolr[ii] = MC.R_EBR[ii];
    R_bsoli[ii] = MC.R_EBI[ii];
  }
  for(ii = 0; ii < MC.R_DEBR.N; ii++){
    R_dbsolr[ii] = MC.R_DEBR[ii];
    R_dbsoli[ii] = MC.R_DEBI[ii];
  }
  //**********************************

  const mwSize dims[] = {1,1};
  plhs[7] = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
  *((unsigned long*) mxGetData(plhs[7])) = MC.seed;

  // Copy topology neighbourhood
  if(nlhs == 9){
    Array<long> HNo;
    Convert_mxArray(&plhs[8], HNo, MC.HN.Nx, MC.HN.Ny);
    for(ii = 0; ii < MC.HN.N; ii++) HNo[ii] = MC.HN[ii];
  }

  if(disable_pbar[0] == 0) {
    mexEvalString("delete(mcwaitbar);");
  }
  mexPrintf("Done\n");
}
