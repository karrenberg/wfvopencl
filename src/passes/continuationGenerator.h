/**
 * @file   continuationGenerator.h
 * @date   02.07.2010
 * @author Ralf Karrenberg
 *
 * This file is distributed under the University of Illinois Open Source
 * License. See the COPYING file in the root directory for details.
 *
 * Copyright (C) 2010, 2011 Saarland University
 *
 */
#ifndef _CONTINUATIONGENERATOR_H
#define _CONTINUATIONGENERATOR_H


#ifdef DEBUG_TYPE
#undef DEBUG_TYPE
#endif
#define DEBUG_TYPE "continuationgenerator"

#include <stack>

#include <llvm/ADT/SetVector.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/FunctionUtils.h> // ExtractCodeRegion
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Pass.h>
#include <llvm/Function.h>
#include <llvm/Module.h>

#include "callSiteBlockSplitter.h"
#include "livenessAnalyzer.h"
#include "../debug.h"
#include "../llvmTools.hpp"

#define WFVOPENCL_FUNCTION_NAME_BARRIER "barrier"
#define WFVOPENCL_BARRIER_SPECIAL_END_ID -1
#define WFVOPENCL_BARRIER_SPECIAL_START_ID 0

using namespace llvm;


// forward declaration of initializer
namespace llvm {
    void initializeContinuationGeneratorPass(PassRegistry&);
}


void printValueMap(ValueMap<const Value*, Value*>& valueMap, raw_ostream& o);

class ContinuationGenerator : public FunctionPass {
private:
    //typedef SetVector<Value*> LiveSetType;
    typedef SetVector<const Value*> LiveSetTypeConst;

    struct BarrierInfo {
        BarrierInfo(
                const unsigned index,
                const unsigned d,
                const CallInst* call,
                const BasicBlock* entryBlock,
                SetVector<const BasicBlock*>* region,
                LiveSetTypeConst* liveInSet,
                const StructType* sType
        ) : id(index),
            depth(d),
            barrier(call),
            continuationEntryBlock(entryBlock),
            continuationRegion(region),
            liveInValues(liveInSet),
            liveValueStructType(sType),
            liveValueStructTypePadded(NULL),
            continuation(NULL)
        {}

        ~BarrierInfo() {}

        const unsigned id;
        const unsigned depth;
        const CallInst* barrier;
        const BasicBlock* continuationEntryBlock;
        SetVector<const BasicBlock*>* continuationRegion;
        LiveSetTypeConst* liveInValues;
        const StructType* liveValueStructType;
        const StructType* liveValueStructTypePadded;

        Function* continuation;

        ValueToValueMapTy* origFnValueMap;

        void print(raw_ostream& o) const {
            o << "barrier id   : " << id << "\n";
            o << "barrier depth: " << depth << "\n";
            if (barrier) o << "barrier      : " << *barrier << " (parent: " << barrier->getParent()->getNameStr() << ")\n";
            else o << "barrier      : none\n";
            if (continuation) o << "continuation : " << continuation->getNameStr() << "\n";
            else o << "continuation : none\n";
            if (liveValueStructType) o << "live value struct type       : " << *liveValueStructType << "\n";
            else o << "live value struct type       : none\n";
            if (liveValueStructTypePadded) o << "padded live value struct type: " << *liveValueStructTypePadded << "\n";
            else o << "padded live value struct type: none\n";
        }
    };

public:
    static char ID; // Pass identification, replacement for typeid
    ContinuationGenerator(const bool verbose_flag = false);
    ~ContinuationGenerator();

    virtual bool runOnFunction(Function &f);

    void print(raw_ostream& o, const Module *M) const;
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
    void releaseMemory();

    // each continuation function receives an additional parameter with this type and name
    void addSpecialParam(const Type* param, const std::string& paramName);

    Function* getBarrierFreeFunction() const;
    typedef SmallVector<Function*, 4> ContinuationVecType;
    void getContinuations(ContinuationVecType& continuations) const;

    typedef DenseMap<unsigned, BarrierInfo*> ContinuationMapType;
    ContinuationMapType* getContinuationMap();

private:
    const bool verbose;
    LoopInfo* loopInfo;
    CallSiteBlockSplitter* callSiteBlockSplitter;
    LivenessAnalyzer* livenessAnalyzer;
    Function* barrierFreeFunction;

    typedef std::vector<const Type*> SpecialParamVecType;
    typedef std::vector<std::string> SpecialParamNameVecType;
    SpecialParamVecType specialParams;
    SpecialParamNameVecType specialParamNames;

    ContinuationMapType continuationMap;

    typedef DenseMap<const CallInst*, unsigned> BarrierIndicesMapType;
    BarrierIndicesMapType barrierIndices;

    typedef SmallVector<CallInst*, 4> BarrierVecType;


    typedef DenseMap<Function*, ValueMap<const Value*, Value*>* > ValueMapsType;
    ValueMapsType valueMaps; // stores mappings from original function for each continuation

    inline const Type* getReturnType(LLVMContext& context);


    inline bool isBarrier(const Value* value) const;
    inline bool blockEndsWithBarrierBeforeTerminator(const BasicBlock* block) const;

#if 0
    void mapBarrierInfo(Function* f, Function* newF, ValueMap<const Value*, Value*>& valueMap, BarrierVecType& barriers, ValueMap<const Value*, Value*>& barrierMapping);
#endif

    // helper for getBarrierInfo()
    unsigned collectBarriersDFS(BasicBlock* block, unsigned depth, BarrierVecType& barriers, SmallVector<unsigned, 4>& barrierDepths, unsigned& maxBarrierDepth, std::set<BasicBlock*>& visitedBlocks);
    
    // Traverse the function in DFS and collect all barriers in post-reversed order.
    // Count how many barriers the function has and assign an id to each barrier
    // TODO: rewrite and use information that barriers are always the last instructions before the terminator in a BB
    inline unsigned getBarrierInfo(Function* f, BarrierVecType& barriers, SmallVector<unsigned, 4>& barrierDepths, unsigned& maxBarrierDepth);

    void findContinuationBlocksDFSOrdered(BasicBlock* block, std::vector<BasicBlock*>& continuationRegion, std::set<BasicBlock*>& visitedBlocks);
    void findContinuationBlocksDFS(BasicBlock* block, SetVector<BasicBlock*>& continuationRegion, std::set<const BasicBlock*>& visitedBlocks);
    void findContinuationBlocksDFS(const BasicBlock* block, SetVector<const BasicBlock*>& continuationRegion, std::set<const BasicBlock*>& visitedBlocks);

    /// definedInRegion - Return true if the specified value is defined in the
    /// cloned region.
    /// From: llvm-2.8svn/lib/Transforms/Utils/CodeExtractor.cpp:definedInRegion()
    bool definedInRegion(Value *V, SetVector<BasicBlock*>& region) const;
    bool definedInRegion(const Value *V, SetVector<const BasicBlock*>& region) const;

    /// definedInCaller - Return true if the specified value is defined in the
    /// function being code cloned, but not in the region being cloned.
    /// These values must be passed in as live-ins to the function.
    /// From: llvm-2.8svn/lib/Transforms/Utils/CodeExtractor.cpp:definedInCaller()
    bool definedOutsideRegion(Value *V, SetVector<BasicBlock*>& region) const;
    bool definedOutsideRegion(const Value *V, SetVector<const BasicBlock*>& region) const;

#if 0
    // generates a continuation function that is called at the point of the barrier
    // mapping of live values is stored in valueMap
    Function* createContinuation(CallInst* barrier, const std::string& newFunName, TargetData* targetData);
#endif

    void removeLiveValStructFromExtractedFnArgType(Function* f, Value* nextContLiveValStructPtr, LLVMContext& context, Instruction* insertBefore);

    //
    // 1. find out which blocks belong to the continuation
    // 2. extract code region (ECR)
    // 3. adjust arg struct of ECR-generated code
    //    3.1 remove live value struct
    //    3.2 if necessary, reorder elements of arg struct to match
    // 3. replace barrier by 'return barrierIndex'
    // 4. remove everything behind ECR-call (remove branch + DCE)
    // 5. remove ECR-generated call and function which is not required
    //    (the corresponding continuation is generated by createContinuation)
    void createSecondaryContinuation(CallInst* barrier, const unsigned barrierIndex, Function* continuation);

    inline bool functionHasBarrier(Function* f);
    inline CallInst* findBarrier(Function* f);

#if 0
    //
    // TODO:
    // - check if continuation has been generated before, if no, save it in continuation map, if yes, delete it
    // - together with continuation, store the order of its live values in the struct
    // - in order to find out this order we need to have value mappings from ExtractCodeRegion
    Function* createContinuationNEW(CallInst* barrier, const unsigned barrierIndex, Function* currentFn, ContinuationMapType& continuationMap);
#endif

    void getContinuationBlocks(const CallInst* barrier, SetVector<const BasicBlock*>& continuationRegion);
    void getLiveValuesForRegion(SetVector<const BasicBlock*>& continuationRegion, DominatorTree& domTree, LiveSetTypeConst& liveInValues, LiveSetTypeConst& liveOutValues);

    const StructType* computeLiveValueStructType(LiveSetTypeConst& liveInValues, LLVMContext& context);
    void createLiveValueStoresForBarrier(BarrierInfo* barrierInfo, Function* f, ValueToValueMapTy& origFnValueMap, LLVMContext& context);

    Function* createContinuationNEW2(BarrierInfo* barrierInfo, const std::string& continuationName, Module* mod);

    void storeBarrierMapping(Function* f, ValueMap<const Value*, Value*>& valueMap);

    Function* eliminateBarriers(Function* origFn, BarrierVecType& barriers, const unsigned numBarriers, const SmallVector<unsigned, 4>& barrierDepths, const unsigned maxBarrierDepth, SpecialParamVecType& specialParams, SpecialParamNameVecType& specialParamNames, TargetData* targetData);

    //--------------------------------------------------------------------//
    // create wrapper function which contains a switch over the barrier id
    // inside a while loop.
    // the wrapper calls the function that corresponds to the barrier id.
    // If the id is the special 'begin' id, it calls the first function
    // (= the remainder of the original kernel).
    // The while loop iterates until the barrier id is set to a special
    // 'end' id.
    // Each function has the same signature receiving only a void*.
    // In case of a continuation, this is a struct which holds the live
    // values that were live at the splitting point.
    // Before returning to the switch, the struct is deleted and the live
    // values for the next call are written into a newly allocated struct
    // (which the void* then points to).
    //
    // Each continuation is wrapped in a loop that iterates over an entire block
    // of input values. Depending on the dimensionality of the input data
    // (determined by analyzing the calls to get_local_id() etc.), one or more
    // nested loops are generated.
    //--------------------------------------------------------------------//
    // Example:
    /*
    void barrierSwitchFn(...) {
        void* data = NULL;
        const unsigned blockSizeDim0 = get_local_size(0);
        while (true) {
            switch (current_barrier_id) {
                case BARRIER_END: return;
                case BARRIER_BEGIN: {
                    for (int localId=0; localId<blockSizeDim0; ++localId) {
                        current_barrier_id = runOrigFunc(..., localId, &data); break;
                    }
                }
                case B0: {
                    // 1D case example
                    for (int localId=0; localId<blockSizeDim0; ++localId) {
                        current_barrier_id = runFunc0(localId, &data); break;
                    }
                }
                case B1: {
                    for (int localId=0; localId<blockSizeDim0; ++localId) {
                        current_barrier_id = runFunc1(localId, &data); break;
                    }
                }
                ...
                case BN: {
                    for (int localId=0; localId<blockSizeDim0; ++localId) {
                        current_barrier_id = runFuncN(localId, &data); break;
                    }
                }
                default: error; break;
            }
        }
    }
    */
    Function* createWrapper(Function* origFunction, ContinuationMapType& cMap, Module* mod, TargetData* targetData);
};

/*
INITIALIZE_PASS_BEGIN(ContinuationGenerator, "continuation-generation", "Continuation Generation", false, false)
INITIALIZE_PASS_DEPENDENCY(CallSiteBlockSplitter)
INITIALIZE_PASS_DEPENDENCY(LoopInfo)
INITIALIZE_PASS_DEPENDENCY(LivenessAnalyzer)
INITIALIZE_PASS_END(ContinuationGenerator, "continuation-generation", "Continuation Generation", false, false)
*/


// Public interface to the ContinuationGeneration pass
namespace llvm {
    FunctionPass* createContinuationGeneratorPass();
}


#endif /* _CONTINUATIONGENERATOR_H */

