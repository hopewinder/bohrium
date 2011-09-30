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
#include <cphvb.h>
#include "cphVBarray.hpp"

cphVBarray::cphVBarray(cphvb_array* arraySpec_, ResourceManager* resourceManager_) 
    : arraySpec(arraySpec_)
    , resourceManager(resourceManager_)
      /* TODO: Do correct mapping 
       * This works for now becaus we only support float32 
       * and bool, and both are mapped to OCL_FLOAT32*/
    , oclType(OCL_FLOAT32)
{}

size_t cphVBArray::size()
{
    assert(arraySpec->base == NULL); // This should only be used for base arrays
    size_t res = cphvb_nelements(arraySpec->ndim, arraySpec->shape);
    size *= oclSizeOf(arraySpec->oclType);
    return size;
}

void cphVBarray::deviceAlloc()
{
    assert(arraySpec->base == NULL); 
    assert(arraySpec->ndim > 0);
    buffer = resourceManager->createBuffer(this.size());
#ifdef DEBUG
    std::cout << "[VE GPU] createBuffer(" << this.size() << ") -> " << (void*)buffer << std::endl;
#endif
    return buffer;
}

cphvb_data_ptr MemoryManager::hostAlloc(cphVBarray* baseArray)
{
    assert(baseArray->base == NULL);
    size_t size = dataSize(baseArray);
    cphvb_data_ptr res = (cphvb_data_ptr)std::malloc(size);
    if (res == NULL)
    {
        throw std::runtime_error("Could not allocate memory on host");
    }
    return res;
}

void MemoryManager::copyToHost(cphVBarray* baseArray)
{
    assert(baseArray->base == NULL);
    assert(baseArray->buffer != 0);
    assert(baseArray->data != NULL);
    if (oclType(baseArray->type) != baseArray->oclType)
    {
        //TODO implement type conversion
        throw std::runtime_error("copyToHost: Type conversion not implemented yet");
    } 
    size_t size = dataSize(baseArray);
#ifdef DEBUG
    std::cout << "[VE GPU] enqueueReadBuffer(" << (void*)baseArray->buffer << ", " << 
        baseArray->data << ", NULL, 0)" << std::endl;
#endif
    resourceManager->enqueueReadBuffer(baseArray->buffer, baseArray->data, NULL, 0);
}

void MemoryManager::copyToDevice(cphVBarray* baseArray)
{
    assert(baseArray->base == NULL);
    assert(baseArray->data != NULL);
    assert(baseArray->buffer != 0);
    size_t size = dataSize(baseArray);
#ifdef DEBUG
    std::cout << "[VE GPU] >enqueueWriteBuffer(" <<  (void*)baseArray->buffer << 
        ", "<< baseArray->data << "(" << baseArray << "), NULL, 0)" << std::endl;
#endif
    if (oclType(baseArray->type) != baseArray->cudaType)
    {
        //TODO implement type conversion
        throw std::runtime_error("copyToDevice: Type conversion not implemented yet");        
    }
    resourceManager->enqueueWriteBuffer(baseArray->buffer, baseArray->data, NULL, 0);
}

void printOn(std::ostream& os) const
{
    os << "cphVBarray ID: " << arraySpec << " {" << std::endl; 
    os << "\towner: " << arraySpec.owner << std::endl; 
    os << "\tbase: " << arraySpec.base << std::endl; 
    os << "\ttype: " << cphvb_type_text(arraySpec.type) << std::endl; 
    os << "\tndim: " << arraySpec.ndim << std::endl; 
    os << "\tstart: " << arraySpec.start << std::endl; 
    for (int i = 0; i < arraySpec.ndim; ++i)
    {
        os << "\tshape["<<i<<"]: " << arraySpec.shape[i] << std::endl;
    } 
    for (int i = 0; i < arraySpec.ndim; ++i)
    {
        os << "\tstride["<<i<<"]: " << arraySpec.stride[i] << std::endl;
    } 
    os << "\tdata: " << arraySpec.data << std::endl; 
    os << "\thas_init_value: " << arraySpec.has_init_value << std::endl;
    switch(arraySpec.type)
    {
    case CPHVB_INT32:
        os << "\tinit_value: " << arraySpec.init_value.int32 << std::endl;
        break;
    case CPHVB_UINT32:
        os << "\tinit_value: " << arraySpec.init_value.uint32 << std::endl;
        break;
    case CPHVB_FLOAT32:
        os << "\tinit_value: " << arraySpec.init_value.float32 << std::endl;
        break;
    }
    os << "\tref_count: " << arraySpec.ref_count << std::endl; 
    os << "}"<< std::endl;
    return os;
}

std::ostream& operator<< (std::ostream& os, 
                          cphVBarray const& array);
{
    array.printOn(os);
    return os;
}
