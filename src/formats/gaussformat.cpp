/**********************************************************************
Copyright (C) 2000 by OpenEye Scientific Software, Inc.
Some portions Copyright (C) 2001-2010 by Geoffrey R. Hutchison
Some portions Copyright (C) 2004 by Chris Morley

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
***********************************************************************/
#include <openbabel/babelconfig.h>

#include <openbabel/obmolecformat.h>

using namespace std;
namespace OpenBabel
{

  class GaussianOutputFormat : public OBMoleculeFormat
  {
  public:
    //Register this format type ID
    GaussianOutputFormat()
    {
      OBConversion::RegisterFormat("gal",this, "chemical/x-gaussian-log");
      OBConversion::RegisterFormat("g92",this);
      OBConversion::RegisterFormat("g94",this);
      OBConversion::RegisterFormat("g98",this);
      OBConversion::RegisterFormat("g03",this);
      OBConversion::RegisterFormat("g09",this);
    }

    virtual const char* Description() //required
    {
      return
        "Gaussian Output\n"
        "Read Options e.g. -as\n"
        "  s  Output single bonds only\n"
        "  b  Disable bonding entirely\n\n";
    };

    virtual const char* SpecificationURL()
    { return "http://www.gaussian.com/";};

    virtual const char* GetMIMEType()
    { return "chemical/x-gaussian-log"; };

    //Flags() can return be any the following combined by | or be omitted if none apply
    // NOTREADABLE  READONEONLY  NOTWRITABLE  WRITEONEONLY
    virtual unsigned int Flags()
    {
      return READONEONLY | NOTWRITABLE;
    };

    /// The "API" interface functions
    virtual bool ReadMolecule(OBBase* pOb, OBConversion* pConv);
  };

  //Make an instance of the format class
  GaussianOutputFormat theGaussianOutputFormat;

  class GaussianInputFormat : public OBMoleculeFormat
  {
  public:
    //Register this format type ID
    GaussianInputFormat()
    {
      OBConversion::RegisterFormat("com",this, "chemical/x-gaussian-input");
      OBConversion::RegisterFormat("gau",this);
      OBConversion::RegisterFormat("gjc",this);
      OBConversion::RegisterFormat("gjf",this);
      OBConversion::RegisterOptionParam("b", NULL, 0, OBConversion::OUTOPTIONS);
      // Command-line keywords
      OBConversion::RegisterOptionParam("k", NULL, 1, OBConversion::OUTOPTIONS);
      // Command-line keyword file
      OBConversion::RegisterOptionParam("f", NULL, 1, OBConversion::OUTOPTIONS);    }

    virtual const char* Description() //required
    {
      return
        "Gaussian 98/03 Input\n"
        "Write Options e.g. -xk\n"
        "  b               Output includes bonds\n"
        "  k  \"keywords\" Use the specified keywords for input\n"
        "  f    <file>     Read the file specified for input keywords\n"
        "  u               Write the crystallographic unit cell, if present.\n\n";
    };

    virtual const char* SpecificationURL()
    {return "http://www.gaussian.com/g_ur/m_input.htm";};

    virtual const char* GetMIMEType()
    { return "chemical/x-gaussian-input"; };

    //Flags() can return be any the following combined by | or be omitted if none apply
    // NOTREADABLE  READONEONLY  NOTWRITABLE  WRITEONEONLY
    virtual unsigned int Flags()
    {
      return NOTREADABLE | WRITEONEONLY;
    };

    ////////////////////////////////////////////////////
    /// The "API" interface functions
    virtual bool WriteMolecule(OBBase* pOb, OBConversion* pConv);

  };

  //Make an instance of the format class
  GaussianInputFormat theGaussianInputFormat;

  ////////////////////////////////////////////////////////////////

  bool GaussianInputFormat::WriteMolecule(OBBase* pOb, OBConversion* pConv)
  {
    OBMol* pmol = dynamic_cast<OBMol*>(pOb);
    if(pmol==NULL)
      return false;

    //Define some references so we can use the old parameter names
    ostream &ofs = *pConv->GetOutStream();
    OBMol &mol = *pmol;

    char buffer[BUFF_SIZE];
    const char *keywords = pConv->IsOption("k",OBConversion::OUTOPTIONS);
    const char *keywordsEnable = pConv->IsOption("k",OBConversion::GENOPTIONS);
    const char *keywordFile = pConv->IsOption("f",OBConversion::OUTOPTIONS);
    bool writeUnitCell = (NULL != pConv->IsOption("u", OBConversion::OUTOPTIONS));
    string defaultKeywords = "#Put Keywords Here, check Charge and Multiplicity.";

    if(keywords)
      {
        defaultKeywords = keywords;
      }

    if (keywordsEnable)
      {
        string model;
        string basis;
        string method;

        OBPairData *pd = (OBPairData *) pmol->GetData("model");
        if(pd)
          model = pd->GetValue();

        pd = (OBPairData *) pmol->GetData("basis");
        if(pd)
          basis = pd->GetValue();

        pd = (OBPairData *) pmol->GetData("method");
        if(pd)
          method = pd->GetValue();

        if(method == "optimize")
          {
            method = "opt";
          }

        if(model != "" && basis != "" && method != "")
          {
            ofs << model << "/" << basis << "," << method << endl;
          }
        else
          {
            ofs << "#Unable to translate keywords!" << endl;
            ofs << defaultKeywords << endl;
          }
      }
    else if (keywordFile)
      {
        ifstream kfstream(keywordFile);
        string keyBuffer;
        if (kfstream)
          {
            while (getline(kfstream, keyBuffer))
              ofs << keyBuffer << endl;
          }
      }
    else
      {
        ofs << defaultKeywords << endl;
      }
    ofs << endl; // blank line after keywords
    ofs << " " << mol.GetTitle() << endl << endl;

    snprintf(buffer, BUFF_SIZE, "%d  %d",
             mol.GetTotalCharge(),
             mol.GetTotalSpinMultiplicity());
    ofs << buffer << endl;

    FOR_ATOMS_OF_MOL(atom, mol)
      {
        if (atom->GetIsotope() == 0)
          snprintf(buffer, BUFF_SIZE, "%-3s      %10.5f      %10.5f      %10.5f",
                   etab.GetSymbol(atom->GetAtomicNum()),
                   atom->GetX(), atom->GetY(), atom->GetZ());
        else
          snprintf(buffer, BUFF_SIZE, "%-3s(Iso=%d) %10.5f      %10.5f      %10.5f",
                   etab.GetSymbol(atom->GetAtomicNum()),
                   atom->GetIsotope(),
                   atom->GetX(), atom->GetY(), atom->GetZ());

        ofs << buffer << endl;
      }
    // Translation vectors
    OBUnitCell *uc = (OBUnitCell*)mol.GetData(OBGenericDataType::UnitCell);
    if (uc && writeUnitCell) {
      uc->FillUnitCell(&mol); // complete the unit cell with symmetry-derived atoms

      vector<vector3> cellVectors = uc->GetCellVectors();
      for (vector<vector3>::iterator i = cellVectors.begin(); i != cellVectors.end(); ++i) {
          snprintf(buffer, BUFF_SIZE, "TV       %10.5f      %10.5f      %10.5f",
                   i->x(),
                   i->y(),
                   i->z());
        ofs << buffer << '\n';
      }
    }

    // Bonds, contributed by Daniel Mansfield
    if (pConv->IsOption("b",OBConversion::OUTOPTIONS))
    {
      // first, make begin.GetIdx < end.GetIdx
      OBBond* bond;
      OBAtom *atom;
      vector<OBEdgeBase*>::iterator j;
      vector<OBNodeBase*>::iterator i;
      OBAtom *bgn, *end;
      for (bond = mol.BeginBond(j); bond; bond = mol.NextBond(j))
        {
          if (bond->GetBeginAtomIdx() > bond->GetEndAtomIdx()) {
            bgn = bond->GetBeginAtom();
            end = bond->GetEndAtom();
            bond->SetBegin(end);
            bond->SetEnd(bgn);
          }
        }

      // this seems inefficient -- perhaps using atom neighbor iterators?
      // -GRH
      for (atom = mol.BeginAtom(i);atom;atom = mol.NextAtom(i))
        {
          ofs << endl << atom->GetIdx() << " ";
          for (bond = mol.BeginBond(j); bond; bond = mol.NextBond(j))
            {
              if (bond->GetBeginAtomIdx() == atom->GetIdx()) {
                snprintf(buffer, BUFF_SIZE, "%d %1.1f ", bond->GetEndAtomIdx(), (float) bond->GetBondOrder());
                ofs << buffer;
              }
            }
        } // iterate through atoms
    } // end writing bonds

    // file should end with a blank line
    ofs << endl;
    return(true);
  }

  static int extract_g234(OpenBabel::OBMol *mol,char *method,
                          double ezpe,double etherm,double eg234)
  {
    // Initiate correction database
    OpenBabel::OBAtomicHeatOfFormationTable *ahof = new OpenBabel::OBAtomicHeatOfFormationTable();
    OpenBabel::OBAtomIterator OBai;
    OpenBabel::OBAtom *OBa;
    OpenBabel::OBElementTable *OBet;
    OpenBabel::OBPairData *OBpd0;
    const char *attr[2] = { "DHf(0K)", "DHf(298.15K)" };
    char valbuf[128];
    int ii,atomid,atomicnumber,found,foundall;
    double ddd0,ddd298,dhof[2];
    
    OBet = new OpenBabel::OBElementTable();
    
    // cout << "Number of entries " << ahof->GetSize() << "\n";
    // cout << "Method " << method << "\n";
    // Now loop over atoms in order to correct the Delta H formation
    OBai = mol->BeginAtoms();
    atomid = 0;
    foundall = 0;
#define HARTREE_TO_KCAL 627.509469
    dhof[0] = eg234;
    dhof[1] = eg234+etherm-ezpe;
    for (OBa = mol->BeginAtom(OBai); (NULL != OBa); OBa = mol->NextAtom(OBai)) 
      {
        atomicnumber = OBa->GetAtomicNum();
        found = ahof->GetHeatOfFormation(OBet->GetSymbol(atomicnumber),method,1,&ddd0,&ddd298);
        if (1 == found)
          {
            dhof[0] += ddd0;
            dhof[1] += ddd298;
            foundall ++;
          }
        //cout << "Atom "<< atomid << " type " << OBa->GetType() << " atomicnumber " << atomicnumber << " element " << OBet->GetSymbol(atomicnumber) <<"\n";
        atomid++;
      }
    if (foundall == atomid) 
      {
        std::string str("method");
        std::string val(method);
        OBpd0   = new OpenBabel::OBPairData();
        OBpd0->SetAttribute(str);
        OBpd0->SetValue(val);
        OBpd0->SetOrigin(fileformatInput);
        mol->SetData(OBpd0);
        for(ii=0; (ii<2); ii++) 
          {
            // Add to molecule properties
            dhof[ii] *= HARTREE_TO_KCAL;
            sprintf(valbuf,"%f",dhof[ii]);
            std::string str(attr[ii]);
            std::string val(valbuf);
            OBpd0   = new OpenBabel::OBPairData();
            OBpd0->SetAttribute(str);
            OBpd0->SetValue(val);
            OBpd0->SetOrigin(fileformatInput);
            mol->SetData(OBpd0);
            
            // cout << str << " = " << val << " kcal/mol.\n";
          }
      }
    else
      {
        // Debug message?
      }
    // Clean up
    delete OBet;
    delete ahof;
    
    if (foundall == atomid)
      return 1;
    else
      return 0;
  }

  static void add_unique_pairdata_to_mol(OpenBabel::OBMol *mol,
                                         const char *attribute,
                                         string buffer,int start)
  {
    int i;
    vector<string> vs;
    OpenBabel::OBPairData *pd;
    std::string method;
    
    tokenize(vs,buffer);
    if (vs.size() >= start) 
      {
        method = vs[start];
        for(i=start+1; (i<vs.size()); i++) 
          {
            method.append(" ");
            method.append(vs[i]);
          }
        pd = (OpenBabel::OBPairData *) mol->GetData(attribute);
        if (NULL == pd) 
          {
            pd = new OpenBabel::OBPairData();
            pd->SetAttribute(attribute);
            pd->SetOrigin(fileformatInput);
            pd->SetValue(method);
            mol->SetData(pd);
          }
        else 
          {
            pd->SetValue(method);
          }
        }  
  }

  // Reading Gaussian output has been tested for G98 and G03 to some degree
  // If you have problems (or examples of older output), please contact
  // the openbabel-discuss@lists.sourceforge.net mailing list and/or post a bug
  bool GaussianOutputFormat::ReadMolecule(OBBase* pOb, OBConversion* pConv)
  {
    OBMol* pmol = pOb->CastAndClear<OBMol>();
    if(pmol==NULL)
      return false;

    //Define some references so we can use the old parameter names
    istream &ifs = *pConv->GetInStream();
    OBMol &mol = *pmol;
    const char* title = pConv->GetTitle();

    char buffer[BUFF_SIZE],method[BUFF_SIZE];
    string str,str1,str2;
    double x,y,z;
    OBAtom *atom;
    vector<string> vs,vs2;
    int charge = 0;
    unsigned int spin = 1;
    bool hasPartialCharges = false;
    string chargeModel; // descriptor for charges (e.g. "Mulliken")

    // Variable for G2/G3/G4 etc. calculations
    double ezpe,etherm,eg234;
    int ezpe_set=0,etherm_set=0,eg234_set=0;

    // Electrostatic potential
    OBFreeGrid *esp = NULL;
    
    // coordinates of all steps
    // Set conformers to all coordinates we adopted
    std::vector<double*> vconf; // index of all frames/conformers
    std::vector<double> coordinates; // coordinates in each frame
    int natoms = 0; // number of atoms -- ensure we don't go to a new job with a different molecule

    // OBConformerData stores information about multiple steps
    // we can change attribute later if needed (e.g., IRC)
    OBConformerData *confData = new OBConformerData();
    confData->SetOrigin(fileformatInput);
    std::vector<unsigned short> confDimensions = confData->GetDimension(); // to be fair, set these all to 3D
    std::vector<double>         confEnergies   = confData->GetEnergies();
    std::vector< std::vector< vector3 > > confForces = confData->GetForces();

    //Vibrational data
    std::vector< std::vector< vector3 > > Lx;
    std::vector<double> Frequencies, Intensities;
    //Rotational data
    std::vector<double> RotConsts(3);
    int RotSymNum=1;
    OBRotationData::RType RotorType;

    // Translation vectors (if present)
    vector3 translationVectors[3];
    int numTranslationVectors = 0;

    //Electronic Excitation data
    std::vector<double> Forces, Wavelengths, EDipole,
      RotatoryStrengthsVelocity, RotatoryStrengthsLength;

    // Orbital data
    std::vector<double> orbitals;
    std::vector<std::string> symmetries;
    int aHOMO, bHOMO, betaStart;

    int i=0;
    bool no_symmetry=false;
    char coords_type[25];

    //Prescan file to find second instance of "orientation:"
    //This will be the kind of coords used in the chk/fchk file
    //Unless the "nosym" keyword has been requested
    while (ifs.getline(buffer,BUFF_SIZE))
      {
        if (strstr(buffer,"Symmetry turned off by external request.") != NULL)
          {
            // The "nosym" keyword has been requested
            no_symmetry = true;
          }
        if (strstr(buffer,"orientation:") !=NULL)
          {
            i++;
            tokenize (vs, buffer);
            strcpy (coords_type, vs[0].c_str());
            strcat (coords_type, " orientation:");
          }
        if ((no_symmetry && i==1) || i==2)
           break;
      }
    // Reset end-of-file pointers etc.
    ifs.clear();
    ifs.seekg(0);  //rewind

    mol.BeginModify();
    while (ifs.getline(buffer,BUFF_SIZE))
      {
        
        if(strstr(buffer, "Entering Gaussian") != NULL)
        {
          //Put some metadata into OBCommentData
          string comment("Gaussian ");

          if(NULL != strchr(buffer,'='))
            {
            comment += strchr(buffer,'=')+2;
            comment += "";
            for(unsigned i=0; i<115, ifs; ++i)
            {
              ifs.getline(buffer,BUFF_SIZE);
              if(strstr(buffer,"Revision") != NULL)
                {
                  if (buffer[strlen(buffer)-1] == ',')
                    {
                      buffer[strlen(buffer)-1] = '\0';
                    }
                  add_unique_pairdata_to_mol(&mol,"program",buffer,0);
                }
              else if(buffer[1]=='#')
              {
                //the line describing the method
                comment += buffer;
                OBCommentData *cd = new OBCommentData;
                cd->SetData(comment);
                cd->SetOrigin(fileformatInput);
                mol.SetData(cd);
                
                tokenize(vs,buffer);
                if (vs.size() > 1) 
                  {
                    char *str = strdup(vs[1].c_str());
                    char *ptr = strchr(str,'/');
                    
                    if (NULL != ptr)
                      {
                        *ptr = ' ';
                        add_unique_pairdata_to_mol(&mol,"basis",ptr,0);
                        *ptr = '\0';
                        add_unique_pairdata_to_mol(&mol,"method",str,0);
                      }
                  }
                
                break;
              }
            }
          }
        }

        else if (strstr(buffer,"Multiplicity") != NULL)
          {
            tokenize(vs, buffer, " \t\n");
            if (vs.size() == 6)
              {
                charge = atoi(vs[2].c_str());
                spin = atoi(vs[5].c_str());
              }

            ifs.getline(buffer,BUFF_SIZE);
          }
        else if (strstr(buffer, coords_type) != NULL)
          {
            numTranslationVectors = 0; // ignore old translationVectors
            ifs.getline(buffer,BUFF_SIZE);      // ---------------
            ifs.getline(buffer,BUFF_SIZE);      // column headings
            ifs.getline(buffer,BUFF_SIZE);	// column headings
            ifs.getline(buffer,BUFF_SIZE);	// ---------------
            ifs.getline(buffer,BUFF_SIZE);
            tokenize(vs,buffer);
            while (vs.size()>4)
              {
                int corr = vs.size()==5 ? -1 : 0; //g94; later versions have an extra column
                x = atof((char*)vs[3+corr].c_str());
                y = atof((char*)vs[4+corr].c_str());
                z = atof((char*)vs[5+corr].c_str());
                int atomicNum = atoi((char*)vs[1].c_str());

                if (atomicNum > 0) // translation vectors are "-2"
                  {
                    if (natoms == 0) { // first time reading the molecule, create each atom
                      atom = mol.NewAtom();
                      atom->SetAtomicNum(atoi((char*)vs[1].c_str()));
                    }
                    coordinates.push_back(x);
                    coordinates.push_back(y);
                    coordinates.push_back(z);
                  }
                else {
                  translationVectors[numTranslationVectors++].Set(x, y, z);
                }

                if (!ifs.getline(buffer,BUFF_SIZE)) {
                  break;
                }
                tokenize(vs,buffer);
              }
            // done with reading atoms
            natoms = mol.NumAtoms();
            if(natoms==0)
              return false;
            // malloc / memcpy
            double *tmpCoords = new double [(natoms)*3];
            memcpy(tmpCoords, &coordinates[0], sizeof(double)*natoms*3);
            vconf.push_back(tmpCoords);
            coordinates.clear();
            confDimensions.push_back(3); // always 3D -- OBConformerData allows mixing 2D and 3D structures
          }
        else if(strstr(buffer,"Dipole moment") != NULL)
            {
              ifs.getline(buffer,BUFF_SIZE); // actual components   X ###  Y #### Z ###
              tokenize(vs,buffer);
              if (vs.size() >= 6)
                {
                  OBVectorData *dipoleMoment = new OBVectorData;
                  dipoleMoment->SetAttribute("Dipole Moment");
                  double x, y, z;
                  x = atof(vs[1].c_str());
                  y = atof(vs[3].c_str());
                  z = atof(vs[5].c_str());
                  dipoleMoment->SetData(x, y, z);
                  dipoleMoment->SetOrigin(fileformatInput);
                  mol.SetData(dipoleMoment);
                }
              if (!ifs.getline(buffer,BUFF_SIZE)) break;
            }
        else if(strstr(buffer,"Traceless Quadrupole moment") != NULL)
            {
              ifs.getline(buffer,BUFF_SIZE); // actual components XX ### YY #### ZZ ###
              tokenize(vs,buffer);
              ifs.getline(buffer,BUFF_SIZE); // actual components XY ### XZ #### YZ ###
              tokenize(vs2,buffer);
              if ((vs.size() >= 6) && (vs2.size() >= 6))
                {
                  double Q[3][3];
                  OpenBabel::OBMatrixData *quadrupoleMoment = new OpenBabel::OBMatrixData;
                  
                  Q[0][0] = atof(vs[1].c_str());
                  Q[1][1] = atof(vs[3].c_str());
                  Q[2][2] = atof(vs[5].c_str());
                  Q[1][0] = Q[0][1] = atof(vs2[1].c_str());
                  Q[2][0] = Q[0][2] = atof(vs2[3].c_str());
                  Q[2][1] = Q[1][2] = atof(vs2[5].c_str());
                  matrix3x3 quad(Q);
                  
                  quadrupoleMoment->SetAttribute("Traceless Quadrupole Moment");
                  quadrupoleMoment->SetData(quad);
                  quadrupoleMoment->SetOrigin(fileformatInput);
                  mol.SetData(quadrupoleMoment);
                }
              if (!ifs.getline(buffer,BUFF_SIZE)) break;
            }
        else if(strstr(buffer,"Exact polarizability") != NULL)
            {
              // actual components XX, YX, YY, XZ, YZ, ZZ 
              tokenize(vs,buffer);
              if (vs.size() >= 8)
                {
                  double Q[3][3];
                  OpenBabel::OBMatrixData *pol_tensor = new OpenBabel::OBMatrixData;
                  
                  Q[0][0] = atof(vs[2].c_str());
                  Q[1][1] = atof(vs[4].c_str());
                  Q[2][2] = atof(vs[7].c_str());
                  Q[1][0] = Q[0][1] = atof(vs[3].c_str());
                  Q[2][0] = Q[0][2] = atof(vs[5].c_str());
                  Q[2][1] = Q[1][2] = atof(vs[6].c_str());
                  matrix3x3 pol(Q);
                  
                  pol_tensor->SetAttribute("Exact polarizability");
                  pol_tensor->SetData(pol);
                  pol_tensor->SetOrigin(fileformatInput);
                  mol.SetData(pol_tensor);
                }
              if (!ifs.getline(buffer,BUFF_SIZE)) break;
            }
        else if(strstr(buffer,"Total atomic charges") != NULL ||
                strstr(buffer,"Mulliken atomic charges") != NULL)
          {
            hasPartialCharges = true;
            chargeModel = "Mulliken";
            ifs.getline(buffer,BUFF_SIZE);	// column headings
            ifs.getline(buffer,BUFF_SIZE);
            tokenize(vs,buffer);
            while (vs.size() >= 3 &&
                   strstr(buffer,"Sum of ") == NULL)
              {
                atom = mol.GetAtom(atoi(vs[0].c_str()));
                if (!atom)
                  break;
                atom->SetPartialCharge(atof(vs[2].c_str()));

                if (!ifs.getline(buffer,BUFF_SIZE)) break;
                tokenize(vs,buffer);
              }
          }
        else if (strstr(buffer, "Atomic Center") != NULL)
          {
            // Data points for ESP calculation
            tokenize(vs,buffer);
            if (NULL == esp)
              esp = new OpenBabel::OBFreeGrid();
            if (vs.size() == 8)
              {
                esp->AddPoint(atof(vs[5].c_str()),atof(vs[6].c_str()),
                              atof(vs[7].c_str()),0);
              }
            else if (vs.size() > 5) 
              {
                double x,y,z;
                if (3 == sscanf(buffer+32,"%10.6f%10.f6%10.6f",&x,&y,&z))
                  {
                    esp->AddPoint(x,y,z,0);
                  }
              }
          }
        else if (strstr(buffer, "ESP Fit Center") != NULL)
          {
            // Data points for ESP calculation
            tokenize(vs,buffer);
            if (NULL == esp)
              esp = new OpenBabel::OBFreeGrid();
            if (vs.size() == 9) 
              {
                esp->AddPoint(atof(vs[6].c_str()),atof(vs[7].c_str()),
                              atof(vs[8].c_str()),0);
              }
            else if (vs.size() > 6) 
              {
                double x,y,z;
                if (3 == sscanf(buffer+32,"%10.6f%10.f6%10.6f",&x,&y,&z))
                  {
                    esp->AddPoint(x,y,z,0);
                  }
              }
          }
        else if (strstr(buffer, "Electrostatic Properties (Atomic Units)") != NULL)
          {
            int i,np;
            OpenBabel::OBFreeGridPoint *fgp;
            OpenBabel::OBFreeGridPointIterator fgpi;
            for(i=0; (i<5); i++)
              {
                ifs.getline(buffer,BUFF_SIZE);	// skip line
              }
            // Assume file is correct and that potentials are present 
            // where they should.
            np = esp->NumPoints();
            fgpi = esp->BeginPoints();
            i = 0;
            for(fgp = esp->BeginPoint(fgpi); (NULL != fgp); fgp = esp->NextPoint(fgpi))
              {
                ifs.getline(buffer,BUFF_SIZE);
                tokenize(vs,buffer);
                if (vs.size() >= 2) 
                  {
                    fgp->SetV(atof(vs[2].c_str()));
                    i++;
                  }
              }
            if (i == np)
              {
                esp->SetAttribute("Electrostatic Potential");
                mol.SetData(esp);
              }
            else
              {
                cout << "Read " << esp->NumPoints() << " ESP points i = " << i << "\n";
              }
          }
        else if (strstr(buffer, "Charges from ESP fit") != NULL)
          {
            hasPartialCharges = true;
            chargeModel = "ESP";
            ifs.getline(buffer,BUFF_SIZE);	// Charge / dipole line
            ifs.getline(buffer,BUFF_SIZE); // column header
            ifs.getline(buffer,BUFF_SIZE); // real charges
            tokenize(vs,buffer);
            while (vs.size() >= 3 &&
                   strstr(buffer,"-----") == NULL)
              {
                atom = mol.GetAtom(atoi(vs[0].c_str()));
                if (!atom)
                  break;
                atom->SetPartialCharge(atof(vs[2].c_str()));

                if (!ifs.getline(buffer,BUFF_SIZE)) break;
                tokenize(vs,buffer);
              }
          }
        else if(strstr(buffer,"Natural Population") != NULL)
          {
            hasPartialCharges = true;
            chargeModel = "NBO";
            ifs.getline(buffer,BUFF_SIZE);	// column headings
            ifs.getline(buffer,BUFF_SIZE);  // again
            ifs.getline(buffer,BUFF_SIZE);  // again (-----)
            ifs.getline(buffer,BUFF_SIZE); // real data
            tokenize(vs,buffer);
            while (vs.size() >= 3 &&
                   strstr(buffer,"=====") == NULL)
              {
                atom = mol.GetAtom(atoi(vs[1].c_str()));
                if (!atom)
                  break;
                atom->SetPartialCharge(atof(vs[2].c_str()));

                if (!ifs.getline(buffer,BUFF_SIZE)) break;
                tokenize(vs,buffer);
              }
          }
        else if(strstr(buffer, " Frequencies -- ")) //vibrational frequencies
        {
          //The info should appear only once as several blocks starting with this line
          tokenize(vs, buffer);
          for(unsigned int i=2; i<vs.size(); ++i)
            Frequencies.push_back(atof(vs[i].c_str()));
          ifs.getline(buffer,BUFF_SIZE); //Red. masses
          ifs.getline(buffer,BUFF_SIZE); //Frc consts
          ifs.getline(buffer,BUFF_SIZE); //IR Inten
          tokenize(vs, buffer);
          for(unsigned int i=3; i<vs.size(); ++i)
            Intensities.push_back(atof(vs[i].c_str()));

          ifs.getline(buffer, BUFF_SIZE); // column labels or Raman intensity
          if(strstr(buffer, "Raman Activ")) {
            ifs.getline(buffer, BUFF_SIZE); // Depolar (P)
            ifs.getline(buffer, BUFF_SIZE); // Depolar (U)
            ifs.getline(buffer, BUFF_SIZE); // column labels
          }
          ifs.getline(buffer, BUFF_SIZE); // actual displacement data
          tokenize(vs, buffer);
          vector<vector3> vib1, vib2, vib3;
          double x, y, z;
          while(vs.size() > 5) {
            for (unsigned int i = 2; i < vs.size()-2; i += 3) {
              x = atof(vs[i].c_str());
              y = atof(vs[i+1].c_str());
              z = atof(vs[i+2].c_str());

              if (i == 2)
                vib1.push_back(vector3(x, y, z));
              else if (i == 5)
                vib2.push_back(vector3(x, y, z));
              else if (i == 8)
                vib3.push_back(vector3(x, y, z));
            }

            if (!ifs.getline(buffer, BUFF_SIZE))
              break;
            tokenize(vs,buffer);
          }
          Lx.push_back(vib1);
          if (vib2.size())
            Lx.push_back(vib2);
          if (vib3.size())
            Lx.push_back(vib3);
        }

        else if(strstr(buffer, " This molecule is "))//rotational data
        {
          if(strstr(buffer, "asymmetric"))
            RotorType = OBRotationData::ASYMMETRIC;
          else if(strstr(buffer, "symmetric"))
            RotorType = OBRotationData::SYMMETRIC;
          else if(strstr(buffer, "linear"))
            RotorType = OBRotationData::LINEAR;
          else
             RotorType = OBRotationData::UNKNOWN;
          ifs.getline(buffer,BUFF_SIZE); //symmetry number
          tokenize(vs, buffer);
          RotSymNum = atoi(vs[3].c_str());
        }

        else if(strstr(buffer, "Rotational constant"))
        {
          tokenize(vs, buffer);
          RotConsts.clear();
          for (unsigned int i=3; i<vs.size(); ++i)
            RotConsts.push_back(atof(vs[i].c_str()));
        }

        else if(strstr(buffer, "alpha electrons")) // # of electrons / orbital
        {
          tokenize(vs, buffer);
          if (vs.size() == 6) {
            // # alpha electrons # beta electrons
            aHOMO = atoi(vs[0].c_str());
            bHOMO = atoi(vs[3].c_str());
          }
        }
        else if(strstr(buffer, "rbital symmetries")) // orbital symmetries
          {
            symmetries.clear();
            std::string label; // used as a temporary to remove "(" and ")" from labels
            int iii,offset = 0;
            bool bDoneSymm;
            
            // Extract both Alpha and Beta symmetries
            for(iii=0; (iii<2); iii++) {
              while (!ifs.eof() && 
                     (NULL == strstr(buffer,"Alpha")) &&
                     (NULL == strstr(buffer,"Beta"))) {
                ifs.getline(buffer, BUFF_SIZE);
              }
              do {
                ifs.getline(buffer, BUFF_SIZE);
                bDoneSymm = (NULL == strstr(buffer, "("));
                if (!bDoneSymm) {
                  tokenize(vs, buffer); 
                
                  if ((NULL != strstr(buffer, "Occupied")) || (NULL != strstr(buffer, "Virtual"))) {
                    offset = 1; // skip first token
                  } else {
                    offset = 0;
                  }
                  for (unsigned int i = offset; i < vs.size(); ++i) {
                    label = vs[i].substr(1, vs[i].length() - 2);
                    symmetries.push_back(label);
                  }
                } 
              } while (!ifs.eof() && !bDoneSymm);
            } // end alpha/beta section
          }
        else if (strstr(buffer, "Alpha") && strstr(buffer, ". eigenvalues --")) {
          orbitals.clear();
          betaStart = 0;
          while (strstr(buffer, ". eigenvalues --")) {
            tokenize(vs, buffer);
            if (vs.size() < 4)
              break;
            if (vs[0].find("Beta") !=string::npos && betaStart == 0) // mark where we switch from alpha to beta
              betaStart = orbitals.size();
            for (unsigned int i = 4; i < vs.size(); ++i) {
              orbitals.push_back(atof(vs[i].c_str()));
            }
            ifs.getline(buffer, BUFF_SIZE);
          }
        }
        else if(strstr(buffer, " Excited State")) // Force and wavelength data
        {
          // The above line appears for each state, so just append the info to the vectors
          tokenize(vs, buffer);
          if (vs.size() == 9) {
            double wavelength = atof(vs[6].c_str());
            double force = atof(vs[8].substr(2).c_str());
            Forces.push_back(force);
            Wavelengths.push_back(wavelength);
          }
        }
        else if(strstr(buffer, " Ground to excited state Transition electric dipole moments (Au):"))
          // Electronic dipole moments
        {
          ifs.getline(buffer, BUFF_SIZE); // Headings
          ifs.getline(buffer, BUFF_SIZE); // First entry
          tokenize(vs, buffer);
          while (vs.size() == 5) {
            double s = atof(vs[4].c_str());
            EDipole.push_back(s);
            ifs.getline(buffer, BUFF_SIZE);
            tokenize(vs, buffer);
          }
        }
        else if(strstr(buffer, "       state          X           Y           Z     R(velocity)")) {
          // Rotatory Strengths
          ifs.getline(buffer, BUFF_SIZE); // First entry
          tokenize(vs, buffer);
          while (vs.size() == 5) {
            double s = atof(vs[4].c_str());
            RotatoryStrengthsVelocity.push_back(s);
            ifs.getline(buffer, BUFF_SIZE);
            tokenize(vs, buffer);
          }
        }
        else if(strstr(buffer, "       state          X           Y           Z     R(length)")) {
          // Rotatory Strengths
          ifs.getline(buffer, BUFF_SIZE); // First entry
          tokenize(vs, buffer);
          while (vs.size() == 5) {
            double s = atof(vs[4].c_str());
            RotatoryStrengthsLength.push_back(s);
            ifs.getline(buffer, BUFF_SIZE);
            tokenize(vs, buffer);
          }
        }

        else if (strstr(buffer, "Forces (Hartrees/Bohr)"))
          {
            ifs.getline(buffer, BUFF_SIZE); // column headers
            ifs.getline(buffer, BUFF_SIZE); // ------
            ifs.getline(buffer, BUFF_SIZE); // real data
          }

        else if (strstr(buffer, "Isotropic = ")) // NMR shifts
          {
            tokenize(vs, buffer);
            if (vs.size() >= 4)
              {
                atom = mol.GetAtom(atoi(vs[0].c_str()));
                OBPairData *nmrShift = new OBPairData();
                nmrShift->SetAttribute("NMR Isotropic Shift");

                string shift = vs[4].c_str();
                nmrShift->SetValue(shift);

                atom->SetData(nmrShift);
              }
          }
        else if(strstr(buffer,"SCF Done:") != NULL)
          {
            tokenize(vs,buffer);
            mol.SetEnergy(atof(vs[4].c_str()) * HARTREE_TO_KCAL);
            confEnergies.push_back(mol.GetEnergy());
          }
/* Temporarily commented out until the handling of energy in OBMol is sorted out
        // MP2 energies also use a different syntax

        // PM3 energies use a different syntax
        else if(strstr(buffer,"E (Thermal)") != NULL)
          {
            ifs.getline(buffer,BUFF_SIZE); //Headers
            ifs.getline(buffer,BUFF_SIZE); //Total energy; what we want
            tokenize(vs,buffer);
            mol.SetEnergy(atof(vs[1].c_str()));
            confEnergies.push_back(mol.GetEnergy());
            }
*/
        else if(strstr(buffer,"Standard basis:") != NULL) 
          {
            add_unique_pairdata_to_mol(&mol,"basis",buffer,2);
          }
        else if(strstr(buffer,"Zero-point correction=") != NULL)
          {
            tokenize(vs,buffer);
            ezpe = atof(vs[2].c_str());
            ezpe_set = 1;
          }
        else if(strstr(buffer,"Thermal correction to Enthalpy=") != NULL)
          {
            tokenize(vs,buffer);
            etherm = atof(vs[4].c_str());
            etherm_set = 1;
          }
        else
          {
            /* This must be the last else */
            int i,nsearch;
            const char *search[] = { "CBS-QB3 (0 K)", "G2(0 K)", "G3(0 K)", "G4(0 K)" };
            const char *mymeth[] = { "CBS-QB3", "G2", "G3", "G4" };
            const int myindex[] = { 3, 2, 2, 2 };
            
            nsearch = sizeof(search)/sizeof(search[0]);
            for(i=0; (i<nsearch); i++) 
              {
                if(strstr(buffer,search[i]) != NULL)
                  {
                    tokenize(vs,buffer);
                    eg234 = atof(vs[myindex[i]].c_str());
                    eg234_set = 1;
                    strcpy(method,mymeth[i]);
                    break;
                  }
              }
          }
      } // end while
    
    // Check whether we have data to extract heat of formation.
    if (ezpe_set && etherm_set && eg234_set) 
      {
        extract_g234(&mol,method,ezpe,etherm,eg234);
      }
      
    if (mol.NumAtoms() == 0) { // e.g., if we're at the end of a file PR#1737209
      mol.EndModify();
      return false;
    }

    mol.EndModify();

    // Set conformers to all coordinates we adopted
    // but remove last geometry -- it's a duplicate
    if (vconf.size() > 1)
      vconf.pop_back();
    mol.SetConformers(vconf);
    mol.SetConformer(mol.NumConformers() - 1);
    // Copy the conformer data too
    confData->SetDimension(confDimensions);
    confData->SetEnergies(confEnergies);
    confData->SetForces(confForces);
    mol.SetData(confData);

    // Attach orbital data, if there is any
    if (orbitals.size() > 0)
      {
        OBOrbitalData *od = new OBOrbitalData;
        if (aHOMO == bHOMO) {
          od->LoadClosedShellOrbitals(orbitals, symmetries, aHOMO);
        } else {
          // we have to separate the alpha and beta vectors
          std::vector<double>      betaOrbitals;
          std::vector<std::string> betaSymmetries;
          unsigned int initialSize = orbitals.size();
          unsigned int symmSize = symmetries.size();
          if (initialSize != symmSize)
            {
              cerr << "Inconsistency: orbitals have " << initialSize << " elements while symmetries have " << symmSize << endl;
            }
          else
            {
              for (unsigned int i = betaStart; i < initialSize; ++i) {
                betaOrbitals.push_back(orbitals[i]);
                if (symmetries.size() > 0)
                  betaSymmetries.push_back(symmetries[i]);
              }
              // ok, now erase the end elements of orbitals and symmetries
              for (unsigned int i = betaStart; i < initialSize; ++i) {
                orbitals.pop_back();
                if (symmetries.size() > 0)
                  symmetries.pop_back();
              }
              // and load the alphas and betas
              od->LoadAlphaOrbitals(orbitals, symmetries, aHOMO);
              od->LoadBetaOrbitals(betaOrbitals, betaSymmetries, bHOMO);
            }
          od->SetOrigin(fileformatInput);
          mol.SetData(od);
        }
      }

    //Attach vibrational data, if there is any, to molecule
    if(Frequencies.size()>0)
    {
      OBVibrationData* vd = new OBVibrationData;
      vd->SetData(Lx, Frequencies, Intensities);
      vd->SetOrigin(fileformatInput);
      mol.SetData(vd);
    }
    //Attach rotational data, if there is any, to molecule
    if(RotConsts[0]!=0.0)
    {
      OBRotationData* rd = new OBRotationData;
      rd->SetData(RotorType, RotConsts, RotSymNum);
      rd->SetOrigin(fileformatInput);
      mol.SetData(rd);
    }
    // Attach unit cell translation vectors if found
    if (numTranslationVectors > 0) {
      OBUnitCell* uc = new OBUnitCell;
      uc->SetData(translationVectors[0], translationVectors[1], translationVectors[2]);
      uc->SetOrigin(fileformatInput);
      mol.SetData(uc);
    }
    //Attach electronic transition data, if there is any, to molecule
    if(Forces.size() > 0 && Forces.size() == Wavelengths.size())
    {
      OBElectronicTransitionData* etd = new OBElectronicTransitionData;
      etd->SetData(Wavelengths, Forces);
      if (EDipole.size() == Forces.size())
        etd->SetEDipole(EDipole);
      if (RotatoryStrengthsLength.size() == Forces.size())
        etd->SetRotatoryStrengthsLength(RotatoryStrengthsLength);
      if (RotatoryStrengthsVelocity.size() == Forces.size())
        etd->SetRotatoryStrengthsVelocity(RotatoryStrengthsVelocity);
      etd->SetOrigin(fileformatInput);
      mol.SetData(etd);
    }

    if (!pConv->IsOption("b",OBConversion::INOPTIONS))
      mol.ConnectTheDots();
    if (!pConv->IsOption("s",OBConversion::INOPTIONS) && !pConv->IsOption("b",OBConversion::INOPTIONS))
      mol.PerceiveBondOrders();

    if (hasPartialCharges) {
      mol.SetPartialChargesPerceived();

      // Annotate that partial charges come from Mulliken
      OBPairData *dp = new OBPairData;
      dp->SetAttribute("PartialCharges");
      dp->SetValue(chargeModel); // Mulliken, ESP, etc.
      dp->SetOrigin(fileformatInput);
      mol.SetData(dp);
    }
    mol.SetTotalCharge(charge);
    mol.SetTotalSpinMultiplicity(spin);

    mol.SetTitle(title);
    return(true);
  }

} //namespace OpenBabel
