/*
This file is part of cphVB and copyright (c) 2012 the cphVB team:
http://cphvb.bitbucket.org

cphVB is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as 
published by the Free Software Foundation, either version 3 
of the License, or (at your option) any later version.

cphVB is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the 
GNU Lesser General Public License along with cphVB. 

If not, see <http://www.gnu.org/licenses/>.
*/

#include <cphvb.h>
#include <cassert>
#include <map>
#include <StaticStore.hpp>
#include "cphvb_vem_cluster.h"
#include "dispatch.h"
#include "pgrid.h"
#include "exec.h"
#include "darray_extension.h"


//Local array information and storage
static std::map<cphvb_intp, cphvb_array*> map_dary2ary;
static std::map<cphvb_array*,darray*> map_ary2dary;
static StaticStore<darray> ary_store(512);

        
int main()
{
    dispatch_msg *msg;
    cphvb_error e;
    
    //Initiate the process grid
    if((e = pgrid_init()) != CPHVB_SUCCESS)
        return e;

    while(1)
    {
        if((e = dispatch_reset()) != CPHVB_SUCCESS)
            return e;

        if((e = dispatch_recv(&msg)) != CPHVB_SUCCESS)
            return e;

        switch(msg->type) 
        {
            case CPHVB_CLUSTER_DISPATCH_INIT:
            {
                char *name = msg->payload;
                printf("Slave (rank %d) received INIT. name: %s\n", pgrid_myrank, name);
                if((e = exec_init(name)) != CPHVB_SUCCESS)
                    return e;
                break;
            }
            case CPHVB_CLUSTER_DISPATCH_SHUTDOWN:
            {
                printf("Slave (rank %d) received SHUTDOWN\n",pgrid_myrank);
                return exec_shutdown(); 
            }
            case CPHVB_CLUSTER_DISPATCH_UFUNC:
            {
                cphvb_intp *id = (cphvb_intp *)msg->payload;
                char *fun = msg->payload+sizeof(cphvb_intp);
                printf("Slave (rank %d) received UFUNC. fun: %s, id: %ld\n",pgrid_myrank, fun, *id);
                if((e = exec_reg_func(fun, id)) != CPHVB_SUCCESS)
                    return e;
                break;
            }
            case CPHVB_CLUSTER_DISPATCH_EXEC:
            {
                //The number of instructions
                cphvb_intp *noi = (cphvb_intp *)msg->payload;                 
                //The master-instruction list
                cphvb_instruction *master_list = (cphvb_instruction *)(msg->payload+sizeof(cphvb_intp));
                //The number of new arrays
                cphvb_intp *noa = (cphvb_intp *)(master_list + *noi);
                //The list of new arrays
                darray *darys = (darray*)(noa+1); //number of new arrays

                printf("Slave (rank %d) received EXEC. noi: %ld, noa: %ld\n",pgrid_myrank, *noi, *noa);
               
                //Insert the new array into the array store and the array maps
                for(cphvb_intp i=0; i < *noa; ++i)
                {
                    darray *dary = ary_store.c_next();
                    *dary = darys[i];
                    dary->global_ary.data = NULL;//We will copy the data at a later time.
                    assert(map_dary2ary.count(dary->id) == 0);
                    assert(map_ary2dary.count(&dary->global_ary) == 0);
                    map_dary2ary[dary->id] = &dary->global_ary;
                    map_ary2dary[&dary->global_ary] = dary;
                } 

                //Update the base-array-pointers
                for(cphvb_intp i=0; i < *noa; ++i)
                {
                    cphvb_array *ary = map_dary2ary[darys[i].id];
                    if(ary->base != NULL)
                    {
                        assert(map_dary2ary.count((cphvb_intp)ary->base) == 1);
                        ary->base = map_dary2ary[(cphvb_intp)ary->base];
                    }
                }
                    
                //Create the local instruction list that reference local arrays
                cphvb_instruction *local_list = (cphvb_instruction *)malloc(*noi*sizeof(cphvb_instruction));
                if(local_list == NULL)
                    return CPHVB_OUT_OF_MEMORY;
                for(cphvb_intp i=0; i < *noi; ++i)
                {
                    cphvb_instruction *master = &master_list[i];
                    cphvb_instruction *local = &local_list[i];
                    int nop = cphvb_operands_in_instruction(master);
                    *local = *master;//Copy instruction
                    //Convert all instructon operands
                    for(cphvb_intp j=0; j<nop; ++j)
                    {   
                        assert(map_dary2ary.count((cphvb_intp)master->operand[j]) == 1);
                        local->operand[j] = map_dary2ary[(cphvb_intp)master->operand[j]];
                        assert(map_ary2dary.count(local->operand[j]) == 1);
                    }
                }



                cphvb_pprint_instr_list(local_list, *noi, "SLAVE");

//                if((e = exec_execute(count, list)) != CPHVB_SUCCESS)
//                    return e;
                free(local_list);
                break;
            }
            default:
                fprintf(stderr, "[VEM-CLUSTER] Slave (rank %d) "
                        "received unknown message type\n", pgrid_myrank);
                MPI_Abort(MPI_COMM_WORLD,CPHVB_ERROR);
        }
    }
    return CPHVB_SUCCESS; 
}
