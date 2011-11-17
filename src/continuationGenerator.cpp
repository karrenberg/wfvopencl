#include "continuationGenerator.h"

#ifdef DEBUG_TYPE
#undef DEBUG_TYPE
#endif
#define DEBUG_TYPE "continuationgenerator"

#include <sstream>

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
#include "llvmTools.hpp"

#include <stack>
#include "llvm/ADT/SetVector.h"

#include "debug.h"

#define WFVOPENCL_FUNCTION_NAME_BARRIER "barrier"
#define WFVOPENCL_BARRIER_SPECIAL_END_ID -1
#define WFVOPENCL_BARRIER_SPECIAL_START_ID 0

using namespace llvm;


// forward declaration of initializer
namespace llvm {
	void initializeContinuationGeneratorPass(PassRegistry&);
}

void printValueMap(ValueMap<const Value*, Value*>& valueMap, raw_ostream& o) {
	o << "\nValueMap:\n";
	for (ValueMap<const Value*, Value*>::iterator it=valueMap.begin(), E=valueMap.end(); it!=E; ++it) {
		const Value* origVal = it->first;
		Value* mappedVal = it->second;
		if (isa<BasicBlock>(origVal) || isa<Function>(origVal)) {
			o << " * " << origVal->getNameStr() << " -> " << mappedVal->getNameStr() << "\n";
		} else {
			o << " * " << *origVal << " -> " << *mappedVal << "\n";
		}
	}
	o << "\n";
}


ContinuationGenerator::ContinuationGenerator(const bool verbose_flag) : FunctionPass(ID), verbose(verbose_flag), barrierFreeFunction(NULL) {}

ContinuationGenerator::~ContinuationGenerator() { releaseMemory(); }

bool ContinuationGenerator::runOnFunction(Function &f) {

	// get call site block splitter
	callSiteBlockSplitter = &getAnalysis<CallSiteBlockSplitter>();

	// get loop info
	loopInfo = &getAnalysis<LoopInfo>();

	// get liveness information
	livenessAnalyzer = &getAnalysis<LivenessAnalyzer>();

	DEBUG_LA( outs() << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"; );
	DEBUG_LA( outs() << "generating continuations...\n"; );
	DEBUG_LA( outs() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n"; );


	assert (f.getParent() && "function has to have a valid parent module!");
	TargetData* targetData = new TargetData(f.getParent());

	BarrierVecType barriers;
	SmallVector<unsigned, 4> barrierDepths;
	unsigned maxBarrierDepth  = 0;
	const unsigned numBarriers = getBarrierInfo(&f, barriers, barrierDepths, maxBarrierDepth);
	DEBUG_LA( outs() << "  number of barriers in function : " << numBarriers << "\n"; );
	DEBUG_LA( outs() << "  maximum block depth of barriers: " << maxBarrierDepth << "\n\n"; );
	assert (numBarriers == callSiteBlockSplitter->getNumCalls());

	if (numBarriers == 0) {
		DEBUG_LA( outs() << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"; );
		DEBUG_LA( outs() << "no barrier found, skipping generation of continuations!\n"; );
		DEBUG_LA( outs() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n"; );
		return false;
	}

	barrierFreeFunction = eliminateBarriers(&f, barriers, numBarriers, barrierDepths, maxBarrierDepth, specialParams, specialParamNames, targetData);

	DEBUG_LA( verifyModule(*f.getParent()); );

	DEBUG_LA( WFVOpenCL::writeFunctionToFile(&f, "debug_kernel_barriers_finished.ll"); );

	DEBUG_LA( outs() << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"; );
	DEBUG_LA( outs() << "generation of continuations finished!\n"; );
	DEBUG_LA( print(outs(), NULL); );
	DEBUG_LA( outs() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n"; );

	return barrierFreeFunction != NULL; // if barrierFreeFunction does not exist, nothing has changed
}

void ContinuationGenerator::print(raw_ostream& o, const Module *M) const {
//		ContinuationVecType contVec;
//		getContinuations(contVec);
//		for (ContinuationVecType::const_iterator it=contVec.begin(), E=contVec.end(); it!=E; ++it) {
//			Function* continuation = *it;
//			continuation->viewCFG();
//		}
}
void ContinuationGenerator::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<CallSiteBlockSplitter>();
	AU.addRequired<LoopInfo>();
	AU.addRequired<LivenessAnalyzer>();
}
void ContinuationGenerator::releaseMemory() {}

// each continuation function receives an additional parameter with this type and name
void ContinuationGenerator::addSpecialParam(const Type* param, const std::string& paramName) {
	specialParams.push_back(param);
	specialParamNames.push_back(paramName);
}

Function* ContinuationGenerator::getBarrierFreeFunction() const { return barrierFreeFunction; }

void ContinuationGenerator::getContinuations(ContinuationVecType& continuations) const {
	assert (continuations.empty());
	continuations.resize(continuationMap.size());
	assert (continuations.size() == continuationMap.size());
	for (DenseMap<unsigned, BarrierInfo*>::const_iterator it=continuationMap.begin(), E=continuationMap.end(); it!=E; ++it) {
		BarrierInfo* binfo = it->second;
		assert (binfo->id < continuationMap.size());
		assert (binfo->id < continuations.size());
		continuations[binfo->id] = binfo->continuation;
	}
}

ContinuationGenerator::ContinuationMapType* ContinuationGenerator::getContinuationMap() { return &continuationMap; }

inline const Type* ContinuationGenerator::getReturnType(LLVMContext& context) {
	return Type::getInt32Ty(context);
}


inline bool ContinuationGenerator::isBarrier(const Value* value) const {
	if (!isa<CallInst>(value)) return false;
	const CallInst* call = cast<CallInst>(value);
	const Function* callee = call->getCalledFunction();
	return callee->getName().equals(WFVOPENCL_FUNCTION_NAME_BARRIER);
}
inline bool ContinuationGenerator::blockEndsWithBarrierBeforeTerminator(const BasicBlock* block) const {
	if (block->getInstList().size() < 2) return false;  // block cannot end with barrier

	const Instruction* secondLastInst = --(--block->end()); // last inst = terminator
	return isBarrier(secondLastInst);
}

#if 0
void ContinuationGenerator::mapBarrierInfo(Function* f, Function* newF, ValueMap<const Value*, Value*>& valueMap, BarrierVecType& barriers, ValueMap<const Value*, Value*>& barrierMapping) {
	assert (f && newF);
	assert (!valueMap.empty());
	DEBUG_LA( outs() << "\nmapping barrier info from function '" << f->getNameStr() << "' to new function '" << newF->getNameStr() << "'...\n"; );

	for (BarrierVecType::iterator it=barriers.begin(), E=barriers.end(); it!=E; ++it) {
		BarrierInfo* binfo = *it;
		const BasicBlock* oldBB = binfo->parentBlock;
#if 1
		binfo->barrier = cast<CallInst>(valueMap[binfo->barrier]);
		binfo->parentBlock = cast<BasicBlock>(valueMap[oldBB]);

		//const Value* oldBarrier = binfo->barrier;
		//barrierMapping[binfo->barrier] = oldBarrier;

		DEBUG_LA( outs() << "  mapped barrier: " << *binfo->barrier << "\n"; );
		DEBUG_LA( outs() << "    parent block: " << oldBB->getNameStr() << " -> " << binfo->parentBlock->getNameStr() << "\n"; );
#else
		CallInst* mappedBarrier = cast<CallInst>(valueMap[binfo->barrier]);
		BasicBlock* mappedParentBlock = cast<BasicBlock>(valueMap[oldBB]);

		BarrierInfo* mappedBarrierInfo = new BarrierInfo(mappedBarrier, mappedParentBlock, binfo->depth);
		mappedBarrierInfo->id = binfo->id;
		mappedBarrierInfo->continuation = binfo->continuation;
		mappedBarrierInfo->liveValueStructType = binfo->liveValueStructType;
		mappedBarriers.push_back(mappedBarrierInfo);

		DEBUG_LA( outs() << "  mapped barrier: " << *mappedBarrierInfo->barrier << "\n"; );
		DEBUG_LA( outs() << "    parent block: " << oldBB->getNameStr() << " -> " << mappedBarrierInfo->parentBlock->getNameStr() << "\n"; );
#endif
	}
}
#endif

// helper for getBarrierInfo()
unsigned ContinuationGenerator::collectBarriersDFS(BasicBlock* block, unsigned depth, BarrierVecType& barriers, SmallVector<unsigned, 4>& barrierDepths, unsigned& maxBarrierDepth, std::set<BasicBlock*>& visitedBlocks) {
	assert (block);
	if (visitedBlocks.find(block) != visitedBlocks.end()) return 0;
	visitedBlocks.insert(block);

	unsigned numBarriers = 0;
	DEBUG_LA( outs() << "collectBarriersDFS(" << block->getNameStr() << ", " << depth << ")\n"; );

	Loop* loop = loopInfo->getLoopFor(block);

	// if this is the header of a loop, recurse into exit-blocks of loop
	if (loop && loop->getHeader() == block) {
		SmallVector<BasicBlock*, 4> exitBlocks;
		loop->getExitBlocks(exitBlocks);

		for (SmallVector<BasicBlock*, 4>::iterator it=exitBlocks.begin(), E=exitBlocks.end(); it!=E; ++it) {
			BasicBlock* exitBB = *it;
			numBarriers += collectBarriersDFS(exitBB, depth+1, barriers, barrierDepths, maxBarrierDepth, visitedBlocks);
		}
		DEBUG_LA( outs() << "  successors of loop with header " << block->getNameStr() << " have " << numBarriers << " barriers!\n"; );
	}

	for (succ_iterator S=succ_begin(block), E=succ_end(block); S!=E; ++S) {
		BasicBlock* succBB = *S;
		numBarriers += collectBarriersDFS(succBB, depth+1, barriers, barrierDepths, maxBarrierDepth, visitedBlocks);
	}
	DEBUG_LA( if (numBarriers > 0) outs() << "  successors of " << block->getNameStr() << " have " << numBarriers << " barriers!\n"; );

	// safety check required because we iterate in reversed order
	if (block->empty()) {
		DEBUG_LA( outs() << "collectBarriersDFS(" << block->getNameStr() << ", " << depth << ") -> " << numBarriers << "\n"; );
		return numBarriers;
	}

	BasicBlock::iterator I = block->end();
	do {
		--I;
		if (!isa<CallInst>(I)) continue;
		CallInst* call = cast<CallInst>(I);

		const Function* callee = call->getCalledFunction();
		if (!callee->getName().equals(WFVOPENCL_FUNCTION_NAME_BARRIER)) continue;

		++numBarriers;

		barriers.push_back(call);
		barrierDepths.push_back(depth);
		DEBUG_LA( outs() << "    barrier found in block '" << block->getNameStr() << "': " << *call << "\n"; );

		if (depth > maxBarrierDepth) maxBarrierDepth = depth;
	} while (I != block->begin());

	DEBUG_LA( outs() << "collectBarriersDFS(" << block->getNameStr() << ", " << depth << ") -> " << numBarriers << "\n"; );

	return numBarriers;
}

// Traverse the function in DFS and collect all barriers in post-reversed order.
// Count how many barriers the function has and assign an id to each barrier
// TODO: rewrite and use information that barriers are always the last instructions before the terminator in a BB
inline unsigned ContinuationGenerator::getBarrierInfo(Function* f, BarrierVecType& barriers, SmallVector<unsigned, 4>& barrierDepths, unsigned& maxBarrierDepth) {
	std::set<BasicBlock*> visitedBlocks;
	const unsigned numBarriers = collectBarriersDFS(&f->getEntryBlock(), 0, barriers, barrierDepths, maxBarrierDepth, visitedBlocks);

	DEBUG_LA(
		outs() << "\nnum barriers: " << numBarriers << "\n";
		for (BarrierVecType::iterator it=barriers.begin(), E=barriers.end(); it!=E; ++it) {
			outs() << " * " << **it << "\n";
		}
		outs() << "\n";
	);

	return numBarriers;
}

void ContinuationGenerator::findContinuationBlocksDFSOrdered(BasicBlock* block, std::vector<BasicBlock*>& continuationRegion, std::set<BasicBlock*>& visitedBlocks) {
	assert (block);
	if (visitedBlocks.find(block) != visitedBlocks.end()) return;
	visitedBlocks.insert(block);

	continuationRegion.push_back(block);

	for (succ_iterator S=succ_begin(block), E=succ_end(block); S!=E; ++S) {
		BasicBlock* succBB = *S;
		findContinuationBlocksDFSOrdered(succBB, continuationRegion, visitedBlocks);
	}
}
void ContinuationGenerator::findContinuationBlocksDFS(BasicBlock* block, SetVector<BasicBlock*>& continuationRegion, std::set<const BasicBlock*>& visitedBlocks) {
	assert (block);
	if (visitedBlocks.find(block) != visitedBlocks.end()) return;
	visitedBlocks.insert(block);

	continuationRegion.insert(block);

	// if we find a barrier, there is no need to copy the rest of the
	// blocks below as well (requires upfront splitting of all barrier-blocks)
	if (blockEndsWithBarrierBeforeTerminator(block)) return;

	for (succ_iterator S=succ_begin(block), E=succ_end(block); S!=E; ++S) {
		BasicBlock* succBB = *S;
		findContinuationBlocksDFS(succBB, continuationRegion, visitedBlocks);
	}
}
void ContinuationGenerator::findContinuationBlocksDFS(const BasicBlock* block, SetVector<const BasicBlock*>& continuationRegion, std::set<const BasicBlock*>& visitedBlocks) {
	assert (block);
	if (visitedBlocks.find(block) != visitedBlocks.end()) return;
	visitedBlocks.insert(block);

	continuationRegion.insert(block);

	// if we find a barrier, there is no need to copy the rest of the
	// blocks below as well (requires upfront splitting of all barrier-blocks)
	if (blockEndsWithBarrierBeforeTerminator(block)) return;

	for (succ_const_iterator S=succ_begin(block), E=succ_end(block); S!=E; ++S) {
		const BasicBlock* succBB = *S;
		findContinuationBlocksDFS(succBB, continuationRegion, visitedBlocks);
	}
}


/// definedInRegion - Return true if the specified value is defined in the
/// cloned region.
/// From: llvm-2.8svn/lib/Transforms/Utils/CodeExtractor.cpp:definedInRegion()
bool ContinuationGenerator::definedInRegion(Value *V, SetVector<BasicBlock*>& region) const {
  if (Instruction *I = dyn_cast<Instruction>(V))
	if (region.count(I->getParent()))
	  return true;
  return false;
}
bool ContinuationGenerator::definedInRegion(const Value *V, SetVector<const BasicBlock*>& region) const {
  if (const Instruction *I = dyn_cast<Instruction>(V))
	if (region.count(I->getParent()))
	  return true;
  return false;
}

/// definedInCaller - Return true if the specified value is defined in the
/// function being code cloned, but not in the region being cloned.
/// These values must be passed in as live-ins to the function.
/// From: llvm-2.8svn/lib/Transforms/Utils/CodeExtractor.cpp:definedInCaller()
bool ContinuationGenerator::definedOutsideRegion(Value *V, SetVector<BasicBlock*>& region) const {
  if (isa<Argument>(V)) return true;
  if (Instruction *I = dyn_cast<Instruction>(V))
	if (!region.count(I->getParent()))
	  return true;
  return false;
}
bool ContinuationGenerator::definedOutsideRegion(const Value *V, SetVector<const BasicBlock*>& region) const {
  if (isa<Argument>(V)) return true;
  if (const Instruction *I = dyn_cast<Instruction>(V))
	if (!region.count(I->getParent()))
	  return true;
  return false;
}

#if 0
// generates a continuation function that is called at the point of the barrier
// mapping of live values is stored in valueMap
Function* ContinuationGenerator::createContinuation(CallInst* barrier, const std::string& newFunName, TargetData* targetData) {
	assert (barrier && targetData);
	assert (barrierIndices.count(barrier));
	const unsigned barrierIndex = barrierIndices[barrier];
	assert (barrier->getParent());
	BasicBlock* parentBlock = barrier->getParent();
	assert (parentBlock->getParent());
	Function* f = parentBlock->getParent();
	assert (f);
	Module* mod = f->getParent();
	assert (mod);

	LLVMContext& context = mod->getContext();

	DEBUG_LA( outs() << "\ngenerating continuation for barrier " << barrierIndex << " in block '" << parentBlock->getNameStr() << "'\n"; );
	DEBUG_LA( outs() << "  barrier: " << *barrier << "\n"; );

	//--------------------------------------------------------------------//
	// First of all, split block at the position of the barrier
	// (barrier has to be in "upper block")
	// This makes a lot of things easier :)
	//--------------------------------------------------------------------//
	// find instruction that precedes barrier (= split point)
	Instruction* splitInst = NULL;
	BasicBlock::iterator I = --(parentBlock->end());
	while (barrier != I) {
		splitInst = I--;
	}
	// 'newBlock' is the first of the new continuation, 'parentBlock' the one with the barrier
	BasicBlock* newBlock = parentBlock->splitBasicBlock(splitInst, parentBlock->getNameStr()+".postbarrier");

	//--------------------------------------------------------------------//
	// Delete the edge from parentBlock to newBlock
	//--------------------------------------------------------------------//
	assert (parentBlock->getTerminator()->use_empty());
	parentBlock->getTerminator()->eraseFromParent();

	//--------------------------------------------------------------------//
	// Create return that returns the id for the next call.
	// This is required to have a correct function for findContinuationBlocksDFS()
	// Note that afterwards, no instruction must be inserted before barrier,
	// but all before returnInst (important for generation of stores)
	//--------------------------------------------------------------------//
	ReturnInst* returnInst = ReturnInst::Create(context, ConstantInt::get(getReturnType(context), barrierIndex, true), barrier);

	//--------------------------------------------------------------------//
	// Erase the barrier.
	// After this, only stores have to be generated to finish the old
	// function's modification.
	//--------------------------------------------------------------------//
	// make sure we remove the barrier from the indices-map ;)
	barrierIndices.erase(barrier);
	assert (barrier->use_empty() && "barriers must not have any uses!");
	barrier->eraseFromParent();

	DEBUG_LA( verifyFunction(*f); );



	//--------------------------------------------------------------------//
	// Find all blocks 'below' parentBlock inside the new function (DFS)
	// Make sure the edge between parentBlock and newBlock has been removed!
	//--------------------------------------------------------------------//
	SetVector<BasicBlock*> copyBlocks;
	std::set<const BasicBlock*> visitedBlocks;
	findContinuationBlocksDFS(newBlock, copyBlocks, visitedBlocks);

	DEBUG_LA(
		outs() << "\nblocks that need to be cloned into the continuation:\n";
		for (SetVector<BasicBlock*>::iterator it=copyBlocks.begin(), E=copyBlocks.end(); it!=E; ++it) {
			outs() << " * " << (*it)->getNameStr() << "\n";
		}
		outs() << "\n";
	);


	//--------------------------------------------------------------------//
	// compute dominator tree of modified original function
	// (somehow does not work with functionpassmanager here)
	//--------------------------------------------------------------------//
	DominatorTree* oldDomTree = new DominatorTree();
	oldDomTree->runOnFunction(*f);


	//--------------------------------------------------------------------//
	// Collect live values to store at barrier/load in new function
	//--------------------------------------------------------------------//
	LiveSetType liveInValues;
	LiveSetType liveOutValues; // probably not even needed...

	/// From: llvm-2.8svn/lib/Transforms/Utils/CodeExtractor.cpp:findInputsOutputs()
	for (SetVector<BasicBlock*>::const_iterator ci = copyBlocks.begin(), ce = copyBlocks.end(); ci != ce; ++ci) {
		BasicBlock *BB = *ci;

		for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
			// If a used value is defined outside the region, it's an input.  If an
			// instruction is used outside the region, it's an output.
			for (User::op_iterator O = I->op_begin(), E = I->op_end(); O != E; ++O) {
				if (definedOutsideRegion(*O, copyBlocks)) liveInValues.insert(*O);
				else if (Instruction* opI = dyn_cast<Instruction>(O)) {
					// Check if the operand does not dominate I after the splitting.
					// If so, consider it live-in (live across loop boundaries).
					// Ignore the operands if they belong to a phi
					if (!isa<PHINode>(I) && !oldDomTree->dominates(opI, I)) liveInValues.insert(*O);
				}
			}

			// Consider uses of this instruction (outputs).
			for (Value::use_iterator UI = I->use_begin(), E = I->use_end(); UI != E; ++UI) {
				if (!definedInRegion(*UI, copyBlocks)) {
					liveOutValues.insert(I);
					break;
				}
			}

		} // for: insts
	} // for: basic blocks

	delete oldDomTree;

	DEBUG_LA(
		outs() << "\nlive-in values of continuation-entry-block '" << newBlock->getNameStr() << "'\n";
		for (LiveSetType::iterator it=liveInValues.begin(), E=liveInValues.end(); it!=E; ++it) {
			outs() << " * " << **it << "\n";
		}
		outs() << "\n";
	);

	//--------------------------------------------------------------------//
	// Create live-value struct-type from live-in values of newBlock
	//--------------------------------------------------------------------//
	std::vector<const Type*> params;
	for (LiveSetType::iterator it=liveInValues.begin(), E=liveInValues.end(); it!=E; ++it) {
		params.push_back((*it)->getType());
	}
	const bool isPacked = false; // true ???
	StructType* sType = StructType::get(context, params, isPacked);
	DEBUG_LA( outs() << "new struct type   : " << *sType << "\n"; );
	DEBUG_LA( outs() << "type size in bits : " << targetData->getTypeSizeInBits(sType) << "\n"; );
	DEBUG_LA( outs() << "alloc size in bits: " << targetData->getTypeAllocSizeInBits(sType) << "\n"; );
	DEBUG_LA( outs() << "alloc size        : " << targetData->getTypeAllocSize(sType) << "\n"; );

	//--------------------------------------------------------------------//
	// Generate stores of the live values before the return where the barrier
	// was (in the old function).
	// After this, the modification of the old function is finished.
	//--------------------------------------------------------------------//
	// pointer to union for live value struct for next call is the last parameter
	Argument* nextContLiveValStructPtr = --(f->arg_end());
	DEBUG_LA( outs() << "pointer to union: " << *nextContLiveValStructPtr << "\n"; );

	// bitcast data pointer to correct struct type for GEP below
	BitCastInst* bc = new BitCastInst(nextContLiveValStructPtr, PointerType::getUnqual(sType), "", returnInst);

	// generate the gep/store combinations for each live value
	unsigned i=0;
	for (LiveSetType::iterator it=liveInValues.begin(), E=liveInValues.end(); it!=E; ++it) {
		std::vector<Value*> indices;
		indices.push_back(ConstantInt::getNullValue(Type::getInt32Ty(context)));
		indices.push_back(ConstantInt::get(context, APInt(32, i++)));
		GetElementPtrInst* gep = GetElementPtrInst::Create(bc, indices.begin(), indices.end(), "", returnInst);
		DEBUG_LA( outs() << "store gep(" << i-1 << "): " << *gep << "\n"; );
		const unsigned align = 16;
		new StoreInst(*it, gep, false, align, returnInst);
	}

	outs() << *f << "\n";
	DEBUG_LA( verifyFunction(*f); );


	//--------------------------------------------------------------------//
	// create new function (the continuation) with the following signature:
	// - returns int (id of next continuation)
	// - special parameters
	// - live-in value parameters
	// - last parameter: void* data (union where live values for next
	//                   continuation are stored before returning)
	//
	// - original parameters of f not required (included in live vals if live)
	//--------------------------------------------------------------------//
	params.clear();
	params.insert(params.end(), specialParams.begin(), specialParams.end());   // special parameters
	params.insert(params.end(), sType->element_begin(), sType->element_end()); // live values
	params.push_back(Type::getInt8PtrTy(context)); // data ptr

	FunctionType* fType = FunctionType::get(getReturnType(context), params, false);

	Function* continuation = Function::Create(fType, Function::ExternalLinkage, newFunName, mod); // TODO: check linkage type

	Function::arg_iterator CA=continuation->arg_begin();
	for (unsigned i=0, e=specialParamNames.size(); i<e; ++i, ++CA) {
		CA->setName(specialParamNames[i]);
	}
	DEBUG_LA( outs() << "\nnew continuation declaration: " << *continuation << "\n"; );

	//--------------------------------------------------------------------//
	// Create mappings of live-in values to arguments for copying of blocks
	//--------------------------------------------------------------------//
	// We need a second value map only for the arguments because valueMap
	// is given to CloneFunctionInto, which may overwrite these.
	ValueMap<const Value*, Value*>* valueMap = new ValueMap<const Value*, Value*>();
	DenseMap<const Value*, Value*> liveValueToArgMap;
	// CA has to point to first live value parameter of continuation
	CA = --continuation->arg_end();
	for (unsigned i=0, e=sType->getNumContainedTypes(); i<e; ++i) {
		--CA;
	}
	DEBUG_LA( outs() << "\nlive value mappings:\n"; );
	for (LiveSetType::iterator it=liveInValues.begin(), E=liveInValues.end(); it!=E; ++it, ++CA) {
		const Value* liveVal = *it;
		(*valueMap)[liveVal] = CA;
		liveValueToArgMap[liveVal] = CA;
		DEBUG_LA( outs() << " * " << *liveVal << " -> " << *CA << "\n"; );
	}


	//--------------------------------------------------------------------//
	// HACK: Copy over entire function and remove all unnecessary blocks.
	//       This is required because CloneBasicBlock does not perform any
	//       remapping. RemapInstructions thus has to be called by hand and
	//       is not available via includes (as of llvm-2.8svn ~May/June 2010 :p).
	//--------------------------------------------------------------------//

	// Therefore, we need to have dummy-mappings for all arguments of the
	// old function that do not map to a live value.
	// TODO: erase dummy values again from value map at some point?
	for (Function::arg_iterator A=f->arg_begin(), AE=f->arg_end(); A!=AE; ++A) {
		if ((*valueMap).find(A) != (*valueMap).end()) continue;
		Value* dummy = UndefValue::get(A->getType());
		(*valueMap)[A] = dummy;
	}

	// create mapping for data pointer (last argument to last argument)
	(*valueMap)[(--(f->arg_end()))] = --(continuation->arg_end());

	SmallVector<ReturnInst*, 2> returns;
	CloneFunctionInto(continuation, f, (*valueMap), returns, ".");



	//--------------------------------------------------------------------//
	// remap values that were no arguments
	// NOTE: apparently, CloneFunctionInto does not look into the valueMap
	//       if it directly finds all references. Due to the fact that we
	//       copy the entire function, live values that are no arguments
	//       are still resolved by their former definitions which only get
	//       erased in the next step.
	// NOTE: actually, this is good behaviour - we must not remap values
	//       that are defined in a block that also appears in the
	//       continuation, e.g. in loops.
	//--------------------------------------------------------------------//

	//--------------------------------------------------------------------//
	// erase all blocks that do not belong to this continuation
	//--------------------------------------------------------------------//
	BasicBlock* dummyBB = BasicBlock::Create(context, "dummy", continuation);
	// iterate over blocks of original fun, but work on blocks of continuation fun
	// -> can't find out block in old fun for given block in new fun via map ;)
	for (Function::iterator BB=f->begin(), BBE=f->end(); BB!=BBE; ) {
		assert ((*valueMap).find(BB) != (*valueMap).end());
		BasicBlock* blockOrig = BB++;
		BasicBlock* blockCopy = cast<BasicBlock>((*valueMap)[blockOrig]);
		if (copyBlocks.count(blockOrig)) continue; // current block is copied

		// current block must not be "copied" -> delete it

		// but first, replace all uses of instructions of block by dummies
		// or arguments in case of live values...
		BasicBlock::iterator IO=blockOrig->begin(); // mapping uses values from old function...
		for (BasicBlock::iterator I=blockCopy->begin(), IE=blockCopy->end(); I!=IE; ++I, ++IO) {
			DEBUG_LA( outs() << "replacing uses of inst: " << *I << "\n"; );
			if (liveInValues.count(IO)) {
				DEBUG_LA( outs() << "  is a live value, replaced with argument: " << *liveValueToArgMap[IO] << "\n"; );
				I->replaceAllUsesWith(liveValueToArgMap[IO]);
			} else {
				I->replaceAllUsesWith(UndefValue::get(I->getType()));
			}

			// erase instruction from value map (pointer will be invalid after deletion)
			(*valueMap).erase(IO);
		}

		// erase block from value map (pointer will be invalid after deletion)
		(*valueMap).erase(blockOrig);

		// remove all incoming values from this block to phis
		DEBUG_LA( outs() << "block: " << blockCopy->getNameStr() << "\n"; );
		for (BasicBlock::use_iterator U=blockCopy->use_begin(), UE=blockCopy->use_end(); U!=UE; ++U) {
			if (!isa<PHINode>(*U)) continue;
			PHINode* phi = cast<PHINode>(*U);
			DEBUG_LA( outs() << "phi: " << *phi << "\n"; );
			if (phi->getBasicBlockIndex(blockCopy) != -1) phi->removeIncomingValue(blockCopy, false);
		}

		blockCopy->replaceAllUsesWith(dummyBB);
		blockCopy->eraseFromParent();
	}

	DEBUG_LA( outs() << "\n"; );

	// erase dummy block
	assert (dummyBB->use_empty());
	dummyBB->eraseFromParent();

	//--------------------------------------------------------------------//
	// if necessary, make new block the entry block of the continuation
	// (= move to front of block list)
	//--------------------------------------------------------------------//
	BasicBlock* entryBlock = cast<BasicBlock>((*valueMap)[newBlock]);
	if (entryBlock != &continuation->getEntryBlock()) {
		entryBlock->moveBefore(&continuation->getEntryBlock());
	}


	//--------------------------------------------------------------------//
	// Make sure all uses of live values that are not dominated anymore are
	// rewired to arguments.
	// NOTE: This can't be done before blocks are deleted ;).
	//--------------------------------------------------------------------//

	// compute dominator tree of continuation
	DominatorTree* domTree = new DominatorTree();
	domTree->runOnFunction(*continuation);

	// set arg_iterator to first live value arg (first args are special values)
	Function::arg_iterator A2 = continuation->arg_begin();
	std::advance(A2, specialParams.size());
	for (LiveSetType::iterator it=liveInValues.begin(), E=liveInValues.end(); it!=E; ++it, ++A2) {
		Value* liveVal = *it;
		DEBUG_LA( outs() << "live value: " << *liveVal << "\n"; );
		if (isa<Argument>(liveVal)) continue;

		// if all uses were already replaced above, skip this value
		if ((*valueMap).find(liveVal) == (*valueMap).end()) {
			DEBUG_LA( outs() << "  all uses already replaced!\n"; );
			continue;
		}

		Value* newLiveVal = (*valueMap)[liveVal];
		DEBUG_LA( outs() << "new live value: " << *newLiveVal << "\n"; );

		// if the value is defined in one of the copied blocks, we must only
		// replace those uses that are not dominated by their definition anymore
		if (Instruction* inst = dyn_cast<Instruction>(liveVal)) {
			if (copyBlocks.count(inst->getParent())) {
				Instruction* newInst = cast<Instruction>(newLiveVal);
				DEBUG_LA( outs() << "live value is defined in copied block: " << inst->getParent()->getNameStr() << "\n"; );
				for (Instruction::use_iterator U=newInst->use_begin(), UE=newInst->use_end(); U!=UE; ) {
					assert (isa<Instruction>(*U));
					Instruction* useI = cast<Instruction>(*U++);
					DEBUG_LA( outs() << "  testing use: " << *useI << "\n"; );
					if (domTree->dominates(newInst, useI)) continue;
					DEBUG_LA( outs() << "    is not dominated, will be replaced by argument: " << *A2 << "\n"; );
					useI->replaceUsesOfWith(newInst, A2);
				}
				continue;
			}
		}

		newLiveVal->replaceAllUsesWith(A2);
	}

	DEBUG_LA( outs() << *continuation << "\n"; );
	DEBUG_LA( outs() << "continuation '" << continuation->getNameStr() << "' generated successfully!\n\n"; );

	DEBUG_LA(
		outs() << "verifying function '" << f->getNameStr() << "'... ";
		verifyFunction(*f);
		outs() << "done.\nverifying function '" << continuation->getNameStr() << "'... ";
		verifyFunction(*continuation);
		outs() << "done.\n";
	);

	assert (continuationMap.count(barrierIndex));
	BarrierInfo* barrierInfo = continuationMap[barrierIndex];

	if (!barrierInfo->continuation) {
		barrierInfo->continuation = continuation;
		barrierInfo->liveValueStructType = sType;
		return continuation;
	} else {
		return NULL;
	}
}
#endif

void ContinuationGenerator::removeLiveValStructFromExtractedFnArgType(Function* f, Value* nextContLiveValStructPtr, LLVMContext& context, Instruction* insertBefore) {
	assert (f && nextContLiveValStructPtr);

	// get call to new fun
	assert (f->getNumUses() == 1);
	CallInst* call = cast<CallInst>(f->use_back());

	// get store-use of nextContLiveValStructPtr (where it is stored to argStruct of f)
	StoreInst* store = NULL;
	for (Value::use_iterator U=nextContLiveValStructPtr->use_begin(), UE=nextContLiveValStructPtr->use_end(); U!=UE; ++U) {
		if (!isa<StoreInst>(*U)) continue;
		store = cast<StoreInst>(*U);
		if (store->getParent() == call->getParent()) break; // found the right one
	}
	assert (store && "could not find location where live val struct is stored to arg struct of extracted region fn!");

	// get the parameter index of the value inside the arg struct
	assert (isa<GetElementPtrInst>(store->getPointerOperand()));
	GetElementPtrInst* liveValGEP = cast<GetElementPtrInst>(store->getPointerOperand());
	Value* indexVal = cast<Value>(liveValGEP->idx_end()-1);
	assert (isa<ConstantInt>(indexVal));
	const ConstantInt* indexConst = cast<ConstantInt>(indexVal);
	const uint64_t index = *indexConst->getValue().getRawData();

	// get arg struct of f
	assert (call->getNumOperands() == 2);
	assert (isa<AllocaInst>(call->getArgOperand(0)));
	AllocaInst* argStruct = cast<AllocaInst>(call->getArgOperand(0));

	// get type of arg struct ("pointerless")
	assert (isa<PointerType>(argStruct->getType()));
	assert (isa<StructType>(argStruct->getType()->getContainedType(0)));
	const StructType* argStructType = cast<StructType>(argStruct->getType()->getContainedType(0));

	// create new struct type with all the same types except the one at 'index'
	std::vector<const Type*> newSubTypes;
	for (unsigned i=0; i<argStructType->getNumElements(); ++i) {
		if (i == index) continue;
		newSubTypes.push_back(argStructType->getContainedType(i));
	}

	const StructType* newArgStructType = StructType::get(context, newSubTypes, argStructType->isPacked());
	assert (newArgStructType->getNumElements() == argStructType->getNumElements()-1);

	DEBUG_LA( outs() << "  arg struct type generated by ECR: " << *argStructType << "\n"; );
	DEBUG_LA( outs() << "  arg struct type generated manually: " << *newArgStructType << "\n"; );

	// directly erase store and corresponding gep (should not be used anywhere else)
	store->eraseFromParent();
	assert (liveValGEP->use_empty());
	liveValGEP->eraseFromParent();

	// bitcast data pointer to adjusted struct type
	BitCastInst* nextLiveValStruct = new BitCastInst(nextContLiveValStructPtr, PointerType::getUnqual(newArgStructType), "", insertBefore);

	// adjust all other geps related to arg struct
	// (replace pointer operand (alloca) by bitcast, adjust indices)
	for (AllocaInst::use_iterator U=argStruct->use_begin(), UE=argStruct->use_end(); U!=UE; ) {
		DEBUG_LA( outs() << "adjusting use of ECR-generated arg struct: " << **U << "\n"; );
		if (isa<CallInst>(*U)) { ++U; continue; } // ignore use that goes into the ECR-generated function
		assert (isa<GetElementPtrInst>(*U));
		GetElementPtrInst* gep = cast<GetElementPtrInst>(*U++);

		Value* idxVal = cast<Value>(gep->idx_end()-1);
		assert (isa<ConstantInt>(idxVal));
		const ConstantInt* idxConst = cast<ConstantInt>(idxVal);
		const uint64_t idx = *idxConst->getValue().getRawData();

		assert (idx != index && "old store of live val struct must not exist anymore!");

		if (idx < index) {
			// index does not have to be modified,
			// so we only have to update the pointer operand
			gep->replaceUsesOfWith(argStruct, nextLiveValStruct);
			continue;
		}

		// otherwise, create new gep (other index, other pointer operand)
		std::vector<Value*> indices;
		// all indices except for the last one remain unchanged
#if 0
		for (GetElementPtrInst::op_iterator IDX=gep->idx_begin(), IDXE=gep->idx_end()-1; IDX!=IDXE; ++IDX) {
			indices.push_back(*IDX);
		}
#else
		assert (gep->getNumIndices() == 2);
		indices.push_back(ConstantInt::getNullValue(idxVal->getType()));
#endif
		indices.push_back(ConstantInt::get(idxVal->getType(), idx-1, false)); // "move" index back one element

		GetElementPtrInst* newGEP = GetElementPtrInst::Create(nextLiveValStruct, indices.begin(), indices.end(), "", gep);
		gep->replaceAllUsesWith(newGEP);
		gep->eraseFromParent();

		DEBUG_LA( outs() << "  new GEP: " << *newGEP << "\n"; );
	}

	// there should only be one use of the ECR-generated arg struct left (the call)
	assert (argStruct->getNumUses() == 1);
}

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
void ContinuationGenerator::createSecondaryContinuation(CallInst* barrier, const unsigned barrierIndex, Function* continuation) {
	assert (barrier && continuation && barrierIndex > 0);
	Module* mod = continuation->getParent();
	assert (mod);
	BasicBlock* parentBlock = barrier->getParent();
	assert (parentBlock);

	LLVMContext& context = mod->getContext();

	DEBUG_LA( outs() << "\ngenerating secondary continuation for barrier " << barrierIndex << " in continuation '" << continuation->getNameStr() << "'\n"; );
	DEBUG_LA( outs() << "  barrier: " << *barrier << "\n"; );

	//--------------------------------------------------------------------//
	// split block at the position of the barrier
	// (barrier has to be in "upper block")
	//--------------------------------------------------------------------//
	// find instruction that precedes barrier (= split point)
	Instruction* splitInst = NULL;
	BasicBlock::iterator I = --(parentBlock->end());
	while (barrier != I) {
		splitInst = I--;
	}
	// 'newBlock' is the first of the new continuation, 'parentBlock' the one with the barrier
	BasicBlock* newBlock = parentBlock->splitBasicBlock(splitInst, parentBlock->getNameStr()+".postbarrier");


	//--------------------------------------------------------------------//
	// ExtractCodeRegion approach
	//--------------------------------------------------------------------//

	// TODO: use domTree computed in createContinuation(), should be the same :p
	DominatorTree* domTree = new DominatorTree();
	domTree->runOnFunction(*continuation);

	// collect all blocks 'below' parentBlock (DFS)
	std::vector<BasicBlock*> copyBlocks;
	std::set<BasicBlock*> visitedBlocks;
	findContinuationBlocksDFSOrdered(newBlock, copyBlocks, visitedBlocks);

	DEBUG_LA( outs() << "\nblocks for ExtractCodeRegion:\n"; );
	std::vector<BasicBlock*> region;
	for (std::vector<BasicBlock*>::iterator it=copyBlocks.begin(), E=copyBlocks.end(); it!=E; ++it) {
		region.push_back(*it);
		DEBUG_LA( outs() << " * " << (*it)->getNameStr() << "\n"; );
	}

	const bool aggregateArgs = true;

	Function* secContinuation = ExtractCodeRegion(*domTree, region, aggregateArgs);

	//--------------------------------------------------------------------//
	//--------------------------------------------------------------------//

	// The live value struct itself is also saved as a live value by ExtractCodeRegion...
	// This leads to a bad struct layout (continuation expects struct without it)
	// -> change type
	Argument* nextContLiveValStructPtr = --(continuation->arg_end());
	removeLiveValStructFromExtractedFnArgType(secContinuation, nextContLiveValStructPtr, context, barrier);

	// get call to new fun
	assert (secContinuation->getNumUses() == 1);
	CallInst* secCall = cast<CallInst>(secContinuation->use_back());

	//--------------------------------------------------------------------//
	// erase branch to block with old return statement
	//--------------------------------------------------------------------//
	secCall->getParent()->getTerminator()->eraseFromParent();

	//--------------------------------------------------------------------//
	// instead, create return that returns the id for the next call
	//--------------------------------------------------------------------//
	ReturnInst::Create(context, ConstantInt::get(getReturnType(context), barrierIndex, true), secCall);

	//--------------------------------------------------------------------//
	// erase call (new function is actually not used, it should already exist :p)
	// and arg struct (last use was the call)
	//--------------------------------------------------------------------//
	assert (secCall->getNumOperands() == 2);
	assert (isa<AllocaInst>(secCall->getArgOperand(0)));
	AllocaInst* argStruct = cast<AllocaInst>(secCall->getArgOperand(0));
	secCall->eraseFromParent();
	assert (argStruct->use_empty());
	argStruct->eraseFromParent();

	//--------------------------------------------------------------------//
	// erase barrier
	//--------------------------------------------------------------------//
	assert (barrier->use_empty() && "barriers must not have any uses!");
	barrier->eraseFromParent();

	//DEBUG_LA( outs() << "secondary continuation:\n" << *secContinuation << "\n"; );
	//DEBUG_LA( outs() << "original function:\n" << *continuation << "\n"; );
}

inline bool ContinuationGenerator::functionHasBarrier(Function* f) {
	for (Function::const_iterator BB=f->begin(), BBE=f->end(); BB!=BBE; ++BB) {
		for (BasicBlock::const_iterator I=BB->begin(), IE=BB->end(); I!=IE; ++I) {
			if (!isa<CallInst>(I)) continue;
			if (isBarrier(I)) return true;
		}
	}
	return false;
}
inline CallInst* ContinuationGenerator::findBarrier(Function* f) {
	// TODO: reverse iterations
	for (Function::iterator BB=f->begin(), BBE=f->end(); BB!=BBE; ++BB) {
		for (BasicBlock::iterator I=BB->begin(), IE=BB->end(); I!=IE; ++I) {
			if (!isa<CallInst>(I)) continue;
			if (isBarrier(I)) return cast<CallInst>(I);
		}
	}
	return NULL;
}

#if 0
//
// TODO:
// - check if continuation has been generated before, if no, save it in continuation map, if yes, delete it
// - together with continuation, store the order of its live values in the struct
// - in order to find out this order we need to have value mappings from ExtractCodeRegion
Function* ContinuationGenerator::createContinuationNEW(CallInst* barrier, const unsigned barrierIndex, Function* currentFn, ContinuationMapType& continuationMap) {
	assert (barrier && currentFn);
	Module* mod = currentFn->getParent();
	assert (mod);
	BasicBlock* parentBlock = barrier->getParent();
	assert (parentBlock);

	LLVMContext& context = mod->getContext();

	DEBUG_LA( outs() << "\ngenerating continuation for barrier " << barrierIndex << " in function '" << currentFn->getNameStr() << "'\n"; );
	DEBUG_LA( outs() << "  barrier: " << *barrier << "\n"; );

	//--------------------------------------------------------------------//
	// split block at the position of the barrier
	// (barrier has to be in "upper block")
	//--------------------------------------------------------------------//
	// find instruction that precedes barrier (= split point)
	Instruction* splitInst = NULL;
	BasicBlock::iterator I = --(parentBlock->end());
	while (barrier != I) {
		splitInst = I--;
	}
	// 'newBlock' is the first of the new continuation, 'parentBlock' the one with the barrier
	BasicBlock* newBlock = parentBlock->splitBasicBlock(splitInst, parentBlock->getNameStr()+".postbarrier");


	//--------------------------------------------------------------------//
	// ExtractCodeRegion approach
	//--------------------------------------------------------------------//

	// dominator tree has to be rebuilt for every barrier (CFG changes every time)
	DominatorTree* domTree = new DominatorTree();
	domTree->runOnFunction(*currentFn);

	// collect all blocks 'below' parentBlock (DFS)
	std::vector<BasicBlock*> copyBlocks;
	std::set<BasicBlock*> visitedBlocks;
	findContinuationBlocksDFSOrdered(newBlock, copyBlocks, visitedBlocks);

	DEBUG_LA( outs() << "\nblocks for ExtractCodeRegion:\n"; );
	std::vector<BasicBlock*> region;
	for (std::vector<BasicBlock*>::iterator it=copyBlocks.begin(), E=copyBlocks.end(); it!=E; ++it) {
		region.push_back(*it);
		DEBUG_LA( outs() << " * " << (*it)->getNameStr() << "\n"; );
	}

	const bool aggregateArgs = true;

	Function* continuation = ExtractCodeRegion(*domTree, region, aggregateArgs);

	//--------------------------------------------------------------------//
	//--------------------------------------------------------------------//

	// The live value struct itself is also saved as a live value by ExtractCodeRegion...
	// This leads to a bad struct layout (continuation expects struct without it)
	// -> change type
	Argument* nextContLiveValStructPtr = --(currentFn->arg_end());
	removeLiveValStructFromExtractedFnArgType(continuation, nextContLiveValStructPtr, context, barrier);

	// get call to new fun
	assert (continuation->getNumUses() == 1);
	CallInst* secCall = cast<CallInst>(continuation->use_back());

	//--------------------------------------------------------------------//
	// erase branch to block with old return statement
	//--------------------------------------------------------------------//
	secCall->getParent()->getTerminator()->eraseFromParent();

	//--------------------------------------------------------------------//
	// instead, create return that returns the id for the next call
	//--------------------------------------------------------------------//
	ReturnInst::Create(context, ConstantInt::get(getReturnType(context), barrierIndex, true), secCall);

	//--------------------------------------------------------------------//
	// erase call (new function is actually not used, it should already exist :p)
	// and arg struct (last use was the call)
	//--------------------------------------------------------------------//
	assert (secCall->getNumOperands() == 2);
	assert (isa<AllocaInst>(secCall->getArgOperand(0)));
	AllocaInst* argStruct = cast<AllocaInst>(secCall->getArgOperand(0));
	secCall->eraseFromParent();
	assert (argStruct->use_empty());
	argStruct->eraseFromParent();

	//--------------------------------------------------------------------//
	// erase barrier
	//--------------------------------------------------------------------//
	assert (barrier->use_empty() && "barriers must not have any uses!");
	barrier->eraseFromParent();

	//DEBUG_LA( outs() << "continuation:\n" << *continuation << "\n"; );
	//DEBUG_LA( outs() << "source function:\n" << *currentFn << "\n"; );
	return continuation;
}
#endif

void ContinuationGenerator::getContinuationBlocks(const CallInst* barrier, SetVector<const BasicBlock*>& continuationRegion) {
	assert (barrier && continuationRegion.empty());
	const BasicBlock* parentBlock = barrier->getParent();
	assert (parentBlock);
	//--------------------------------------------------------------------//
	// Find all blocks 'below' parentBlock inside the new function (DFS)
	//--------------------------------------------------------------------//
	assert (parentBlock->getTerminator()->getNumSuccessors() == 1 && "barrier-parentblocks with more than one successor are currently not supported!");
	std::set<const BasicBlock*> visitedBlocks;
	findContinuationBlocksDFS(parentBlock->getTerminator()->getSuccessor(0), continuationRegion, visitedBlocks);
}
void ContinuationGenerator::getLiveValuesForRegion(SetVector<const BasicBlock*>& continuationRegion, DominatorTree& domTree, LiveSetTypeConst& liveInValues, LiveSetTypeConst& liveOutValues) {
	//--------------------------------------------------------------------//
	// Collect live values to store at barrier/load in new function
	//--------------------------------------------------------------------//
#if 0
	/// From: llvm-2.8svn/lib/Transforms/Utils/CodeExtractor.cpp:findInputsOutputs()
	// IMPORTANT: This does not find any phis that have inputs that are live across loop boundaries
	for (SetVector<const BasicBlock*>::const_iterator ci = continuationRegion.begin(), ce = continuationRegion.end(); ci != ce; ++ci) {
		const BasicBlock *BB = *ci;

		for (BasicBlock::const_iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
			// If a used value is defined outside the region, it's an input.  If an
			// instruction is used outside the region, it's an output.
			for (User::const_op_iterator O = I->op_begin(), E = I->op_end(); O != E; ++O) {
				if (definedOutsideRegion(*O, continuationRegion)) liveInValues.insert(*O);
				else if (const Instruction* opI = dyn_cast<Instruction>(O)) {
					// Check if the operand does not dominate I after the splitting.
					// If so, consider it live-in (live across loop boundaries).
					// Ignore the operands if they belong to a phi
					if (!isa<PHINode>(I) && !domTree.dominates(opI, I)) liveInValues.insert(*O);
				}
			}

			// Consider uses of this instruction (outputs).
			for (Value::const_use_iterator UI = I->use_begin(), E = I->use_end(); UI != E; ++UI) {
				if (!definedInRegion(*UI, continuationRegion)) {
					liveOutValues.insert(I);
					break;
				}
			}

		} // for: insts
	} // for: basic blocks
#else
	LivenessAnalyzer::LiveSetType* liveInSet = livenessAnalyzer->getBlockLiveInValues(continuationRegion[0]);
	for (LivenessAnalyzer::LiveSetType::const_iterator it=liveInSet->begin(), E=liveInSet->end(); it!=E; ++it) {
		liveInValues.insert(*it);
	}
#endif
}
const StructType* ContinuationGenerator::computeLiveValueStructType(LiveSetTypeConst& liveInValues, LLVMContext& context) {
	//--------------------------------------------------------------------//
	// Create live-value struct-type from live-in values of newBlock
	//--------------------------------------------------------------------//
	std::vector<const Type*> params;
	for (LiveSetTypeConst::const_iterator it=liveInValues.begin(), E=liveInValues.end(); it!=E; ++it) {
		params.push_back((*it)->getType());
	}
	const bool isPacked = false; // TODO: true ???
	StructType* sType = StructType::get(context, params, isPacked);

	return sType;
}


void ContinuationGenerator::createLiveValueStoresForBarrier(BarrierInfo* barrierInfo, Function* f, ValueToValueMapTy& origFnValueMap, LLVMContext& context) {
	assert (barrierInfo && f);
	assert (origFnValueMap.count(barrierInfo->barrier));

	const CallInst* origBarrier = barrierInfo->barrier;
	CallInst* barrier = cast<CallInst>(origFnValueMap[origBarrier]);

	Argument* nextContLiveValStructPtr = --(f->arg_end());
	LiveSetTypeConst& liveInValues = *barrierInfo->liveInValues;
	const StructType* liveValueStructTypePadded = barrierInfo->liveValueStructTypePadded;
	assert (liveValueStructTypePadded);
	//--------------------------------------------------------------------//
	// Generate stores of the live values before the return where the barrier
	// was (in the old function).
	// After this, the modification of the old function is finished.
	//--------------------------------------------------------------------//

	// bitcast data pointer to correct struct type for GEP below
	// NOTE: Here we have to use the padded type to get the correct layout.
	BitCastInst* bc = new BitCastInst(nextContLiveValStructPtr, PointerType::getUnqual(liveValueStructTypePadded), "", barrier);

	// generate the gep/store combinations for each live value
	unsigned i=0;
	for (LiveSetTypeConst::const_iterator it=liveInValues.begin(), E=liveInValues.end(); it!=E; ++it) {
		DEBUG_LA( outs() << "      generating store for live value: " << **it << "\n"; );
		const Value* origLiveVal = *it;
		assert (origFnValueMap.count(origLiveVal));
		Value* liveVal = origFnValueMap[origLiveVal];

		std::vector<Value*> indices;
		indices.push_back(ConstantInt::getNullValue(Type::getInt32Ty(context)));
		indices.push_back(ConstantInt::get(context, APInt(32, i++)));
		GetElementPtrInst* gep = GetElementPtrInst::Create(bc, indices.begin(), indices.end(), "", barrier);
		DEBUG_LA( outs() << "        store gep(" << i-1 << "): " << *gep << "\n"; );
		const unsigned align = 16;
		new StoreInst(liveVal, gep, false, align, barrier);
	}
}

Function* ContinuationGenerator::createContinuationNEW2(BarrierInfo* barrierInfo, const std::string& continuationName, Module* mod) {
	assert (barrierInfo && mod && continuationName != "");
	LLVMContext& context = mod->getContext();

	//const CallInst* barrier = barrierInfo->barrier; // not required -> DO NOT USE (function is called with barrier == NULL for start continuation)
	//const unsigned barrierIndex = barrierInfo->id; // not required
	SetVector<const BasicBlock*>& continuationRegion = *barrierInfo->continuationRegion;
	LiveSetTypeConst& liveInValues = *barrierInfo->liveInValues;
	const StructType* liveValueStructType = barrierInfo->liveValueStructType;

	const Function* origFn = barrierInfo->continuationEntryBlock->getParent();

	assert (barrierInfo->continuation == NULL && "must not generate the same continuation twice!");

	//--------------------------------------------------------------------//
	// create new function (the continuation) with the following signature:
	// - returns int (id of next continuation)
	// - special parameters (get_global_id, get_local_size, num_groups)
	// - live-in value parameters OR origFn parameters (for start continuation, stored in live values/struct type)
	// - last parameter: void* nextContLiveVals (union where live values for next
	//                   continuation are stored before returning)
	//
	// = create new function with new signature and clone all blocks
	// The former return statements now all return -1 (special end id)
	//--------------------------------------------------------------------//
	std::vector<const Type*> params;
	params.insert(params.end(), specialParams.begin(), specialParams.end());   // special parameters
	params.insert(params.end(), liveValueStructType->element_begin(), liveValueStructType->element_end()); // live values
	params.push_back(Type::getInt8PtrTy(context)); // data ptr

	FunctionType* continuationType = FunctionType::get(getReturnType(context), params, false);

	Function* continuation = Function::Create(continuationType, Function::ExternalLinkage, continuationName, mod); // TODO: check linkage type

	Function::arg_iterator CA = continuation->arg_begin();
	for (unsigned i=0, e=specialParamNames.size(); i<e; ++i, ++CA) {
		CA->setName(specialParamNames[i]);
	}
	DEBUG_LA( outs() << "\n    new continuation declaration: " << *continuation << "\n"; );


	//--------------------------------------------------------------------//
	// Create mappings of live-in values to arguments for copying of blocks
	//--------------------------------------------------------------------//
	// We need a second value map only for the arguments because valueMap
	// is given to CloneFunctionInto, which may overwrite these.
	ValueToValueMapTy* valueMap = new ValueToValueMapTy();
	DenseMap<const Value*, Value*> liveValueToArgMap;
	// CA has to point to first live value parameter of continuation
	CA = --continuation->arg_end();
	//std::advance(CA, 0-liveValueStructType->getNumContainedTypes());
	for (unsigned i=0, e=liveValueStructType->getNumContainedTypes(); i<e; ++i) {
		--CA;
	}
	DEBUG_LA( outs() << "    live value mappings:\n"; );
	for (LiveSetTypeConst::const_iterator it=liveInValues.begin(), E=liveInValues.end(); it!=E; ++it, ++CA) {
		const Value* liveVal = *it;
		(*valueMap)[liveVal] = CA;
		liveValueToArgMap[liveVal] = CA;
		DEBUG_LA( outs() << "     * " << *liveVal << " -> " << *CA << "\n"; );
	}


	//--------------------------------------------------------------------//
	// HACK: Copy over entire function and remove all unnecessary blocks.
	//       This is required because CloneBasicBlock does not perform any
	//       remapping. RemapInstructions thus has to be called by hand and
	//       is not available via includes (as of llvm-2.8svn ~May/June 2010 :p).
	//--------------------------------------------------------------------//

	// Therefore, we need to have dummy-mappings for all arguments of the
	// old function that do not map to a live value.
	// TODO: erase dummy values again from value map at some point?
	for (Function::const_arg_iterator A=origFn->arg_begin(), AE=origFn->arg_end(); A!=AE; ++A) {
		if ((*valueMap).find(A) != (*valueMap).end()) continue;
		Value* dummy = UndefValue::get(A->getType());
		(*valueMap)[A] = dummy;
	}

	SmallVector<ReturnInst*, 2> returns;
	CloneFunctionInto(continuation, origFn, (*valueMap), false, returns, ".");


	// Replace return instructions by 'return -1'
	for (unsigned i=0; i<returns.size(); ++i) {
		BasicBlock* retBlock = returns[i]->getParent();
		returns[i]->eraseFromParent();
		ReturnInst::Create(context, ConstantInt::get(getReturnType(context), WFVOPENCL_BARRIER_SPECIAL_END_ID, true), retBlock);
	}

	// Replace branches behind barriers by corresponding return ids
	// (capsulate continuation region)
	// NOTE: must not erase barriers yet (still required for generation of of live value stores)
	DEBUG_LA( outs() << "\n    replacing branches that leave continuation region after barriers by return with barrier id!\n"; );
	for (SetVector<const BasicBlock*>::const_iterator BB=continuationRegion.begin(), BBE=continuationRegion.end(); BB!=BBE; ++BB) {
		const BasicBlock* blockOrig = *BB;
		assert ((*valueMap).find(blockOrig) != (*valueMap).end());
		BasicBlock* blockCopy = cast<BasicBlock>((*valueMap)[blockOrig]);

		if (!blockEndsWithBarrierBeforeTerminator(blockOrig)) continue;

		const Instruction* secondLastInst = --(--blockOrig->end()); // last inst = terminator

		// erase old branch and create return
		blockCopy->getTerminator()->eraseFromParent();
		const unsigned barrierIndex = barrierIndices[cast<CallInst>(secondLastInst)];
		ReturnInst::Create(context, ConstantInt::get(getReturnType(context), barrierIndex, true), blockCopy);

		DEBUG_LA( outs() << "      replaced branch behind block '" << blockCopy->getNameStr() << "'!\n"; );
	}


	//--------------------------------------------------------------------//
	// remap values that were no arguments
	// NOTE: apparently, CloneFunctionInto does not look into the valueMap
	//       if it directly finds all references. Due to the fact that we
	//       copy the entire function, live values that are no arguments
	//       are still resolved by their former definitions which only get
	//       erased in the next step.
	// NOTE: actually, this is good behaviour - we must not remap values
	//       that are defined in a block that also appears in the
	//       continuation, e.g. in loops.
	//--------------------------------------------------------------------//

	//--------------------------------------------------------------------//
	// erase all blocks that do not belong to this continuation
	//--------------------------------------------------------------------//
	DEBUG_LA( outs() << "\n    removing blocks from continuation that do not belong to continuation region...\n"; );
	BasicBlock* dummyBB = BasicBlock::Create(context, "dummy", continuation);
	// iterate over blocks of original fun, but work on blocks of continuation fun
	// -> can't find out block in old fun for given block in new fun via map ;)
	for (Function::const_iterator BB=origFn->begin(), BBE=origFn->end(); BB!=BBE; ) {
		assert ((*valueMap).find(BB) != (*valueMap).end());
		const BasicBlock* blockOrig = BB++;
		BasicBlock* blockCopy = cast<BasicBlock>((*valueMap)[blockOrig]);
		if (continuationRegion.count(blockOrig)) continue; // current block is copied

		// current block must not be "copied" -> delete it
		DEBUG_LA( outs() << "      block can be removed from continuation: " << blockCopy->getNameStr() << "!\n"; );

		// but first, replace all uses of instructions of block by dummies
		// or arguments in case of live values...
		BasicBlock::const_iterator IO=blockOrig->begin(); // mapping uses values from old function...
		for (BasicBlock::iterator I=blockCopy->begin(), IE=blockCopy->end(); I!=IE; ++I, ++IO) {
			DEBUG_LA( outs() << "        replacing uses of inst: " << *I << "\n"; );
			if (liveInValues.count(IO)) {
				DEBUG_LA( outs() << "          is a live value, replaced with argument: " << *liveValueToArgMap[IO] << "\n"; );
				I->replaceAllUsesWith(liveValueToArgMap[IO]);
				// also replace instruction in value map
				(*valueMap).erase(IO);
				(*valueMap)[IO] = liveValueToArgMap[IO];
			} else {
				I->replaceAllUsesWith(UndefValue::get(I->getType()));
				// erase instruction from value map (pointer will be invalid after deletion)
				(*valueMap).erase(IO);
			}
		}

		// erase block from value map (pointer will be invalid after deletion)
		(*valueMap).erase(blockOrig);

		// remove all incoming values from this block to phis
		DEBUG_LA( outs() << "        removing incoming values to phis from block '" << blockCopy->getNameStr() << "'...\n"; );
		for (BasicBlock::use_iterator U=blockCopy->use_begin(), UE=blockCopy->use_end(); U!=UE; ++U) {
			if (!isa<PHINode>(*U)) continue;
			PHINode* phi = cast<PHINode>(*U);
			if (phi->getBasicBlockIndex(blockCopy) != -1) {
				DEBUG_LA( outs() << "          removing incoming value from phi: " << *phi << "\n"; );
				phi->removeIncomingValue(blockCopy, false);
			}
		}

		blockCopy->replaceAllUsesWith(dummyBB);
		blockCopy->eraseFromParent();
	}

	// erase dummy block
	assert (dummyBB->use_empty());
	dummyBB->eraseFromParent();

	//--------------------------------------------------------------------//
	// if necessary, make new block the entry block of the continuation
	// (= move to front of block list)
	//--------------------------------------------------------------------//
	const BasicBlock* continuationEntryBlock = barrierInfo->continuationEntryBlock; // TODO: are we sure that block is always added first?
	BasicBlock* newEntryBlock = cast<BasicBlock>((*valueMap)[continuationEntryBlock]);
	if (newEntryBlock != &continuation->getEntryBlock()) {
		newEntryBlock->moveBefore(&continuation->getEntryBlock());
		DEBUG_LA( outs() << "\n    moved new entry block '" << newEntryBlock->getNameStr() << "' to begin of function!\n"; );
	}


	//--------------------------------------------------------------------//
	// Make sure all uses of live values that are not dominated anymore are
	// rewired to arguments.
	// NOTE: This can't be done before blocks are deleted ;).
	//--------------------------------------------------------------------//

	// compute dominator tree of continuation
	DominatorTree* continuationDomTree = new DominatorTree();
	continuationDomTree->runOnFunction(*continuation);

	DEBUG_LA( outs() << "\n    rewiring live values that are not properly dominated after splitting...\n"; );

	// set arg_iterator to first live value arg (first args are special values)
	Function::arg_iterator A2 = continuation->arg_begin();
	std::advance(A2, specialParams.size());
	for (LiveSetTypeConst::const_iterator it=liveInValues.begin(), E=liveInValues.end(); it!=E; ++it, ++A2) {
		const Value* liveVal = *it;
		DEBUG_LA( outs() << "      testing live value: " << *liveVal << "\n"; );
		if (isa<Argument>(liveVal)) continue;

		// if all uses were already replaced above, skip this value
		// (conditions map to the two cases where uses of instructions are replaced above)
		if ((*valueMap).find(liveVal) == (*valueMap).end() || (*valueMap)[liveVal] == A2) {
			DEBUG_LA( outs() << "      all uses already replaced!\n"; );
			continue;
		}

		Value* newLiveVal = (*valueMap)[liveVal];
		DEBUG_LA( outs() << "    new live value: " << *newLiveVal << "\n"; );

		// if the value is defined in one of the copied blocks, we must only
		// replace those uses that are not dominated by their definition anymore
		if (const Instruction* inst = dyn_cast<Instruction>(liveVal)) {
			if (continuationRegion.count(inst->getParent())) {
				Instruction* newInst = cast<Instruction>(newLiveVal);
				DEBUG_LA( outs() << "    live value is defined in copied block: " << inst->getParent()->getNameStr() << "\n"; );
				for (Instruction::use_iterator U=newInst->use_begin(), UE=newInst->use_end(); U!=UE; ) {
					assert (isa<Instruction>(*U));
					Instruction* useI = cast<Instruction>(*U++);
					DEBUG_LA( outs() << "      testing use: " << *useI << "\n"; );
					if (continuationDomTree->dominates(newInst, useI)) continue;
					DEBUG_LA( outs() << "        is not dominated, will be replaced by argument: " << *A2 << "\n"; );
					useI->replaceUsesOfWith(newInst, A2);
				}
				continue;
			}
		}

		newLiveVal->replaceAllUsesWith(A2);
	}

	DEBUG_LA( outs() << *continuation << "\n"; );
	DEBUG_LA( outs() << "continuation '" << continuation->getNameStr() << "' generated successfully!\n\n"; );

	DEBUG_LA(
		outs() << "verifying function '" << origFn->getNameStr() << "'... ";
		verifyFunction(*origFn);
		outs() << "done.\nverifying function '" << continuation->getNameStr() << "'... ";
		verifyFunction(*continuation);
		outs() << "done.\n";
	);

	barrierInfo->continuation = continuation;
	barrierInfo->origFnValueMap = valueMap;
	return continuation;
}

void ContinuationGenerator::storeBarrierMapping(Function* f, ValueMap<const Value*, Value*>& valueMap) {
	assert (f);
	// we have a "forward" mapping oldFn -> newFn, so we have to loop over
	// all barriers of the old function, check if the barrier also exists
	// in the new one, and insert it if it does exist.
	outs() << "\nmapping barriers...\n";
	for (BarrierIndicesMapType::iterator it=barrierIndices.begin(), E=barrierIndices.end(); it!=E; ++it) {
		unsigned index = it->second;
		const CallInst* barrierOrig = it->first;
		assert (barrierOrig);
		outs() << " (" << index << ") " << *barrierOrig;

		if (!valueMap.count(barrierOrig)) {
			outs() << " NOT FOUND IN VALUEMAP!\n";
			continue;
		}

		assert (isa<CallInst>(valueMap[barrierOrig]));
		CallInst* barrierNew = cast<CallInst>(valueMap[barrierOrig]);
		outs() << "  -> " << *barrierNew << "\n";
		barrierIndices[barrierNew] = index;
	}
}

Function* ContinuationGenerator::eliminateBarriers(Function* origFn, BarrierVecType& barriers, const unsigned numBarriers, const SmallVector<unsigned, 4>& barrierDepths, const unsigned maxBarrierDepth, SpecialParamVecType& specialParams, SpecialParamNameVecType& specialParamNames, TargetData* targetData) {
	assert (origFn && targetData && origFn->getParent());
	assert (origFn->getReturnType()->isVoidTy());
	Module* mod = origFn->getParent();
	LLVMContext& context = mod->getContext();

	DEBUG_LA( outs() << "\neliminateBarriers(" << origFn->getNameStr() << ")\n"; );

	DominatorTree* domTree = new DominatorTree();
	domTree->runOnFunction(*origFn);

	DEBUG_LA( outs() << *origFn << "\n"; );

	//--------------------------------------------------------------------//
	// Generate order in which barriers should be replaced:
	// Barriers with highest depth come first, barriers with same depth
	// are ordered nondeterministically unless they live in the same block,
	// in which case their order is determined by their dominance relation.
	//--------------------------------------------------------------------//
	SmallVector<BarrierInfo*, 4> orderedBarriers; // TODO: required??

	// Store all information about each barrier in one struct
	// - id
	// - depth
	// - callsite
	// - region of continuation
	// - live values of that region
	// - corresponding live value struct type
	DEBUG_LA( outs() << "\ncollecting information about barriers...\n"; );

	//
	// add 'start-continuation' (which is not related to any barrier)
	// TODO: remove duplicate code (pull into loop below)
	//
	{
		const CallInst* barrier = NULL;
		const unsigned barrierIndex = WFVOPENCL_BARRIER_SPECIAL_START_ID;
		DEBUG_LA( outs() << "\n  start function with index " << barrierIndex << "\n"; );
		const unsigned depth = 0;
		SetVector<const BasicBlock*>* continuationRegion = new SetVector<const BasicBlock*>();
		std::set<const BasicBlock*> visitedBlocks;
		findContinuationBlocksDFS(&origFn->getEntryBlock(), *continuationRegion, visitedBlocks);
		DEBUG_LA(
			outs() << "\n    blocks that need to be cloned into the start-continuation:\n";
			for (SetVector<const BasicBlock*>::const_iterator it=continuationRegion->begin(), E=continuationRegion->end(); it!=E; ++it) {
				outs() << "     * " << (*it)->getNameStr() << "\n";
			}
		);
		// add arguments to live in values (required to handle start continuation like any other)
		// create struct type with types of arguments
		LiveSetTypeConst* liveInValues = new LiveSetTypeConst();
		std::vector<const Type*> params;
		for (Function::const_arg_iterator A=origFn->arg_begin(), AE=origFn->arg_end(); A!=AE; ++A) {
			liveInValues->insert(A);
			params.push_back(A->getType());
		}
		const bool isPacked = false; // TODO: true?
		const StructType* liveValStructType = StructType::get(context, params, isPacked);

		BarrierInfo* binfo = new BarrierInfo(barrierIndex, depth, barrier, &origFn->getEntryBlock(), continuationRegion, liveInValues, liveValStructType);
		orderedBarriers.push_back(binfo); // IMPORTANT: we do not want to generate a normal continuation for this! :)

		barrierIndices[binfo->barrier] = barrierIndex; // save barrier -> id mapping
		continuationMap[barrierIndex] = binfo;

		DEBUG_LA( outs() << "\n  added start function with index " << barrierIndex << "!\n"; );
	}


	// 0 is reserved for 'start'-function, so the last index is numBarriers and 0 is not used
	unsigned barrierIndex = 1;
	SmallVector<unsigned, 4>::const_iterator it2 = barrierDepths.begin();
	for (BarrierVecType::iterator it=barriers.begin(), E=barriers.end(); it!=E; ++it, ++it2, ++barrierIndex) {
		CallInst* barrier = *it;
		assert (barrier);
		DEBUG_LA( outs() << "\n  barrier " << barrierIndex << " (parent: " << barrier->getParent()->getNameStr() << "): " << *barrier << "\n"; );

		//----------------------------------------------------------------//
		// get depth
		//----------------------------------------------------------------//
		const unsigned depth = *it2;
		DEBUG_LA( outs() << "\n    depth: " << depth << "\n"; );

		//----------------------------------------------------------------//
		// find basic block region for continuation
		//----------------------------------------------------------------//
		SetVector<const BasicBlock*>* continuationRegion = new SetVector<const BasicBlock*>();
		getContinuationBlocks(barrier, *continuationRegion);

		DEBUG_LA(
			outs() << "\n    blocks that need to be cloned into the continuation:\n";
			for (SetVector<const BasicBlock*>::const_iterator it=continuationRegion->begin(), E=continuationRegion->end(); it!=E; ++it) {
				outs() << "     * " << (*it)->getNameStr() << "\n";
			}
		);

		//----------------------------------------------------------------//
		// find live in values of continuation region
		//----------------------------------------------------------------//
		LiveSetTypeConst* liveInValues = new LiveSetTypeConst();
		LiveSetTypeConst* liveOutValues = new LiveSetTypeConst();
		getLiveValuesForRegion(*continuationRegion, *domTree, *liveInValues, *liveOutValues);

		DEBUG_LA(
			outs() << "\n    live-in values of continuation region:\n";
			for (LiveSetTypeConst::iterator it=liveInValues->begin(), E=liveInValues->end(); it!=E; ++it) {
				outs() << "     * " << **it << "\n";
			}
		);

		//----------------------------------------------------------------//
		// create struct type for live values
		//----------------------------------------------------------------//
		const StructType* liveValStructType = computeLiveValueStructType(*liveInValues, context);

		DEBUG_LA(
			outs() << "\n    new struct type   : " << *liveValStructType << "\n";
			outs() << "    type size in bits : " << targetData->getTypeSizeInBits(liveValStructType) << "\n";
			outs() << "    alloc size in bits: " << targetData->getTypeAllocSizeInBits(liveValStructType) << "\n";
			outs() << "    alloc size        : " << targetData->getTypeAllocSize(liveValStructType) << "\n";
			outs() << "    store size in bits: " << targetData->getTypeStoreSizeInBits(liveValStructType) << "\n";
			outs() << "    store size        : " << targetData->getTypeStoreSize(liveValStructType) << "\n";
		);

		//----------------------------------------------------------------//
		// store all information about this barrier
		//----------------------------------------------------------------//
		BarrierInfo* binfo = new BarrierInfo(barrierIndex, depth, barrier, (*continuationRegion)[0], continuationRegion, liveInValues, liveValStructType);
		orderedBarriers.push_back(binfo);

		barrierIndices[binfo->barrier] = barrierIndex; // save barrier -> id mapping
		continuationMap[barrierIndex] = binfo;

		DEBUG_LA( outs() << "\n  added barrier " << barrierIndex << " (parent: " << barrier->getParent()->getNameStr() << "): " << *barrier << "\n"; );
	}

	DEBUG_LA( outs() << "\nsuccessfully collected information about all barriers!\n"; );


	//--------------------------------------------------------------------//
	// Create a continuation for each barrier of the original function
	//--------------------------------------------------------------------//
	DEBUG_LA( outs() << "\ngenerating continuation functions...\n"; );
	const unsigned numContinuationFunctions = numBarriers+1;
	const std::string& functionName = origFn->getNameStr();

	for (SmallVector<BarrierInfo*, 4>::iterator it=orderedBarriers.begin(), E=orderedBarriers.end(); it!=E; ++it) {
		BarrierInfo* barrierInfo = *it;

		// special case for start continuation (barrier is NULL)
		if (it == orderedBarriers.begin()) {
			assert (barrierInfo->id == 0 && barrierInfo->barrier == NULL);
			createContinuationNEW2(barrierInfo, functionName+"_begin", mod);
			continue;
		}

		const CallInst* barrier = barrierInfo->barrier;

		assert (barrierIndices.find(barrier) != barrierIndices.end() && "forgot to map barrier index of barriers in continuation?");
		const unsigned barrierIndex = barrierIndices[barrier];

		std::stringstream sstr;
		sstr << functionName << "_cont_" << barrierIndex;

		DEBUG_LA( outs() << "\n  generating continuation for barrier " << barrierIndex << " (parent: " << barrier->getParent()->getNameStr() << ")...\n"; );

		createContinuationNEW2(barrierInfo, sstr.str(), mod);
	}

	DEBUG_LA( outs() << "\ngeneration of continuation functions finished!\n"; );
	assert (continuationMap.size() == numContinuationFunctions);




	//--------------------------------------------------------------------//
	// determine the size of each union of the live value struct
	// in order to generate struct types of equal size for valid padding
	// NOTE: This has to be done AFTER creation of continuations (we do not
	//       want to have the padding elements be part of the signature)
	// NOTE: This has to be done BEFORE generating stores (we need to have
	//       correct bitcasts)
	//--------------------------------------------------------------------//
	DEBUG_LA( outs() << "\ndetermining size of each live value struct and union size...\n"; );
	uint64_t unionMaxSize = 0;
	for (ContinuationMapType::iterator it=continuationMap.begin(), E=continuationMap.end(); it!=E; ++it) {
		BarrierInfo* binfo = it->second;
		const uint64_t size = targetData->getTypeAllocSize(binfo->liveValueStructType);
		if (size > unionMaxSize) unionMaxSize = size;
	}

	DEBUG_LA( outs() << "  max alloc-size: " << unionMaxSize << "\n"; );

	// adjust structs of all continuations whose live value struct is smaller than maxsize
	for (ContinuationMapType::iterator it=continuationMap.begin(), E=continuationMap.end(); it!=E; ++it) {
		BarrierInfo* binfo = it->second;
		const StructType* liveValueStructType = binfo->liveValueStructType;
		const uint64_t size = targetData->getTypeSizeInBits(liveValueStructType) / 8;
		DEBUG_LA( outs() << "  exact size of struct for continuation " << binfo->id << ": " << size << "\n"; );
		if (size == unionMaxSize) {
			binfo->liveValueStructTypePadded = binfo->liveValueStructType; // no need to pad, size matches
			continue;
		}
		assert (size < unionMaxSize);
		DEBUG_LA( outs() << "    does not match max alloc size (" << unionMaxSize << "), creating padding of size " << unionMaxSize-size << "...\n"; );

		std::vector<const Type*> params;
		params.insert(params.end(), liveValueStructType->element_begin(), liveValueStructType->element_end());
		params.push_back(ArrayType::get(Type::getInt8Ty(context), unionMaxSize-size));

		const bool isPacked = false; // TODO: true?
		binfo->liveValueStructTypePadded = StructType::get(context, params, isPacked);
	}



	//-------------
	// For each barrier, transform all remaining calls in all continuations
	// into returns of the barrierIndex and the corresponding stores
	//-------------
	DEBUG_LA( outs() << "\nreplacing barriers by live value store instructions and returns...\n"; );
	for (Function::const_iterator BB=origFn->begin(), BBE=origFn->end(); BB!=BBE; ++BB) {
		if (!blockEndsWithBarrierBeforeTerminator(BB)) continue;

		const CallInst* origBarrier = cast<CallInst>(--(--BB->end())); // last inst = terminator
		assert (barrierIndices.find(origBarrier) != barrierIndices.end() && "forgot to map barrier index of barriers in continuation?");
		const unsigned barrierIndex = barrierIndices[origBarrier];
		BarrierInfo* barrierInfo = continuationMap[barrierIndex];

		DEBUG_LA( outs() << "\n  replacing barrier " << barrierIndex << " (parent: " << origBarrier->getParent()->getNameStr() << ")...\n"; );

		// Now iterate over all continuations and see if this barrier still
		// exists. If yes, replace it with corresponding stores and return.
		for (ContinuationMapType::iterator it=continuationMap.begin(), E=continuationMap.end(); it!=E; ++it) {
			// get barrier info of this continuation
			BarrierInfo* contInfo = it->second;
			// get value mapping from origFn to this continuation
			ValueToValueMapTy& origFnValueMap = *contInfo->origFnValueMap;

			// if origBarrier does not exist in valuemap, we don't have to do anything
			if (!origFnValueMap.count(origBarrier)) continue;

			DEBUG_LA( outs() << "\n    barrier still exists in continuation '" << contInfo->continuation->getNameStr() << "'!\n"; );

			Function* f = contInfo->continuation;
			createLiveValueStoresForBarrier(barrierInfo, f, origFnValueMap, context);

			// erase barrier from new function
			CallInst* barrier = cast<CallInst>(origFnValueMap[origBarrier]);
			barrier->eraseFromParent();

			DEBUG_LA( outs() << "\n    live value store generation for barrier " << barrierIndex << " in continuation '" << contInfo->continuation->getNameStr() << "' finished.\n"; );

			DEBUG_LA( verifyFunction(*f); );
		}
	}
	DEBUG_LA( outs() << "\nlive value store generation finished!\n"; );

	//--------------------------------------------------------------------//
	// Check if all barriers in all functions (original and continuations) were eliminated.
	//--------------------------------------------------------------------//
	DEBUG_LA(
		outs() << "\nTesting for remaining barriers in continuations... ";
		bool err = false;
		for (ContinuationMapType::iterator it=continuationMap.begin(), E=continuationMap.end(); it!=E; ++it) {
			BarrierInfo* binfo = it->second;
			Function* continuation = binfo->continuation;
			assert (continuation);
			assert (binfo->liveValueStructType);

			for (Function::iterator BB=continuation->begin(), BBE=continuation->end(); BB!=BBE; ++BB) {
				for (BasicBlock::iterator I=BB->begin(), IE=BB->end(); I!=IE; ++I) {
					if (!isa<CallInst>(I)) continue;
					if (isBarrier(I)) {
						errs() << "\nERROR: barrier not eliminated in continuation '" << continuation->getNameStr() << "': " << *I << "\n";
						err = true;
					}
				}
			}
		}
		if (!err) outs() << "done.\n";
		else {
			outs() << "\n";
			assert (false && "there were barriers left in continuations! (bad ordering?)");
		}
	);


	DEBUG_LA( outs() << "\ncontinuation generation finished!\n"; );

	DEBUG_LA(
		outs() << "\ncontinuations:\n";
		for (ContinuationMapType::const_iterator it=continuationMap.begin(), E=continuationMap.end(); it!=E; ++it) {
			const BarrierInfo* barrierInfo = it->second;
			barrierInfo->print(outs());
			outs() << "\n";
		}
		outs() << "\n";
	);

	return createWrapper(origFn, continuationMap, mod, targetData);
}

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
Function* ContinuationGenerator::createWrapper(Function* origFunction, ContinuationMapType& cMap, Module* mod, TargetData* targetData) {
	const unsigned numContinuationFunctions = cMap.size();
	const FunctionType* fTypeOld = origFunction->getFunctionType();
	const std::string& functionName = origFunction->getNameStr();

	LLVMContext& context = mod->getContext();

	Function* wrapper = Function::Create(fTypeOld, Function::ExternalLinkage, functionName+"_barrierswitch", mod); // TODO: check linkage type

	IRBuilder<> builder(context);

	// create entry block
	BasicBlock* entryBB = BasicBlock::Create(context, "entry", wrapper);

	// create blocks for while loop
	BasicBlock* headerBB = BasicBlock::Create(context, "while.header", wrapper);
	BasicBlock* latchBB = BasicBlock::Create(context, "while.latch", wrapper);

	// create call blocks (switch targets)
	BasicBlock** callBBs = new BasicBlock*[numContinuationFunctions]();
	for (unsigned i=0; i<numContinuationFunctions; ++i) {
		std::stringstream sstr;
		sstr << "switch." << i;  // "0123456789ABCDEF"[x] would be okay if we could guarantee a max size for continuations :p
		callBBs[i] = BasicBlock::Create(context, sstr.str(), wrapper);
	}

	// create exit block
	BasicBlock* exitBB = BasicBlock::Create(context, "exit", wrapper);


	//--------------------------------------------------------------------//
	// fill entry block
	//--------------------------------------------------------------------//
	builder.SetInsertPoint(entryBB);

	// generate union for live value structs
	assert (cMap[0] && cMap[0]->liveValueStructTypePadded);
	const uint64_t unionSize = targetData->getTypeAllocSize(cMap[0]->liveValueStructTypePadded);
	DEBUG_LA( outs() << "union size for live value structs: " << unionSize << " bytes\n"; );
	DEBUG_LA(
		unsigned testUnionSize = unionSize;
		for (ContinuationMapType::iterator it=cMap.begin(), E=cMap.end(); it!=E; ++it) {
			BarrierInfo* bit = it->second;
			const StructType* liveValueStructTypePadded = bit->liveValueStructTypePadded;
			assert (liveValueStructTypePadded);
			const uint64_t typeSize = targetData->getTypeSizeInBits(liveValueStructTypePadded) / 8;
			outs() << "padded type: " << *liveValueStructTypePadded << "\n";
			outs() << "type size: " << typeSize << "\n";
			assert (testUnionSize == typeSize && "type sizes for union have to match exactly!");
		}
	);
	// allocate memory for union
	Value* allocSize = ConstantInt::get(context, APInt(32, unionSize));
	AllocaInst* dataPtr = builder.CreateAlloca(Type::getInt8Ty(context), allocSize, "liveValueUnion");
	dataPtr->setAlignment(16);

	builder.CreateBr(headerBB);

	//--------------------------------------------------------------------//
	// fill header
	//--------------------------------------------------------------------//
	builder.SetInsertPoint(headerBB);
	PHINode* current_barrier_id_phi = builder.CreatePHI(Type::getInt32Ty(context), "current_barrier_id");
	current_barrier_id_phi->addIncoming(ConstantInt::getNullValue(Type::getInt32Ty(context)), entryBB);

	SwitchInst* switchI = builder.CreateSwitch(current_barrier_id_phi, exitBB, numContinuationFunctions);
	for (unsigned i=0; i<numContinuationFunctions; ++i) {
		// add case for each continuation
		switchI->addCase(ConstantInt::get(context, APInt(32, i)), callBBs[i]);
	}


	//--------------------------------------------------------------------//
	// fill call blocks
	//--------------------------------------------------------------------//
	// NOTE: We insert loads here also for the start continuation (which
	//       only uses function parameters). However, they can be removed
	//       easily by DCE, so we don't care.
	// NOTE: They are not erased if debug-runtime is enabled (uses in printf).
	CallInst** calls = new CallInst*[numContinuationFunctions]();
	for (unsigned i=0; i<numContinuationFunctions; ++i) {
		BasicBlock* block = callBBs[i];
		builder.SetInsertPoint(block);

		// extract arguments from live value struct (dataPtr)
		// NOTE: Here we have to use the original struct type without padding
		BarrierInfo* binfo = cMap[i];
		const StructType* sType = binfo->liveValueStructTypePadded;
		const bool hasPadding = binfo->liveValueStructType != sType;
		const unsigned numLiveVals = hasPadding ? sType->getNumElements()-1 : sType->getNumElements();
		Value** contArgs = new Value*[numLiveVals]();

		Value* bc = builder.CreateBitCast(dataPtr, PointerType::getUnqual(sType)); // cast data pointer to correct pointer to struct type

		for (unsigned j=0; j<numLiveVals; ++j) {
			std::vector<Value*> indices;
			indices.push_back(ConstantInt::getNullValue(Type::getInt32Ty(context)));
			indices.push_back(ConstantInt::get(context, APInt(32, j)));
			Value* gep = builder.CreateGEP(bc, indices.begin(), indices.end(), "");
			DEBUG_LA( outs() << "load gep(" << j << "): " << *gep << "\n"; );
			LoadInst* load = builder.CreateLoad(gep, false, "");
			load->setAlignment(16);
			load->setVolatile(false);
			contArgs[j] = load;
		}


		// create the calls to the continuations
		// the first block holds the call to the (remainder of the) original function,
		// which receives the original arguments plus the data pointer.
		// All other blocks receive the extracted live-in values plus the data pointer.
		SmallVector<Value*, 4> args;

		// special parameters (insert dummies -> have to be replaced externally after the pass is finished)
		for (SpecialParamVecType::iterator it=specialParams.begin(), E=specialParams.end(); it!=E; ++it) {
			args.push_back(UndefValue::get(*it));
		}
		// begin-fn: original parameters
		// others  : live values
		if (i == 0) {
			for (Function::arg_iterator A=wrapper->arg_begin(), AE=wrapper->arg_end(); A!=AE; ++A) {
				assert (isa<Value>(A));
				args.push_back(cast<Value>(A));
			}
		} else {
			for (unsigned j=0; j<numLiveVals; ++j) {
				args.push_back(contArgs[j]);
			}
		}
		args.push_back(dataPtr); // data ptr

		DEBUG_LA(
			outs() << "\narguments for call to continuation '" << binfo->continuation->getNameStr() << "':\n";
			for (SmallVector<Value*, 4>::iterator it=args.begin(), E=args.end(); it!=E; ++it) {
				outs() << " * " << **it << "\n";
			}
			outs() << "\n";
		);

		std::stringstream sstr;
		sstr << "continuation." << i;  // "0123456789ABCDEF"[x] would be okay if we could guarantee a max number of continuations :p
		calls[i] = builder.CreateCall(cMap[i]->continuation, args.begin(), args.end(), sstr.str());
		DEBUG_LA( outs() << "created call for continuation '" << cMap[i]->continuation->getNameStr() << "':" << *calls[i] << "\n"; );
		// TODO: set attributes etc.

		builder.CreateBr(latchBB);

		delete [] contArgs;
	}

	//--------------------------------------------------------------------//
	// fill latch
	//--------------------------------------------------------------------//
	builder.SetInsertPoint(latchBB);

	// create phi for next barrier id coming from each call inside the switch
	PHINode* next_barrier_id_phi = builder.CreatePHI(Type::getInt32Ty(context), "next_barrier_id");
	for (unsigned i=0; i<numContinuationFunctions; ++i) {
		next_barrier_id_phi->addIncoming(calls[i], callBBs[i]);
	}

	// add the phi as incoming value to the phi in the loop header
	current_barrier_id_phi->addIncoming(next_barrier_id_phi, latchBB);

	// create check whether id is the special end id ( = is negative)
	// if yes, exit the loop, otherwise go on iterating
	Value* cond = builder.CreateICmpSLT(next_barrier_id_phi, ConstantInt::getNullValue(Type::getInt32Ty(context)), "exitcond");
	builder.CreateCondBr(cond, exitBB, headerBB);

	//--------------------------------------------------------------------//
	// fill exit
	//--------------------------------------------------------------------//
	//CallInst::CreateFree(dataPtr, exitBB); //not possible with builder
	builder.SetInsertPoint(exitBB);
	builder.CreateRetVoid();


	delete [] calls;
	delete [] callBBs;

	return wrapper;
}

char ContinuationGenerator::ID = 0;

INITIALIZE_PASS_BEGIN(ContinuationGenerator, "continuation-generation", "Continuation Generation", false, false)
INITIALIZE_PASS_DEPENDENCY(CallSiteBlockSplitter)
INITIALIZE_PASS_DEPENDENCY(LoopInfo)
INITIALIZE_PASS_DEPENDENCY(LivenessAnalyzer)
INITIALIZE_PASS_END(ContinuationGenerator, "continuation-generation", "Continuation Generation", false, false)


// Public interface to the ContinuationGeneration pass
namespace llvm {
	FunctionPass* createContinuationGeneratorPass() {
		return new ContinuationGenerator();
	}
}
