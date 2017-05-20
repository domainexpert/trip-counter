// LLVM loop trip count calculation tester
//
// Portions copyright 2017 National University of Singapore
// See LICENSE.md for the license information.
//
// Based on discussions in these articles:
// http://lists.llvm.org/pipermail/llvm-dev/2017-May/113143.html
// http://stackoverflow.com/questions/13834364/how-to-get-loop-bounds-in-llvm
// http://lists.llvm.org/pipermail/llvm-dev/2011-March/038502.html
// https://groups.google.com/forum/#!topic/llvm-dev/1oNNBPMSqBg

#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"

#include <set>
#include <vector>

class TripCounter : public llvm::ModulePass {
  typedef std::vector<llvm::Loop*> LoopNest;
  typedef std::vector<const llvm::Value*> LoopNestBounds;

  // From http://stackoverflow.com/questions/13834364/how-to-get-loop-bounds-in-llvm
  void getLoopNestBounds(llvm::ScalarEvolution &SE, const LoopNest &Nest, LoopNestBounds &LBounds) {
      for (unsigned d = 0, n = Nest.size(); d != n; ++d) {
          if (SE.hasLoopInvariantBackedgeTakenCount(Nest[d])) {
              const llvm::SCEV *C = SE.getBackedgeTakenCount(Nest[d]);
              const llvm::SCEVConstant *CC = llvm::dyn_cast<const llvm::SCEVConstant>(C);
              LBounds.push_back(CC->getValue());
              llvm::errs() << CC->getValue()->getValue() << " iterations\n";
          }
          else {
              LBounds.push_back(0);
              llvm::errs() << "---- 0 iterations for the nest ----" << "\n";
          }
      }
  }

public:
  static char ID;
	
  TripCounter(): ModulePass(ID) {}
	
  virtual bool runOnModule(llvm::Module &M) {
    LoopNestBounds nestBounds;

    for (llvm::Module::iterator func = M.begin(), fe = M.end(); func != fe;
         ++func) {
      if (func->isDeclaration())
        continue;

      const llvm::LoopInfo &LI = getAnalysis<llvm::LoopInfo>(*func);

      llvm::ScalarEvolution &SE = getAnalysis<llvm::ScalarEvolution>(*func);

      LoopNest nest;
      for (llvm::LoopInfo::iterator it = LI.begin(), ie = LI.end(); it != ie;
           ++it) {
	  nest.push_back(*it);
      }

      getLoopNestBounds(SE, nest, nestBounds);
    }


    return false; // does not modify program
  }
  
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<llvm::LoopInfo>();
    AU.addRequired<llvm::ScalarEvolution>();
  }
  
};

char TripCounter::ID = 0;
static llvm::RegisterPass<TripCounter> X("trip-counter", "Extract loop trip counts",
					 false, false);

/*
 * Main.
 */
int main(int argc, char **argv)
{
  if (argc < 2) {
    llvm::errs() << "Filename unspecified\n";
    return 1;
  }

  llvm::LLVMContext &Context = llvm::getGlobalContext();
  llvm::SMDiagnostic Err;
  llvm::Module *M = ParseIRFile(argv[1], Err, Context);

  if (M == 0) {
    llvm::errs() << "ERROR: failed to load " << argv[0] << "\n";
    return 1;
  }

  llvm::PassManager PM;
  PM.add(new llvm::ScalarEvolution());
  PM.add(new llvm::LoopInfo());
  PM.add(new TripCounter());
  PM.run(*M);
	
  return 0;
}

