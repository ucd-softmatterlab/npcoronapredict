#define PARALLEL

#include "Config.h"
#include "CommandLineParser.h"
#include "ConfigFileReader.h"
#include "HamakerConstants.h"
#include "TargetList.h"
#include "Surface.h"
#include "PDBFile.h"
#include "Potentials.h"

#include <omp.h>
#include <cmath>
#include <random>
#include <iomanip>

constexpr double        gds             = 0.22;
constexpr double        delta           = 0.8;
constexpr double        angle_delta     = 5.0 * (M_PI / 180.0);
constexpr int           ncols           = 72;
constexpr int           nrows           = 36;
constexpr int           iterations      = nrows * ncols;
constexpr int           samples         = 16;
constexpr int           steps           = 512;
constexpr double        dz              = delta / steps;

std::random_device randomEngine;
std::uniform_real_distribution<double> random_angle_offset(0.0, angle_delta);

void WriteMapFile(const double *adsorption_energy, const double *adsorption_error, const double radius,
        const double zeta, const std::string& name, const std::string& directory) {
    std::string filename = directory + "/" + TargetList::Filename(name) + "_" + std::to_string(static_cast<int>(radius))
        + "_" + std::to_string(static_cast<int>(1000 * zeta)) + ".map";
    std::clog << "Info: Saving map to: "<< filename << "\n";
    std::ofstream handle(filename.c_str());
    double phi, theta;
    for (int i = 0; i < iterations; ++i) { 
        phi   = (i % ncols) * 5.0;
        theta = (i / ncols) * 5.0;
        handle << std::left << std::setw(7) << std::fixed << std::setprecision(1) << phi;
        handle << std::left << std::setw(7) << std::fixed << std::setprecision(1) << theta;
        handle << std::left << std::setw(14) << std::fixed << std::setprecision(5) << adsorption_energy[i];
        handle << std::left << std::setw(14) << std::fixed << std::setprecision(5) << adsorption_error[i];
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
   
    for (int i = 0; i < iterations; ++i) {
        sin_theta       = std::sin((i / ncols) * angle_delta);
        T               += adsorption_energy[i] * sin_theta * std::exp(-1.0 * adsorption_energy[i]);
        Z               += sin_theta * std::exp(-1.0 * adsorption_energy[i]);
        simpleAverage   += sin_theta * adsorption_energy[i];
        error           += sin_theta * adsorption_error[i];
    }

    simpleAverage   /= iterations;
    error           /= iterations;

    std::cout << std::setw(10) << std::left << TargetList::Filename(filename);
    std::cout << std::setw(10) << std::fixed << std::setprecision(1) << radius;
    std::cout << std::setw(14) << std::fixed << std::setprecision(5) << simpleAverage;
    std::cout << std::setw(14) << std::fixed << std::setprecision(5) << (T/Z);
    std::cout << std::setw(14) << std::fixed << std::setprecision(5) << error;
    std::cout << std::endl;
}

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

void SquareXY (const int size, double *x, double *y) {
    for (int i = 0; i < size; ++i) {
        x[i] *= x[i];
        y[i] *= y[i];
    }
}

void ShiftZ (const int size, double *z) {
    const double z_shift = *std::max_element(z, z + size);
    for (int i = 0; i < size; ++i) {
        z[i] -= z_shift;
    }
}

void Sum (const int size, double* result, double *arr) {
    *result = 0.0;
    for (int i = 0; i < size; ++i) {
        *result += arr[i];
    }
}

void Integrate(const int size, const double dz, const double init_energy, const double *energy, const double *ssd, double *adsorption) {
    long double area = 0.0;
    for (int i = 0; i < size; ++i) {
        area += static_cast<long double>(ssd[i] * ssd[i] * dz) * std::exp(static_cast<long double>(-1.0 * (energy[i] - init_energy))); 
    }
    const double factor = 3.0 / std::fabs(std::pow(ssd[0], 3.0) - std::pow(ssd[size - 1], 3.0));
    *adsorption = -1.0 * std::log(factor * area);
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

void AdsorptionEnergies(const PDB& pdb, const Potentials& potentials, const double radius, const int angle_offset, const int n_angles, double *adsorption_energy, double *adsorption_error) { 
    // Decleare all variables at the begining 
    const int               size            = pdb.m_id.size();
    const double            start           = radius + gds;
    const double            stop            = start + delta;

    int                     i;
    int                     j;
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
    double energy[size];
    double total_energy[steps];
    double SSD[steps];
    double sample_energy[samples];

    // Iterate over angles
    for (int angle = 0; angle < n_angles; ++angle) {
        
        phi   = ((angle_offset + angle) % ncols) * angle_delta * M_PI / 180.0;
        theta = ((angle_offset + angle) / ncols) * angle_delta * M_PI / 180.0;

	    // Sample a angle multiple times 
        for (int sample = 0; sample < samples; ++sample) {

    	    // Rotation step
    	    phi_adjusted   = -1.0 * (phi + random_angle_offset(randomEngine));
    	    theta_adjusted = M_PI - (theta + random_angle_offset(randomEngine));
            Rotate(size, phi_adjusted, theta_adjusted, pdb.m_x, pdb.m_y, pdb.m_z, x, y, z);
   
            // Convert to SSD
            ShiftZ(size, z);

            // Pre-square x and y
            SquareXY (size, x, y);
            
            // Get bulk energy
            for (i = 0; i < size; ++i) {
                distance  = std::sqrt(x[i] + y[i] + (z[i] - stop) * (z[i] - stop)) - radius; // Center To Surface Distance
                energy[i] = static_cast<double>(potentials[pdb.m_id[i]].Value(distance));
            }
            Sum(size, &init_energy, energy);

            // Integration step
            for (i = 0; i < steps; ++i) {
                ssd     = start + i * dz;
                SSD[i]  = ssd;
                for (j = 0; j < size; ++j) {
                    distance  = std::sqrt(x[j] + y[j] + (z[j] - ssd) * (z[j] - ssd)) - radius; // Center To Surface Distance
                    energy[j] = static_cast<double>(potentials[pdb.m_id[j]].Value(distance));
                }
                Sum(size, &(total_energy[i]), energy);
            }
            
            // Integrate the results
            Integrate(steps, dz, init_energy, total_energy, SSD, &(sample_energy[sample]));
        }
  
        // Mean of all the samples
        MeanAndSD(samples, &(adsorption_energy[angle_offset + angle]), &(adsorption_error[angle_offset + angle]), sample_energy); 
    }
}

void SurfaceScan(const PDB& pdb, const Potentials& potentials, const double zeta, const double radius, const Config& config) {
    std::clog << "Info: Processing '" << pdb.m_name << "' (R = " << radius << ")\n";

    double adsorption_energy[iterations] = {};
    double adsorption_error[iterations]  = {};
    
    const int n_threads         = omp_get_max_threads();
    const int n_angles          = 1 + iterations / static_cast<double>(n_threads);
    const int n_angles_final    = n_threads * n_angles - iterations;

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
                    potentials, 
                    radius, 
                    thread * n_angles, 
                    (thread == n_threads - 1 ? n_angles_final : n_angles), 
                    adsorption_energy, 
                    adsorption_error
            ); 
        }
    }
    

    WriteMapFile(adsorption_energy, adsorption_error, radius, zeta, pdb.m_name, config.m_outputDirectory); 
    PrintStatistics(adsorption_energy, adsorption_error, radius, pdb.m_name);
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
    PDBs              pdbs(targetList.m_paths, config.AminoAcidIdMap());

    for (const double nanoparticleRadius : config.m_nanoparticleRadii) {
        for (const double zetaPotential : config.m_zetaPotential) {
            Potentials potentials(surfaces, hamakerConstants, zetaPotential, nanoparticleRadius, config);
            for (const auto& pdb : pdbs) {
                SurfaceScan(pdb, potentials, zetaPotential, nanoparticleRadius, config);
            }
        }
    }

    return 0;
}

