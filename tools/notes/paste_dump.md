
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