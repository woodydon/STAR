#include "sjdbInsertJunctions.h"
#include "sjdbLoadFromStream.h"
#include "sjdbLoadFromFiles.h"
#include "sjdbPrepare.h"
#include "ErrorWarning.h"
// #include "streamFuns.h"
#include "SjdbClass.h"
#include "loadGTF.h"
#include "sjdbBuildIndex.h"
#include "streamFuns.h"

void sjdbInsertJunctions(Parameters *P, Parameters *P1, Genome &genome) {
        
    SjdbClass sjdbLoci;        
    time_t rawtime;

    //load 1st pass junctions
    if (P->twoPass.pass1sjFile.size()>0)
    {
        ifstream sjdbStreamIn ( P->twoPass.pass1sjFile.c_str() );   
        if (sjdbStreamIn.fail()) {
            ostringstream errOut;
            errOut << "FATAL INPUT error, could not open input file with junctions from the 1st pass=" << P->twoPass.pass1sjFile <<"\n";
            exitWithError(errOut.str(),std::cerr, P->inOut->logMain, EXIT_CODE_INPUT_FILES, *P);
        };
        sjdbLoadFromStream(sjdbStreamIn, sjdbLoci);
        sjdbLoci.priority.resize(sjdbLoci.chr.size(),0);
        time ( &rawtime );
        P->inOut->logMain << timeMonthDayTime(rawtime) << "   Loaded database junctions from the 1st pass file: " << P->twoPass.pass1sjFile <<": "<<sjdbLoci.chr.size()<<" total junctions\n\n";
    };
    
    //load from junction files
    if (P->sjdbFileChrStartEnd.at(0)!="-")
    {
        sjdbLoadFromFiles(P,sjdbLoci);
        sjdbLoci.priority.resize(sjdbLoci.chr.size(),10);
        P->inOut->logMain << timeMonthDayTime(rawtime) << "   Loaded database junctions from the sjdbFileChrStartEnd file(s), " << sjdbLoci.chr.size()<<" total junctions\n\n";        
    };
    
    //load from GTF
    if (P->sjdbGTFfile!="-")
    {
        loadGTF(sjdbLoci, P, P->sjdbInsert.outDir);
        sjdbLoci.priority.resize(sjdbLoci.chr.size(),20);
        P->inOut->logMain << timeMonthDayTime(rawtime) << "   Loaded database junctions from the GTF file: " << P->sjdbGTFfile<<": "<<sjdbLoci.chr.size()<<" total junctions\n\n";
    };
    
    //load from the already generated genome
    if (P->sjdbN>0)
    {
        ifstream sjdbStreamIn;
        ifstrOpen(P->genomeDir+"/sjdbList.out.tab", "ERROR_012003", "SOLUTION: re-generate the genome in genomeDir=" + P->genomeDir, P, sjdbStreamIn);
        sjdbLoadFromStream(sjdbStreamIn, sjdbLoci);
        sjdbLoci.priority.resize(sjdbLoci.chr.size(),30);
        time ( &rawtime );
        P->inOut->logMain << timeMonthDayTime(rawtime) << "   Loaded database junctions from the generated genome " << P->genomeDir+"/sjdbList.out.tab" <<": "<<sjdbLoci.chr.size()<<" total junctions\n\n";
    };

    char *Gsj=new char [2*P->sjdbLength*sjdbLoci.chr.size()+1];//arry to store junction sequences, will be filled in sjdbPrepare
    sjdbPrepare (sjdbLoci, P, P->chrStart[P->nChrReal], P->sjdbInsert.outDir, genome.G, Gsj);//P->nGenome - change when replacing junctions
    time ( &rawtime );
    P->inOut->logMain  << timeMonthDayTime(rawtime) << "   Finished preparing junctions" <<endl;

    //insert junctions into the genome and SA and SAi
    sjdbBuildIndex (P, P1, Gsj, genome.G, genome.SA, genome.SA2, genome.SAi);
    delete [] Gsj; //junction sequences have been added to G
    time ( &rawtime ); 
    *P->inOut->logStdOut  << timeMonthDayTime(rawtime) << " ..... Finished inserting 1st pass junctions into genome" <<endl;

    if (P->sjdbInsert.save=="All")
    {//save big files
        ofstream genomeOut;
        ofstrOpen(P->sjdbInsert.outDir+"/Genome","ERROR_012004", P, genomeOut);
        fstreamWriteBig(genomeOut,genome.G,P->nGenome,P->sjdbInsert.outDir+"/Genome","ERROR_012005",P);
        genomeOut.close();

        ofstrOpen(P->sjdbInsert.outDir+"/SA","ERROR_012006", P, genomeOut);
        fstreamWriteBig(genomeOut,(char*) genome.SA.charArray, (streamsize) genome.SA.lengthByte, P->sjdbInsert.outDir+"/SA","ERROR_012007",P);
        genomeOut.close();


        ofstrOpen(P->sjdbInsert.outDir+"/SAindex","ERROR_012008", P, genomeOut);
        fstreamWriteBig(genomeOut, (char*) &P->genomeSAindexNbases, sizeof(P->genomeSAindexNbases),P->sjdbInsert.outDir+"/SAindex","ERROR_012009",P);
        fstreamWriteBig(genomeOut, (char*) P->genomeSAindexStart, sizeof(P->genomeSAindexStart[0])*(P->genomeSAindexNbases+1),P->sjdbInsert.outDir+"/SAindex","ERROR_012010",P);
        fstreamWriteBig(genomeOut,  genome.SAi.charArray, genome.SAi.lengthByte,P->sjdbInsert.outDir+"/SAindex","ERROR_012011",P);
        genomeOut.close();
    };

    //re-calculate genome-related parameters
    P->winBinN = P->nGenome/(1LLU << P->winBinNbits)+1;
};
