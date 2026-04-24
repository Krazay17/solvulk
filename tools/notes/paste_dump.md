
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