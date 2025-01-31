#ifndef PDB_FILE__H__
#define PDB_FILE__H__

#include "StringFormat.h"

#include <cmath>
#include <string>
#include <vector>
#include <fstream>
#include <numeric>
#include <iostream>
#include <algorithm>
#include <unordered_map>

class PDB {
public:
    const std::vector<double>   m_x;
    const std::vector<double>   m_y;
    const std::vector<double>   m_z;
    const std::vector<int>      m_id;
    const double                m_length;
    const std::string           m_name;
    const std::vector<double>   m_occupancy;
    const std::vector<double>   m_rmsd;



    PDB(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& z,
    const std::vector<int>& id, const double length, const std::string& name, const std::vector<double>& occupancy, const std::vector<double>& rmsd)
        : m_x(x), m_y(y), m_z(z), m_id(id), m_length(length), m_name(name), m_occupancy(occupancy), m_rmsd(rmsd)
    {}
};

PDB ReadPDBFile(const std::string&, const std::unordered_map<std::string, std::size_t>&,     const std::unordered_map<std::string,std::string>&,     const int , const double, const double ,const bool   );

class PDBs : public std::vector<PDB> {
public:
    PDBs(const std::vector<std::string>& filenames,
        const std::unordered_map<std::string, std::size_t>& aminoAcidIdMap, 
        const std::unordered_map<std::string, std::string>& ligandMap,

const int inputDisorderStrat = 0, const double disorderMin = -5.0, const double disorderMax = 50.0, const bool readLigands = false) {
        this->reserve(filenames.size());
        for (const auto& filename : filenames) {
            this->push_back(ReadPDBFile(filename, aminoAcidIdMap, ligandMap, inputDisorderStrat, disorderMin, disorderMax, readLigands));
        }
    }
};

PDB ReadPDBFile(const std::string& filename, const std::unordered_map<std::string, std::size_t>& aminoAcidIdMap,   const std::unordered_map<std::string,std::string>& ligandMap,   const int inputDisorderStrat = 0, const double disorderMin = -5.0, const double disorderMax = 50.0, const bool readLigands=false) {
    std::ifstream handle(filename.c_str());
    if (!handle.is_open()) {
        std::cerr << "Error: Could not find pdb file '" << filename << "'\n";
        std::exit(1);
    }

    std::string line;
    std::string tag;
    int disorderStrat = inputDisorderStrat;

    
    std::vector<double> x;
    std::vector<double> y;
    std::vector<double> z;
    std::vector<int>    id;
    std::vector<double> occupancy;
    std::vector<double> bfactor;
    std::vector<double> rmsd;
    bool bIsAlphaFold = false;
    while (std::getline(handle, line)) {
        bool foundAtom = false;
        bool atomIsLigand = false;

        if( line.find("ALPHAFOLD") != std::string::npos){
        //std::cout <<"ALPHAFOLD detected, assuming this is an AlphaFold structure \n";
        bIsAlphaFold = true;
        }

        if( line.substr(0, 4) == "ATOM" && line.substr(13, 2) == "CA" ){
        foundAtom = true;
        tag = line.substr(17, 3);
        StringFormat::Strip(tag);

        }
        if( line.substr(0,6) == "HETATM" && readLigands == true){
        foundAtom = true;
        atomIsLigand = true;

        //attempt to parse line as a potential ligand
                std::string ligandID = line.substr(17,3);
                std::string ligandAtomID = line.substr(12,4) ;
                StringFormat::Strip(ligandID);
                StringFormat::Strip(ligandAtomID);
               std::string trialTag = "";
               if( ligandMap.count( ligandID+"-"+ligandAtomID) == 1 ){
               trialTag = ligandMap.at(ligandID+"-"+ligandAtomID)  ;
              }
              if(trialTag != ""){ 
              tag = trialTag;
              //std::cout << "Parsing " << ligandID << "-" << ligandAtomID << " as " << tag << "\n";

              }
              else{
              foundAtom = false;
              }
  
              //std::cout << "Not Parsing " << ligandID << "-" << ligandAtomID << " as no CG substitute found found \n";
              
        }


        if(line.size() > 3 && foundAtom == true) {
            try {
                x.emplace_back(0.1 * std::stod(line.substr(30, 8)));
                y.emplace_back(0.1 * std::stod(line.substr(38, 8)));
                z.emplace_back(0.1 * std::stod(line.substr(46, 8)));
                //std::cout << 0.1 * std::stod(line.substr(30, 8)) << "," << 0.1 * std::stod(line.substr(38, 8)) << "," << 0.1 * std::stod(line.substr(46, 8))<<"\n";
                //std::cout << line << "\n";
                //std::cout << line.substr(56,4) << "\n";
                occupancy.emplace_back(  std::stod(line.substr(54,6)));
                bfactor.emplace_back( std::stod(line.substr(60,6)));
                //tag = line.substr(16, 4); // When using lipids
                double rmsdVal = 0;
                if( bIsAlphaFold == false){
                rmsdVal = 0.1*sqrt(  std::stod( line.substr(60,6))/( 8.0 * 3.1415 * 3.1415) ) ;
                }
                else{
                rmsdVal = 0.05;
                }
                rmsd.emplace_back(rmsdVal);
                //std::cout << "read AA with bfactor" <<  std::stod(line.substr(60,6)) << " to  rmsd " << rmsdVal << "\n"; 
                id.emplace_back(aminoAcidIdMap.at(tag));
            }
            catch (const std::invalid_argument& ia) {
                std::cerr << "Error: Failed to parse pdb file '" << filename << "'\n";
                std::cerr << "Error: Failed on line: " << line << "\n";
                std::exit(1);
            }
            catch (const std::out_of_range& oor) {
                std::cerr << "Error: Encountered a molecule in the pdb file which was not in the config file\n";
                std::cerr << "Error: pdb filename = '" << filename << "', molecule name = '" << tag << "'\n";
                std::exit(1);
            }
        }
        else if (line.size() > 5 && line.substr(0, 6) == "ENDMDL") {
            break;
        }
    }

    handle.close();
     /*
    for (int i = 0; i < static_cast<int>(x.size()); ++i) {
        std::cout << x[i] << " " <<  y[i] << " " << z[i] << "\n";
    }
   */
   
   //Strategies for dealing with disordered residues:
   //0   = default = do nothing
   //1 = shift to COM defined by ordered residues, leave active
   // 2 = shift to COM defined by ordered residues, set occupancy to 0
   //note that disordered is defined using AlphaFold convention for bfactor, pIDDT < 50 = disordered. If you're using non-AF proteins then this will give bad results so set disorderStrat = 0 for those.
    // Center protein
    double mean_x = 0.0, mean_y = 0.0, mean_z = 0.0;
    double totalOcc = 0;
    double numRes = 0;
    for (int i = 0; i < static_cast<int>(x.size()); ++i) {
        int isDisordered = 0;
        if(bfactor[i]>disorderMin && bfactor[i] < disorderMax){
        isDisordered = 1;
        }
        if(disorderStrat == 2 && isDisordered == 1){
        occupancy[i] = 0.01;
        }
    
    
        if(disorderStrat == 0 ||  isDisordered == 0){ //if the residue is ordered or we don't care about disorder then it contributes to the COM
        mean_x += x[i]*occupancy[i];
        mean_y += y[i]*occupancy[i];
        mean_z += z[i]*occupancy[i];
        totalOcc += occupancy[i]; 
        }
        
        numRes +=1;

    }
   if(totalOcc > 0.5){
    mean_x /= totalOcc;
    mean_y /= totalOcc;
    mean_z /= totalOcc;
    }
    else{
    mean_x = 0;
    mean_y = 0;
    mean_z = 0;
    }
    for (int i = 0; i < static_cast<int>(x.size()); ++i) {
    
    
        int isDisordered = 0;
        if(bfactor[i]>disorderMin && bfactor[i] < disorderMax){
        isDisordered = 1;
        }
    
    
    if(disorderStrat == 0 || isDisordered == 0){ //apathetic to disorder or ordered: shift relative to COM
        x[i] -= mean_x;
        y[i] -= mean_y;
        z[i] -= mean_z;
    }
    else{ //disordered when we care about that: set to the COM
        x[i] = 0;
        y[i] = 0;
        z[i] = 0;
    }
      
    }
    
    // Lenght
    double max_x = 0.0, max_y = 0.0, max_z = 0.0;

    for (int i = 0; i < static_cast<int>(x.size()); ++i) {
        if (std::fabs(x[i]) > max_x) {
            max_x = std::fabs(x[i]);
        }
        if (std::fabs(y[i]) > max_y) {
            max_y = std::fabs(y[i]);
        }
        if (std::fabs(z[i]) > max_z) {
            max_z = std::fabs(z[i]);
        }
    }

    const double length = std::sqrt(max_x * max_x + max_y * max_y + max_z * max_z);

    std::string name = TargetList::Filename(filename);
    std::cout << numRes <<  " total CA, total occupancy: " << totalOcc << "\n";
    return PDB(x, y, z, id, length, name,occupancy,rmsd);
}



#endif
