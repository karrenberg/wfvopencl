#include "callSiteBlockSplitter.h"

#include <llvm/Support/raw_ostream.h>

#include <llvm/Pass.h>
#include <llvm/Function.h>
#include <llvm/Module.h>
#include "llvm/ADT/SetVector.h"

//#include "llvmTools.hpp"
#include <llvm/ADT/DenseMap.h>
#include <llvm/Instructions.h>

using namespace llvm;


// forward declaration of initializer
namespace llvm {
    void initializeCallSiteBlockSplitterPass(PassRegistry&);
}

CallSiteBlockSplitter::CallSiteBlockSplitter() : FunctionPass(ID), verbose(true), calleeName("barrier"), numCalls(0) { errs() << "empty constructor CallSiteBlockSplitter() should never be called!\n"; }

CallSiteBlockSplitter::CallSiteBlockSplitter(const std::string& fnName, const bool verbose_flag) : FunctionPass(ID), verbose(verbose_flag), calleeName(fnName), numCalls(0) {}

CallSiteBlockSplitter::~CallSiteBlockSplitter() { releaseMemory(); }

bool CallSiteBlockSplitter::runOnFunction(Function &f) {
    DEBUG_CSBS( outs() << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"; );
    DEBUG_CSBS( outs() << "splitting basic blocks at callsites of function " << calleeName << "...\n"; );
    DEBUG_CSBS( outs() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n"; );

    DEBUG_CSBS( WFVOpenCL::writeFunctionToFile(&f, "debug_splitting_before.ll"); );
    splitBlocksAtCallSites(&f);
    DEBUG_CSBS( WFVOpenCL::writeFunctionToFile(&f, "debug_splitting_after.ll"); );

    DEBUG_CSBS( verifyModule(*f.getParent()); );

    DEBUG_CSBS( outs() << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"; );
    DEBUG_CSBS( outs() << "splitting of basic blocks at callsites of function " << calleeName << " finished!\n"; );
    DEBUG_CSBS( print(outs(), NULL); );
    DEBUG_CSBS( outs() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n"; );

    return numCalls != 0;
}

void CallSiteBlockSplitter::print(raw_ostream& o, const Module *M) const {}

void CallSiteBlockSplitter::getAnalysisUsage(AnalysisUsage &AU) const {}

void CallSiteBlockSplitter::releaseMemory() {}

const CallSiteBlockSplitter::CallIndicesMapType CallSiteBlockSplitter::getCallIndexMap() const { return callIndices; }

const unsigned CallSiteBlockSplitter::getNumCalls() const { return numCalls; }

bool CallSiteBlockSplitter::isSplitCall(const Value* value) const {
    if (!isa<CallInst>(value)) return false;
    const CallInst* call = cast<CallInst>(value);
    const Function* callee = call->getCalledFunction();
    return callee->getName().equals(calleeName);
}

// - split basic blocks at each valid splitting instruction
void CallSiteBlockSplitter::splitBlocksAtCallSites(Function* f) {
    // split basic blocks and collect barriers
    SetVector<CallInst*> barriers;
    bool changed = true;
    while (changed) {
        changed = false;
        for (Function::iterator BB=f->begin(), BBE=f->end(); BB!=BBE; ++BB) {
            // we do a forward iteration over instructions, so we know that
            // if the last instruction in this block is a barrier, we can
            // skip the block (do not split again).
            if (BB->getInstList().size() < 2) continue;
            assert (BB->getTerminator());
            Instruction* lastInst = --(--BB->end()); // -1 = terminator, -2 = last inst before terminator
            assert (lastInst);

            if (isa<CallInst>(lastInst) && barriers.count(cast<CallInst>(lastInst))) continue;

            // otherwise, see if we have a barrier in this block
            for (BasicBlock::iterator I=BB->begin(), IE=BB->end(); I!=IE; ++I) {
                if (!isSplitCall(I)) continue;

                CallInst* barrier = cast<CallInst>(I);

                assert (!barriers.count(barrier) && "must not attempt to split twice at the same barrier!");

                barriers.insert(barrier);
                callIndices[barrier] = numCalls++;

                // split block at the position of the barrier
                // (barrier has to be in "upper block")

                // find instruction that precedes barrier (= split point)
                Instruction* splitInst = NULL;
                BasicBlock::iterator I2 = --(BB->end());
                while (barrier != I2) {
                    splitInst = I2--;
                }
                // 'newBlock' is the "lower" block, 'parentBlock' the "upper" one with the barrier
                BB->splitBasicBlock(splitInst, BB->getNameStr()+".postbarrier");

                changed = true;
                break;
            }
            if (changed) break;
        }
    }

    return;
}

char CallSiteBlockSplitter::ID = 0;

INITIALIZE_PASS_BEGIN(CallSiteBlockSplitter, "callsite-block-splitting", "CallSite Block Splitting", false, false)
//INITIALIZE_PASS_DEPENDENCY() // has no dependency :p
INITIALIZE_PASS_END(CallSiteBlockSplitter, "callsite-block-splitting", "CallSite Block Splitting", false, false)

// Public interface to the CallSiteBlockSplitter pass
namespace llvm {
    FunctionPass* createCallSiteBlockSplitterPass(const std::string& fnName) {
        return new CallSiteBlockSplitter(fnName);
    }
}

