```json
//vcpkg
{
  "name": "solvulk",
  "version-string": "0.1.0",
  "dependencies": [
    "cglm",
    "libwebp"
  ]
}
//launch.json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Launch SolVulk (MSVC)",
            "type": "cppvsdbg",         // Changed from cppdbg
            "request": "launch",
            "program": "${workspaceFolder}/build/debug/bin/SolApp.exe",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}/build/debug/bin",
            "environment": [],
            "console": "integratedTerminal",
            "preLaunchTask": "CMake: build"
        }
    ]
}
//c_cpp_properties.json
{
    "configurations": [
        {
            "name": "Win32",
            "includePath": [
                "${workspaceFolder}/**"
            ],
            "defines": [
                "_DEBUG",
                "UNICODE",
                "_UNICODE"
            ],
            "windowsSdkVersion": "10.0.26100.0",
            "compilerPath": "cl.exe",
            "cStandard": "c11",
            "cppStandard": "c++17",
            "intelliSenseMode": "windows-msvc-x64",
            "configurationProvider": "ms-vscode.cmake-tools"
        }
    ],
    "version": 4
}
```

```c
void Build_Tris(SolModel *model) {
  model_collision_tris[model->modelId]->count = model->indice_count / 3;
  model->tri_count = model->indice_count / 3;
  model->tris      = malloc(model->tri_count * sizeof(SolTri));

  uint32_t triIdx = 0;
  for (uint32_t m = 0; m < model->mesh_count; m++) {
    SolMesh *mesh = &model->meshes[m];
    for (uint32_t i = 0; i < mesh->indexCount; i += 3) {
      CollisionTri *t = &model->tris[triIdx++];

      uint32_t i0 = model->indices[mesh->indexOffset + i + 0];
      uint32_t i1 = model->indices[mesh->indexOffset + i + 1];
      uint32_t i2 = model->indices[mesh->indexOffset + i + 2];

      t->a = *(vec3s *)model->vertices[mesh->vertexOffset + i0].position;
      t->b = *(vec3s *)model->vertices[mesh->vertexOffset + i1].position;
      t->c = *(vec3s *)model->vertices[mesh->vertexOffset + i2].position;

      // Face normal from edges
      vec3s e1    = glms_vec3_sub(t->b, t->a);
      vec3s e2    = glms_vec3_sub(t->c, t->a);
      vec3s cross = glms_vec3_cross(e1, e2);
      float len   = glms_vec3_norm(cross);
      t->normal   = len > 0.00001f ? glms_vec3_scale(cross, 1.0f / len)
                                   : (vec3s){0, 1, 0};

      t->center = glms_vec3_scale(
          glms_vec3_add(glms_vec3_add(t->a, t->b), t->c), 1.0f / 3.0f);

      float da  = glms_vec3_norm(glms_vec3_sub(t->a, t->center));
      float db  = glms_vec3_norm(glms_vec3_sub(t->b, t->center));
      float dc  = glms_vec3_norm(glms_vec3_sub(t->c, t->center));
      t->bounds = fmaxf(da, fmaxf(db, dc));
    }
  }
}


void Raycast_Tri_Dynamic(World *world, SolRay ray) {
  SolRayResult result = {0};

  for (u32 r = 0; r < world->tris->range_count; r++) {
    DynamicTriRange *range = &world->tris->ranges[r];
    CompXform       *xform = &world->xforms[range->entity_id];

    // Inverse-transform ray into model-local space
    mat3s invRot = glms_quat_mat3(glms_quat_inv(xform->drawQuat));
    vec3s localOrigin =
        glms_mat3_mulv(invRot, glms_vec3_sub(result.pos, xform->drawPos));
    vec3s localDir = glms_mat3_mulv(invRot, ray.dir);

    // Test against this entity's tris
    for (u32 t = range->start; t < range->start + range->count; t++) {
      SolTri *tri = &world->tris->dynamic_tris[t];
      vec3s         localNormal;
      float dist = Ray_Tri_Test(localOrigin, localDir, tri, &localNormal);

      if (dist > 0 && dist < result.dist) {
        result.hit  = true;
        result.dist = dist;
        result.pos  = glms_vec3_add(result.pos, glms_vec3_scale(ray.dir,
        dist));
        // Transform normal back to world space
        mat3s rot       = glms_quat_mat3(xform->drawQuat);
        result.norm     = glms_mat3_mulv(rot, localNormal);
        result.triIndex = t;
      }
    }
  }
}

int required = HAS_CONTROLLER | HAS_XFORM;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompXform *xform = &world->xforms[id];
            CompController *controller = &world->controllers[id];

            vec3s pos = glms_vec3_sub(xform->pos, glms_vec3_scale(controller->lookdir, 10.0f));
            vec3s target = xform->pos;

            vec3 finalPos = {pos.x, pos.y, pos.z};
            vec3 finalTarget = {target.x, target.y, target.z};

            Render_Camera_Update(finalPos, finalTarget);
            
            // Sol_Debug_Add("CamPosX", pos.x);
            // Sol_Debug_Add("CamPosY", pos.y);
            // Sol_Debug_Add("CamPosZ", pos.z);

            break;
        }
    }

    
SolRayResult Raycast_Static_Table_Tri(PhysxGroup *group, SolRay ray)
{
    SolRayResult result = {0};

    int          checks = 0;
    SpatialCell  cell   = Spatial_Cell_Get(ray.pos, group->table.cellSize);
    SolCollision col    = {0};
    for (int c = 0; c < 27; c++)
    {
        u32 entry = group->table.head[cell.neighborHashes[c] & (group->table.size - 1)];
        while (entry != SPATIAL_NULL)
        {
            if (checks > 0x1ff)
                break;
                u32 index = group->table.value[entry];
            SolTri *tri = &group->tris[index];
            vec3s   normal;
            float   t = Ray_Tri_Test(ray.pos, ray.dir, tri, &normal);
            
            if (t > 0 && t <= result.dist)
            {
                result.dist     = t;
                result.hit      = true;
                result.dist     = t;
                result.norm     = normal;
                result.triIndex = index;
                result.pos      = glms_vec3_add(ray.pos, glms_vec3_scale(ray.dir, t));
                break;
            }
            entry  = group->table.next[entry];
        }
    }

    return result;
}

SolRayResult Raycast_Dynamic_Grid_Tri(PhysxGroup *group, SolRay ray)
{
    SolRayResult result = {0};
    return result;
}

SolRayResult Raycast_Dynamic_Table_Tri(PhysxGroup *group, SolRay ray)
{
    SolRayResult result = {0};
    return result;
}


int Sol_Descriptor_Build(SolVkState *vkstate, SolDescriptor *desc)
{

    Sol_Descriptor_Build(&vkstate, sizeof(SceneUBO), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               &descriptors[DESC_SCENE_UBO]);
    Sol_Descriptor_Build(&vkstate, sizeof(ModelSSBO) * MAX_MODEL_INSTANCES, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                               VK_SHADER_STAGE_VERTEX_BIT, &descriptors[DESC_MODEL_SSBO]);
    return 0;
}

int Sol_Descriptor_Buildall(SolVkState *vkstate)
{
    for (int i = 0; i < DESC_COUNT; i++)
        Sol_Descriptor_Build(&vkstate, &descriptors[i]);

    return 0;
}

int Sol_Pipeline_Buildall(SolVkState *vkstate)
{

    for (int i = 0; i < PIPE_COUNT; i++)
    {
        Sol_Pipeline_Build(&vkstate, &pipe_config[i]);
    }

    // ─── Text Pipeline ──────────────────────────────────────────
    SolResource metrics = Sol_LoadResource("ID_FONT_METRICS");
    if (metrics.data)
        Sol_ParseFontMetrics((const char *)metrics.data, 224.0f, 224.0f, fontGlyphs);

    SolResource fontRes = Sol_LoadResource("ID_FONT_ATLAS");
    if (!fontRes.data)
        return 1;

    if (Sol_UploadImage(vkstate, fontRes.data, 224, 224, VK_FORMAT_R8G8B8A8_UNORM, &pipeText.fontAtlas) != 0)
        return 1;

    if (Sol_CreateDescriptorImage(vkstate, pipeText.fontAtlas.view, pipeText.fontAtlas.sampler,
                                  VK_SHADER_STAGE_FRAGMENT_BIT, &pipeText.fontDesc) != 0)
        return 1;

    SolPipelineConfig textConfig = {
        .vertResource      = "ID_SHADER_TEXTV",
        .fragResource      = "ID_SHADER_TEXTF",
        .depthTest         = 0,
        .alphaBlend        = 0,
        .cullMode          = VK_CULL_MODE_NONE,
        .pushRangeSize     = sizeof(ShaderPushText),
        .pushStageFlags    = VK_SHADER_STAGE_VERTEX_BIT,
        .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    if (Sol_Pipeline_Build(vkstate, &textConfig, &pipeText.fontDesc.layout, 1, &pipeText.pipe) != 0)
        return 1;

    // ─── 3D Mesh Pipeline ───────────────────────────────────────
    if (Sol_Descriptor_Build(vkstate, sizeof(SceneUBO), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, &sceneUBO) != 0)
        return 1;

    if (Sol_Descriptor_Build(vkstate, sizeof(ModelSSBO) * MAX_MODEL_INSTANCES,
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                   VK_SHADER_STAGE_VERTEX_BIT, &pipeModel.modelSSBO) != 0)
        return 1;

    VkDescriptorSetLayout meshLayouts[] = {
        sceneUBO.layout,
        pipeModel.modelSSBO.layout,
    };

    SolPipelineConfig meshConfig = {
        .vertResource      = "ID_SHADER_MODELV",
        .fragResource      = "ID_SHADER_MODELF",
        .depthTest         = 1,
        .alphaBlend        = 1,
        .cullMode          = VK_CULL_MODE_NONE,
        .pushRangeSize     = sizeof(SolMaterial),
        .pushStageFlags    = VK_SHADER_STAGE_FRAGMENT_BIT,
        .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .type              = VERTEX_TRI,
    };

    if (Sol_Pipeline_Build(vkstate, &meshConfig, meshLayouts, 2, &pipeModel.pipe) != 0)
        return 1;

    // ─── 2D Rect Pipeline ───────────────────────────────────────
    SolPipelineConfig rectConfig = {
        .vertResource      = "ID_SHADER_RECTV",
        .fragResource      = "ID_SHADER_RECTF",
        .depthTest         = 0,
        .alphaBlend        = 1,
        .cullMode          = VK_CULL_MODE_NONE,
        .pushRangeSize     = sizeof(float) * 32,
        .pushStageFlags    = VK_SHADER_STAGE_VERTEX_BIT,
        .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    if (Sol_Pipeline_Build(vkstate, &rectConfig, NULL, 0, &pipeRect.pipe) != 0)
        return 1;

    SolPipelineConfig lineConfig = {
        .vertResource      = "ID_SHADER_LINEV",
        .fragResource      = "ID_SHADER_LINEF",
        .depthTest         = 1,
        .alphaBlend        = 1,
        .cullMode          = VK_CULL_MODE_NONE,
        .pushRangeSize     = 0,
        .pushStageFlags    = 0,
        .type              = VERTEX_LINE,
        .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
    };

    VkDescriptorSetLayout lineLayouts[] = {
        sceneUBO.layout,
    };

    if (Sol_Pipeline_Build(vkstate, &pipe_config[PIPE_LINE], lineLayouts, 1, &pipeRay.pipe) != 0)
        return 1;

    Sol_CreateFrameBuffer(&solvkstate, sizeof(SolLineVertex) * MAX_LINE_VERTICES, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                          &lineBuffer);

    return 0;
}


SolCollision Collide_Sphere_Tri(CompBody *body, CompXform *xform, SolTri *tri)
{
    SolCollision result   = {0};
    vec3s       *localPos = &xform->pos;

    vec3s closestP = ClosestPointOnTriangle(*localPos, tri->a, tri->b, tri->c);
    vec3s delta    = glms_vec3_sub(*localPos, closestP);
    float distSq   = glms_vec3_dot(delta, delta);

    float radiusSq = body->radius * body->radius;
    if (distSq >= radiusSq)
        return result;

    float dist = sqrtf(distSq);

    vec3s normal      = dist > 0.0001f ? glms_vec3_scale(delta, 1.0f / dist) : tri->normal;
    float penetration = body->radius - dist;
    *localPos         = glms_vec3_add(*localPos, glms_vec3_scale(normal, penetration));

    float velAlongNormal = glms_vec3_dot(body->vel, normal);

    if (velAlongNormal < 0)
    {
        body->vel = glms_vec3_sub(body->vel, glms_vec3_scale(normal, velAlongNormal * body->restitution));
    }

    result.didCollide = true;
    result.pos        = closestP;
    result.normal     = normal;
    result.vel        = body->vel;

    return result;
}


void System_Interact_Tick(World *world, double dt, double time)
{
    SolMouse mouse = Sol_Input_GetMouse();

    int rayId = -1;
    if (world->systemBits & SYS_BIT(WORLD_SYS_PHYSX))
    {
        SolRayResult screenRay = {0};
        if (!mouse.locked)
            screenRay = Sol_ScreenRaycast(world, mouse.x, mouse.y, (SolRay){.mask = 0b11, .dist = 100.0f});
        if (screenRay.hit && screenRay.entId)
            rayId = screenRay.entId;

        Sol_Debug_Add("SelectedEnt", rayId);
    }

    int required = HAS_INTERACT | HAS_BODY2;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompInteract *interact   = &world->interacts[id];
        bool          wasPressed = interact->state & INTERACT_PRESSED;
        interact->state &= ~INTERACT_PRESSED;
        interact->state &= ~INTERACT_CLICKED;
        if (world != interactingEnt.world || id != interactingEnt.id)
        {
            interact->state &= ~INTERACT_HOVERED;
        }
        CompBody2d *body = &world->body2d[id];
        if (id != movingId)
        {
            interact->state &= ~INTERACT_MOVING;
            if (world->masks[id] & HAS_PARENT)
            {
                if (body->overlapCount)
                {
                    Sol_Parent_SetActive(world, id, true);
                    Sol_Parent_SetWithOffset(world, id, body->overlapping[0]);
                }
            }
        }

        if (id != interactingEnt.id)
            continue;

        interact->state |= INTERACT_HOVERED;
        if (mouse.buttons[SOL_MOUSE_MIDDLE] && !movingId)
        {
            SetMoving(world, interact, id);
        }

        if (mouse.buttons[SOL_MOUSE_LEFT])
        {
            interact->state |= INTERACT_PRESSED;
            if (wasPressed && !movingId)
            {
                if (glms_vec2_distance(interact->pressPos, Sol_Input_GetMouseUI()) > 2.0f)
                    SetMoving(world, interact, id);
            }
            else
            {
                interact->pressPos = Sol_Input_GetMouseUI();
            }

            if (interact->onHold.callbackFunc)
                interact->onHold.callbackFunc(interact->state, interact->onHold.callbackData);
        }
        else if (wasPressed && !(interact->state & INTERACT_MOVING))
        {
            interact->state |= INTERACT_CLICKED;
            interact->state &= ~INTERACT_PRESSED;
            if (interact->state & INTERACT_TOGGLEABLE)
                interact->state ^= INTERACT_TOGGLED;

            if (interact->onClick.callbackFunc)
                interact->onClick.callbackFunc(interact->state, interact->onClick.callbackData);
        }
        else
        {
            movingId = 0;
        }
    }
}

```CMAKE
#--------------------------------------
# compile textures
#--------------------------------------
set(IMAGE_DIR "${CMAKE_SOURCE_DIR}/assets/images")
set(CIMAGE_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/assets/cimages")
file(MAKE_DIRECTORY ${CIMAGE_OUTPUT_DIR})
set(IMAGES
    ${IMAGE_DIR}/RedSky2.png
)
set(TEXCONV_EXE ${CMAKE_SOURCE_DIR}/tools/texconv.exe)
foreach(IMAGE ${IMAGES})
    get_filename_component(IMAGE_NAME ${IMAGE} NAME)
    set(IMAGE_OUTPUT "${CIMAGE_OUTPUT_DIR}/${IMAGE_NAME}.dds")
    add_custom_command(
        OUTPUT ${IMAGE_OUTPUT}
        COMMAND ${TEXCONV_EXE}
            -f BC7_UNORM
            -m 0
            -y
            -o ${CIMAGE_OUTPUT_DIR}
            ${IMAGE}
        DEPENDS ${IMAGE}
        COMMENT "Converting texture: ${IMAGE_NAME}"
    )
    list(APPEND CIMAGE_OUTPUTS ${IMAGE_OUTPUT})
endforeach()
add_custom_target(images DEPENDS ${CIMAGE_OUTPUTS})
add_dependencies(solvulk images)
add_dependencies(solapp images)
```


    // float freq = (GLM_PI_2f + look->pitch) * 200.0f;

    // Sol_Audio_SineFreq(0, freq);
    // Sol_Audio_SineFreq(1, freq);
    // Sol_Audio_SineFreq(2, freq);
    // Sol_Audio_SineFreq(3, freq);

    // if (Sol_Input_KeyDown(SOL_KEY_2))
    //     Sol_Audio_SetVolumeSine(0, 0.5f);
    // else
    //     Sol_Audio_SetVolumeSine(0, 0);
    // if (Sol_Input_KeyDown(SOL_KEY_3))
    //     Sol_Audio_SetVolumeSine(1, 0.5f);
    // else
    //     Sol_Audio_SetVolumeSine(1, 0);
    // if (Sol_Input_KeyDown(SOL_KEY_4))
    //     Sol_Audio_SetVolumeSine(2, 0.5f);
    // else
    //     Sol_Audio_SetVolumeSine(2, 0);
    // if (Sol_Input_KeyDown(SOL_KEY_5))
    //     Sol_Audio_SetVolumeSine(3, 0.5f);
    // else
    //     Sol_Audio_SetVolumeSine(3, 0);

    
// static int FindTopMost(World *world)
// {
//     int topZ     = INT_MIN;
//     int winnerId = -1;
//     for (int i = 0; i < world->activeCount; i++)
//     {
//         int id = world->activeEntities[i];
//         if (!WHas(world, id, topmost_required))
//             continue;
//         if (WHas(world, id, BITC(HAS_BODY2)))
//         {
//             vec4s bounds = {
//                 Sol_Xform_GetPos(world, id).x,
//                 Sol_Xform_GetPos(world, id).y,
//                 Sol_Body2d_GetDims(world, id).x,
//                 Sol_Body2d_GetDims(world, id).y,
//             };
//             if (Sol_Check_2d_Collision(Sol_Input_GetMouseUI(), bounds))
//             {
//                 int z = world->body2d[id].zindex;
//                 if (z > topZ)
//                 {
//                     topZ     = z;
//                     winnerId = id;
//                 }
//             }
//         }
//         if (winnerId != -1)
//             continue;
//         if ((world->systemBits & BITC(WORLD_SYS_PHYSX)) && WHas(world, id, BITC(HAS_BODY3)))
//         {
//             SolRayResult result =
//                 Sol_ScreenRaycast(world, Sol_Input_GetMouse().x, Sol_Input_GetMouse().y, (SolRay){.dist = 15.0f});
//             if (result.hit)
//                 winnerId = result.entId;
//         }
//     }
//     return winnerId;
// }

// void Sol_Interact_Update(World **worlds, int count)
// {
//     SolMouse mouse = Sol_Input_GetMouse();
//     // =========================================================================
//     // STATE 1: ACTIVE DRAGGING LAYER
//     // =========================================================================
//     if (interactingEnt.movingId != 0)
//     {
//         // Continuous Dragging Phase: Exit early and let your move system update transforms
//         if (mouse.buttons[SOL_MOUSE_LEFT])
//         {
//             return;
//         }

//         // Drop & Cleanup Phase: Executes the exact frame the mouse button is released
//         World        *mWorld   = interactingEnt.world;
//         int           mId      = interactingEnt.movingId;
//         CompInteract *interact = &mWorld->interacts[mId];
//         CompBody2d   *body     = &mWorld->body2d[mId];

//         interact->state &= ~INTERACT_DRAGGING;

//         // MOVING GUARD: Clear the pressed state flag right here!
//         // This ensures State 2 cannot trigger a click event on the drop frame.
//         interact->state &= ~INTERACT_PRESSED;

//         if (mWorld->masks[mId] & BITC(HAS_PARENT))
//         {
//             if (body->overlapCount)
//             {
//                 Sol_Parent_SetActive(mWorld, mId, true);
//                 Sol_Parent_SetWithOffset(mWorld, mId, body->overlapping[0]);
//             }
//         }

//         // Terminate dragging identifiers completely
//         interactingEnt.movingId = 0;

//         // Let the state fall through to scan whatever is underneath the mouse on drop frame
//     }

//     // =========================================================================
//     // STATE 2: PASSIVE SCAN & CLICK EVALUATION LAYER
//     // =========================================================================
//     int    winner      = -1;
//     World *winnerWorld = NULL;

//     // 1. Scan worlds backwards from front-most screen projection (UI down to Gameplay)
//     for (int w = 0; w < count; w++)
//     {
//         World *world = worlds[w];
//         if (!world->doesSimulate)
//             continue;

//         int foundId = FindTopMost(world);
//         if (foundId != -1)
//         {
//             winner      = foundId;
//             winnerWorld = world;
//             break;
//         }
//     }

//     // 2. Hover Transitions (O(1) State Change Flags)
//     if (winner != interactingEnt.id || winnerWorld != interactingEnt.world)
//     {
//         // Wipe old focus targets cleanly
//         if (interactingEnt.world && interactingEnt.id != -1)
//         {
//             int oldId = interactingEnt.id;
//             if (interactingEnt.world->masks[oldId] & BITC(HAS_INTERACT))
//             {
//                 CompInteract *oldInteract = &interactingEnt.world->interacts[oldId];
//                 oldInteract->state &= ~INTERACT_HOVERED;
//                 oldInteract->state &= ~INTERACT_PRESSED;
//                 oldInteract->state &= ~INTERACT_CLICKED;
//             }
//         }

//         // Bind global tracker to our current physical layer target
//         interactingEnt.id    = winner;
//         interactingEnt.world = winnerWorld;
//     }

//     // 3. Action Selection & Click Processing
//     if (interactingEnt.id != -1 && interactingEnt.world != NULL)
//     {
//         int           id       = interactingEnt.id;
//         World        *world    = interactingEnt.world;
//         CompInteract *interact = &world->interacts[id];

//         // This will now correctly read false if the item was just dropped!
//         bool wasPressed = interact->state & INTERACT_PRESSED;

//         // Reset single-frame transient execution triggers
//         interact->state &= ~INTERACT_PRESSED;
//         interact->state &= ~INTERACT_CLICKED;
//         interact->state |= INTERACT_HOVERED;

//         // Button Down Evaluator Pass
//         if (mouse.buttons[SOL_MOUSE_LEFT])
//         {
//             interact->state |= INTERACT_PRESSED;

//             if (wasPressed)
//             {
//                 // Break deadzone bounds threshold to turn click state into active move state
//                 if (glms_vec2_distance(interact->pressPos, Sol_Input_GetMouseUI()) > 1.0f)
//                 {
//                     SetMoving(world, interact, id);
//                 }
//             }
//             else
//             {
//                 // Frame exact click touchdown snapshot
//                 interact->pressPos = Sol_Input_GetMouseUI();
//             }

//             if (interact->onHold.callbackFunc)
//                 interact->onHold.callbackFunc(interact->state, interact->onHold.callbackData);
//         }
//         // Button Released Evaluator Pass (Clean standard item clicking)
//         else if (wasPressed)
//         {
//             interact->state |= INTERACT_CLICKED;

//             if (interact->state & INTERACT_TOGGLEABLE)
//                 interact->state ^= INTERACT_TOGGLED;

//             if (interact->onClick.callbackFunc)
//                 interact->onClick.callbackFunc(interact->state, interact->onClick.callbackData);
//         }
//     }
//     else
//     {
//         interactingEnt.id    = 0;
//         interactingEnt.world = NULL;
//     }
//     Sol_Tooltip_Update(SOL_TIMESTEP);
// }


// void Sol_Interact_SetMoving(World *world, int id)
// {
//     CompInteract *interact = &world->interacts[id];
//     interact->state |= INTERACT_DRAGGING;
//     interact->grabOffset =
//         glms_vec2_sub(Sol_Input_GetMouseUI(), (vec2s){world->xforms[id].pos.x, world->xforms[id].pos.y});
//     if (world->masks[id] & BITC(HAS_PARENT))
//         Sol_Parent_SetActive(world, id, false);
// }


// void Chain_Lightning_Recursive(World *world, int dealer, int target, int last, float damage, int count)
// {
//     Sol_Vital_Damage(world, target, dealer, damage);
//     count--;
//     if (count < 1)
//         return;
//     if (count >= 64)
//     {
//         Chain_Lightning(world, dealer, target, 0, damage, count);
//         return;
//     }

//     SolRayResult results[64];
//     SolRay       ray         = {.pos = Sol_Xform_GetPos(world, target), .mask = 1, .ignoreEnt = target};
//     int          hits        = Sol_SphereCast(world, ray, 5.0f, results, 64);
//     float        closest     = 999999.9f;
//     float        farthest    = 0;
//     int          closestId   = 0;
//     for (int i = 0; i < hits; i++)
//     {
//         SolRayResult result = results[i];
//         if (!Sol_Owner_GetHostile(world, dealer, result.entId))
//             continue;
//         if (result.entId == last)
//             continue;
//         float distance = Sol_Xform_DistanceTo2(world, target, result.entId);
//         if (distance > farthest)
//         {
//             closest   = distance;
//             farthest  = distance;
//             closestId = result.entId;
//         }
//     }

//     if (closestId > 0)
//     {
//         //Sol_Ribbon_AddBetweenEntities(world, target, closestId, RIBBONKIND_TRAIL, 1.0f,
//         //                              (vec4s){0.7f, 1.0f, 0.0f, 1.0f});
//         Sol_Emitter_Spawn(world, EMITTERKIND_BURST_SPARKS, Sol_Xform_GetPos(world, closestId),
//                           (vec4s){0.7f, 1.0f, 0.0f, 1.0f}, 0.2f);
//         Chain_Lightning(world, dealer, closestId, target, damage, count);
//     }
// }

```c
void Sol_Skeleton_Pose(SolSkeleton *skel, PoseRequest *req)
{
    static vec3   poseT[MAX_BONES];
    static versor poseR[MAX_BONES];
    static vec3   poseS[MAX_BONES];

    // Rest pose
    for (int i = 0; i < skel->boneCount; i++)
    {
        memcpy(&poseT[i], &skel->bones[i].restTrans, sizeof(vec3));
        memcpy(&poseR[i], &skel->bones[i].restRot, sizeof(vec4));
        memcpy(&poseS[i], &skel->bones[i].restScale, sizeof(vec3));
    }

    // Pre-pass: when override is fading out, we still need the lower layers underneath
    // for the fade to blend INTO. So we run lower layers first, then the override on top
    // with its weight.
    bool overrideActive = (req->layers[ANIM_LAYER_OVERRIDE].anim != -1);
    bool overrideFading = overrideActive && (req->layerWeight[ANIM_LAYER_OVERRIDE] < 1.0f);

    int startLayer, endLayer;
    if (overrideActive && !overrideFading)
    {
        // Fully overriding — only run override
        startLayer = ANIM_LAYER_OVERRIDE;
        endLayer   = ANIM_LAYER_OVERRIDE + 1;
    }
    else
    {
        // Either not overriding at all, or overriding with partial weight (run all)
        startLayer = 0;
        endLayer   = ANIM_LAYER_COUNT;
    }

    for (int L = startLayer; L < endLayer; L++)
    {
        AnimBlend *blend  = &req->layers[L];
        BoneMask  *mask   = &req->masks[L];
        float      weight = req->layerWeight[L];

        if (blend->anim == -1 || weight == 0.0f)
            continue;

        // Sample current
        vec3   layerT[MAX_BONES];
        versor layerR[MAX_BONES];
        vec3   layerS[MAX_BONES];
        Sample_Animation_Pose(skel, blend->anim, blend->seek, layerT, layerR, layerS);

        // Cross-fade with previous in same layer
        if (blend->lastAnim != -1 && blend->blendFactor < 1.0f)
        {
            vec3   prevT[MAX_BONES];
            versor prevR[MAX_BONES];
            vec3   prevS[MAX_BONES];
            
            Sample_Animation_Pose(skel, blend->lastAnim, blend->lastSeek, prevT, prevR, prevS);

            for (int i = 0; i < skel->boneCount; i++)
            {
                glm_vec3_lerp(prevT[i], layerT[i], blend->blendFactor, layerT[i]);
                if (glm_quat_dot(layerR[i], prevR[i]) < 0.0f)
                {
                    prevR[i][0] = -prevR[i][0];
                    prevR[i][1] = -prevR[i][1];
                    prevR[i][2] = -prevR[i][2];
                    prevR[i][3] = -prevR[i][3];
                }
                glm_quat_nlerp(prevR[i], layerR[i], blend->blendFactor, layerR[i]);
                glm_quat_normalize(layerR[i]);
                glm_vec3_lerp(prevS[i], layerS[i], blend->blendFactor, layerS[i]);
            }
        }

        // Apply to final pose
        for (int i = 0; i < skel->boneCount; i++)
        {
            if (!mask->layerOwns[i])
                continue;

            if (weight >= 1.0f)
            {
                memcpy(&poseT[i], &layerT[i], sizeof(vec3));
                memcpy(&poseR[i], &layerR[i], sizeof(vec4));
                memcpy(&poseS[i], &layerS[i], sizeof(vec3));
            }
            else
            {
                glm_vec3_lerp(poseT[i], layerT[i], weight, poseT[i]);
                if (glm_quat_dot(poseR[i], layerR[i]) < 0.0f)
                {
                    layerR[i][0] = -layerR[i][0];
                    layerR[i][1] = -layerR[i][1];
                    layerR[i][2] = -layerR[i][2];
                    layerR[i][3] = -layerR[i][3];
                }
                glm_quat_nlerp(poseR[i], layerR[i], weight, poseR[i]);
                glm_quat_normalize(layerR[i]);
                glm_vec3_lerp(poseS[i], layerS[i], weight, poseS[i]);
            }
        }
    }

    // Skinning matrices (same)
    mat4 world[MAX_BONES];
    for (int i = 0; i < skel->boneCount; i++)
    {
        mat4 local, rotM;
        glm_mat4_identity(local);
        glm_translate(local, poseT[i]);
        glm_quat_mat4(poseR[i], rotM);
        glm_mat4_mul(local, rotM, local);
        glm_scale(local, poseS[i]);

        int parent = skel->bones[i].parent;
        if (parent < 0)
            glm_mat4_copy(local, world[i]);
        else
            glm_mat4_mul(world[parent], local, world[i]);

        glm_mat4_mul(world[i], skel->bones[i].inverseBind, req->outBones[i]);
    }
}

for (int m = 0; m < MAX_MAPPED_SKILLS; m++)
{
    if (ability->bindings[m].dirtyApply)
    {
        Sol_Ability_Bind(world, id, m, ability->bindings[m].pendingState,
                         ability->bindings[m].pendingRarity, ability->bindings[m].pendingBonusDamage,
                         ability->bindings[m].pendingBonusBuffs,
                         ability->bindings[m].pendingBonusEffects);
        ability->bindings[m].dirtyApply = false;
    }
}

static int required = BITC(HAS_ACTIVE) | BITC(HAS_ABILITY);
for (int i = 0; i < world->activeCount; i++)
{
    int id = world->activeEntities[i];
    if (!WHas(world, id, required))
        continue;
    CompAbility *ability = Sol_Ability_Get(world, id);
    if (ability_state_func[ability->state].draw)
        ability_state_func[ability->state].draw(world, id, dt, time);
}


static void Ability_Step(World *world, double dt, double time)
{
    static int required = BITC(HAS_ACTIVE) | BITC(HAS_ABILITY);

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!WHas(world, id, required))
            continue;
        if (Sol_Combat_GetDead(world, id))
        {
            Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, 0, true);
            continue;
        }
        if (Sol_Buff_HasBuff(world, id, BUFFKIND_STUN))
            continue;

        CompAbility    *ability    = Sol_Ability_Get(world, id);
        CompController *controller = Sol_Controller_Get(world, id);
        if (!controller)
            continue;

        SolActions actions = controller->actionState;
        for (int m = 0; m < MAX_MAPPED_SKILLS; m++)
        {
            if (ability->bindings[m].dirtyApply)
            {
                Sol_Ability_Bind(world, id, m, ability->bindings[m].pendingState, ability->bindings[m].pendingRarity,
                                 ability->bindings[m].pendingBonusDamage, ability->bindings[m].pendingBonusBuffs,
                                 ability->bindings[m].pendingBonusEffects);
                ability->bindings[m].dirtyApply = false;
            }
        }
        for (int m = 0; m < MAX_MAPPED_SKILLS; m++)
        {
            SkillBinding *b = &ability->bindings[m];
            if (b->boundState == ABILITY_STATE_IDLE || b->actionBit == ACTION_NONE)
                continue;

            bool pressed               = (actions & b->actionBit) != 0;
            ability->stateData[m].held = pressed;

            if (pressed && ability->activeSlot != m)
            {
                Sol_Ability_SetState(world, id, b->boundState, m, false);
            }
        }

        if (ability->state != ABILITY_STATE_IDLE)
        {
            ability_state_func[ability->state].update(world, id, dt);
        }
    }
}


void Sol_Ability_RequestBind(World *world, int id, u32 slot, u32 ability, u32 rarity, float bonusDamage, u32
bonusBuffs,
                             u32 bonusEffects)
{
    CompAbility *a = Sol_Ability_Get(world, id);
    if (slot >= ABILITY_SLOTS)
        return;
    SkillBinding *b = &a->bindings[slot];
    if (b->pendingState == ability && b->pendingRarity == rarity)
        return;
    b->pendingState        = ability;
    b->pendingRarity       = rarity;
    b->pendingBonusDamage  = bonusDamage;
    b->pendingBonusBuffs   = bonusBuffs;
    b->pendingBonusEffects = bonusEffects;

    b->dirtyApply = true;
    b->dirtySend  = true;
}

void Sol_Ability_Bind(World *world, int id, u32 slot, u32 ability, u32 rarity, float bonusDamage, u32 bonusBuffs,
                      u32 bonusEffects)
{
    CompAbility *a = Sol_Ability_Get(world, id);
    if (slot >= ABILITY_SLOTS)
        return;

    SkillBinding *b      = &a->bindings[slot];
    b->boundState        = ability;
    b->boundRarity       = rarity;
    b->boundBonusDamage  = bonusDamage;
    b->boundBonusBuffs   = bonusBuffs;
    b->boundBonusEffects = bonusEffects;

    b->pendingState        = ability;
    b->pendingRarity       = rarity;
    b->pendingBonusDamage  = bonusDamage;
    b->pendingBonusBuffs   = bonusBuffs;
    b->pendingBonusEffects = bonusEffects;

    AbilityStateData *data = &a->stateData[slot];
    data->cooldown    = ability_config[ability][rarity].cooldown;
    data->duration    = ability_config[ability][rarity].duration;
    data->damage      = ability_config[ability][rarity].damage + bonusDamage;
    data->buffs       = ability_config[ability][rarity].buffMask | bonusBuffs;
    data->effects     = ability_config[ability][rarity].effectMask | bonusEffects;
    Sol_Weapon_Equip(world, id, ability, slot);
}

        // SolSweptHit ccdHit;
        // if (Collisions_Swept_Static_Tris(world, id, ws, xform, body3, fdt, &ccdHit))
        // {
        //     // 1. Advance position right up to the impact point (leaving 0.001f skin distance)
        //     float safeT = fmaxf(0.0f, ccdHit.t - 0.001f);
        //     xform->pos  = glms_vec3_add(xform->pos, glms_vec3_scale(body3->vel, fdt * safeT));

        //     // 2. Reflect velocity along surface normal (Slide or Bounce)
        //     float velDotN = glms_vec3_dot(body3->vel, ccdHit.normal);
        //     if (velDotN < 0.0f)
        //     {
        //         // Zero out normal velocity (sliding response)
        //         body3->vel =
        //             glms_vec3_sub(body3->vel, glms_vec3_scale(ccdHit.normal, velDotN * (1.0f + body3->restitution)));
        //     }
        // }
        // else
        // {
        //     // No sweep collision; step full fdt position
        //     xform->pos = glms_vec3_add(xform->pos, glms_vec3_scale(body3->vel, fdt));
        // }


    // SpatialCell cell = Spatial_Cell_GetNeighbors(xform->pos, sys->dynamic_table.cellSize);

    // for (int n = 0; n < 27; n++)
    // {
    //     u32 cellHash = cell.neighborHashes[n];
    //     u32 entry    = SpatialTable_GetEntry(&sys->dynamic_table, cellHash);

    //     while (entry != SPATIAL_NULL)
    //     {
    //         int idB = (int)sys->dynamic_table.value[entry];

    //         if (idA < idB) // Deduplicate pairs
    //         {
    //             SolBody3 *other_body = Sol_Comp_Get(world, idB, SolBody3);

    //             // Don't collide two immovable bodies
    //             if ((body->mass > 0.0f || other_body->mass > 0.0f) && Sol_Physx_DoesCollide(body, other_body) &&
    //                 shape_pair_test[body->shape][other_body->shape])
    //             {
    //                 SolContact contact;
    //                 if (shape_pair_test[body->shape][other_body->shape](world, idA, idB, &contact))
    //                 {
    //                     if (contacts->count < MAX_THREAD_CONTACTS)
    //                     {
    //                         contact.id  = idA;
    //                         contact.idB = idB;

    //                         contacts->contacts[contacts->count++] = contact;
    //                     }
    //                 }
    //             }
    //         }
    //         entry = sys->dynamic_table.next[entry];
    //     }
    // }
    
    // if (user_hit.hoverId != -1 && user_hit.hoverWorld)
    // {
    //     Sol_Interact_RemState(user_hit.hoverWorld, user_hit.hoverId, INTERACT_HOVERED);
    //     Sol_Interact_RemState(user_hit.hoverWorld, user_hit.hoverId, INTERACT_CLICKED);
    // }
    // user_hit.hoverId = -1;
    // for (int i = 0; i < solState.worldCount; i++)
    // {
    //     World *world = solState.worlds[i];
    //     if (!world || !world->doesSimulate)
    //         continue;
    //     int topmost = Sol_Interact_GetTopmost(world);
    //     if (topmost != -1)
    //     {
    //         user_hit.hoverId    = topmost;
    //         user_hit.hoverWorld = world;
    //         Sol_Interact_AddState(world, user_hit.hoverId, INTERACT_HOVERED);
    //         break;
    //     }
    // }

    // SolMouse mouse = Sol_Input_GetMouse();
    // if (user_hit.focusId != -1)
    // {
    //     if (user_hit.isDragging)
    //     {
    //         Sol_Interact_DragEntityTo(user_hit.focusWorld, user_hit.focusId,
    //                                   (vec3s){Sol_Input_GetMouseUI().x, Sol_Input_GetMouseUI().y});
    //         if (mouse.buttonsReleased[SOL_MOUSE_LEFT])
    //         {
    //             Sol_Interact_EndDrag(user_hit.focusWorld, user_hit.focusId);
    //             user_hit.focusId    = -1;
    //             user_hit.isDragging = false;
    //         }
    //     }
    //     else
    //     {
    //         if (glms_ivec2_distance2(user_hit.pressPos, (ivec2s){Sol_Input_GetMouse().x, Sol_Input_GetMouse().y}) >
    //             1.0f)
    //         {
    //             user_hit.isDragging = true;
    //         }
    //         if (mouse.buttonsReleased[SOL_MOUSE_LEFT])
    //         {
    //             Sol_Interact_RemState(user_hit.focusWorld, user_hit.focusId, INTERACT_PRESSED);
    //             Sol_Interact_AddState(user_hit.focusWorld, user_hit.focusId, INTERACT_CLICKED);
    //             user_hit.focusId = -1;
    //         }
    //         Sol_Interact_AddState(user_hit.focusWorld, user_hit.focusId, INTERACT_PRESSED);
    //     }
    // }
    // else if (user_hit.hoverId != -1)
    // {
    //     if (mouse.buttons[SOL_MOUSE_LEFT])
    //     {
    //         user_hit.focusId    = user_hit.hoverId;
    //         user_hit.focusWorld = user_hit.hoverWorld;
    //         user_hit.isFocusUi  = user_hit.isHoverUi;
    //         user_hit.pressPos   = (ivec2s){Sol_Input_GetMouse().x, Sol_Input_GetMouse().y};
    //     }
    // }