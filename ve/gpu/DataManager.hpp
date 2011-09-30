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

#ifndef __DATAMANAGER_HPP
#define __DATAMANAGER_HPP

#include <map>
#include <CL/cl.hpp>
#include "MemoryManager.hpp"
#include "InstructionBatch.hpp"

typedef std::map<cphVBarray* base, cphVBarray* view> WriteLockTable;
typedef std::map<cphVBarray*, cl::Buffer> Base2BufferMap;
typedef std::map<cphVBarray* ,cphVBarray*> Operand2BaseMap;

class DataManager
{
private:
    MemoryManager* memoryManager;
    WriteLockTable writeLockTable;
    Base2BufferMap base2Buffer;
    Operand2BaseMap op2Base;
    InstructionBatch* activeBatch;
    void _sync(cphVBarray* baseArray);
    void _flush(cphVBarray* view);
    void initCudaArray(cphVBarray* baseArray);
    void mapOperands(cphVBarray* operands[],
                     int nops);
public:
    DataManager(MemoryManager* memoryManager_);
    void lock(cphVBarray* operands[], 
              int nops, 
              InstructionBatch* batch);
    void release(cphVBarray* baseArray);
    void sync(cphVBarray* baseArray);
    void flush(cphVBarray* view);
    void discard(cphVBarray* baseArray);
    void flushAll();
    void batchEnd();
};

#endif
