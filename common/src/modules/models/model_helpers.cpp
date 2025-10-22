#include "pch.hpp"

#include "model_helpers.hpp"

#include "model_components.hpp"
#include "modules/maths/vec.hpp"
#include "modules/renderer/renderer_helpers.hpp"
#include "modules/sdl/sdl_exception.hpp"
#include <SDL3/SDL_gpu.h>
#include <cstdint>

namespace game2d {

// vertex_data = {
// quad
// { -0.5f, -0.5f, 0.0f, /*uv*/ 0.0f, 0.0f }, //
// { 0.5f, -0.5f, 0.0f, /*uv*/ 1.0f, 0.0f },  //
// { 0.5f, 0.5f, 0.0f, /*uv*/ 1.0f, 1.0f },   //
// { -0.5f, 0.5f, 0.0f, /*uv*/ 0.0f, 1.0f },  //
// };
// index_data = {
// { 0 }, { 1 }, { 2 }, { 0 }, { 2 }, { 3 },
// };

// cube
// vertex_data = {
//   { -1.0f, -1.0f, 1.0f, 0.0f, 0.0f },  // 0 - front bottom left
//   { 1.0f, -1.0f, 1.0f, 1.0f, 0.0f },   // 1 - front bottom right
//   { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },    // 2 - front top right
//   { -1.0f, 1.0f, 1.0f, 0.0f, 1.0f },   // 3 - front top left
//   { -1.0f, -1.0f, -1.0f, 1.0f, 0.0f }, // 4 - back bottom left
//   { 1.0f, -1.0f, -1.0f, 0.0f, 0.0f },  // 5 - back bottom right
//   { 1.0f, 1.0f, -1.0f, 0.0f, 1.0f },   // 6 - back top right
//   { -1.0f, 1.0f, -1.0f, 1.0f, 1.0f }   // 7 - back top left
// };
// index_data = {
//   { 0 }, { 1 }, { 2 }, { 2 }, { 3 }, { 0 }, //
//   { 5 }, { 4 }, { 7 }, { 7 }, { 6 }, { 5 }, //
//   { 3 }, { 2 }, { 6 }, { 6 }, { 7 }, { 3 }, //
//   { 4 }, { 0 }, { 3 }, { 3 }, { 7 }, { 4 }, //
//   { 1 }, { 5 }, { 6 }, { 6 }, { 2 }, { 1 }, //
//   { 4 }, { 0 }, { 3 }, { 3 }, { 7 }, { 4 }  //
// };

glm::mat4
assimp_mat4_to_glm_mat4(const aiMatrix4x4& from)
{
  glm::mat4 to;
  // clang-format off
  to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
  to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
  to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
  to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
  // clang-format on
  return to;
}

void
set_vertex_bonedata_to_default(MeshVertex& v)
{
  for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
    v.bone_ids[i] = -1;
    v.weights[i] = 0.0f;
  }
};

void
set_vertex_bonedata(MeshVertex& v, int bone_id, float weight)
{
  for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
    if (v.bone_ids[i] < 0) { // not yet set
      v.bone_ids[i] = bone_id;
      v.weights[i] = weight;
      return;
    }
  }
  SDL_Log("No more influence bones available.");
};

void
extract_bone_weight_for_vertices(Model& model, std::vector<MeshVertex>& vertices, aiMesh* mesh, const aiScene* scene)
{
  auto& bone_info = model.bone_info;
  int bone_counter = 0;

  for (uint32_t i = 0; i < mesh->mNumBones; i++) {
    int bone_id = -1;
    aiBone* bone = mesh->mBones[i];
    std::string bone_name = bone->mName.C_Str();

    auto exiting_bone =
      std::find_if(bone_info.begin(), bone_info.end(), [&bone_name](const BoneInfo& b) { return b.name == bone_name; });

    if (exiting_bone == bone_info.end()) {
      BoneInfo new_bone_info;
      new_bone_info.name = bone_name;
      new_bone_info.id = bone_counter; // generate an id
      new_bone_info.offset = assimp_mat4_to_glm_mat4(bone->mOffsetMatrix);
      bone_info.push_back(new_bone_info);
      bone_id = bone_counter;
      bone_counter++;
    } else {
      bone_id = (*exiting_bone).id;
    }
    assert(bone_id != -1);

    int count = 0;
    auto weights = mesh->mBones[i]->mWeights;
    auto n_weights = mesh->mBones[i]->mNumWeights;
    // if (n_weights == 0)
    //   SDL_Log("Bone %s has no weights for mesh: %s", bone_name.c_str(), mesh->mName.C_Str());

    for (int i = 0; i < n_weights; i++) {
      auto vertex_id = weights[i].mVertexId;
      auto weight = weights[i].mWeight;
      assert(vertex_id <= vertices.size());

      if (weight != 0)
        count++;

      // SDL_Log("Bone %s influencing vertex: %d, weight: %f", bone_name.c_str(), vertex_id, weight);
      set_vertex_bonedata(vertices[vertex_id], bone_id, weight);
    }
    if (count > 0)
      SDL_Log(
        "bone: %s, influencing %d/%d verts in mesh %s ", bone_name.c_str(), count, mesh->mNumVertices, mesh->mName.C_Str());
  }
}

Mesh
process_mesh(Model& model, aiMesh* mesh, const aiScene* scene)
{
  std::vector<MeshVertex> vertices;
  std::vector<MeshIndex> indices;
  // std::vector<std::string> textures;

  // verts
  for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
    MeshVertex v;
    set_vertex_bonedata_to_default(v);

    v.position.x = mesh->mVertices[i].x;
    v.position.y = mesh->mVertices[i].y;
    v.position.z = mesh->mVertices[i].z;

    // if (mesh->HasNormals()) {
    //   v.normal.x = mesh->mNormals[i].x;
    //   v.normal.y = mesh->mNormals[i].y;
    //   v.normal.z = mesh->mNormals[i].z;
    // }

    if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
    {
      v.uv.x = mesh->mTextureCoords[0][i].x;
      v.uv.y = mesh->mTextureCoords[0][i].y;
    } else
      v.uv = glm::vec2(0.0f, 0.0f);

    // if (mesh->mMaterialIndex >= 0) {
    //   aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
    //   aiColor3D color(0.f, 0.f, 0.f);
    //   mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
    //   v.colour.r = color.r;
    //   v.colour.g = color.g;
    //   v.colour.b = color.b;
    // }

    vertices.push_back(v);
  }

  // indices
  for (int i = 0; i < mesh->mNumFaces; i++) {
    aiFace face = mesh->mFaces[i];
    for (auto j = 0; j < face.mNumIndices; j++)
      indices.push_back((MeshIndex)face.mIndices[j]);
  }

  // Materials
  if (mesh->mMaterialIndex >= 0) {
    aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

    // aiColor3D color(0.f, 0.f, 0.f);
    // mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
    // v.colour.r = color.r;
    // v.colour.g = color.g;
    // v.colour.b = color.b;

    // const auto get_texture_paths = [&mat](const aiTextureType& type) -> std::vector<std::string> {
    //   std::vector<std::string> result;
    //   for (int i = 0; i < mat->GetTextureCount(type); i++) {
    //     aiString str;
    //     mat->GetTexture(type, i, &str);
    //     result.push_back(str.C_Str());
    //   }
    //   return result;
    // };
    // auto base = get_texture_paths(aiTextureType_BASE_COLOR);
    // textures = textures.insert(textures.end(), base.begin(), base.end());
    // textures = get_texture_paths(aiTextureType_DIFFUSE);
    // textures = get_texture_paths(aiTextureType_SPECULAR);
  }

  // Animations
  // Each aiBone contains the information like how much influence
  // this bone will have on a set of vertices on the mesh.
  extract_bone_weight_for_vertices(model, vertices, mesh, scene);

  return Mesh(std::move(vertices), std::move(indices));
}

void
process_node(Model& model, aiNode* node, const aiScene* scene)
{
  for (int i = 0; i < node->mNumMeshes; i++) {
    aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    model.meshes.push_back(std::move(process_mesh(model, mesh, scene)));
  }
  for (int i = 0; i < node->mNumChildren; i++)
    process_node(model, node->mChildren[i], scene);
};

Model
load_model(SDL_GPUDevice* device, const std::string path)
{
  SDL_Log("Loading model... %s", path.c_str());
  auto start = std::chrono::high_resolution_clock::now();

  Assimp::Importer importer;
  const auto* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
  if (!scene)
    throw std::runtime_error("failed to load model: " + path);
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  SDL_Log("Model loaded in %zu ms.", duration);

  // todo: replace this to support multiple models
  aiNode* root = scene->mRootNode;
  Model model;
  model.path = path;
  process_node(model, root, scene);

  //
  // now the model is loaded, do the sdl3 stuff.
  //

  for (auto& mesh : model.meshes) {

    auto& vertex_data = mesh.vertex_data;
    auto& index_data = mesh.index_data;
    const Uint32 vertex_data_mem_size = sizeof(MeshVertex) * vertex_data.size();
    const Uint32 index_data_mem_size = sizeof(MeshIndex) * index_data.size();

    // Create the vertex buffer
    const auto vertex_buffer_info = SDL_GPUBufferCreateInfo{
      .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
      .size = vertex_data_mem_size,
    };
    mesh.vertex_buffer = SDL_CreateGPUBuffer(device, &vertex_buffer_info);
    if (!mesh.vertex_buffer)
      throw SDLException("Failed to create GpuBuffer");
    SDL_SetGPUBufferName(device, mesh.vertex_buffer, "VertexBuffer");

    // Create an index buffer
    const auto index_buffer_info = SDL_GPUBufferCreateInfo{
      .usage = SDL_GPU_BUFFERUSAGE_INDEX,
      .size = index_data_mem_size,
    };
    mesh.index_buffer = SDL_CreateGPUBuffer(device, &index_buffer_info);
    if (!mesh.index_buffer)
      throw SDLException("Failed to create GpuBuffer");
    SDL_SetGPUBufferName(device, mesh.index_buffer, "IndexBuffer");

    // To get data in to the vertex buffer, we have to use a transfer buffer.
    const auto transfer_buffer_info = SDL_GPUTransferBufferCreateInfo{
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = vertex_data_mem_size + index_data_mem_size,
    };

    // Map the buffer in to cpu memory
    auto* transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_info);
    if (!transfer_buffer)
      throw SDLException("Unable to SDL_CreateGPUTransferBuffer()");

    // Copy the data in
    auto* transfer_data = (Uint8*)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    std::span vertex_buffer_data = { reinterpret_cast<MeshVertex*>(transfer_data), vertex_data.size() };
    std::ranges::copy(vertex_data, vertex_buffer_data.begin());
    std::span index_buffer_data = { reinterpret_cast<MeshIndex*>(transfer_data + vertex_data_mem_size), index_data.size() };
    std::ranges::copy(index_data, index_buffer_data.begin());
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer); // note: need to unmap before aquire gpu command

    // Upload the vertex_buffer and index_buffer
    SDL_GPUCommandBuffer* upload_cmd_buf = SDL_AcquireGPUCommandBuffer(device);
    if (upload_cmd_buf == nullptr)
      throw SDLException("Could not aquire GPU command buffer");
    SDL_GPUCopyPass* upload_copy_pass = SDL_BeginGPUCopyPass(upload_cmd_buf);
    {
      auto src = SDL_GPUTransferBufferLocation{ .transfer_buffer = transfer_buffer, .offset = 0 };
      auto dst = SDL_GPUBufferRegion{ .buffer = mesh.vertex_buffer, .size = vertex_buffer_info.size };
      SDL_UploadToGPUBuffer(upload_copy_pass, &src, &dst, false);

      src.offset = vertex_buffer_info.size;
      dst.buffer = mesh.index_buffer;
      dst.size = index_buffer_info.size;
      SDL_UploadToGPUBuffer(upload_copy_pass, &src, &dst, false);
    }
    SDL_EndGPUCopyPass(upload_copy_pass);

    const auto submit = SDL_SubmitGPUCommandBuffer(upload_cmd_buf);
    if (!submit)
      throw SDLException("Could not SDL_SubmitGPUCommandBuffer()");
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

    SDL_Log("loaded mesh with %zu vertices, %zu indices, %zu bones",
            vertex_data.size(),
            index_data.size(),
            model.bone_info.size());
  }

  return model;
};

SDL_GPUGraphicsPipeline*
create_model_pipeline(SDL_GPUDevice* device,
                      SDL_Window* window,
                      const ShaderInput& vert,
                      const ShaderInput& frag,
                      const SDL_GPUSampleCount sample_count)
{
  SDL_GPUShader* vert_shader = nullptr;
  SDL_GPUShader* frag_shader = nullptr;

  vert_shader = game2d::LoadShader(device, vert);
  if (vert_shader == NULL) {
    SDL_Log("Failed to create vert shader");
    exit(SDL_APP_FAILURE); // explode
  };

  frag_shader = game2d::LoadShader(device, frag);
  if (frag_shader == NULL) {
    SDL_Log("Failed to create frag shader");
    exit(SDL_APP_FAILURE); // explode
  };

  const std::vector<SDL_GPUColorTargetDescription> color_target_desc{ {
    .format = SDL_GetGPUSwapchainTextureFormat(device, window),
  } };

  const std::vector<SDL_GPUVertexBufferDescription> vertex_buffer_descriptions{
    {
      .slot = 0,
      .pitch = sizeof(MeshVertex),
      .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
      .instance_step_rate = 0,
    },
  };

  // Setup to match the vertex shader layout
  // clang-format off
  const std::vector<SDL_GPUVertexAttribute> vertex_attributes{
    { .location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_INT4, .offset = offsetof(MeshVertex, bone_ids) },
    { .location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset = offsetof(MeshVertex, weights) },
    { .location = 2, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset = offsetof(MeshVertex, position) },
    { .location = 3, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(MeshVertex, uv) },
    // { .location = 2, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset = offsetof(MeshVertex, colour) },
    // { .location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = offsetof(MeshVertex, normal) },
  };
  //clang-format on

  const auto format = SDL_GetGPUSwapchainTextureFormat(device, window);
  if (!SDL_GPUTextureSupportsSampleCount(device, format, sample_count)) {
    SDL_Log("Sample count %d not supported", (1 << sample_count));
    exit(SDL_APP_FAILURE); // explode
  }
  SDL_Log("Creating 3d model pipeline with msaa: %d", (1 << sample_count));

  // Create the pipelines.
  SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
    .vertex_shader = vert_shader,
    .fragment_shader = frag_shader,
    .vertex_input_state = SDL_GPUVertexInputState{
      .vertex_buffer_descriptions = vertex_buffer_descriptions.data(),
      .num_vertex_buffers = (Uint32)vertex_buffer_descriptions.size(),
      .vertex_attributes = vertex_attributes.data(),
    	.num_vertex_attributes = (Uint32)vertex_attributes.size(),
    },
    .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
    .multisample_state = {
      .sample_count = sample_count,
    },
    .depth_stencil_state = {
        .compare_op = SDL_GPU_COMPAREOP_LESS,
        .enable_depth_test = true,
        .enable_depth_write = true,
    },
    .target_info = { .color_target_descriptions = color_target_desc.data(),
                     .num_color_targets = (Uint32)color_target_desc.size(),
                     .depth_stencil_format = get_depth_stencil_format(device) ,
                     .has_depth_stencil_target = true,
                    },
  };

  // pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
  SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
  if (pipeline == nullptr) {
    throw SDLException("Failed to create Fill GraphicsPipeline()");
    exit(SDL_APP_FAILURE); // crash
  }

  // can release shaders after creating pipelines
  SDL_Log("Releasing shaders... be free!");
  SDL_ReleaseGPUShader(device, vert_shader);
  SDL_ReleaseGPUShader(device, frag_shader);

  return pipeline;
}

} // namespace game2d