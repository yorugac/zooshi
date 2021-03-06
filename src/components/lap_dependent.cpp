// Copyright 2015 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "components/lap_dependent.h"

#include "components/rail_denizen.h"
#include "components/services.h"
#include "corgi_component_library/physics.h"
#include "corgi_component_library/rendermesh.h"

CORGI_DEFINE_COMPONENT(fpl::zooshi::LapDependentComponent,
                       fpl::zooshi::LapDependentData)

namespace fpl {
namespace zooshi {

using scene_lab::SceneLab;
using corgi::component_library::PhysicsComponent;
using corgi::component_library::RenderMeshComponent;

void LapDependentComponent::Init() {
  auto services = entity_manager_->GetComponent<ServicesComponent>();
  // Scene Lab is not guaranteed to be present in all versions of the game.
  // Only set up callbacks if we actually have a Scene Lab.
  SceneLab* scene_lab = services->scene_lab();
  if (scene_lab) {
    scene_lab->AddOnEnterEditorCallback([this]() { ActivateAllEntities(); });
    scene_lab->AddOnExitEditorCallback([this]() { DeactivateAllEntities(); });
  }
}

void LapDependentComponent::AddFromRawData(corgi::EntityRef& entity,
                                           const void* raw_data) {
  auto lap_dependent_def = static_cast<const LapDependentDef*>(raw_data);
  LapDependentData* lap_dependent_data = AddEntity(entity);
  lap_dependent_data->min_lap = lap_dependent_def->min_lap();
  lap_dependent_data->max_lap = lap_dependent_def->max_lap();
}

corgi::ComponentInterface::RawDataUniquePtr
LapDependentComponent::ExportRawData(const corgi::EntityRef& entity) const {
  const LapDependentData* data = GetComponentData(entity);
  if (data == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;
  LapDependentDefBuilder builder(fbb);
  builder.add_min_lap(data->min_lap);
  builder.add_max_lap(data->max_lap);

  fbb.Finish(builder.Finish());
  return fbb.ReleaseBufferPointer();
}

void LapDependentComponent::InitEntity(corgi::EntityRef& /*entity*/) {}

void LapDependentComponent::UpdateAllEntities(corgi::WorldTime /*delta_time*/) {
  corgi::EntityRef raft =
      entity_manager_->GetComponent<ServicesComponent>()->raft_entity();
  if (!raft) return;
  RailDenizenData* raft_rail_denizen = Data<RailDenizenData>(raft);
  float lap = raft_rail_denizen != nullptr
                  ? raft_rail_denizen->total_lap_progress
                  : 0.0f;
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    LapDependentData* data = GetComponentData(iter->entity);
    if (lap >= data->min_lap && lap <= data->max_lap) {
      if (data->currently_active) continue;
      data->currently_active = true;
      auto rm_component = entity_manager_->GetComponent<RenderMeshComponent>();
      if (rm_component) {
        rm_component->SetVisibilityRecursively(iter->entity, true);
      }
      auto phys_component = entity_manager_->GetComponent<PhysicsComponent>();
      if (phys_component) {
        phys_component->EnablePhysics(iter->entity);
      }
    } else if (data->currently_active) {
      data->currently_active = false;
      auto rm_component = entity_manager_->GetComponent<RenderMeshComponent>();
      if (rm_component) {
        rm_component->SetVisibilityRecursively(iter->entity, false);
      }
      auto phys_component = entity_manager_->GetComponent<PhysicsComponent>();
      if (phys_component) {
        phys_component->DisablePhysics(iter->entity);
      }
    }
  }
}

void LapDependentComponent::ActivateAllEntities() {
  // Make sure all entities are activated and visible.
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    ActivateEntity(iter->entity);
  }
}

void LapDependentComponent::DeactivateAllEntities() {
  // Deactivate them all, as they reactivate during update.
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    DeactivateEntity(iter->entity);
  }
}

void LapDependentComponent::ActivateEntity(corgi::EntityRef& entity) {
  auto data = GetComponentData(entity);
  if (!data) return;

  data->currently_active = true;
  auto rm_component = entity_manager_->GetComponent<RenderMeshComponent>();
  if (rm_component) {
    rm_component->SetVisibilityRecursively(entity, true);
  }
  auto phys_component = entity_manager_->GetComponent<PhysicsComponent>();
  if (phys_component) {
    phys_component->EnablePhysics(entity);
  }
}

void LapDependentComponent::DeactivateEntity(corgi::EntityRef& entity) {
  auto data = GetComponentData(entity);
  if (!data) return;

  data->currently_active = false;
  auto rm_component = entity_manager_->GetComponent<RenderMeshComponent>();
  if (rm_component) {
    rm_component->SetVisibilityRecursively(entity, false);
  }
  auto phys_component = entity_manager_->GetComponent<PhysicsComponent>();
  if (phys_component) {
    phys_component->DisablePhysics(entity);
  }
}

}  // zooshi
}  // fpl
