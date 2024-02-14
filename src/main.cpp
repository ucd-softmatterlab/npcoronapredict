#define PARALLEL

#include "Config.h"
#include "CommandLineParser.h"
#include "ConfigFileReader.h"
#include "HamakerConstants.h"
#include "TargetList.h"
#include "Surface.h"
#include "PDBFile.h"

#include "NPTargetList.h"
#include "NPFile.h"

#include "Potentials.h"

#include <omp.h>
#include <cmath>
#include <random>
#include <iomanip>
#include <sys/stat.h>
 
#include <chrono>
#include <ctime>    


//constexpr double        gds             = 0.22;
constexpr double        gds             = 0.0;
constexpr double        delta           = 2.0;
constexpr double        angle_delta     = 5.0 * (M_PI / 180.0);
constexpr int           ncols           = 72;
constexpr int           nrows           = 36;
constexpr int           iterations      = nrows * ncols;
constexpr int           samples         = 128;
constexpr int           steps           = 512;
constexpr double        dz              = delta / (steps - 1); //for non-uniform NPs this is updated for the integration
//define the Boltzmann and Avogadro constants for energy conversions
constexpr double        kbConst       =  1.380649e-23;
constexpr double        naConst       =  6.02214076e23; 



std::random_device randomEngine;
std::uniform_real_distribution<double> random_angle_offset(0.0, angle_delta);

//Define the version number to provide metadata, following the semantic versioning convention https://semver.org 
//In brief: Increment the first number if UA is no longer backwards compatible, i.e. old input files won't work or the output files won't work with pre-existing scripts.
//          Increment the second number if there's new functionality, e.g. new NP shapes, new output, etc.
//          Increment the third number if no functionality has been modified but bug fixes have been applied which may change output.
//When you increment a number, all the following numbers should be reset to zero. E.g. If we're at 1.2.3 and a bug fix is applied, move to 1.2.4 , if we then add new functionality, 1.3.0, then a new version entirely, 2.0.0 

std::string getUAVersion(){
    static std::string uaVersionID("1.0.0"); 
    return uaVersionID;
}




bool CheckForPrecalculated(const double *adsorption_energy, const double *adsorption_error, const double *mfpt_val, const double *minloc_val, const double radius,
        const double zeta, const std::string& name, const std::string& directory, const std::string& npName, int isMFPT=0, int appendAngle = 0, double omegaAngle=0) {
std::string filename;
std::string cylinderFileNameAppend;

if(appendAngle == 0){
cylinderFileNameAppend = "";
} 
else{
cylinderFileNameAppend = "_"  + std::to_string(static_cast<int>(omegaAngle));
}


    if(isMFPT==1){
     filename = directory + "/" + npName+"/"+TargetList::Filename(name) + "_" + std::to_string(static_cast<int>(radius))
        + "_" + std::to_string(static_cast<int>(1000 * zeta)) + cylinderFileNameAppend  +  "_mfpt.uam";
    }
else{
    filename = directory + "/" + npName+"/"+TargetList::Filename(name) + "_" + std::to_string(static_cast<int>(radius))
        + "_" + std::to_string(static_cast<int>(1000 * zeta)) + cylinderFileNameAppend  +  ".uam";
}
    //std::clog << "Info: testing for map at: "<< filename << "\n";
struct stat buffer;
return (stat (filename.c_str(), &buffer) == 0); 

}

void WriteMapFile(const double *adsorption_energy, const double *adsorption_error, const double *mfpt_val, const double *minloc_val, const double *numcontacts_val, const double radius,
        const double zeta, const std::string& name, const std::string& directory,  const std::string& npName ,  double temperature=300.0,  int isMFPT=0, int appendAngle = 0, double omegaAngle=0) {
std::string filename;
std::string cylinderFileNameAppend;


auto outputTimeStamp = std::chrono::system_clock::now();
std::time_t end_time = std::chrono::system_clock::to_time_t(outputTimeStamp);
//If directory doesn't exist, create it.
//boost::filesystem::create_directory(directory);

if(appendAngle == 0){
cylinderFileNameAppend = "";
} 
else{
cylinderFileNameAppend = "_"  + std::to_string(static_cast<int>(omegaAngle));
}


    if(isMFPT==1){
     filename = directory + "/" + npName+"/"+TargetList::Filename(name) + "_" + std::to_string(static_cast<int>(radius))
        + "_" + std::to_string(static_cast<int>(1000 * zeta)) + cylinderFileNameAppend  +  "_mfpt.uam";
    }
else{
    filename = directory + "/" + npName+"/"+ TargetList::Filename(name) + "_" + std::to_string(static_cast<int>(radius))
        + "_" + std::to_string(static_cast<int>(1000 * zeta)) + cylinderFileNameAppend  +  ".uam";
}
    std::clog << "Info: Saving map to: "<< filename << "\n";
    std::ofstream handle(filename.c_str());
    double phi, theta;
    std::string timestamp;
    timestamp = std::ctime(&end_time);
    timestamp.erase( timestamp.end() - 1);
    handle << "#Results generated at: " << timestamp  << " using UA version: " << getUAVersion() << "\n";
    handle << "#" << npName << " - " << name << "\n";
    handle << "#phi-LeftHandEdge theta-LeftHandEdge EAds/kbT=300 SDEV(Eads)/kbT=300 min_surf-surf-dist/nm mfpt*DiffusionCoeff/nm^2 EAds/kJ/mol min_ProtSurf_NPCentre-dist/nm omega NumContacts\n"; 
    for (int i = 0; i < iterations; ++i) { 
        phi   = (i % ncols) * 5.0;
        theta = (i / ncols) * 5.0;
        handle << std::left << std::setw(7) << std::fixed << std::setprecision(1) << phi;
        handle << std::left << std::setw(7) << std::fixed << std::setprecision(1) << theta;

        
        //handle << std::left << std::setw(14) << std::fixed << std::scientific << std::setprecision(5) << adsorption_energy[i];
        
        handle << std::left << std::setw(14) << std::fixed << std::setprecision(5) << adsorption_energy[i];
        
        handle << std::left << std::setw(14) << std::fixed << std::setprecision(5) << adsorption_error[i];

        handle << std::left << std::setw(14) << std::fixed << std::setprecision(5) << minloc_val[i];

        handle << std::left << std::setw(14) << std::fixed << std::scientific << std::setprecision(5) << mfpt_val[i];
        
        handle << std::left << std::setw(14) << std::fixed << std::setprecision(5) << adsorption_energy[i] * 300.0 * kbConst * naConst / 1000.0;
        
                handle << std::left << std::setw(14) << std::fixed << std::setprecision(5) << minloc_val[i] + radius;

        handle << std::left << std::setw(7) << std::fixed << std::setprecision(1) << omegaAngle;
        
        
         handle << std::left << std::setw(14) << std::fixed << std::setprecision(5) << numcontacts_val[i] ;

        handle << "\n";
    }
    handle.close();
}

void PrintStatistics(const double *adsorption_energy, const double *adsorption_error, const double radius, const std::string& filename) {
    double          sin_theta;
    double          error           = 0.0;
    double          simpleAverage   = 0.0;
    long double     T               = 0.0;
    long double     Z               = 0.0;
    double sinThetaTot = 0.0;
    double minEnergy = 50.0;
    for (int i = 0; i < iterations; ++i) {
        sin_theta       = std::sin((i / ncols) * angle_delta);
        T               += adsorption_energy[i] * sin_theta * std::exp(-1.0 * adsorption_energy[i]);
        Z               += sin_theta * std::exp(-1.0 * adsorption_energy[i]);
        simpleAverage   += sin_theta * adsorption_energy[i];
        error           += sin_theta * adsorption_error[i];
        sinThetaTot     += sin_theta;
        if(adsorption_energy[i] < minEnergy){
        minEnergy = adsorption_energy[i];
        }
    }

    simpleAverage   /= sinThetaTot;
    error           /= iterations;
    double boltzAverage = T/Z;
    if(isnan(boltzAverage)){
    boltzAverage = minEnergy;
    }


    std::cout << std::setw(10) << std::left << TargetList::Filename(filename);
    std::cout << std::setw(10) << std::fixed << std::setprecision(1) << radius;
    std::cout << std::setw(14) << std::fixed << std::setprecision(5) << simpleAverage;
    std::cout << std::setw(14) << std::fixed << std::setprecision(5) << boltzAverage;
    std::cout << std::setw(14) << std::fixed << std::setprecision(5) << error;
    std::cout << std::endl;
}
//Phi,theta here are adjusted from the  phi,theta defining the orientation by phi -> -phi, theta -> pi - theta
//in terms of the angles given here, this corresponds to a rotation around the z axis of phi, followed by a rotation around y of theta.
inline void Rotate (const int size, const double phi, const double theta,
const std::vector<double>& cx, const std::vector<double>& cy, const std::vector<double>& cz,
double *x, double *y, double *z) {
    double R[3][3];
    
    R[0][0] = std::cos(theta) * std::cos(phi);
    R[0][1] = -1.0 * std::cos(theta) * std::sin(phi);
    R[0][2] = std::sin(theta);
    R[1][0] = std::sin(phi);
    R[1][1] = std::cos(phi);
    R[2][0] = -1.0 * std::sin(theta) * std::cos(phi);
    R[2][1] = std::sin(theta) * std::sin(phi);
    R[2][2] = std::cos(theta);

    for(int i = 0; i < size; ++i) {
        x[i] = cx[i] * R[0][0] + cy[i] * R[0][1] + cz[i] * R[0][2];
        y[i] = cx[i] * R[1][0] + cy[i] * R[1][1];
        z[i] = cx[i] * R[2][0] + cy[i] * R[2][1] + cz[i] * R[2][2];





    }


}
//as the above function, except followed by a rotation  of omega around the z axis
inline void Rotate3 (const int size, const double phi, const double theta, const double omega,
const std::vector<double>& cx, const std::vector<double>& cy, const std::vector<double>& cz,
double *x, double *y, double *z) {
    double R[3][3];

    R[0][0] = std::cos(theta) * std::cos(phi) * std::cos(omega) - std::sin(omega) * std::sin(phi);
    R[0][1] = -1.0 * std::cos(theta) * std::sin(phi) * std::cos(omega) - std::cos(phi)*std::sin(omega);
    R[0][2] = std::sin(theta)*std::cos(omega);
    R[1][0] = std::sin(phi)*std::cos(omega) + std::cos(phi) * std::cos(theta)*std::sin(omega);
    R[1][1] = std::cos(phi)*std::cos(omega) - std::cos(theta)*std::sin(omega)*std::sin(phi);
    R[1][2] = std::sin(omega) * std::sin(theta);
    R[2][0] = -1.0 * std::sin(theta) * std::cos(phi);
    R[2][1] = std::sin(theta) * std::sin(phi);
    R[2][2] = std::cos(theta);

    for(int i = 0; i < size; ++i) {
        x[i] = cx[i] * R[0][0] + cy[i] * R[0][1] + cz[i] * R[0][2];
        y[i] = cx[i] * R[1][0] + cy[i] * R[1][1] + cz[i] * R[1][2];
        z[i] = cx[i] * R[2][0] + cy[i] * R[2][1] + cz[i] * R[2][2];





    }


}



inline void JitterPDB (const int size, const double jitterMagnitude,const std::vector<double>& cx, const std::vector<double>& cy, const std::vector<double>& cz,
double *x, double *y, double *z) {
/*
    R[0][0] = std::cos(theta) * std::cos(phi) * std::cos(omega) - std::sin(omega) * std::sin(phi);
    R[0][1] = -1.0 * std::cos(theta) * std::sin(phi) * std::cos(omega) - std::cos(phi)*std::sin(omega);
    R[0][2] = std::sin(theta)*std::cos(omega);
    R[1][0] = std::sin(phi)*std::cos(omega) + std::cos(phi) * std::cos(theta)*std::sin(omega);
    R[1][1] = std::cos(phi)*std::cos(omega) - std::cos(theta)*std::sin(omega)*std::sin(phi);
    R[1][2] = std::sin(omega) * std::sin(theta);
    R[2][0] = -1.0 * std::sin(theta) * std::cos(phi);
    R[2][1] = std::sin(theta) * std::sin(phi);
    R[2][2] = std::cos(theta);
*/
    std::normal_distribution<double> jitterDist(0.0, jitterMagnitude);

    for(int i = 0; i < size; ++i) {
        x[i] = cx[i]  + jitterDist(randomEngine);
        y[i] = cy[i] + jitterDist(randomEngine);
        z[i] = cz[i] + jitterDist(randomEngine);





    }


}






//rotate the PDB anticlockwise by omega around the z-axis
inline void RotateZ (const int size, const double omega,
const std::vector<double>& cx, const std::vector<double>& cy, const std::vector<double>& cz,
double *x, double *y, double *z) {
    double R[3][3];

    R[0][0] =  std::cos(omega);
    R[0][1] = - std::sin(omega);
    R[0][2] = 0;
    R[1][0] = std::sin(omega);
    R[1][1] = std::cos(omega);
    R[1][2] = 0;
    R[2][0] = 0;
    R[2][1] = 0;
    R[2][2] = 1;

    for(int i = 0; i < size; ++i) {
        x[i] = cx[i] * R[0][0] + cy[i] * R[0][1] + cz[i] * R[0][2];
        y[i] = cx[i] * R[1][0] + cy[i] * R[1][1];
        z[i] = cx[i] * R[2][0] + cy[i] * R[2][1] + cz[i] * R[2][2];





    }


}


void SquareXY (const int size, double *x, double *y) {
    for (int i = 0; i < size; ++i) {
        x[i] *= x[i];
        y[i] *= y[i];
    }
}

void ShiftZ (const int size, double *z) {
    const double z_shift = *std::min_element(z, z + size);
    for (int i = 0; i < size; ++i) {
         // cout << z[i] << "->" <<  z[i] - z_shift << "\n";
        z[i] -= z_shift;
    }
}

void Sum (const int size, double* result, double *arr) {
    *result = 0.0;
    for (int i = 0; i < size; ++i) {
        *result += arr[i];
    }
}
//note that for this integration routine and all others, "ssd" is actually the distance between the centre of the NP and the closest point of the protein.
void Integrate(const int size, const double dz, const double init_energy, const double *energy, const double *ssd, double *adsorption,double temperature) {
    long double area = 0.0;
    double minEnergy = 500.0;
    for (int i = 0; i < size; ++i) {
        double energyDiff = energy[i] - init_energy;
        if(energyDiff < minEnergy){
        minEnergy = energyDiff;
        }
        area += static_cast<long double>(ssd[i] * ssd[i] * dz) * std::exp(static_cast<long double>(-1.0 * (energyDiff))); 
    }
    const double factor = 3.0 / std::fabs(std::pow(ssd[0], 3.0) - std::pow(ssd[size - 1], 3.0));
   

    double adsorptionEnergy = -1.0 * (temperature/300.0) * std::log(factor * area);
    if(isnan(adsorptionEnergy)){
    adsorptionEnergy = minEnergy;
    }
    *adsorption = adsorptionEnergy;


}


/*


*/

void IntegrateMFPT(const int size, const double dz, const double init_energy, const double *energy, const double *ssd, double *mfpt, int npType, int calculateMFPT,double temperature) {
    long double res = 0.0;
    long double innerRes = 0.0;
    long double outerRes = 0.0;

if(calculateMFPT!=0){

//the ssd list contains the positions, note that the ssd[0] corresponds to the furthest distance and ssd[size] is the closest. 

int minimumRange = 0; //number of steps to go away from the minimum when averaging over the initial position
int stepSize = 5;
int correctionFactorPower = 0;
if(npType == 1){
correctionFactorPower = 2;
}
else if(npType == 2 || npType == 4 || npType==5){
correctionFactorPower = 1;
}

//find the location of the deepest minima. the integral defining the MFPT is most strongly peaked around here.
int minI = 0;
double minEnergy = energy[0];
for(int i =0;i<size;++i){
if(energy[i] < minEnergy){
minEnergy  = energy[i];
minI = i;
}
}



for(int k = std::max(minI-minimumRange*stepSize,0); k < std::min(size,minI+minimumRange*stepSize+1); k+=stepSize){
//integrate from the end point at ssd[0] to r0 at ssd[k]
    res = 0.0;
    for (int i = 0; i < k; ++i) { //integrate over "y", runs from r0 to the escape point
        //here ssd[i] = y and ssd[j] = rprime
        innerRes = 0;
        for(int j =size; j > i; --j){ //integrate over "x": from touching the NP to the  value of y
            innerRes +=  dz*std::exp(static_cast<long double>(energy[i]  -1.0*(  energy[j] )))  * pow(ssd[j]/ ssd[i], correctionFactorPower ); 
        }
        res+= dz    * innerRes;
    } //at this point res contains the MFPT for a given initial position
    outerRes += dz*res* pow(ssd[k], correctionFactorPower )*std::exp(static_cast<long double>(  -1.0*(  energy[k] ))); //weight the MFPT by the probability for that initial position
}
double z = 0;
for(int i=std::max(minI-minimumRange*stepSize,0);i<std::min(size,minI+minimumRange*stepSize+1);i+=stepSize){
 z+= dz* pow(ssd[i], correctionFactorPower )*std::exp(static_cast<long double>(  -1.0*(  energy[i] )));
 //calculate the partition function, used to normalise the average MFPT
}
     *mfpt = outerRes/z;

}
else{
*mfpt = -1;
}





}



void IntegrateCylinder(const int size, const double dz, const double init_energy, const double *energy, const double *ssd, double *adsorption,double temperature) {
    long double area = 0.0;
    for (int i = 0; i < size; ++i) {

if(std::isnan(energy[i])){
std::cout << "Cylinder integration: found nan at " << i << " " <<   "\n";
}
if(std::isinf(energy[i])){
std::cout << "Cylinder integration: found inf at " << i << " " <<   "\n";
}

        area += static_cast<long double>(ssd[i]  * dz) * std::exp(static_cast<long double>(-1.0 * (energy[i] - init_energy))); 
    }
    const double factor = 2.0 / std::fabs(std::pow(ssd[0], 2.0) - std::pow(ssd[size - 1], 2.0));

    if(factor*area < 0){ 
 std::cout << "Cylinder integration:  warning: factor*area < 0, unphysical result " << factor << " " << area << "\n";    
}



    *adsorption = -1.0 * (temperature/300.0)  * std::log(factor * area);
}


void IntegrateCube(const int size, const double dz, const double init_energy, const double *energy, const double *ssd, double *adsorption,double temperature) {
    long double area = 0.0;
    for (int i = 0; i < size; ++i) {
        area += static_cast<long double>(dz  ) * std::exp(static_cast<long double>(-1.0 * (energy[i] - init_energy))); 
    }
    const double factor = 1.0 / std::fabs(std::pow(ssd[0], 1.0) - std::pow(ssd[size - 1], 1.0));
    *adsorption = -1.0  * (temperature/300.0) * std::log(factor * area);
}



void MeanAndSD(const int size, double *mean, double *sd, double *arr) {
    double lmean = 0;
    double lsd   = 0;
    for (int i = 0; i < size; ++i) {
        lmean += arr[i];
    }
    lmean /= size;
    for (int i = 0; i < size; ++i) {
        lsd += std::pow(arr[i] - lmean, 2.0);
    }
    lsd /= size;
    
    *mean   = lmean;
    *sd     = std::sqrt(lsd);
 
}

//geometry -specific radius for a bead at (sqrt(x),sqrt(y),z) at an NP-protein distance start.
double getDistance(double x, double y, double z, double start, double radius,int npType){
double distance;


                if(npType == 2 || npType == 4 || npType==5){
                //for cylinder NPs we only take into consideration the radial distance with the cylinder aligned along the x-axis.
                distance    = std::sqrt(y +  (z + start) * (z + start)) - radius; // AA Center To NP Surface Distance (originally stop, changed to start)
                }
                else if(npType == 3){
                //for cubes we consider only the "vertical" distance, i.e. we assume that each bead is approximately at the centre of the cube
                distance = std::sqrt(  (z +start) * (z + start)) - radius;
                }
                else{
                distance    = std::sqrt(x + y + (z + start) * (z + start)) - radius; // Center To Surface Distance
//               std::cout << i << " "  << phi << " "  << theta << " " <<  x[i] << " " << y[i] << " " << z[i] << " " <<distance << "\n";

                }

return distance;
}

//get the distance between the centre of an AA bead and the surface of an NP
double getDistance3D(double aaX, double aaY, double aaZ, double npX, double npY, double npZ, double npRadius, int npType){
double distance = 0;

                if(npType == 2 || npType == 4 || npType==5){
                //for cylinder NPs we only take into consideration the radial distance with the cylinder aligned along the x-axis.
                distance    = std::sqrt((aaY - npY)*(aaY-npY) +     (aaZ - npZ)*(aaZ-npZ) ) - npRadius; // AA Center To NP Surface Distance (originally stop, changed to start)
                }
                else if(npType == 3){
                //for cubes we consider only the "vertical" distance, i.e. we assume that each bead is approximately at the centre of the cube
                distance = std::sqrt( (aaZ - npZ)*(aaZ-npZ)   ) - npRadius;
                }
                else{
                distance    = std::sqrt((aaX - npX)*(aaX-npX) + (aaY - npY)*(aaY-npY) +     (aaZ - npZ)*(aaZ-npZ)   ) - npRadius; // sphere surface to x,y,z distance
 

                }

return distance;

}



void AdsorptionEnergies(const PDB& pdb,const NP& np, const Config& config, const Potentials& potentials, const double radius, const double outerRadius,const int angle_offset, const int n_angles, double *adsorption_energy, double *adsorption_error, double *mfpt_val, double *mfpt_err, double *minloc_val, double *minloc_err,  double *numcontacts_val,   double *numcontacts_err, const std::string& pdbname, const std::string& outputdirectory, const std::string& npName,      int npType = 1, double imaginary_radius = -1, int calculateMFPT = 0, double cylinderAngleDeg=0,double zeta =0,int savePotentials = 0,double temperature=300.0, double overlapPenalty=0.0) { 

    // Decleare all variables at the begining . Integration runs from the start at the largest value of r and proceeds inwards to the stop value, nominally R_{NP}.
    const int               size            = pdb.m_id.size();
    const double            stop            = imaginary_radius < 0 ? radius + gds : imaginary_radius + gds; //gds = 0 and imaginary radius is unused so this should always just return radius (inner bounding radius)
    const double            start           =  outerRadius + delta;  //stop + delta;

    int                     i;
    int                     j;
    int			k;
    double                  energy;
    double                  theta;
    double                  phi;
    double                  theta_adjusted;
    double                  phi_adjusted;
    double                  init_energy;
    double                  distance;
    double                  ssd;

     
    
    double x[size];
    double y[size];
    double z[size];
    double actualDZ = (start - stop)/(steps-1);
    double total_energy[steps];
    double SSD[steps];
    double sample_energy[samples];
    double sample_mfpt[samples];
    double sample_minloc[samples];
    double sample_numcontacts[samples];
    double closestSCD[size]; //closest allowed NP_nominal_surface to aa_centre distance allowed for each AA
   //NP component properties are looked up via e.g.: np.m_radius[m_npBeadType[j]]
   //where j is the index of that bead
   //get the number of NP components
   int numNPBeads = np.m_npBeadType.size() ;
   //decide how many NP beads to sum over. By default, only the isotropic-averaged NP is used unless the beads are explicitly treated separately, in which case we sum over all beads
               int numNPBeadSummation = 1;
            if(config.m_sumNPPotentials == false){
            numNPBeadSummation = numNPBeads;
            }
   
   


    // Iterate over angles
    for (int angle = 0; angle < n_angles; ++angle) {
        
        phi   = ((angle_offset + angle) % ncols) * angle_delta;
        theta = ((angle_offset + angle) / ncols) * angle_delta;
           double cylinderAngle = cylinderAngleDeg * M_PI/180.0;



	    // Sample a angle multiple times.The sample = samples run is not included in averaging but instead used to generate output potentials  
        for (int sample = 0; sample < samples+1 ; ++sample) {
  //         double cylinderAngle = 90 * M_PI/180.0;

    	    // Rotation step - random sampling is enabled if the number of samples to generate is > 1. If not, it just calculates it at the bin center

            if(sample <  samples){
    	    phi_adjusted   = -1.0 * (phi + random_angle_offset(randomEngine));
    	    theta_adjusted = M_PI - (theta + random_angle_offset(randomEngine));
             }
             else{
            phi_adjusted   = -1.0 * (phi  + angle_delta/2);
            theta_adjusted = M_PI - (theta + angle_delta/2);
             //std::cout << "final sample, fixing angle to midpoint" << sample << " " <<  samples << "\n";
              }

            Rotate3(size, phi_adjusted, theta_adjusted,cylinderAngle, pdb.m_x, pdb.m_y, pdb.m_z, x, y, z);
            //JitterPDB(size, 0.1, pdb.m_x, pdb.m_y, pdb.m_z, x, y, z );


            // Convert to SSD except because this doesn't take into account the radius of the AA bead it's really the distance from the surface of the NP to the centre of the closest AA bead.
            //After this, the minimum z-coordinate of the protein is equal to zero.
            //the NP is also defined with its centre at 0,0,0 so make sure that all distance calculations properly offset the protein first by adding start or ssd as appropriate
            ShiftZ(size, z);


            //Find the closest approach
            
       
            
            for( i = 0; i< size; ++i){ //loop over AA beads
             double closestAllowedSSD = 0;
            
             closestSCD[i] = 0;
             
                  if(config.m_sumNPPotentials == true){
             
             if(  overlapPenalty > 0){
              double aaBeadRadius = config.m_aminoAcidRadii[ pdb.m_id[i] ] * config.m_overlapRadiusFactor ;
              //std::cout << pdb.m_id[i] <<  " " << config.m_aminoAcidRadii[ pdb.m_id[i] ]   <<"\n";
                   for( j = 0; j < numNPBeads  ; ++j){
                       //std::cout << i << ":" << j << "\n";
                       double npBeadRadius = np.m_radius[np.m_npBeadType[j]] * config.m_overlapRadiusFactor;

                       double distTerm1 = pow( aaBeadRadius + npBeadRadius, 2) - pow( x[i] - np.m_x[j],2 ) - pow( y[i] - np.m_y[j],2 );
                      //std::cout << distTerm1 << "\n";
                       if(distTerm1 > 0){
                                  
                            double ssdAtTouching1 = np.m_z[j] -aaBeadRadius  - z[i] - radius +  sqrt( distTerm1)  ;
                            //double ssdAtTouching2 = np.m_z[j] -aaBeadRadius  - z[i] - radius - sqrt( distTerm1 )  ;
                             
                            if( ssdAtTouching1 > closestAllowedSSD){ 
                            closestAllowedSSD = ssdAtTouching1;

                            }

                       }

                    }


               if(  closestAllowedSSD + aaBeadRadius > closestSCD[i]  ){
                     closestSCD[i] = closestAllowedSSD + aaBeadRadius ; //correct for the fact that here SSD is the surface-surface distance while everywhere else ssd means surface-centre distance because David.
               }

            }
           }
           
           }
              //x,y no longer squared
            
            init_energy = 0.0;

            for (i = 0; i < size; ++i) {
            

            
            
            for(k = 0; k<numNPBeadSummation; ++k) {
            
              if(config.m_sumNPPotentials == false){
              distance = getDistance3D(x[i],y[i],z[i] + start,  np.m_x[k] , np.m_y[k] , np.m_z[k] , np.m_radius[np.m_npBeadType[k]]    ,np.m_shape[np.m_npBeadType[k]]);
              }
              else{
              distance = getDistance3D(x[i],y[i],z[i] + start,  0 , 0 , 0 , radius,npType);
              }
                double energyAtDist = pdb.m_occupancy[i] *  static_cast<double>(potentials[pdb.m_id[i]].Value(distance, np.m_npBeadType[k] ))  ;
                if(energyAtDist > 1){
                //std::cout << "large shift (" << energyAtDist << ") for distance " << distance << "\n";
                }
                //std::cout << pdb.m_occupancy[i] << "\n";
                init_energy +=  pdb.m_occupancy[i] *  static_cast<double>(potentials[pdb.m_id[i]].Value(distance, np.m_npBeadType[k] ));



              //find the distance of closest approach

  




         }             
            }
 



//std::cout << "total shift/num. amino acid: " <<  init_energy/size << "\n";
            // build the distance-potential array
              // 
            double minLoc = start;
            double minEnergy = 0;
            int numContactsAtMin = 0;
            int numContactsAtStep = 0;
            int resHasContacted = 0;

            for (i = 0; i < steps; ++i) {
                ssd     = start - i * actualDZ;
                energy  = 0;
                 numContactsAtStep = 0;
                //loop over each residue. loop variables: i = ssd index, j = residue index
                for (j = 0; j < size; ++j) {
               resHasContacted = 0;
 
               double appliedOverlapPenalty = 0.0;
 
                   //loop over each NP component present
                    for(k = 0; k<numNPBeadSummation; ++k) {
              //distance is AA centre to NP (Nominal) surface
                            if(config.m_sumNPPotentials == false){
              distance = getDistance3D(x[j],y[j],z[j] + ssd,  np.m_x[k] , np.m_y[k] , np.m_z[k] , np.m_radius[np.m_npBeadType[k]]    ,np.m_shape[np.m_npBeadType[k]]);
              }
              else{
              distance = getDistance3D(x[j],y[j],z[j] + ssd,  0 , 0 , 0 , radius,npType);
              
              
              
                             if(distance < closestSCD[j]){
                //std::cout <<"applying overlap penalty "<<overlapPenalty << " at scd " << distance << " due to restriciton " << closestSCD[j] << "\n";
                appliedOverlapPenalty = overlapPenalty;
                }
              
              
              }

               if(distance < 0.5){
               resHasContacted = 1;
               }

                energy +=  pdb.m_occupancy[j]*appliedOverlapPenalty +      pdb.m_occupancy[j] * static_cast<double>(potentials[ pdb.m_id[j]].Value(distance, np.m_npBeadType[k] ));
                 if(std::isnan(pdb.m_occupancy[j] * static_cast<double>(potentials[pdb.m_id[j]].Value(distance, np.m_npBeadType[k] )))){
                  std::cout << "NaN generated by res. " << j << " type: "  << pdb.m_id[j]  <<   " at " << distance << "with NP bead " << k << " of type " << np.m_npBeadType[k] << "\n";
                    }

                }
                
                
                numContactsAtStep += resHasContacted;
                
                }
                
                SSD[i]          = ssd;
                total_energy[i] = energy;

               //std::cout << SSD[i] << " " << total_energy[i] << "\n";

               if(energy < minEnergy){
               minEnergy = energy;
               minLoc = ssd;
               numContactsAtMin = numContactsAtStep;
               }

            }

           if(sample == samples){
          //for the special run at the bin-center, write out the potential and exit the loop to avoid overloading the results arrays
           //std::cout << "preparing potential output \n";
if(savePotentials==1){
std::string filename;
std::string cylinderFileNameAppend;
if(npType == 2 || npType == 4 || npType==5){
cylinderFileNameAppend = "_"  + std::to_string(static_cast<int>(cylinderAngleDeg));
}
else{
cylinderFileNameAppend = "";
}

  //std::cout<<phi << " " << theta << "\n";
          filename = outputdirectory + "/" + npName + "/"+ pdbname + "_"+std::to_string(static_cast<int>(radius)) + "_" + std::to_string(static_cast<int>(1000 * zeta))+ "_" + std::to_string(static_cast<int>(phi*180/M_PI))+ "_" + std::to_string(static_cast<int>(theta*180/M_PI))+ cylinderFileNameAppend  +".uap";
    std::ofstream handle(filename.c_str());
             handle<<"#ssd,E(kbT)\n";
            for (i = 0; i < steps; ++i) {
                handle << (SSD[i] - radius) << ", " << total_energy[i] << "\n";
            }
    handle.close();
}
          continue;
          }

           sample_minloc[sample] = minLoc - radius;
           sample_numcontacts[sample] = numContactsAtMin;
         /*   for (i = 0; i < steps; ++i) {
                std::cout << (SSD[i] - radius) << " " << total_energy[i] << "\n"; 
            }*/
           // exit(0);

            // Integrate the results
                if(npType == 2 || npType == 4 || npType==5){
            IntegrateCylinder(steps, actualDZ, init_energy, total_energy, SSD, &(sample_energy[sample]),temperature);
            IntegrateMFPT(steps, actualDZ, init_energy, total_energy, SSD, &(sample_mfpt[sample]), npType,calculateMFPT,temperature);
            }
             else if(npType == 3){
             IntegrateCube(steps, actualDZ, init_energy, total_energy, SSD, &(sample_energy[sample]),temperature);
            IntegrateMFPT(steps, actualDZ, init_energy, total_energy, SSD, &(sample_mfpt[sample]), npType,calculateMFPT,temperature);
          }
           else{


            Integrate(steps, actualDZ, init_energy, total_energy, SSD, &(sample_energy[sample]), temperature);

         //and also integrate to get the product of the MFPT and the diffusion constant, MFPT*D. 


            IntegrateMFPT(steps, actualDZ, init_energy, total_energy, SSD, &(sample_mfpt[sample]), npType, calculateMFPT,temperature);

 


        }



        }
  
        // Mean of all the samples
        MeanAndSD(samples, &(adsorption_energy[angle_offset + angle]), &(adsorption_error[angle_offset + angle]), sample_energy); 
        MeanAndSD(samples, &(mfpt_val[angle_offset + angle]), &(mfpt_err[angle_offset + angle]), sample_mfpt); 
         MeanAndSD(samples, &(minloc_val[angle_offset + angle]), &(minloc_err[angle_offset + angle]), sample_minloc);
        MeanAndSD(samples, &(numcontacts_val[angle_offset + angle]), &(numcontacts_err[angle_offset + angle]), sample_numcontacts);

 
    }
}

void SurfaceScan(const PDB& pdb, const Potentials& potentials, const double zeta, const double radius, const double outerRadius,const Config& config, double omegaAngle, const NP& np) {
    std::clog << "Info: Processing '" << pdb.m_name << "' (R = " << radius << ")\n";

    const double imaginary_radius = config.m_imaginary_radius;

    double adsorption_energy[iterations] = {};
    double mfpt_val[iterations] = {};
    double adsorption_error[iterations]  = {};
        double mfpt_err[iterations] = {};
   double minloc_val[iterations] = {};
   double minloc_err[iterations] = {};
   
    double numcontacts_val[iterations] = {};
   double numcontacts_err[iterations] = {};  
   
int appendAngle = 0;   
 if(config.m_npType == 2 || config.m_npType == 4 || config.m_npType == 5){
 appendAngle = 1;
   }  
   if(config.m_npType == 1 && omegaAngle > 0.1){
   appendAngle = 1;
   }
   
    bool checkPrecalc = CheckForPrecalculated(adsorption_energy, adsorption_error, mfpt_val,minloc_val,radius, zeta, pdb.m_name, config.m_outputDirectory, np.m_name, 0,appendAngle,omegaAngle);
     //std::clog << checkPrecalc << "\n";
    if(checkPrecalc == true){
        std::clog << "Info: Target '" <<np.m_name << ":" << pdb.m_name << "' (R = " << radius << ") already calculated, skipping. \n";
    }
    else{
    #ifdef PARALLEL  
    const int n_threads         =   omp_get_max_threads();
    #else
    const int n_threads         = 1;
    #endif
  
    const int n_per_thread      = iterations / n_threads;
    const int n_remaining       = iterations % n_threads; 

    int thread;
    #ifdef PARALLEL  
    #pragma omp parallel shared(pdb, potentials, adsorption_energy, adsorption_error), private(thread)
    #endif
    {
        #ifdef PARALLEL  
        #pragma omp for schedule(dynamic, 1)
        #endif
        for (thread = 0; thread < n_threads; ++thread) {
            AdsorptionEnergies(
                    pdb, 
                    np,
                    config,
                    potentials, 
                    radius, 
                    outerRadius,
                    thread * n_per_thread  + (thread < n_remaining ? thread : n_remaining), 
                    n_per_thread + (thread < n_remaining), 
                    adsorption_energy, 
                    adsorption_error,
                    mfpt_val,
                    mfpt_err,
                    minloc_val,
                    minloc_err,
                    numcontacts_val,
                    numcontacts_err,
                    pdb.m_name,
                    config.m_outputDirectory,
                    np.m_name,
                    config.m_npType,
                    imaginary_radius,
                    config.m_calculateMFPT,
                    omegaAngle,
                    zeta,
                    config.m_savePotentials,
                    config.m_temperature,
                    config.m_overlapPenalty

            ); 
        }
    }
    

  
    WriteMapFile(adsorption_energy, adsorption_error, mfpt_val,minloc_val, numcontacts_val, radius, zeta, pdb.m_name, config.m_outputDirectory, np.m_name, config.m_temperature,     0,appendAngle,omegaAngle); 
    //WriteMapFile(mfpt_val, mfpt_err, radius, zeta, pdb.m_name, config.m_outputDirectory,1,isCylinder,cylinderAngle); 
    PrintStatistics(adsorption_energy, adsorption_error, radius, pdb.m_name);
    
    
    }
}

int main(const int argc, const char* argv[]) {
    CommandLineParser commandLine(argc, argv);
    ConfigFileReader  configFile(commandLine.ConfigFileName());

    Config config;
    configFile.UpdateConfig(config);
    commandLine.UpdateConfig(config);

    HamakerConstants  hamakerConstants(config.m_hamakerFile);
    TargetList        targetList(config.m_pdbTargets);
    SurfacePMFs       surfaces(config.m_pmfDirectory, config.m_pmfPrefix, config.m_aminoAcids);
    PDBs              pdbs(targetList.m_paths, config.AminoAcidIdMap()  , config.m_disorderStrat , config.m_disorderMinBound, config.m_disorderMaxBound);

    std::vector<double>          omegaAngleSet;
    // config.m_npTargets is a list of strings - targets to look at for .np files 
    
    
    int omegaDelta = 180;
    if(config.m_npType == 2 || config.m_npType == 4 || config.m_npType == 5){
    omegaDelta = 45;
     }
     //if final rotation angles omega have been manually set in the config file, use these. Else generate them based on the NP type.
     
    if( config.m_omegaAngles.size() == 0){
    for(int i = 0; i < 180; i = i + omegaDelta){
    omegaAngleSet.push_back(i);
    std::cout << "Adding automatic rotation angle: " << i << "\n";
     }
    }
    else{
    for( auto & omegaAngle : config.m_omegaAngles){
    omegaAngleSet.push_back(  omegaAngle ) ;
        std::cout << "Adding rotation angle: " << omegaAngle << "\n";
    }
    }
     
    boost::filesystem::create_directory(config.m_outputDirectory);
    boost::filesystem::create_directory(config.m_outputDirectory+"/nps");
    
    
    std::string configFileIn = commandLine.ConfigFileName();
    //Uncomment this line to re-enable config file saving
    //boost::filesystem::copy_file(configFileIn, config.m_outputDirectory+"/"+configFileIn, boost::filesystem::copy_options::overwrite_existing);
    
    //Or uncomment this line if you're on Ubuntu 18.04 and Boost is locked to a version with the slightly different naming convention that they then changed
    // boost::filesystem::copy_file(configFileIn, config.m_outputDirectory+"/"+configFileIn, boost::filesystem::copy_option::overwrite_if_exists);


     //generate some NPs 
        std::string npFileDir = config.m_outputDirectory+"/nps/"+config.m_configFile +  "_NPs";
              boost::filesystem::create_directory(npFileDir);
     //manually generate NPs , save them to a folder for storage, read this folder in as input to make sure generated NPs are handled identically to read-in NPs and so that their structure is kept
    if(   config.m_multiNP == 0){   
         int npID = 1;
    for (const double nanoparticleRadius : config.m_nanoparticleRadii) {
        for (const double zetaPotential : config.m_zetaPotential) {
            
             //std::to_string(static_cast<int>(radius))    + "_" + std::to_string(static_cast<int>(1000 * zeta))
            //x,y,z,radius,zeta,coreFactor=1,surfFactor=1,shape, hamakerFile,pmfFile,pmfCutoff,correctionType
            std::string npTypeOutString = "TYPE," + to_string(nanoparticleRadius)+"," + to_string(zetaPotential) + "," + "1,1," + to_string(config.m_npType) +","+ config.m_hamakerFile +","+ config.m_pmfDirectory+"," + to_string(config.m_PMFCutoff) + ","+to_string( config.m_npType )+"\n";
            std::cout << npTypeOutString;
            
           std::string npBeadOutString = "BEAD,0,0,0,0\n";
            std::cout << npBeadOutString;
            
            std::string npIDString = "np"+to_string(npID)+"R_"+to_string(static_cast<int>(nanoparticleRadius))+"_ZP_"+to_string(  static_cast<int>( zetaPotential*1000));
            std::string npOutLoc = npFileDir+"/"+npIDString+".np";
            npID++;
            
             std::ofstream handle(npOutLoc.c_str());
             handle<<"#NP file generated by UnitedAtom\n";
             handle << npTypeOutString;
             handle << npBeadOutString;
             handle.close();
            
        }
    }
    config.m_npTargets.emplace_back(npFileDir); 
    }
    // 
        NPTargetList npTargetList(config.m_npTargets);
        NPs nps(npTargetList.m_paths, config.AminoAcidIdMap());
    
   //scan over multiple NP files 

    for( const auto& np: nps){
    std::string npEnergyDir = config.m_outputDirectory+"/"+np.m_name;
    boost::filesystem::create_directory(npEnergyDir);
    double zetaPotential = np.m_zetaName;
    double nanoparticleBoundingRadius;
    double nanoparticleOuterBoundingRadius;
    if(config.m_boundingRadius < 0){
    nanoparticleBoundingRadius = np.m_boundRadius;
    nanoparticleOuterBoundingRadius = np.m_outerBoundRadius;
    }
    else{
    nanoparticleBoundingRadius = config.m_boundingRadius;
    nanoparticleOuterBoundingRadius = config.m_boundingRadius+0.01;
    }
    std::cout << "Scanning from " << nanoparticleBoundingRadius << " to " << nanoparticleOuterBoundingRadius + 2.0 << "\n";
    Potentials potentials(surfaces, hamakerConstants, zetaPotential, nanoparticleBoundingRadius, config,np);
    for (const auto& pdb : pdbs){
    
    for( auto & omegaAngle : omegaAngleSet){
    SurfaceScan(pdb,potentials,zetaPotential,nanoparticleBoundingRadius,nanoparticleOuterBoundingRadius,config,omegaAngle,np);
    }
    
    
    }
    }
    

    return 0;
}

