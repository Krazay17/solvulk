#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include <cglm/struct.h>

#include "sol_core.h"

SolBank         bank                       = {0};
static SolModel cpuModels[SOL_MODEL_COUNT] = {0};

// name of the resource linked in the resources.rc
static const char *modelResourceName[SOL_MODEL_COUNT] = {
    "ID_MODEL_WIZARD",
    "ID_MODEL_WORLD0",
    "ID_MODEL_WORLD1",
    "ID_MODEL_WORLD2",
};

SolBank *Sol_Load_Resources()
{
    for (int i = 0; i < SOL_MODEL_COUNT; ++i)
    {
        cpuModels[i] = Sol_LoadModel(modelResourceName[i]);
        Sol_UploadModel(&cpuModels[i], i);
    }

    bank.fonts[SOL_FONT_ICE].metrics = Sol_LoadResource("ID_FONT_METRICS");
    bank.fonts[SOL_FONT_ICE].atlas   = Sol_LoadResource("ID_FONT_ATLAS");
    bank.fonts[SOL_FONT_ICE].width   = 224;
    bank.fonts[SOL_FONT_ICE].height  = 224;
    Sol_UploadImage(bank.fonts[SOL_FONT_ICE].atlas.data, 224, 224, VK_FORMAT_R8G8B8A8_UNORM, SOL_IMAGE_FONT);

    return &bank;
}

SolBank *Sol_Getbank()
{
    return &bank;
}

SolModel *Sol_GetModel(SolModelId id)
{
    return &cpuModels[id];
}

char *Sol_ReadFile(const char *filename, long *outSize)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        MessageBox(NULL, filename, "Failed to open file", MB_OK);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char *buffer = malloc(size);
    fread(buffer, 1, size, file);
    fclose(file);

    *outSize = size;
    return buffer;
}

SolResource Sol_LoadResource(const char *resourceName)
{
    SolResource res = {0};

    HMODULE hmod = NULL;

    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                      (LPCTSTR)Sol_LoadResource, &hmod);

    HRSRC hResource = FindResource(hmod, resourceName, RT_RCDATA);
    if (!hResource && hmod != NULL)
    {
        hResource = FindResource(NULL, resourceName, RT_RCDATA);
        hmod      = NULL; // Reset hmod so LoadResource/SizeofResource use the EXE
    }
    if (!hResource)
    {
        printf("Failed to find resource %s\n", resourceName);
        return res;
    }

    HGLOBAL hLoaded = LoadResource(hmod, hResource);
    if (!hLoaded)
    {
        printf("Failed to load resource %s\n", resourceName);
        return res;
    }

    res.data = LockResource(hLoaded);
    res.size = SizeofResource(hmod, hResource);

    return res;
}

SolModel Sol_LoadModel(const char *resourceName)
{
    SolModel model = {0};

    SolResource res = Sol_LoadResource(resourceName);
    if (!res.data)
        return model;

    cgltf_options options = {0};
    cgltf_data   *data    = NULL;
    if (cgltf_parse(&options, res.data, res.size, &data) != cgltf_result_success)
        return model;
    cgltf_load_buffers(&options, data, NULL);

    model.mesh_count = (uint32_t)data->meshes_count;
    model.meshes     = calloc(model.mesh_count, sizeof(SolMesh));

    // Pass 1: count totals
    for (uint32_t m = 0; m < model.mesh_count; m++)
    {
        cgltf_mesh *src = &data->meshes[m];
        if (src->primitives_count == 0)
            continue;
        cgltf_primitive *prim = &src->primitives[0];

        cgltf_accessor *posAcc = NULL;
        for (uint32_t a = 0; a < prim->attributes_count; a++)
            if (prim->attributes[a].type == cgltf_attribute_type_position)
                posAcc = prim->attributes[a].data;

        if (!posAcc)
            continue;

        model.meshes[m].vertexCount = (uint32_t)posAcc->count;
        model.meshes[m].indexCount  = prim->indices ? (uint32_t)prim->indices->count : 0;
        model.vertex_count += model.meshes[m].vertexCount;
        model.indice_count += model.meshes[m].indexCount;
    }

    // Single allocation
    model.vertices = calloc(model.vertex_count, sizeof(SolVertex));
    model.indices  = malloc(model.indice_count * sizeof(uint32_t));

    // Pass 2: fill data
    uint32_t vOff = 0, iOff = 0;
    for (uint32_t m = 0; m < model.mesh_count; m++)
    {
        cgltf_mesh *src = &data->meshes[m];
        if (src->primitives_count == 0)
            continue;
        cgltf_primitive *prim = &src->primitives[0];

        SolMesh *dst      = &model.meshes[m];
        dst->vertexOffset = vOff;
        dst->indexOffset  = iOff;

        // Material
        dst->material = (SolMaterial){.baseColor = {1, 1, 1, 1}, .roughness = 1.0f};
        if (prim->material && prim->material->has_pbr_metallic_roughness)
        {
            cgltf_pbr_metallic_roughness *pbr = &prim->material->pbr_metallic_roughness;
            memcpy(dst->material.baseColor, pbr->base_color_factor, 16);
            dst->material.metallic  = pbr->metallic_factor;
            dst->material.roughness = pbr->roughness_factor;
        }

        // Accessors
        cgltf_accessor *posAcc = NULL, *nrmAcc = NULL, *uvAcc = NULL;
        for (uint32_t a = 0; a < prim->attributes_count; a++)
        {
            cgltf_attribute *attr = &prim->attributes[a];
            if (attr->type == cgltf_attribute_type_position)
                posAcc = attr->data;
            if (attr->type == cgltf_attribute_type_normal)
                nrmAcc = attr->data;
            if (attr->type == cgltf_attribute_type_texcoord)
                uvAcc = attr->data;
        }
        if (!posAcc)
            continue;

        // Vertices
        for (uint32_t v = 0; v < dst->vertexCount; v++)
        {
            SolVertex *vert = &model.vertices[vOff + v];
            cgltf_accessor_read_float(posAcc, v, vert->position, 3);
            if (nrmAcc)
                cgltf_accessor_read_float(nrmAcc, v, vert->normal, 3);
            if (uvAcc)
                cgltf_accessor_read_float(uvAcc, v, vert->uv, 2);
        }

        // Indices
        if (prim->indices)
            for (uint32_t i = 0; i < dst->indexCount; i++)
                model.indices[iOff + i] = (uint32_t)cgltf_accessor_read_index(prim->indices, i);

        vOff += dst->vertexCount;
        iOff += dst->indexCount;
    }

    model.tri_count = model.indice_count / 3;
    model.tris      = malloc(model.tri_count * sizeof(SolTri));

    uint32_t triIdx = 0;
    for (uint32_t m = 0; m < model.mesh_count; m++)
    {
        SolMesh *mesh = &model.meshes[m];
        for (uint32_t i = 0; i < mesh->indexCount; i += 3)
        {
            SolTri *t = &model.tris[triIdx++];

            uint32_t i0 = model.indices[mesh->indexOffset + i + 0];
            uint32_t i1 = model.indices[mesh->indexOffset + i + 1];
            uint32_t i2 = model.indices[mesh->indexOffset + i + 2];

            t->a = *(vec3s *)model.vertices[mesh->vertexOffset + i0].position;
            t->b = *(vec3s *)model.vertices[mesh->vertexOffset + i1].position;
            t->c = *(vec3s *)model.vertices[mesh->vertexOffset + i2].position;

            // Face normal from edges
            vec3s e1    = glms_vec3_sub(t->b, t->a);
            vec3s e2    = glms_vec3_sub(t->c, t->a);
            vec3s cross = glms_vec3_cross(e1, e2);
            float len   = glms_vec3_norm(cross);
            t->normal   = len > 0.00001f ? glms_vec3_scale(cross, 1.0f / len) : (vec3s){0, 1, 0};

            t->center = glms_vec3_scale(glms_vec3_add(glms_vec3_add(t->a, t->b), t->c), 1.0f / 3.0f);

            float da  = glms_vec3_norm(glms_vec3_sub(t->a, t->center));
            float db  = glms_vec3_norm(glms_vec3_sub(t->b, t->center));
            float dc  = glms_vec3_norm(glms_vec3_sub(t->c, t->center));
            t->bounds = fmaxf(da, fmaxf(db, dc));
        }
    }

    cgltf_free(data);
    return model;
}

void Sol_FreeModel(SolModel *model)
{
    free(model->vertices);
    free(model->indices);
    free(model->meshes);
    memset(model, 0, sizeof(SolModel));
}