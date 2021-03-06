Info<< "Reading Solver options" << nl << endl;
IOdictionary solverOptions
(
    IOobject
    (
        "solverOptions",
        runTime.constant(),
        runTime.db(),
        IOobject::MUST_READ,
        IOobject::NO_WRITE
    )
);

// Read the kinetic scheme in XML format

label inertIndex;
OpenSMOKE::ThermodynamicsMap_CHEMKIN<double>*                thermodynamicsMapXML; 
OpenSMOKE::KineticsMap_CHEMKIN<double>*                     kineticsMapXML;
OpenSMOKE::ThermodynamicsMap_Surface_CHEMKIN<double>*        surfaceThermodynamicsMapXML; 
OpenSMOKE::KineticsMap_Surface_CHEMKIN<double>*         surfaceKineticsMapXML;
OpenSMOKE::TransportPropertiesMap_CHEMKIN<double>*             transportMapXML;

const dictionary& kineticsDictionary = solverOptions.subDict("Kinetics");
{
    //- Kinetics folder
    Foam::string kinetics_folder= kineticsDictionary.lookup("folder");
    
    // Reading homogeneous data
    Info<< " * loading homogeneous kinetics...\n" << endl;
    {    
        word kinetics("opensmoke");
        boost::filesystem::path path_kinetics = kinetics_folder;

        rapidxml::xml_document<> doc;
        std::vector<char> xml_string;
        OpenSMOKE::OpenInputFileXML(doc, xml_string,path_kinetics / "kinetics.xml");

        double tStart = OpenSMOKE::OpenSMOKEGetCpuTime();
        
        thermodynamicsMapXML = new OpenSMOKE::ThermodynamicsMap_CHEMKIN<double>(doc); 
        transportMapXML = new OpenSMOKE::TransportPropertiesMap_CHEMKIN<double>(doc); 
        kineticsMapXML = new OpenSMOKE::KineticsMap_CHEMKIN<double>(*thermodynamicsMapXML, doc); 
                            
        double tEnd = OpenSMOKE::OpenSMOKEGetCpuTime();
        std::cout << " * Time to read XML file: " << tEnd-tStart << std::endl;
    }
    
    // Reading heterogeneous data
    Info<< " * loading heterogeneous kinetics...\n" << endl;
    {    
        word kinetics("opensmoke");
        boost::filesystem::path path_kinetics = kinetics_folder;

        rapidxml::xml_document<> doc;
        std::vector<char> xml_string;
        OpenSMOKE::OpenInputFileXML(doc, xml_string,path_kinetics / "kinetics.surface.xml");

        double tStart = OpenSMOKE::OpenSMOKEGetCpuTime();
        
        surfaceThermodynamicsMapXML = new OpenSMOKE::ThermodynamicsMap_Surface_CHEMKIN<double>(doc); 
        surfaceKineticsMapXML = new OpenSMOKE::KineticsMap_Surface_CHEMKIN<double>(*surfaceThermodynamicsMapXML, doc); 
                            
        double tEnd = OpenSMOKE::OpenSMOKEGetCpuTime();
        std::cout << " * Time to read XML file: " << tEnd-tStart << std::endl;
    }    
}

//- Internal models
enum { STRANG_REACTION_TRANSPORT_REACTION, STRANG_REACTION_TRANSPORT, STRANG_TRANSPORT_REACTION, STRANG_REACTION_TRANSPORT_HYBRID, STRANG_TRANSPORT_REACTION_MOMENTUM} strangAlgorithm;
enum { DIFFUSIVITY_MODEL_MULTICOMPONENT, DIFFUSIVITY_MODEL_LEWIS_NUMBERS} diffusivityModel;

// Physical model
Eigen::VectorXd LewisNumbers;
const dictionary& physicalModelDictionary = solverOptions.subDict("PhysicalModel");
{
    // Diffusivity of species
    {
        word diffusivity(physicalModelDictionary.lookup("diffusivityModel"));
        if (diffusivity == "multi-component")        diffusivityModel = DIFFUSIVITY_MODEL_MULTICOMPONENT;
        else if (diffusivity == "lewis-numbers")    diffusivityModel = DIFFUSIVITY_MODEL_LEWIS_NUMBERS;
        else
        {
            Info << "Wrong diffusivityModel option: multi-component || lewis-numbers" << endl;
            abort();
        }
        
        if (diffusivityModel == DIFFUSIVITY_MODEL_LEWIS_NUMBERS)
        {
            LewisNumbers.resize(thermodynamicsMapXML->NumberOfSpecies());
            
            const dictionary& lewisNumberDictionary = solverOptions.subDict("LewisNumbers");
            for(int i=0;i<thermodynamicsMapXML->NumberOfSpecies();i++)
                LewisNumbers(i) = readScalar(lewisNumberDictionary.lookup(thermodynamicsMapXML->NamesOfSpecies()[i]));
            
            Info << endl;
            Info << "Diffusivities of species will be calculated using the following Lewis numbers: " << endl;
            for(int i=0;i<thermodynamicsMapXML->NumberOfSpecies();i++)
                Info << " " << thermodynamicsMapXML->NamesOfSpecies()[i] << "\t" << LewisNumbers(i) << endl;
            Info << endl;
        }
    }
}

Info<< "Solver options correctly read" << nl << endl;

