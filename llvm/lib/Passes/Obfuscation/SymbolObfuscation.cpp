//
// Created by pinping on 2023/9/5.
//

#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "SymbolObfuscation.h"
#include <string>
#include <iostream>
#include <cstdlib>

using namespace llvm;
using namespace std;

static string obfcharacters="-_.|/\\`+,=()*:";

int seed = 0;
string randomString(int length){
  string name;
  name.resize(length);
  srand(seed);
  seed++;
  for(int i=0;i<length;i++){
    name[i]=obfcharacters[rand()%(obfcharacters.length())];
  }
  return "f_" + name;
}

PreservedAnalyses SymbolObfuscationPass::run(Module &M, ModuleAnalysisManager &AM) {
  //F.setName(randomString(16));

  errs()<<"Symbol Obfuscation Pass 开始\n";
  for(Module::iterator Fun=M.begin();Fun!=M.end();Fun++){
    Function &F=*Fun;
    if (F.getName().str().compare("main")==0){
      errs()<<"Skipping main\n";
    }
    else if(F.empty()==false){
      //Rename
      string newname = randomString(16);
      errs()<<"Renaming Function: "<<F.getName()<<"\n";
      errs()<<"New Function Name: "<<newname<<"\n";
      F.setName(newname);
    }
    else{
      errs()<<"Skipping External Function: "<<F.getName()<<"\n";
    }
  }
  return PreservedAnalyses::all();
}

SymbolObfuscationPass *llvm::createSymbolObfuscation(bool flag){
  return new SymbolObfuscationPass(flag);
}