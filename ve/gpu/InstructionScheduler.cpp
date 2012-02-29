/*
 * Copyright 2011 Troels Blum <troels@blum.dk>
 *
 * This file is part of cphVB <http://code.google.com/p/cphvb/>.
 *
 * cphVB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cphVB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cphVB. If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <cassert>
#include <stdexcept>
#include <cphvb.h>
#include "InstructionScheduler.hpp"
#define DEBUG

InstructionScheduler::InstructionScheduler(ResourceManager* resourceManager_) 
    : resourceManager(resourceManager_) 
    , batch(0)
{}
inline void InstructionScheduler::schedule(cphvb_instruction* inst)
{
#ifdef DEBUG
    cphvb_pprint_instr(inst);
#endif
    switch (inst->opcode)
    {
    case CPHVB_NONE:
        break;
    case CPHVB_RELEASE:
        sync(inst->operand[0]);
        discard(inst->operand[0]);
        break;
    case CPHVB_SYNC:
        sync(inst->operand[0]);
        break;
    case CPHVB_DISCARD:
        discard(inst->operand[0]);
        break;
    case CPHVB_USERFUNC:
        userdeffunc(inst->userfunc);
        break;
    default:
        ufunc(inst);
    }
}

void InstructionScheduler::forceFlush()
{
    //TODO 
}

void InstructionScheduler::schedule(cphvb_intp instructionCount,
                                    cphvb_instruction* instructionList)
{
#ifdef DEBUG
    std::cout << "[VE GPU] InstructionScheduler: recieved batch with " << 
        instructionCount << " instructions." << std::endl;
#endif
    for (cphvb_intp i = 0; i < instructionCount; ++i)
    {
        //TODO check instructionList->status
        schedule(instructionList++);
    }
    
    /* End of batch cleanup */
    executeBatch();
}

void InstructionScheduler::executeBatch()
{
    if (batch)
    {
        batch->run(resourceManager);
        delete batch;
    }
    batch = 0;
}

void InstructionScheduler::sync(cphvb_array* base)
{
    //TODO postpone sync
    assert(base->base == NULL);
    // We may recieve sync for arrays I don't own
    ArrayMap::iterator it = arrayMap.find(base);
    if  (it == arrayMap.end())
    {
        return;
    }
    if (batch && batch->write(it->second))
    {
        executeBatch();
    }
    it->second->sync();
}

void InstructionScheduler::discard(cphvb_array* base)
{
    //TODO postpone discard
    assert(base->base == NULL);
    // We may recieve sync for arrays I don't own
    ArrayMap::iterator it = arrayMap.find(base);
    if  (it == arrayMap.end())
    {
        return;
    }
    if (batch && batch->access(it->second))
    {
        executeBatch();
    }
    delete it->second;
    arrayMap.erase(it);
}

void InstructionScheduler::userdeffunc(cphvb_userfunc* userfunc)
{
    throw std::runtime_error("User defined functiones not supported.");
}

void InstructionScheduler::ufunc(cphvb_instruction* inst)
{
    //TODO Find out if we support the operation before copying data to device

    int nops = cphvb_operands(inst->opcode);
    assert(nops > 0);
    std::vector<BaseArray*> operandBase(nops);
    for (int i = 0; i < nops; ++i)
    {
        cphvb_array* operand = inst->operand[i];
        // Is it a new base array we haven't heard of before?
        if (!cphvb_scalar(operand)) // Not a scalar
        {
            cphvb_array* base = cphvb_base_array(operand);
            ArrayMap::iterator it = arrayMap.find(base);
            if (it == arrayMap.end())
            {
                // Then create it
                operandBase[i] = new BaseArray(base, resourceManager);
                arrayMap[base] = operandBase[i];
            }
            else
            {
                operandBase[i] = it->second;
            }
        }
    }
    if (batch)
    {
        try 
        {
            batch->add(inst, operandBase);
        } 
        catch (BatchException& be)
        {
            executeBatch();
            batch = new InstructionBatch(inst, operandBase);
        } 
    }
    else
    {
        batch = new InstructionBatch(inst, operandBase);
    }
}
