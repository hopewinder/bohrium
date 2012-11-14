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
#include "cphvb_vem_cluster.h"
#include "dispatch.h"
#include "pgrid.h"
#include "exec.h"
        
int main()
{
    dispatch_msg *msg;
    cphvb_error e;
    
    //Initiate the process grid
    if((e = pgrid_init()) != CPHVB_SUCCESS)
        return e;

    //Initiate the dispatch system
    if((e = dispatch_init()) != CPHVB_SUCCESS)
        return e;

    while(1)
    {
        if((e = dispatch_recv(&msg)) != CPHVB_SUCCESS)
            return e;

        switch(msg->type) 
        {
            case CPHVB_CLUSTER_DISPATCH_INIT:
            {
                printf("Slave (rank %d) received CPHVB_CLUSTER_DISPATCH_INIT\n", pgrid_myrank);
                char *name = msg->payload;
                printf("name: %s\n",name);
                if((e = exec_init(name)) != CPHVB_SUCCESS)
                    return e;
                break;
            }
            case CPHVB_CLUSTER_DISPATCH_SHUTDOWN:
                printf("Slave (rank %d) received CPHVB_CLUSTER_DISPATCH_SHUTDOWN\n",pgrid_myrank);
                return exec_shutdown(); 
            default:
                fprintf(stderr, "[VEM-CLUSTER] Slave (rank %d) "
                        "received unknown message type\n", pgrid_myrank);
                return CPHVB_ERROR; 
        }
    }
    return CPHVB_SUCCESS; 
}
