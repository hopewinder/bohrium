/*
This file is part of Bohrium and copyright (c) 2012 the Bohrium team:
http://bohrium.bitbucket.org

Bohrium is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as 
published by the Free Software Foundation, either version 3 
of the License, or (at your option) any later version.

Bohrium is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the 
GNU Lesser General Public License along with Bohrium. 

If not, see <http://www.gnu.org/licenses/>.
*/
#include <iostream>

#include "traits.hpp"           // Traits for assigning type to constants and multi_arrays.
#include "cppb.hpp"
#include "runtime.hpp"

namespace bh {

template <typename T>
void multi_array<T>::init()     // Pseudo-default constructor
{
    key     = keys++;
    is_temp = false;

    storage.insert(key, new bh_array);
    assign_array_type<T>(&storage[key]);
}

template <typename T>           // Default constructor
multi_array<T>::multi_array()
{
    init();
    std::cout << "[array-none" << key << "]" << std::endl;
}

template <typename T>           // Copy constructor
multi_array<T>::multi_array(const multi_array<T>& operand)
{
    init();

    storage[this->key].base        = NULL;
    storage[this->key].ndim        = storage[operand.getKey()].ndim;
    storage[this->key].start       = storage[operand.getKey()].start;
    for(bh_index i=0; i< storage[operand.getKey()].ndim; i++) {
        storage[this->key].shape[i] = storage[operand.getKey()].shape[i];
    }
    for(bh_index i=0; i< storage[operand.getKey()].ndim; i++) {
        storage[this->key].stride[i] = storage[operand.getKey()].stride[i];
    }
    storage[this->key].data        = NULL;

    std::cout << "[array-copy: " << this->key << "]" << std::endl;
}

template <typename T>       // "Vector-like" constructor
multi_array<T>::multi_array(unsigned int n)
{
    init();

    std::cout << "[array-1d: " << this->key << "]" << std::endl;
    storage[this->key].base        = NULL;
    storage[this->key].ndim        = 1;
    storage[this->key].start       = 0;
    storage[this->key].shape[0]    = n;
    storage[this->key].stride[0]   = 1;
    storage[this->key].data        = NULL;
}


template <typename T>       // "Matrix-like" constructor
multi_array<T>::multi_array(unsigned int m, unsigned int n)
{
    init();

    std::cout << "[array-2d: " << this->key << "]" << std::endl;
    storage[this->key].base        = NULL;
    storage[this->key].ndim        = 2;
    storage[this->key].start       = 0;
    storage[this->key].shape[0]    = m;
    storage[this->key].stride[0]   = n;
    storage[this->key].shape[1]    = n;
    storage[this->key].stride[1]   = 1;
    storage[this->key].data        = NULL;
}

template <typename T>       // "Matrix-like" constructor
multi_array<T>::multi_array(unsigned int d2, unsigned int d1, unsigned int d0)
{
    init();

    std::cout << "[array-3d: " << this->key << "]" << std::endl;
    storage[this->key].base        = NULL;
    storage[this->key].ndim        = 3;
    storage[this->key].start       = 0;
    storage[this->key].shape[0]    = d2;
    storage[this->key].stride[0]   = d1;
    storage[this->key].shape[1]    = d1;
    storage[this->key].stride[1]   = d0;
    storage[this->key].shape[2]    = d0;
    storage[this->key].stride[2]   = 1;
    storage[this->key].data        = NULL;
}


template <typename T>       // Deconstructor
multi_array<T>::~multi_array()
{
    std::cout << "De-allocating=" << this->key << std::endl;
    Runtime::instance()->enqueue((bh_opcode)BH_FREE, *this);
    Runtime::instance()->enqueue((bh_opcode)BH_DISCARD, *this);
}

template <typename T>
unsigned int multi_array<T>::getKey() const
{
    return this->key;
}

template <typename T>
bool multi_array<T>::isTemp() const
{
    return is_temp;
}

template <typename T>
void multi_array<T>::setTemp(bool is_temp)
{
    is_temp = is_temp;
}

template <typename T>
typename multi_array<T>::iterator multi_array<T>::begin()
{
    Runtime::instance()->enqueue((bh_opcode)BH_SYNC, *this);
    Runtime::instance()->flush();

    return multi_array<T>::iterator(storage[this->key]);
}

template <typename T>
typename multi_array<T>::iterator multi_array<T>::end()
{
    return multi_array<T>::iterator();
}

template <typename T>
multi_array<T>& multi_array<T>::operator++()
{
    Runtime::instance()->enqueue((bh_opcode)BH_ADD, *this, *this, (T)1);
    return *this;
}

template <typename T>
multi_array<T>& multi_array<T>::operator++(int)
{
    Runtime::instance()->enqueue((bh_opcode)BH_ADD, *this, *this, (T)1);
    return *this;
}

template <typename T>
multi_array<T>& multi_array<T>::operator--()
{
    Runtime::instance()->enqueue((bh_opcode)BH_SUBTRACT, *this, *this, (T)1);
    return *this;
}

template <typename T>
multi_array<T>& multi_array<T>::operator--(int)
{
    Runtime::instance()->enqueue((bh_opcode)BH_SUBTRACT, *this, *this, (T)1);
    return *this;
}

}
