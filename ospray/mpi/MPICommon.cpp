// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "MPICommon.h"

namespace ospray {
  namespace mpi {
    
    void init(int *ac, const char **av)
    {
      int initialized = false;
      MPI_CALL(Initialized(&initialized));
      
      if (!initialized) {
        // MPI_Init(ac,(char ***)&av);
        int required = MPI_THREAD_MULTIPLE;
        int provided = 0;
        MPI_CALL(Init_thread(ac,(char ***)&av,required,&provided));
        if (provided != required)
          throw std::runtime_error("MPI implementation does not offer multi-threading capabilities");
      }
      world.comm = MPI_COMM_WORLD;
      MPI_CALL(Comm_rank(MPI_COMM_WORLD,&world.rank));
      MPI_CALL(Comm_size(MPI_COMM_WORLD,&world.size));
    }

  } // ::ospray::mpi
} // ::ospray
