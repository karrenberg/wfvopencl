/**
 * @file   callSiteBlockSplitter.h
 * @date   10.09.2010
 * @author Ralf Karrenberg
 *
 * This file is distributed under the University of Illinois Open Source
 * License. See the COPYING file in the root directory for details.
 *
 * Copyright (C) 2010, 2011 Saarland University
 *
 * TODO: void* as last param is hardcoded
 * TODO: replacing all returns by return -1 is hardcoded
 */
#ifndef _CALLSITEBLOCKSPLITTER_H
#define _CALLSITEBLOCKSPLITTER_H


#ifdef DEBUG_TYPE
#undef DEBUG_TYPE
#endif
#define DEBUG_TYPE "callsiteblocksplitter"

#include <llvm/Support/raw_ostream.h>

#include <llvm/Pass.h>
#include <llvm/Function.h>
#include <llvm/Module.h>
#include "llvm/ADT/SetVector.h"

//#include "llvmTools.hpp"
#include <llvm/ADT/DenseMap.h>
#include <llvm/Instructions.h>

#ifdef DEBUG
#define DEBUG_CSBS(x) do { x } while (false)
#else
#define DEBUG_CSBS(x) ((void)0)
#endif

#ifdef NDEBUG // force debug output disabled
#undef DEBUG_CSBS
#define DEBUG_CSBS(x) ((void)0)
#endif

using namespace llvm;


// forward declaration of initializer
namespace llvm {
    void initializeCallSiteBlockSplitterPass(PassRegistry&);
}

class CallSiteBlockSplitter : public FunctionPass {
public:
    static char ID; // Pass identification, replacement for typeid
    CallSiteBlockSplitter();
    explicit CallSiteBlockSplitter(const std::string& fnName, const bool verbose_flag = false);
    ~CallSiteBlockSplitter();

    virtual bool runOnFunction(Function &f);

    void print(raw_ostream& o, const Module *M) const;
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
    void releaseMemory();

    typedef DenseMap<CallInst*, unsigned> CallIndicesMapType;
    inline const CallIndicesMapType getCallIndexMap() const;
    inline const unsigned getNumCalls() const;

private:
    const bool verbose;
    const std::string calleeName;
    unsigned numCalls;
    CallIndicesMapType callIndices;

    inline bool isSplitCall(const Value* value) const;

    // - split basic blocks at each valid splitting instruction
    void splitBlocksAtCallSites(Function* f);
};

/*
// TODO header or impl?
INITIALIZE_PASS_BEGIN(CallSiteBlockSplitter, "callsite-block-splitting", "CallSite Block Splitting", false, false)
//INITIALIZE_PASS_DEPENDENCY() // has no dependency :p
INITIALIZE_PASS_END(CallSiteBlockSplitter, "callsite-block-splitting", "CallSite Block Splitting", false, false)
*/

// Public interface to the CallSiteBlockSplitter pass
namespace llvm {
    FunctionPass* createCallSiteBlockSplitterPass(const std::string& fnName);
}


#endif /* _CALLSITEBLOCKSPLITTER_H */

