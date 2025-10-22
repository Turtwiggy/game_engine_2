#include "pch.hpp"

#include "model_animation_components.hpp"

namespace game2d {

Bone
create_bone(const std::string& name, int id, const aiNodeAnim* channel)
{
  Bone b;
  b.name = name;
  b.id = id;

  for (int i = 0; i < channel->mNumPositionKeys; i++) {
    const auto aiPosition = channel->mPositionKeys[i].mValue;
    const auto timestamp = channel->mPositionKeys[i].mTime;
    KeyPosition pos;
    pos.position.x = aiPosition.x;
    pos.position.y = aiPosition.y;
    pos.position.z = aiPosition.z;
    pos.timestamp = timestamp;
    b.positions.push_back(pos);
  }

  for (int i = 0; i < channel->mNumRotationKeys; i++) {
    const auto aiOrientation = channel->mRotationKeys[i].mValue;
    const auto timestamp = channel->mRotationKeys[i].mTime;
    KeyRotation data;
    data.orientation.x = aiOrientation.x;
    data.orientation.y = aiOrientation.y;
    data.orientation.z = aiOrientation.z;
    data.orientation.w = aiOrientation.w;
    data.timestamp = timestamp;
    b.rotations.push_back(data);
  }

  for (int i = 0; i < channel->mNumScalingKeys; i++) {
    const auto scale = channel->mScalingKeys[i].mValue;
    const auto timestamp = channel->mScalingKeys[i].mTime;
    KeyScale data;
    data.scale.x = scale.x;
    data.scale.y = scale.y;
    data.scale.z = scale.z;
    data.timestamp = timestamp;
    b.scales.push_back(data);
  }

  return b;
};

int
get_position_index(const Bone& b, float time)
{
  for (int i = 0; i < b.positions.size() - 1; ++i)
    if (time < b.positions[i + 1].timestamp)
      return i;
  return 0;
};

int
get_rotation_index(const Bone& b, float time)
{
  for (int i = 0; i < b.rotations.size() - 1; ++i)
    if (time < b.rotations[i + 1].timestamp)
      return i;
  return 0;
};

int
get_scale_index(const Bone& b, float time)
{
  for (int i = 0; i < b.scales.size() - 1; ++i)
    if (time < b.scales[i + 1].timestamp)
      return i;
  return 0;
};

// Gets normalized value for Lerp & Slerp
float
get_scale_factor(float last, float next, float time)
{
  float scaleFactor = 0.0f;
  float midWayLength = time - last;
  float framesDiff = next - last;
  scaleFactor = midWayLength / framesDiff;
  return scaleFactor;
};

/*figures out which position keys to interpolate b/w and performs the interpolation
and returns the translation matrix*/
glm::mat4
interpolate_position(const Bone& b, float time)
{
  if (b.positions.size() == 1)
    return glm::translate(glm::mat4(1.0f), b.positions[0].position);

  const int p0 = get_position_index(b, time);
  const int p1 = p0 + 1;
  const auto& b0 = b.positions[p0];
  const auto& b1 = b.positions[p1];
  const float t = get_scale_factor(b0.timestamp, b1.timestamp, time);
  const glm::vec3 pos = glm::mix(b0.position, b1.position, t);
  return glm::translate(glm::mat4(1.0f), pos);
};

glm::mat4
interpolate_rotation(const Bone& b, float time)
{
  if (b.rotations.size() == 1) {
    const auto rotation = glm::normalize(b.rotations[0].orientation);
    return glm::toMat4(rotation);
  }

  const int p0 = get_rotation_index(b, time);
  const int p1 = p0 + 1;
  const auto& b0 = b.rotations[p0];
  const auto& b1 = b.rotations[p1];
  const float t = get_scale_factor(b0.timestamp, b1.timestamp, time);
  auto rot = glm::slerp(b0.orientation, b1.orientation, t);
  rot = glm::normalize(rot);
  return glm::toMat4(rot);
};

glm::mat4
interpolate_scale(const Bone& b, float time)
{
  if (b.scales.size() == 1)
    return glm::scale(glm::mat4(1.0f), b.scales[0].scale);

  const int p0 = get_scale_index(b, time);
  const int p1 = p0 + 1;
  const auto& b0 = b.scales[p0];
  const auto& b1 = b.scales[p1];
  const auto t = get_scale_factor(b0.timestamp, b1.timestamp, time);
  const auto scale = glm::mix(b0.scale, b1.scale, t);
  return glm::scale(glm::mat4(1.0f), scale);
};

void
update_bone(Bone& b, float time)
{
  const glm::mat4 translation = interpolate_position(b, time);
  const glm::mat4 rotation = interpolate_rotation(b, time);
  const glm::mat4 scale = interpolate_scale(b, time);
  b.local_transform = translation * rotation * scale;
};

void
read_hierarchy_data(AssimpNodeData& dest, const aiNode* src)
{
  assert(src);

  dest.name = src->mName.data;

  // convert from aiMatrix4x4 to glm::mat4
  const auto from = src->mTransformation;
  auto& to = dest.transformation;

  // convert from aiMatrix4x4 to glm::mat4
  auto assimp_mat4_to_glm_mat4 = [](const aiMatrix4x4& from) -> glm::mat4 {
    glm::mat4 to;
    // clang-format off
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    // clang-format on
    return to;
  };
  to = assimp_mat4_to_glm_mat4(from);

  for (int i = 0; i < src->mNumChildren; i++) {
    AssimpNodeData new_data;
    read_hierarchy_data(new_data, src->mChildren[i]);
    dest.children.push_back(new_data);
  }
};

void
read_missing_bones(Animation& a, const aiAnimation* anim, const Model& m)
{
  const auto& bone_info = m.bone_info; // get bone info from model class
  const auto bone_count = m.bone_info.size();
  SDL_Log("When loading animation, model has %zu bones", bone_count);

  for (int i = 0; i < anim->mNumChannels; i++) {
    const auto channel = anim->mChannels[i];
    const std::string name = channel->mNodeName.data;

    const auto bone =
      std::find_if(bone_info.begin(), bone_info.end(), [&name](const BoneInfo& b) { return b.name == name; });

    if (bone == bone_info.end()) {
      SDL_Log("(WARNING) Bone: %s missing from model?", name.c_str());
      continue;
    }

    a.bones.push_back(create_bone(name, (*bone).id, channel));
  }

  a.bone_info = m.bone_info;
};

Animation
load_animation(const Model& model)
{
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(model.path, aiProcess_Triangulate);
  assert(scene && scene->mRootNode);

  Animation result;

  // if (scene->mNumAnimations == 0)
  //   return data; // exit

  SDL_Log("Animation: %s", scene->mAnimations[0]->mName.C_Str());
  auto anim = scene->mAnimations[0];
  result.duration = anim->mDuration;
  result.ticks = anim->mTicksPerSecond;

  read_hierarchy_data(result.root, scene->mRootNode);
  read_missing_bones(result, anim, model);

  // check animation bones & check model bones
  assert(model.bone_info.size() == result.bone_info.size());

  return result;
};

void
load_animations(SINGLE_AnimatorComponent& anims, SINGLE_ModelsComponent& models)
{
  anims.final_bone_matrices.reserve(100);
  for (int i = 0; i < 100; i++)
    anims.final_bone_matrices.push_back(glm::mat4(1.0f));
}

//
// animator
//

void
play_animation(SINGLE_AnimatorComponent& anims, Animation* a)
{
  SDL_Log("playing new animation");
  anims.current_animation = a;
  anims.current_time = 0.0f;

  anims.final_bone_matrices.clear();
  for (int i = 0; i < anims.amount_of_bones; i++)
    anims.final_bone_matrices.push_back(glm::mat4(1.0f));
}

void
calculate_bone_transforms(SINGLE_AnimatorComponent& anims, const AssimpNodeData* node, const glm::mat4& parent)
{
  const auto& node_name = node->name;
  glm::mat4 node_transform = node->transformation;
  // SDL_Log("calculate_bone_transforms: %s", node_name.c_str());

  const auto bone = std::find_if(anims.current_animation->bones.begin(),
                                 anims.current_animation->bones.end(),
                                 [&node_name](const Bone& b) { return b.name == node_name; });

  if (bone != anims.current_animation->bones.end()) {
    // SDL_Log("animation has bone,...");
    update_bone((*bone), anims.current_time);
    node_transform = bone->local_transform;
  }

  const auto global_transformation = parent * node_transform;
  // SDL_Log("first element of mat4: %f", global_transformation[0][0]);

  const auto& bone_info = anims.current_animation->bone_info;
  const auto has_bone_info =
    std::find_if(bone_info.begin(), bone_info.end(), [&node_name](const BoneInfo& bi) { return bi.name == node_name; });
  if (has_bone_info != bone_info.end()) {
    // SDL_Log("animation has bone info,...");
    const auto index = (*has_bone_info).id;
    const auto offset = (*has_bone_info).offset;
    anims.final_bone_matrices[index] = global_transformation * offset;
  }

  for (int i = 0; i < node->children.size(); i++)
    calculate_bone_transforms(anims, &node->children[i], global_transformation);
}

void
update_animation(SINGLE_AnimatorComponent& anims, float dt)
{
  if (anims.current_animation) {
    anims.current_time += anims.current_animation->ticks * dt;
    anims.current_time = fmod(anims.current_time, anims.current_animation->duration);
    // SDL_Log("animation current time: %f", anims.current_time);

    calculate_bone_transforms(anims, &anims.current_animation->root, glm::mat4(1.0f));
    int k = 1;
  }
}

} // namespace game2d