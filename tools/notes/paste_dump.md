
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


// void Raycast_Tri_Dynamic(World *world, SolRay ray) {
//   SolRayResult result = {0};

//   for (u32 r = 0; r < world->tris->range_count; r++) {
//     DynamicTriRange *range = &world->tris->ranges[r];
//     CompXform       *xform = &world->xforms[range->entity_id];

//     // Inverse-transform ray into model-local space
//     mat3s invRot = glms_quat_mat3(glms_quat_inv(xform->drawQuat));
//     vec3s localOrigin =
//         glms_mat3_mulv(invRot, glms_vec3_sub(result.pos, xform->drawPos));
//     vec3s localDir = glms_mat3_mulv(invRot, ray.dir);

//     // Test against this entity's tris
//     for (u32 t = range->start; t < range->start + range->count; t++) {
//       SolTri *tri = &world->tris->dynamic_tris[t];
//       vec3s         localNormal;
//       float dist = Ray_Tri_Test(localOrigin, localDir, tri, &localNormal);

//       if (dist > 0 && dist < result.dist) {
//         result.hit  = true;
//         result.dist = dist;
//         result.pos  = glms_vec3_add(result.pos, glms_vec3_scale(ray.dir,
//         dist));
//         // Transform normal back to world space
//         mat3s rot       = glms_quat_mat3(xform->drawQuat);
//         result.norm     = glms_mat3_mulv(rot, localNormal);
//         result.triIndex = t;
//       }
//     }
//   }
// }

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


// int Sol_Descriptor_Build(SolVkState *vkstate, SolDescriptor *desc)
// {

//     Sol_Descriptor_Build(&vkstate, sizeof(SceneUBO), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//                                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
//                                &descriptors[DESC_SCENE_UBO]);
//     Sol_Descriptor_Build(&vkstate, sizeof(ModelSSBO) * MAX_MODEL_INSTANCES, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//                                VK_SHADER_STAGE_VERTEX_BIT, &descriptors[DESC_MODEL_SSBO]);
//     return 0;
// }

// int Sol_Descriptor_Buildall(SolVkState *vkstate)
// {
//     for (int i = 0; i < DESC_COUNT; i++)
//         Sol_Descriptor_Build(&vkstate, &descriptors[i]);

//     return 0;
// }

int Sol_Pipeline_Buildall(SolVkState *vkstate)
{

    // for (int i = 0; i < PIPE_COUNT; i++)
    // {
    //     Sol_Pipeline_Build(&vkstate, &pipe_config[i]);
    // }

    // // ─── Text Pipeline ──────────────────────────────────────────
    // SolResource metrics = Sol_LoadResource("ID_FONT_METRICS");
    // if (metrics.data)
    //     Sol_ParseFontMetrics((const char *)metrics.data, 224.0f, 224.0f, fontGlyphs);

    // SolResource fontRes = Sol_LoadResource("ID_FONT_ATLAS");
    // if (!fontRes.data)
    //     return 1;

    // if (Sol_UploadImage(vkstate, fontRes.data, 224, 224, VK_FORMAT_R8G8B8A8_UNORM, &pipeText.fontAtlas) != 0)
    //     return 1;

    // if (Sol_CreateDescriptorImage(vkstate, pipeText.fontAtlas.view, pipeText.fontAtlas.sampler,
    //                               VK_SHADER_STAGE_FRAGMENT_BIT, &pipeText.fontDesc) != 0)
    //     return 1;

    // SolPipelineConfig textConfig = {
    //     .vertResource      = "ID_SHADER_TEXTV",
    //     .fragResource      = "ID_SHADER_TEXTF",
    //     .depthTest         = 0,
    //     .alphaBlend        = 0,
    //     .cullMode          = VK_CULL_MODE_NONE,
    //     .pushRangeSize     = sizeof(SolTextPush),
    //     .pushStageFlags    = VK_SHADER_STAGE_VERTEX_BIT,
    //     .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    // };

    // if (Sol_Pipeline_Build(vkstate, &textConfig, &pipeText.fontDesc.layout, 1, &pipeText.pipe) != 0)
    //     return 1;

    // // ─── 3D Mesh Pipeline ───────────────────────────────────────
    // if (Sol_Descriptor_Build(vkstate, sizeof(SceneUBO), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    //                                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, &sceneUBO) != 0)
    //     return 1;

    // if (Sol_Descriptor_Build(vkstate, sizeof(ModelSSBO) * MAX_MODEL_INSTANCES,
    // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    //                                VK_SHADER_STAGE_VERTEX_BIT, &pipeModel.modelSSBO) != 0)
    //     return 1;

    // VkDescriptorSetLayout meshLayouts[] = {
    //     sceneUBO.layout,
    //     pipeModel.modelSSBO.layout,
    // };

    // SolPipelineConfig meshConfig = {
    //     .vertResource      = "ID_SHADER_MODELV",
    //     .fragResource      = "ID_SHADER_MODELF",
    //     .depthTest         = 1,
    //     .alphaBlend        = 1,
    //     .cullMode          = VK_CULL_MODE_NONE,
    //     .pushRangeSize     = sizeof(SolMaterial),
    //     .pushStageFlags    = VK_SHADER_STAGE_FRAGMENT_BIT,
    //     .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    //     .type              = VERTEX_TRI,
    // };

    // if (Sol_Pipeline_Build(vkstate, &meshConfig, meshLayouts, 2, &pipeModel.pipe) != 0)
    //     return 1;

    // // ─── 2D Rect Pipeline ───────────────────────────────────────
    // SolPipelineConfig rectConfig = {
    //     .vertResource      = "ID_SHADER_RECTV",
    //     .fragResource      = "ID_SHADER_RECTF",
    //     .depthTest         = 0,
    //     .alphaBlend        = 1,
    //     .cullMode          = VK_CULL_MODE_NONE,
    //     .pushRangeSize     = sizeof(float) * 32,
    //     .pushStageFlags    = VK_SHADER_STAGE_VERTEX_BIT,
    //     .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    // };

    // if (Sol_Pipeline_Build(vkstate, &rectConfig, NULL, 0, &pipeRect.pipe) != 0)
    //     return 1;

    // SolPipelineConfig lineConfig = {
    //     .vertResource      = "ID_SHADER_LINEV",
    //     .fragResource      = "ID_SHADER_LINEF",
    //     .depthTest         = 1,
    //     .alphaBlend        = 1,
    //     .cullMode          = VK_CULL_MODE_NONE,
    //     .pushRangeSize     = 0,
    //     .pushStageFlags    = 0,
    //     .type              = VERTEX_LINE,
    //     .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
    // };

    // VkDescriptorSetLayout lineLayouts[] = {
    //     sceneUBO.layout,
    // };

    // if (Sol_Pipeline_Build(vkstate, &pipe_config[PIPE_LINE], lineLayouts, 1, &pipeRay.pipe) != 0)
    //     return 1;

    // Sol_CreateFrameBuffer(&solvkstate, sizeof(SolLineVertex) * MAX_LINE_VERTICES, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    //                       &lineBuffer);

    // return 0;
}