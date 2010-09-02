/**
 * @file   continuationGenerator.h
 * @date   02.07.2010
 * @author Ralf Karrenberg
 *
 * Copyright (C) 2010 Saarland University
 *
 * This file is part of packetizedOpenCLDriver.
 *
 * packetizedOpenCLDriver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * packetizedOpenCLDriver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with packetizedOpenCLDriver.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef _CONTINUATIONGENERATOR_H
#define	_CONTINUATIONGENERATOR_H


#ifdef DEBUG_TYPE
#undef DEBUG_TYPE
#endif
#define DEBUG_TYPE "continuationgenerator"

#include <llvm/Support/raw_ostream.h>

#include <llvm/Pass.h>
#include <llvm/Function.h>
#include <llvm/Module.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Transforms/Utils/FunctionUtils.h>

#include "livenessAnalyzer.h"
#include "llvmTools.hpp"

#ifdef DEBUG
#define DEBUG_LA(x) do { x } while (false)
#else
#define DEBUG_LA(x) ((void)0)
#endif

#ifdef NDEBUG // force debug output disabled
#undef DEBUG_LA
#define DEBUG_LA(x) ((void)0)
#endif

#define PACKETIZED_OPENCL_DRIVER_FUNCTION_NAME_BARRIER "barrier"
#define PACKETIZED_OPENCL_DRIVER_BARRIER_SPECIAL_END_ID -1
#define PACKETIZED_OPENCL_DRIVER_BARRIER_SPECIAL_START_ID 0

using namespace llvm;

namespace {

class ContinuationGenerator : public FunctionPass {
private:
	struct BarrierInfo {
		BarrierInfo(CallInst* call, BasicBlock* parentBB, unsigned d)
			: id(0), depth(d), barrier(call), parentBlock(parentBB), continuation(NULL), liveValueStructType(NULL) {}

		~BarrierInfo() { delete valueMap; }

		unsigned id;
		unsigned depth;
		CallInst* barrier;
		BasicBlock* parentBlock; // parent block of original function (might have been split due to other barriers)
		Function* continuation;
		StructType* liveValueStructType;

		ValueMap<const Value*, Value*>* valueMap; // stores mapping from original function to continuation

		void print(raw_ostream& o) const {
			o << "barrier id: " << id << "\n";
			o << "barrier depth: " << depth << "\n";
			//if (barrier) o << "barrier: " << *barrier << "\n"; // barriers do not exist anymore :p
			o << "parent block: " << (parentBlock ? parentBlock->getNameStr() : "none") << " (" << (parentBlock ? parentBlock->getParent()->getNameStr() : "") << ")\n";
			o << "continuation: " << (continuation ? continuation->getNameStr() : "none") << "\n";
			if (liveValueStructType) o << "live value struct type: " << *liveValueStructType << "\n";
			else o << "live value struct type: none\n";
		}
	};

public:
	static char ID; // Pass identification, replacement for typeid
	ContinuationGenerator(const bool verbose_flag = false) : FunctionPass(ID), verbose(verbose_flag), barrierFreeFunction(NULL) {}
	~ContinuationGenerator() { releaseMemory(); }

	virtual bool runOnFunction(Function &f) {

		// get loop info
		loopInfo = &getAnalysis<LoopInfo>();

		// get liveness information
		livenessAnalyzer = &getAnalysis<LivenessAnalyzer>();

		DEBUG_LA( outs() << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"; );
		DEBUG_LA( outs() << "generating continuations...\n"; );
		DEBUG_LA( outs() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n"; );

		DEBUG_LA( PacketizedOpenCLDriver::writeFunctionToFile(&f, "debug_before_barriers.ll"); );

		assert (f.getParent() && "function has to have a valid parent module!");
		TargetData* targetData = new TargetData(f.getParent());

		BarrierVecType barriers;
		unsigned maxBarrierDepth  = 0;
		const unsigned numBarriers = getBarrierInfo(&f, barriers, maxBarrierDepth);
		DEBUG_LA( outs() << "  number of barriers in function : " << numBarriers << "\n"; );
		DEBUG_LA( outs() << "  maximum block depth of barriers: " << maxBarrierDepth << "\n\n"; );

		if (numBarriers == 0) {
			DEBUG_LA( outs() << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"; );
			DEBUG_LA( outs() << "no barrier found, skipping generation of continuations!\n"; );
			DEBUG_LA( outs() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n"; );
			return false;
		}

		barrierFreeFunction = eliminateBarriers(&f, barriers, numBarriers, maxBarrierDepth, specialParams, specialParamNames, targetData);

		DEBUG_LA( verifyModule(*f.getParent()); );

		DEBUG_LA( outs() << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"; );
		DEBUG_LA( outs() << "generation of continuations finished!\n"; );
		DEBUG_LA( print(outs(), NULL); );
		DEBUG_LA( outs() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n"; );

		return barrierFreeFunction != NULL; // if barrierFreeFunction does not exist, nothing has changed
	}

	void print(raw_ostream& o, const Module *M) const {
//		ContinuationVecType contVec;
//		getContinuations(contVec);
//		for (ContinuationVecType::const_iterator it=contVec.begin(), E=contVec.end(); it!=E; ++it) {
//			Function* continuation = *it;
//			continuation->viewCFG();
//		}
	}
	virtual void getAnalysisUsage(AnalysisUsage &AU) const {
		AU.addRequired<LoopInfo>();
		AU.addRequired<LivenessAnalyzer>();
	}
	void releaseMemory() {}

	// each continuation function receives an additional parameter with this type and name
	inline void addSpecialParam(const Type* param, const std::string& paramName) {
		specialParams.push_back(param);
		specialParamNames.push_back(paramName);
	}

	inline Function* getBarrierFreeFunction() const { return barrierFreeFunction; }
	typedef SmallVector<Function*, 4> ContinuationVecType;
	inline void getContinuations(ContinuationVecType& continuations) const {
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

	typedef DenseMap<unsigned, BarrierInfo*> ContinuationMapType;
	inline ContinuationMapType* getContinuationMap() { return &continuationMap; }

private:
	const bool verbose;
	LoopInfo* loopInfo;
	LivenessAnalyzer* livenessAnalyzer;
	Function* barrierFreeFunction;

	typedef std::vector<const Type*> SpecialParamVecType;
	typedef std::vector<std::string> SpecialParamNameVecType;
	SpecialParamVecType specialParams;
	SpecialParamNameVecType specialParamNames;

	ContinuationMapType continuationMap;

	typedef SmallVector<BarrierInfo*, 4> BarrierVecType;

	void mapBarrierInfo(Function* f, Function* newF, ValueMap<const Value*, Value*>& valueMap, BarrierVecType& barriers, ValueMap<const Value*, Value*>& barrierMapping, const unsigned maxBarrierDepth) {
		assert (f && newF);
		assert (!valueMap.empty());
		DEBUG_LA( outs() << "\nmapping barrier info from function '" << f->getNameStr() << "' to new function '" << newF->getNameStr() << "'...\n"; );

		for (BarrierVecType::iterator it=barriers.begin(), E=barriers.end(); it!=E; ++it) {
			BarrierInfo* binfo = *it;
			const BasicBlock* oldBB = binfo->parentBlock;
#if 1
			Value* oldBarrier = binfo->barrier;
			binfo->barrier = cast<CallInst>(valueMap[binfo->barrier]);
			binfo->parentBlock = cast<BasicBlock>(valueMap[oldBB]);

			barrierMapping[binfo->barrier] = oldBarrier;

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

	// helper for getBarrierInfo()
	unsigned collectBarriersDFS(BasicBlock* block, unsigned depth, BarrierVecType& barriers, unsigned& maxBarrierDepth, std::set<BasicBlock*>& visitedBlocks) {
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
				numBarriers += collectBarriersDFS(exitBB, depth+1, barriers, maxBarrierDepth, visitedBlocks);
			}
			DEBUG_LA( outs() << "  successors of loop with header " << block->getNameStr() << " have " << numBarriers << " barriers!\n"; );
		}

		for (succ_iterator S=succ_begin(block), E=succ_end(block); S!=E; ++S) {
			BasicBlock* succBB = *S;
			numBarriers += collectBarriersDFS(succBB, depth+1, barriers, maxBarrierDepth, visitedBlocks);
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
			if (!callee->getName().equals(PACKETIZED_OPENCL_DRIVER_FUNCTION_NAME_BARRIER)) continue;

			++numBarriers;

			BarrierInfo* bi = new BarrierInfo(call, block, depth);
			barriers.push_back(bi);
			DEBUG_LA( outs() << "    barrier found in block '" << block->getNameStr() << "': " << *call << "\n"; );

			if (depth > maxBarrierDepth) maxBarrierDepth = depth;
		} while (I != block->begin());

		DEBUG_LA( outs() << "collectBarriersDFS(" << block->getNameStr() << ", " << depth << ") -> " << numBarriers << "\n"; );

		return numBarriers;
	}
	
	// Traverse the function in DFS and collect all barriers in post-reversed order.
	// Count how many barriers the function has and assign an id to each barrier
	inline unsigned getBarrierInfo(Function* f, BarrierVecType& barriers, unsigned& maxBarrierDepth) {
		std::set<BasicBlock*> visitedBlocks;
		const unsigned numBarriers = collectBarriersDFS(&f->getEntryBlock(), 0, barriers, maxBarrierDepth, visitedBlocks);

		DEBUG_LA(
			outs() << "\nnum barriers: " << numBarriers << "\n";
			for (BarrierVecType::iterator it=barriers.begin(), E=barriers.end(); it!=E; ++it) {
				BarrierInfo* binfo = *it;
				outs() << " * " << *binfo->barrier << "\n";
			}
			outs() << "\n";
		);

		return numBarriers;
	}

	void findContinuationBlocksDFSOrdered(BasicBlock* block, std::vector<BasicBlock*>& copyBlocks, std::set<BasicBlock*>& visitedBlocks) {
		assert (block);
		if (visitedBlocks.find(block) != visitedBlocks.end()) return;
		visitedBlocks.insert(block);

		copyBlocks.push_back(block);

		for (succ_iterator S=succ_begin(block), E=succ_end(block); S!=E; ++S) {
			BasicBlock* succBB = *S;
			findContinuationBlocksDFSOrdered(succBB, copyBlocks, visitedBlocks);
		}
	}
	void findContinuationBlocksDFS(const BasicBlock* block, std::set<const BasicBlock*>& copyBlocks, std::set<const BasicBlock*>& visitedBlocks) {
		assert (block);
		if (visitedBlocks.find(block) != visitedBlocks.end()) return;
		visitedBlocks.insert(block);

		copyBlocks.insert(block);

		for (succ_const_iterator S=succ_begin(block), E=succ_end(block); S!=E; ++S) {
			const BasicBlock* succBB = *S;
			findContinuationBlocksDFS(succBB, copyBlocks, visitedBlocks);
		}
	}


	// generates a continuation function that is called at the point of the barrier
	void createContinuation(BarrierInfo* barrierInfo, const std::string& newFunName, TargetData* targetData) {
		assert (barrierInfo && targetData);
		const unsigned barrierIndex = barrierInfo->id;
		CallInst* barrier = barrierInfo->barrier;
		BasicBlock* parentBlock = barrierInfo->parentBlock;
		assert (barrier && parentBlock);
		assert (barrier->getParent());
		Function* f = barrier->getParent()->getParent();
		assert (f);
		Module* mod = f->getParent();
		assert (mod);

		LLVMContext& context = mod->getContext();

		DEBUG_LA( outs() << "\ngenerating continuation for barrier " << barrierIndex << " in block '" << parentBlock->getNameStr() << "'\n"; );
		DEBUG_LA( outs() << "  barrier: " << *barrierInfo->barrier << "\n"; );

		assert (barrier->getParent() == parentBlock && "barrier worklist ordering wrong?");

		//--------------------------------------------------------------------//
		// get live values for this block
		// NOTE: This only fetches live values of former parent block
		//       in order to prevent recalculating live value information for
		//       entire function.
		// For this, it does not matter if blocks are split already.
		//--------------------------------------------------------------------//
		const LivenessAnalyzer::LiveSetType* liveInValuesOrig = livenessAnalyzer->getBlockLiveInValues(parentBlock);
		const LivenessAnalyzer::LiveSetType* liveOutValuesOrig = livenessAnalyzer->getBlockLiveOutValues(parentBlock);
		assert (liveInValuesOrig);
		assert (liveOutValuesOrig);

		// before splitting, get block-internal live values
		// (values that are defined inside block and live over the barrier)
		LivenessAnalyzer::LiveSetType internalLiveValues;
		//LivenessAnalyzer::LiveSetType internalNonLiveValues;
		livenessAnalyzer->getBlockInternalLiveInValues(barrier, internalLiveValues);

		// we have to copy the live-in value set before modifying it
		LivenessAnalyzer::LiveSetType* liveInValues = new LivenessAnalyzer::LiveSetType();
		liveInValues->insert(liveInValuesOrig->begin(), liveInValuesOrig->end());

		// merge block-internal live values with liveInValues
		liveInValues->insert(internalLiveValues.begin(), internalLiveValues.end());
		// remove all values that die in same block but above barrier
		livenessAnalyzer->removeBlockInternalNonLiveInValues(barrier, *liveInValues, *liveOutValuesOrig);

		DEBUG_LA(
			outs() << "\n\nFinal Live-In values of block '" << parentBlock->getNameStr() << "':\n";
			for (LivenessAnalyzer::LiveSetType::iterator it=liveInValues->begin(), E=liveInValues->end(); it!=E; ++it) {
				outs() << " * " << **it << "\n";
			}
			outs() << "\n\nOriginal Live-In values of block '" << parentBlock->getNameStr() << "':\n";
			for (LivenessAnalyzer::LiveSetType::iterator it=liveInValuesOrig->begin(), E=liveInValuesOrig->end(); it!=E; ++it) {
				outs() << " * " << **it << "\n";
			}
			outs() << "\nInternal Live-In values of block '" << parentBlock->getNameStr() << "':\n";
			for (LivenessAnalyzer::LiveSetType::iterator it=internalLiveValues.begin(), E=internalLiveValues.end(); it!=E; ++it) {
				outs() << " * " << **it << "\n";
			}
			outs() << "\nOriginal Live-Out values of block '" << parentBlock->getNameStr() << "':\n";
			for (LivenessAnalyzer::LiveSetType::iterator it=liveOutValuesOrig->begin(), E=liveOutValuesOrig->end(); it!=E; ++it) {
				outs() << " * " << **it << "\n";
			}
			outs() << "\n";
		);




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
		// create struct with live-in values of newBlock
		//--------------------------------------------------------------------//
		std::vector<const Type*> params;
		for (LivenessAnalyzer::LiveSetType::iterator it=liveInValues->begin(), E=liveInValues->end(); it!=E; ++it) {
			params.push_back((*it)->getType());
		}
		StructType* sType = StructType::get(context, params, false);
		DEBUG_LA( outs() << "new struct type: " << *sType << "\n"; );
		DEBUG_LA( outs() << "type size in bits : " << targetData->getTypeSizeInBits(sType) << "\n"; );
		DEBUG_LA( outs() << "alloc size in bits: " << targetData->getTypeAllocSizeInBits(sType) << "\n"; );
		DEBUG_LA( outs() << "alloc size        : " << targetData->getTypeAllocSize(sType) << "\n"; );

		// pointer to union for live value struct for next call is the last parameter
		Argument* nextContLiveValStructPtr = --(f->arg_end());
		DEBUG_LA( outs() << "pointer to union: " << *nextContLiveValStructPtr << "\n"; );

		// bitcast data pointer to correct struct type for GEP below
		BitCastInst* bc = new BitCastInst(nextContLiveValStructPtr, PointerType::getUnqual(sType), "", barrier);

		// store values
		unsigned i=0;
		for (LivenessAnalyzer::LiveSetType::iterator it=liveInValues->begin(), E=liveInValues->end(); it!=E; ++it) {
			std::vector<Value*> indices;
			indices.push_back(ConstantInt::getNullValue(Type::getInt32Ty(context)));
			indices.push_back(ConstantInt::get(context, APInt(32, i++)));
			GetElementPtrInst* gep = GetElementPtrInst::Create(bc, indices.begin(), indices.end(), "", barrier);
			DEBUG_LA( outs() << "store gep(" << i-1 << "): " << *gep << "\n"; );
			const unsigned align = 16;
			new StoreInst(*it, gep, false, align, barrier);
		}


		//--------------------------------------------------------------------//
		// delete the edge from parentBlock to newBlock
		//--------------------------------------------------------------------//
		assert (parentBlock->getTerminator()->use_empty());
		parentBlock->getTerminator()->eraseFromParent();

		//--------------------------------------------------------------------//
		// create return that returns the id for the next call
		//--------------------------------------------------------------------//
		const Type* returnType = Type::getInt32Ty(context);
		ReturnInst::Create(context, ConstantInt::get(returnType, barrierIndex, true), barrier);

		//--------------------------------------------------------------------//
		// (dead code elimination should remove newBlock and all blocks below
		// that are dead.)
		//--------------------------------------------------------------------//
		// nothing to do here

		//--------------------------------------------------------------------//
		// erase barrier
		//--------------------------------------------------------------------//
		//if (!barrier->use_empty()) barrier->replaceAllUsesWith(Constant::getNullValue(barrier->getType()));
		assert (barrier->use_empty() && "barriers must not have any uses!");
		barrier->eraseFromParent();



		//--------------------------------------------------------------------//
		// create new function with the following signature:
		// - returns int (id of next continuation)
		// - special parameters
		// - live-in value parameters
		// - last parameter: void* data (union where live values for next
		//                   continuation are stored before returning)
		//
		// - original parameters of f not required (included in live vals if necessary)
		//--------------------------------------------------------------------//
		params.clear();
		//params.insert(params.end(), f->getFunctionType()->subtype_begin(), f->getFunctionType()->subtype_end()); // original parameters
		params.insert(params.end(), specialParams.begin(), specialParams.end());   // special parameters
		params.insert(params.end(), sType->element_begin(), sType->element_end()); // live values
		params.push_back(Type::getInt8PtrTy(context)); // data ptr

		FunctionType* fType = FunctionType::get(returnType, params, false);

		Function* continuation = Function::Create(fType, Function::ExternalLinkage, newFunName, mod); // TODO: check linkage type

		Function::arg_iterator CA=continuation->arg_begin();
		for (unsigned i=0, e=specialParamNames.size(); i<e; ++i, ++CA) {
			CA->setName(specialParamNames[i]);
		}
		DEBUG_LA( outs() << "\nnew continuation declaration: " << *continuation << "\n"; );

		//--------------------------------------------------------------------//
		// create mappings of values
		//--------------------------------------------------------------------//
		// create mappings of live-in values to arguments for copying of blocks
		DEBUG_LA( outs() << "\nlive value mappings:\n"; );
		ValueMap<const Value*, Value*>* valueMap = new ValueMap<const Value*, Value*>();
		DenseMap<const Value*, Value*> liveValueToArgMap;
		// CA has to point to first live value parameter of continuation
		CA = --continuation->arg_end();
		for (unsigned i=0, e=sType->getNumContainedTypes(); i<e; ++i) {
			--CA;
		}
		for (LivenessAnalyzer::LiveSetType::iterator it=liveInValues->begin(), E=liveInValues->end(); it!=E; ++it, ++CA) {
			const Value* liveVal = *it;
			(*valueMap)[liveVal] = CA;
			liveValueToArgMap[liveVal] = CA;
			DEBUG_LA( outs() << " * " << *liveVal << " -> " << *CA << "\n"; );
		}


		//--------------------------------------------------------------------//
		// copy all blocks 'below' parentBlock inside the new function (DFS)
		// and map all uses of live values to the loads generated in the last step
		//--------------------------------------------------------------------//
		std::set<const BasicBlock*> copyBlocks;
		std::set<const BasicBlock*> visitedBlocks;
		findContinuationBlocksDFS(newBlock, copyBlocks, visitedBlocks);

		DEBUG_LA(
			outs() << "\ncloning blocks into continuation...\n";
			for (std::set<const BasicBlock*>::iterator it=copyBlocks.begin(), E=copyBlocks.end(); it!=E; ++it) {
				outs() << " * " << (*it)->getNameStr() << "\n";
			}
			outs() << "\n";
		);

		// HACK: Copy over entire function and remove all unnecessary blocks.
		//       This is required because CloneBasicBlock does not perform any
		//       remapping. RemapInstructions thus has to be called by hand and
		//       is not available via includes (as of llvm-2.8svn ~May/June 2010 :p).

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



		// remap values that were no arguments
		// NOTE: apparently, CloneFunctionInto does not look into the valueMap
		//       if it directly finds all references. Due to the fact that we
		//       copy the entire function, live values that are no arguments
		//       are still resolved by their former definitions which only get
		//       erased in the next step.
		// NOTE: actually, this is good behaviour - we must not remap values
		//       that are defined in a block that also appears in the
		//       continuation, e.g. in loops.
		// TODO: Are we sure that the ordering is always correct with LiveSetType
		//       being a set? :p

		// erase all blocks that do not belong to this continuation
		BasicBlock* dummyBB = BasicBlock::Create(context, "dummy", continuation);
		// iterate over blocks of original fun, but work on blocks of continuation fun
		// -> can't find out block in old fun for given block in new fun via map ;)
		for (Function::iterator BB=f->begin(), BBE=f->end(); BB!=BBE; ) {
			assert ((*valueMap).find(BB) != (*valueMap).end());
			BasicBlock* blockOrig = BB++;
			BasicBlock* blockCopy = cast<BasicBlock>((*valueMap)[blockOrig]);
			if (copyBlocks.find(blockOrig) != copyBlocks.end()) continue;

			// block must not be "copied" -> delete it

			// but first, replace all uses of instructions of block by dummies
			// or arguments in case of live values...
			BasicBlock::iterator IO=blockOrig->begin(); // mapping uses values from old function...
			for (BasicBlock::iterator I=blockCopy->begin(), IE=blockCopy->end(); I!=IE; ++I, ++IO) {
				DEBUG_LA( outs() << "replacing uses of inst: " << *I << "\n"; );
				if (liveInValues->find(IO) != liveInValues->end()) {
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

		// if necessary, make new block the entry block of the continuation
		// (= move to front of block list)
		BasicBlock* entryBlock = cast<BasicBlock>((*valueMap)[newBlock]);
		if (entryBlock != &continuation->getEntryBlock()) {
			entryBlock->moveBefore(&continuation->getEntryBlock());
		}


		// Make sure all uses of live values that are not dominated anymore are
		// rewired to arguments.
		// NOTE: This can't be done before blocks are deleted ;).

		// compute dominator tree (somehow does not work with functionpassmanager)
		DominatorTree* domTree = new DominatorTree();
		domTree->runOnFunction(*continuation);
		DEBUG_LA( domTree->print(outs(), mod); );

		// set arg_iterator to first live value arg (first args are special values
		Function::arg_iterator A2 = continuation->arg_begin();
		std::advance(A2, specialParams.size());
		for (LivenessAnalyzer::LiveSetType::iterator it=liveInValues->begin(), E=liveInValues->end(); it!=E; ++it, ++A2) {
			const Value* liveVal = *it;
			DEBUG_LA( outs() << "live value: " << *liveVal << "\n"; );
			if (isa<Argument>(liveVal)) continue;

			// if all uses already replaced above, skip this value
			if ((*valueMap).find(liveVal) == (*valueMap).end()) {
				DEBUG_LA( outs() << "  all uses already replaced!\n"; );
				continue;
			}

			Value* newLiveVal = (*valueMap)[liveVal];
			DEBUG_LA( outs() << "new live value: " << *newLiveVal << "\n"; );

			// if the value is defined in one of the copied blocks, we must only
			// replace those uses that are not dominated by their definition anymore
			if (const Instruction* inst = dyn_cast<Instruction>(liveVal)) {
				if (copyBlocks.find(inst->getParent()) != copyBlocks.end()) {
					Instruction* newInst = cast<Instruction>(newLiveVal);
					DEBUG_LA( outs() << "live value is defined in copied block: " << (*copyBlocks.find(inst->getParent()))->getNameStr() << "\n"; );
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

		//DEBUG_LA( outs() << *continuation << "\n"; );
		DEBUG_LA( outs() << "continuation '" << continuation->getNameStr() << "' generated successfully!\n\n"; );

		DEBUG_LA(
			outs() << "verifying function '" << f->getNameStr() << "'... ";
			verifyFunction(*f);
			outs() << "done.\nverifying function '" << continuation->getNameStr() << "'... ";
			verifyFunction(*continuation);
			outs() << "done.\n";
		);

		barrierInfo->continuation = continuation;
		barrierInfo->liveValueStructType = sType;
		barrierInfo->valueMap = valueMap;
	}

	void removeLiveValStructFromExtractedFnArgType(Function* f, Value* nextContLiveValStructPtr, LLVMContext& context, Instruction* insertBefore) {
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
	void createSecondaryContinuation(CallInst* barrier, const unsigned barrierIndex, Function* continuation) {
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
		const Type* returnType = Type::getInt32Ty(context);
		ReturnInst::Create(context, ConstantInt::get(returnType, barrierIndex, true), secCall);

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

	Function* eliminateBarriers(Function* f, BarrierVecType& barriers, const unsigned numBarriers, const unsigned maxBarrierDepth, SpecialParamVecType& specialParams, SpecialParamNameVecType& specialParamNames, TargetData* targetData) {
		assert (f && targetData);
		assert (f->getReturnType()->isVoidTy());
		Module* mod = f->getParent();
		assert (mod);
		LLVMContext& context = mod->getContext();

		const std::string& functionName = f->getNameStr();
		DEBUG_LA( outs() << "\neliminateBarriers(" << functionName << ")\n"; );

		//--------------------------------------------------------------------//
		// change return value of f to return unsigned (barrier id)
		// and adjust parameters:
		// - original parameters
		// - special parameters (get_global_id, get_local_size, num_groups)
		// - void* nextContLiveVals : pointer to live value union where live-in
		//                            values of next continuation are stored
		//
		// = create new function with new signature and clone all blocks
		// The former return statements now all return -1 (special end id)
		//--------------------------------------------------------------------//
		const FunctionType* fTypeOld = f->getFunctionType();
		std::vector<const Type*> params;
		params.insert(params.end(), specialParams.begin(), specialParams.end()); // "special" parameters
		params.insert(params.end(), fTypeOld->param_begin(), fTypeOld->param_end()); // parameters of original kernel
		params.push_back(Type::getInt8PtrTy(context)); // void*-parameter (= live value struct return param)
		const FunctionType* fTypeNew = FunctionType::get(Type::getInt32Ty(context), params, false);
		Function* newF = Function::Create(fTypeNew, Function::ExternalLinkage, functionName+"_begin", mod); // TODO: check linkage type

		// set names of special parameters
		unsigned paramIdx = 0;
		for (Function::arg_iterator A=newF->arg_begin(); paramIdx < specialParamNames.size(); ++A, ++paramIdx) {
			A->setName(specialParamNames[paramIdx]);
		}
		(--newF->arg_end())->setName("nextContLiveVals");

		// specify mapping of parameters and set names of original parameters
		// (special parameters come first, so we have to start at the first original parameter)
		// NOTE: only iterate to second last param (data ptr was not in original function)
		ValueMap<const Value*, Value*> valueMap;
		unsigned argIdx = 0;
		for (Function::arg_iterator A2=f->arg_begin(), A=newF->arg_begin(), AE=--newF->arg_end(); A!=AE; ++A, ++argIdx) {
			if (argIdx < specialParams.size()) continue; // don't increment A2
			//DEBUG_LA( outs() << "  " << *A2 << " -> " << *A << "\n"; );
			valueMap[A2] = A;
			A->takeName(A2);
			++A2;
		}
		SmallVector<ReturnInst*, 2> returns;

		CloneAndPruneFunctionInto(newF, f, valueMap, returns, ".");

		for (unsigned i=0; i<returns.size(); ++i) {
			BasicBlock* retBlock = returns[i]->getParent();
			returns[i]->eraseFromParent();
			ReturnInst::Create(context, ConstantInt::get(fTypeNew->getReturnType(), PACKETIZED_OPENCL_DRIVER_BARRIER_SPECIAL_END_ID, true), retBlock);
		}

		DEBUG_LA( PacketizedOpenCLDriver::writeFunctionToFile(newF, "debug_cont_begin_beforesplitting.ll"); );

		// map the live values of the original function to the new one
		livenessAnalyzer->mapLiveValues(f, newF, valueMap);

		// map the barriers of the original function to the new one
		ValueMap<const Value*, Value*> barrierMapping; // new to old mapping, currently not used
		mapBarrierInfo(f, newF, valueMap, barriers, barrierMapping, maxBarrierDepth);

		//
		// now, all live values and barriers refer to values in newF,
		// which will be the first continuation function in the end
		//

		//--------------------------------------------------------------------//
		// Generate order in which barriers should be replaced:
		// Barriers with highest depth come first, barriers with same depth
		// are ordered nondeterministically unless they live in the same block,
		// in which case their order is determined by their dominance relation.
		//--------------------------------------------------------------------//
		DenseMap<const CallInst*, unsigned> barrierIndices;
		SmallVector<BarrierInfo*, 4> orderedBarriers;

		// 0 is reserved for 'start'-function, so the last index is numBarriers and 0 is not used
		unsigned barrierIndex = 1;
		for (BarrierVecType::iterator it=barriers.begin(), E=barriers.end(); it!=E; ++it, ++barrierIndex) {
			BarrierInfo* binfo = *it;
			orderedBarriers.push_back(binfo);
			binfo->id = barrierIndex; // set id
			assert (binfo->barrier);
			barrierIndices[binfo->barrier] = barrierIndex; // save newF-barrier -> id mapping for secondary continuations
			continuationMap[barrierIndex] = binfo; // make sure continuationMap can already be queried for index -> barrier
			DEBUG_LA( outs() << "  added barrier " << barrierIndex << ": " << *binfo->barrier << "\n"; );
			DEBUG_LA( outs() << "    parent block: " << binfo->parentBlock->getNameStr() << "\n"; );
		}



		//--------------------------------------------------------------------//
		// call eliminateBarrier() for each barrier in newFunction
		//--------------------------------------------------------------------//
		const unsigned numContinuationFunctions = numBarriers+1;
		continuationMap[0] = new BarrierInfo(NULL, NULL, 0);
		continuationMap[0]->continuation = newF;
		continuationMap[0]->liveValueStructType = StructType::get(context, false);

		// Loop over barriers and generate a continuation for each one.
		// NOTE: newF is modified each time
		//       (blocks split, loading/storing of live value structs, ...)
		for (SmallVector<BarrierInfo*, 4>::iterator it=orderedBarriers.begin(), E=orderedBarriers.end(); it!=E; ++it) {
			BarrierInfo* barrierInfo = *it;
			const unsigned barrierIndex = barrierInfo->id;
			assert (barrierIndex != 0 && "index 0 is reserved for original function, must not appear here!");
			assert (barrierInfo->parentBlock->getParent() == newF);

			std::stringstream sstr;
			sstr << functionName << "_cont_" << barrierIndex;

			createContinuation(barrierInfo, sstr.str(), targetData);

			assert (barrierInfo->continuation);
			assert (barrierInfo->liveValueStructType);
			//continuationMap[barrierIndex] = barrierInfo;
			//barrierInfo->continuation->viewCFG();

			// TODO: Don't just randomly pick one remaining barrier... there might be
			//       nested loops etc.
			//       If not, we can just pick the "uppermost" barrier and skip
			//       all that are dominated (actual continuations are generated by
			//       above call inside newF, not here).
			// NOTE: We rely on the assumption that if there is a barrier remaining
			//       in the continuation, the same barrier also still has to be
			//       in newF.
			WHILE:
			for (Function::iterator BB=barrierInfo->continuation->begin(), BBE=barrierInfo->continuation->end(); BB!=BBE; ++BB) {
				for (BasicBlock::iterator I=BB->begin(), IE=BB->end(); I!=IE; ) {
					if (!isa<CallInst>(I)) { ++I; continue; }
					CallInst* call = cast<CallInst>(I++);

					const Function* callee = call->getCalledFunction();
					if (callee->getName().equals(PACKETIZED_OPENCL_DRIVER_FUNCTION_NAME_BARRIER)) {
						DEBUG_LA( outs() << "\nbarrier not eliminated in continuation '" << barrierInfo->continuation->getNameStr() << "': " << *call << "\n"; );

						// find the corresponding barrier in newF
						for (ValueMap<const Value*, Value*>::iterator it2=barrierInfo->valueMap->begin(), E2=barrierInfo->valueMap->end(); it2!=E2; ++it2) {
							Value* contVal = it2->second; // new value (refers to continuation)
							if (contVal != call) continue;

							// now we know that the first field is the barrier in newF we can get the index of
							const CallInst* newFBarrierConst = cast<CallInst>(it2->first);

							// determine id of the barrier
							const unsigned secondaryBarrierIndex = barrierIndices[newFBarrierConst];
							assert (secondaryBarrierIndex != barrierIndex);
							assert (secondaryBarrierIndex != 0);

							DEBUG_LA( outs() << "  barrier id: " << secondaryBarrierIndex << "\n"; );
							DEBUG_LA( outs() << "  newF: " << newF->getNameStr() << "\n"; );
							DEBUG_LA( outs() << "  cont: " << barrierInfo->continuation->getNameStr() << "\n"; );
							// do not generate a new continuation but create loads/stores to correct live value struct
							// and generate a return statement with the appropriate id
							createSecondaryContinuation(call, secondaryBarrierIndex, barrierInfo->continuation);

							goto WHILE; // break out of all loops and start iteration again (blocks changed)
						}
						assert (false && "secondary continuation generation failed!!");
					}
				}
			}

			//barrierInfo->continuation->viewCFG();
		}
		//continuationMap[0]->continuation->viewCFG();

		assert (continuationMap.size() == numContinuationFunctions);


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
						CallInst* call = cast<CallInst>(I);

						const Function* callee = call->getCalledFunction();
						if (callee->getName().equals(PACKETIZED_OPENCL_DRIVER_FUNCTION_NAME_BARRIER)) {
							errs() << "\nERROR: barrier not eliminated in continuation '" << continuation->getNameStr() << "': " << *call << "\n";
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


		DEBUG_LA( outs() << "replaced all barriers by continuations!\n"; );

		DEBUG_LA(
			outs() << "\ncontinuations:\n";
			for (ContinuationMapType::const_iterator it=continuationMap.begin(), E=continuationMap.end(); it!=E; ++it) {
				const BarrierInfo* barrierInfo = it->second;
				barrierInfo->print(outs());
				outs() << "\n";
			}
			outs() << "\n";
		);

		return createWrapper(f, continuationMap, mod, targetData);
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
	Function* createWrapper(Function* origFunction, ContinuationMapType& cMap, Module* mod, TargetData* targetData) {
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
		unsigned unionSize = 0;
		for (ContinuationMapType::iterator it=cMap.begin(), E=cMap.end(); it!=E; ++it) {
			BarrierInfo* bit = it->second;
			StructType* liveValueStructType = bit->liveValueStructType;
			assert (liveValueStructType);
			const unsigned typeSize = targetData->getTypeAllocSize(liveValueStructType);
			if (unionSize < typeSize) unionSize = typeSize;
		}
		DEBUG_LA( outs() << "union size for live value structs: " << unionSize << "\n"; );
		// allocate memory for union
		Value* allocSize = ConstantInt::get(context, APInt(32, unionSize));
		Value* dataPtr = builder.CreateAlloca(Type::getInt8Ty(context), allocSize, "liveValueUnion");

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
		CallInst** calls = new CallInst*[numContinuationFunctions]();
		for (unsigned i=0; i<numContinuationFunctions; ++i) {
			BasicBlock* block = callBBs[i];
			builder.SetInsertPoint(block);

			// extract arguments from live value struct (dataPtr)
			BarrierInfo* binfo = cMap[i];
			const StructType* sType = binfo->liveValueStructType;
			const unsigned numLiveVals = sType->getNumElements(); // 0 for begin-fn
			Value** contArgs = new Value*[numLiveVals]();

			Value* bc = builder.CreateBitCast(dataPtr, PointerType::getUnqual(sType)); // cast data pointer to correct pointer to struct type

			for (unsigned j=0; j<numLiveVals; ++j) {
				std::vector<Value*> indices;
				indices.push_back(ConstantInt::getNullValue(Type::getInt32Ty(context)));
				indices.push_back(ConstantInt::get(context, APInt(32, j)));
				Value* gep = builder.CreateGEP(bc, indices.begin(), indices.end(), "");
				DEBUG_LA( outs() << "load gep(" << j << "): " << *gep << "\n"; );
				LoadInst* load = builder.CreateLoad(gep, false, "");
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


};

} // namespace

char ContinuationGenerator::ID = 0;
static RegisterPass<ContinuationGenerator> CG("continuation-generation", "Continuation Generation");

// Public interface to the ContinuationGeneration pass
namespace llvm {
	FunctionPass* createContinuationGeneratorPass() {
		return new ContinuationGenerator();
	}
}


#endif	/* _CONTINUATIONGENERATOR_H */

