/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "coidevice_common.h"
#include <stdio.h>
#include <sink/COIPipeline_sink.h>
#include <sink/COIProcess_sink.h>
#include <sink/COIBuffer_sink.h>
#include <common/COIMacros_common.h>
#include <common/COISysInfo_common.h>
#include <common/COIEvent_common.h>
#include <iostream>
#include "handle.h"
//ospray
#include "ospray/common/model.h"
#include "ospray/common/data.h"
#include "ospray/geometry/trianglemesh.h"
#include "ospray/camera/camera.h"
#include "ospray/volume/volume.h"
#include "ospray/render/renderer.h"
#include "ospray/render/renderer.h"
#include "ospray/render/loadbalancer.h"
#include "ospray/texture/texture2d.h"
#include "ospray/lights/light.h"
// stl
#include <algorithm>

using namespace std;

namespace ospray {
  namespace coi {

    COINATIVELIBEXPORT
    void ospray_coi_initialize(uint32_t         in_BufferCount,
                               void**           in_ppBufferPointers,
                               uint64_t*        in_pBufferLengths,
                               void*            in_pMiscData,
                               uint16_t         in_MiscDataLength,
                               void*            in_pReturnValue,
                               uint16_t         in_ReturnValueLength)
    {
      UNREFERENCED_PARAM(in_BufferCount);
      UNREFERENCED_PARAM(in_ppBufferPointers);
      UNREFERENCED_PARAM(in_pBufferLengths);
      UNREFERENCED_PARAM(in_pMiscData);
      UNREFERENCED_PARAM(in_MiscDataLength);
      UNREFERENCED_PARAM(in_ReturnValueLength);
      UNREFERENCED_PARAM(in_pReturnValue);

      int32 *deviceInfo = (int32*)in_pMiscData;
      int deviceID = deviceInfo[0];
      int numDevices = deviceInfo[1];

      cout << "!osp:coi: initializing device #" << deviceID
           << " (" << (deviceID+1) << "/" << numDevices << ")" << endl;

      ospray::TiledLoadBalancer::instance = new ospray::InterleavedTiledLoadBalancer(deviceID,numDevices);
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_model(uint32_t         numBuffers,
                              void**           bufferPtr,
                              uint64_t*        bufferSize,
                              void*            argsPtr,
                              uint16_t         argsSize,
                              void*            retVal,
                              uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      // OSPModel *model = ospNewModel();
      Handle handle = args.get<Handle>();
      //      cout << "!osp:coi: new model " << handle.ID() << endl;
      Model *model = new Model;
      handle.assign(model);
      COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_trianglemesh(uint32_t         numBuffers,
                                     void**           bufferPtr,
                                     uint64_t*        bufferSize,
                                     void*            argsPtr,
                                     uint16_t         argsSize,
                                     void*            retVal,
                                     uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      Handle handle = args.get<Handle>();
      // cout << "!osp:coi: new trimesh " << handle.ID() << endl;
      TriangleMesh *mesh = new TriangleMesh;
      handle.assign(mesh);
      COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_data(uint32_t         numBuffers,
                             void**           bufferPtr,
                             uint64_t*        bufferSize,
                             void*            argsPtr,
                             uint16_t         argsSize,
                             void*            retVal,
                             uint16_t         retValSize)
    {

      DataStream args(argsPtr);
      // OSPModel *model = ospNewModel();
      Handle handle = args.get<Handle>();
      int    nitems = args.get<int32>(); 
      int    format = args.get<int32>(); 
      int    flags  = args.get<int32>(); 

      cout << "=======================================================" << endl;
      cout << "!osp:coi: new data " << handle.ID() << endl;
      PRINT(numBuffers);
      PRINT(bufferPtr);
      PRINT(bufferSize);
      PRINT(bufferPtr[0]);
      PRINT(bufferSize[0]);
      void *init = bufferPtr[0];\

      static std::vector<void *> alreadyCreated_ptr;
      static std::vector<size_t> alreadyCreated_size;
      cout << "---------------------------------" << endl;
      cout << "Already created buffers: " << alreadyCreated_ptr.size() << endl;
      for (int i=0;i<alreadyCreated_ptr.size();i++) {
        cout << "checksum data " << i << endl;
        PRINT(computeCheckSum(alreadyCreated_ptr[i],alreadyCreated_size[i]));
      }
      alreadyCreated_ptr.push_back(bufferPtr[0]);
      alreadyCreated_size.push_back(bufferSize[0]);
      
      // PRINT((int*)*(int64*)init);

      COIBufferAddRef(bufferPtr[0]);

      if (format == OSP_STRING)
        throw std::runtime_error("data arrays of strings not currently supported on coi device ...");

      if (format == OSP_OBJECT) {
        //cout << "FOUND DATA BUFFER THAT CONTAINS ACTUAL DATA OR OBJECTS - TRANSLATING HANDLES!" << endl;
        Handle *in = (Handle *)bufferPtr[0];
        ManagedObject **out = (ManagedObject **)bufferPtr[0];
        for (int i=0;i<nitems;i++) {
          if (in[i]) {
            out[i] = in[i].lookup();
            out[i]->refInc();
          } else
            out[i] = 0;
        }
        Data *data = new Data(nitems,(OSPDataType)format,
                              out,flags|OSP_DATA_SHARED_BUFFER);
        handle.assign(data);
      } else {
        Data *data = new Data(nitems,(OSPDataType)format,
                              bufferPtr[0],flags|OSP_DATA_SHARED_BUFFER);
        handle.assign(data);
      }
      COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_geometry(uint32_t         numBuffers,
                                 void**           bufferPtr,
                                 uint64_t*        bufferSize,
                                 void*            argsPtr,
                                 uint16_t         argsSize,
                                 void*            retVal,
                                 uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      // OSPModel *model = ospNewModel();
      Handle handle = args.get<Handle>();
      const char *type = args.getString();
      //      cout << "!osp:coi: new geometry " << handle.ID() << " " << type << endl;

      Geometry *geom = Geometry::createGeometry(type);
      handle.assign(geom);

      COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_framebuffer(uint32_t         numBuffers,
                                    void**           bufferPtr,
                                    uint64_t*        bufferSize,
                                    void*            argsPtr,
                                    uint16_t         argsSize,
                                    void*            retVal,
                                    uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      Handle handle = args.get<Handle>();
      vec2i size = args.get<vec2i>();
      uint32 mode = args.get<uint32>();
      uint32 channels = args.get<uint32>();

      int32 *pixelArray = (int32*)bufferPtr[0];

      FrameBuffer::ColorBufferFormat colorBufferFormat
        = (FrameBuffer::ColorBufferFormat)mode;
      bool hasDepthBuffer = (channels & OSP_FB_DEPTH)!=0;
      bool hasAccumBuffer = (channels & OSP_FB_ACCUM)!=0;
      
      FrameBuffer *fb = new LocalFrameBuffer(size,colorBufferFormat,
                                             hasDepthBuffer,hasAccumBuffer,
                                             pixelArray);
      handle.assign(fb);

      COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_framebuffer_clear(uint32_t         numBuffers,
                                      void**           bufferPtr,
                                      uint64_t*        bufferSize,
                                      void*            argsPtr,
                                      uint16_t         argsSize,
                                      void*            retVal,
                                      uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      // OSPModel *model = ospNewModel();
      Handle _fb = args.get<Handle>();
      FrameBuffer *fb = (FrameBuffer*)_fb.lookup();
      const uint32 channel = args.get<uint32>();
      fb->clear(channel);
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_camera(uint32_t         numBuffers,
                                 void**           bufferPtr,
                                 uint64_t*        bufferSize,
                                 void*            argsPtr,
                                 uint16_t         argsSize,
                                 void*            retVal,
                                 uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      // OSPModel *model = ospNewModel();
      Handle handle = args.get<Handle>();
      const char *type = args.getString();
      // cout << "!osp:coi: new camera " << handle.ID() << " " << type << endl;

      Camera *geom = Camera::createCamera(type);
      handle.assign(geom);

      COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_volume(uint32_t         numBuffers,
                                 void**           bufferPtr,
                                 uint64_t*        bufferSize,
                                 void*            argsPtr,
                                 uint16_t         argsSize,
                                 void*            retVal,
                                 uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      // OSPModel *model = ospNewModel();
      Handle handle = args.get<Handle>();
      const char *type = args.getString();
      cout << "!osp:coi: new volume " << handle.ID() << " " << type << endl;

      Volume *geom = Volume::createVolume(type);
      handle.assign(geom);

      COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_volume_from_file(uint32_t         numBuffers,
                                 void**           bufferPtr,
                                 uint64_t*        bufferSize,
                                 void*            argsPtr,
                                 uint16_t         argsSize,
                                 void*            retVal,
                                 uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      Handle handle = args.get<Handle>();
      const char *filename = args.getString();
      const char *type = args.getString();
      cout << "!osp:coi: new volume " << handle.ID() << " " << type << endl;

      Volume *geom = Volume::createVolume(filename, type);
      handle.assign(geom);

      COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_transfer_function(uint32_t         numBuffers,
                                 void**           bufferPtr,
                                 uint64_t*        bufferSize,
                                 void*            argsPtr,
                                 uint16_t         argsSize,
                                 void*            retVal,
                                 uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      Handle handle = args.get<Handle>();
      const char *type = args.getString();
      cout << "!osp:coi: new transfer function " << handle.ID() << " " << type << endl;

      TransferFunction *transferFunction = TransferFunction::createTransferFunction(type);
      handle.assign(transferFunction);

      COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_renderer(uint32_t         numBuffers,
                                 void**           bufferPtr,
                                 uint64_t*        bufferSize,
                                 void*            argsPtr,
                                 uint16_t         argsSize,
                                 void*            retVal,
                                 uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      Handle handle = args.get<Handle>();
      const char *type = args.getString();

      Renderer *geom = Renderer::createRenderer(type);
      handle.assign(geom);

      COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_material(uint32_t         numBuffers,
                                 void**           bufferPtr,
                                 uint64_t*        bufferSize,
                                 void*            argsPtr,
                                 uint16_t         argsSize,
                                 void*            retVal,
                                 uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      Handle handle = args.get<Handle>();
      Handle _renderer = args.get<Handle>();
      const char *type = args.getString();

      Renderer *renderer = (Renderer*)_renderer.lookup();
      Material *mat = NULL;
      if (renderer)
        mat = renderer->createMaterial(type);
      if (!mat)
        mat = Material::createMaterial(type);
      
      if (mat) {
        *(int*)retVal = true;
        handle.assign(mat);
      } else {
        *(int*)retVal = false;
      }

      COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_light(uint32_t         numBuffers,
                              void**           bufferPtr,
                              uint64_t*        bufferSize,
                              void*            argsPtr,
                              uint16_t         argsSize,
                              void*            retVal,
                              uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      Handle handle = args.get<Handle>();
      Handle _renderer = args.get<Handle>();
      const char *type = args.getString();
      Renderer *renderer = (Renderer *)_renderer.lookup();

      Light *light = NULL;
      COIProcessProxyFlush(); 

      if (renderer)
        light = renderer->createLight(type);
      if (!light)
        light = Light::createLight(type);
      assert(light);
      handle.assign(light);

      COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_texture2d(uint32_t         numBuffers,
                                  void**           bufferPtr,
                                  uint64_t*        bufferSize,
                                  void*            argsPtr,
                                  uint16_t         argsSize,
                                  void*            retVal,
                                  uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      Handle handle = args.get<Handle>();
      int    width  = args.get<int32>(); 
      int    height = args.get<int32>(); 
      int    type   = args.get<int32>(); 
      int    flags  = args.get<int32>(); 

      COIBufferAddRef(bufferPtr[0]);

      Texture2D *tx = Texture2D::createTexture(width, height, (OSPDataType)type, bufferPtr[0], flags);

      handle.assign(tx);
      COIProcessProxyFlush();
    }
                                
    COINATIVELIBEXPORT
    void ospray_coi_add_geometry(uint32_t         numBuffers,
                                 void**           bufferPtr,
                                 uint64_t*        bufferSize,
                                 void*            argsPtr,
                                 uint16_t         argsSize,
                                 void*            retVal,
                                 uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      Handle model = args.get<Handle>();
      Handle geom  = args.get<Handle>();

      Model *m = (Model*)model.lookup();
      m->geometry.push_back((Geometry*)geom.lookup());
      COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_set_material(uint32_t         numBuffers,
                                 void**           bufferPtr,
                                 uint64_t*        bufferSize,
                                 void*            argsPtr,
                                 uint16_t         argsSize,
                                 void*            retVal,
                                 uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      Handle geom = args.get<Handle>();
      Handle mat  = args.get<Handle>();

      Geometry *g = (Geometry*)geom.lookup();
      g->setMaterial((Material*)mat.lookup());
      COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_commit(uint32_t         numBuffers,
                           void**           bufferPtr,
                           uint64_t*        bufferSize,
                           void*            argsPtr,
                           uint16_t         argsSize,
                           void*            retVal,
                           uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      Handle handle = args.get<Handle>();

      ManagedObject *obj = handle.lookup();
      Assert(obj);
      obj->commit();

      // hack, to stay compatible with earlier version
      Model *model = dynamic_cast<Model *>(obj);
      if (model) {
        model->finalize();
      }

      COIProcessProxyFlush();
    }

    /*! remove an existing geometry from a model */
    struct GeometryLocator {
      bool operator()(const embree::Ref<ospray::Geometry> &g) const {
        return ptr == &*g;
      }
      Geometry *ptr;
    };

    COINATIVELIBEXPORT
    void ospray_coi_remove_geometry(uint32_t         numBuffers,
                                    void**           bufferPtr,
                                    uint64_t*        bufferSize,
                                    void*            argsPtr,
                                    uint16_t         argsSize,
                                    void*            retVal,
                                    uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      Handle _model = args.get<Handle>();
      Handle _geometry = args.get<Handle>();

      Model *model = (Model*)_model.lookup();
      Geometry *geometry = (Geometry*)_geometry.lookup();

      GeometryLocator locator;
      locator.ptr = geometry;
      Model::GeometryVector::iterator it = std::find_if(model->geometry.begin(), model->geometry.end(), locator);
      if(it != model->geometry.end()) {
        model->geometry.erase(it);
      }

      COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_render_frame(uint32_t         numBuffers,
                                 void**           bufferPtr,
                                 uint64_t*        bufferSize,
                                 void*            argsPtr,
                                 uint16_t         argsSize,
                                 void*            retVal,
                                 uint16_t         retValSize)
    {
      // PING;
      // COIProcessProxyFlush();

      DataStream args(argsPtr);
      Handle _fb       = args.get<Handle>();
      Handle renderer = args.get<Handle>();
      uint32 channelFlags = args.get<uint32>();
      // cout << "!osp:coi: renderframe " << _fb.ID() << " " << renderer.ID() << endl;
      FrameBuffer *fb = (FrameBuffer*)_fb.lookup();
      Renderer *r = (Renderer*)renderer.lookup();
      r->renderFrame(fb,channelFlags);
      // if (fb->renderTask) {
      //   fb->renderTask->done.sync();
      //   fb->renderTask = NULL;
      // }

      // PING;
      // COIProcessProxyFlush();

    }

    COINATIVELIBEXPORT
    void ospray_coi_render_frame_sync(uint32_t         numBuffers,
                                      void**           bufferPtr,
                                      uint64_t*        bufferSize,
                                      void*            argsPtr,
                                      uint16_t         argsSize,
                                      void*            retVal,
                                      uint16_t         retValSize)
    {
      // currently all rendering is synchronous, anyway ....

      // DataStream args(argsPtr);
      // Handle _fb       = args.get<Handle>();
      // FrameBuffer *fb = (FrameBuffer*)_fb.lookup();
      // if (fb->renderTask) {
      //   fb->renderTask->done.sync();
      //   fb->renderTask = NULL;
      // }
    }

    COINATIVELIBEXPORT
    void ospray_coi_set_value(uint32_t         numBuffers,
                              void**           bufferPtr,
                              uint64_t*        bufferSize,
                              void*            argsPtr,
                              uint16_t         argsSize,
                              void*            retVal,
                              uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      Handle target = args.get<Handle>();
      const char *name = args.getString();
      ManagedObject *obj = target.lookup();
      if (!obj) return;
    
      Assert(obj);

      OSPDataType type = args.get<OSPDataType>();

      switch (type) {
      case OSP_INT : obj->set(name,args.get<int32>()); break;
      case OSP_INT2: obj->set(name,args.get<vec2i>()); break;
      case OSP_INT3: obj->set(name,args.get<vec3i>()); break;
      case OSP_INT4: obj->set(name,args.get<vec4i>()); break;

      case OSP_UINT : obj->set(name,args.get<uint32>()); break;
      case OSP_UINT2: obj->set(name,args.get<vec2ui>()); break;
      case OSP_UINT3: obj->set(name,args.get<vec3ui>()); break;
      case OSP_UINT4: obj->set(name,args.get<vec4ui>()); break;

      case OSP_FLOAT : obj->set(name,args.get<float>()); break;
      case OSP_FLOAT2: obj->set(name,args.get<vec2f>()); break;
      case OSP_FLOAT3: obj->set(name,args.get<vec3f>()); break;
      case OSP_FLOAT4: obj->set(name,args.get<vec4f>()); break;
        
      case OSP_STRING: obj->set(name,args.getString()); break;
      case OSP_OBJECT: obj->set(name,args.get<Handle>().lookup()); break;

      default:
        throw "ospray_coi_set_value no timplemented for given data type";
      }
    }
  }
}

int main(int ac, char **av)
{
  printf("!osp:coi: ospray_coi_worker starting up.\n");

  // initialize embree. (we need to do this here rather than in
  // ospray::init() because in mpi-mode the latter is also called
  // in the host-stubs, where it shouldn't.
  std::stringstream embreeConfig;
  rtcInit("verbose=2");
  //rtcInit("verbose=2,traverser=single,threads=1");
  //rtcInit("verbose=2,traverser=chunk");

  //rtcInit(NULL);

  assert(rtcGetError() == RTC_NO_ERROR);
  ospray::TiledLoadBalancer::instance = NULL;

  COIPipelineStartExecutingRunFunctions();
  COIProcessProxyFlush();
  COIProcessWaitForShutdown();
}
  
