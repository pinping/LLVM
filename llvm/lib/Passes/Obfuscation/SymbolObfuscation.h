//
// Created by pinping on 2023/9/5.
//

#ifndef LLVM_SYMBOLOBFUSCATION_H
#define LLVM_SYMBOLOBFUSCATION_H

#include "llvm/IR/PassManager.h"

namespace llvm {

  class SymbolObfuscationPass : public PassInfoMixin<SymbolObfuscationPass>  {
    public:
      bool flag;
      SymbolObfuscationPass(bool flag){
        this->flag = flag;
      }
      PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
  };
  SymbolObfuscationPass *createSymbolObfuscation(bool flag);
}
#endif // LLVM_SYMBOLOBFUSCATION_H
