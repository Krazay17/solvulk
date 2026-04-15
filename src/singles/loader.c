#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "sol_core.h"

static SolModel cpuModels[SOL_MODEL_COUNT] = {0};

// name of the resource linked in the resources.rc
static const char *modelResourceName[SOL_MODEL_COUNT] = {
    "ID_MODEL_WIZARD",
    "ID_MODEL_WORLD0",
    "ID_MODEL_WORLD1",
    "ID_MODEL_WORLD2",
};

SolModel *Sol_GetModel(SolModelId id)
{
    return &cpuModels[id];
}

void Sol_Loader_LoadModels()
{
    for (int i = 0; i < SOL_MODEL_COUNT; ++i)
    {
        cpuModels[i] = Sol_LoadModel(modelResourceName[i]);
        Sol_UploadModel(&cpuModels[i], i);
    }
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

    GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCTSTR)Sol_LoadResource,
        &hmod);

    HRSRC hResource = FindResource(hmod, resourceName, RT_RCDATA);
    if (!hResource && hmod != NULL)
    {
        hResource = FindResource(NULL, resourceName, RT_RCDATA);
        hmod = NULL; // Reset hmod so LoadResource/SizeofResource use the EXE
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
    cgltf_data *data = NULL;
    if (cgltf_parse(&options, res.data, res.size, &data) != cgltf_result_success)
        return model;
    cgltf_load_buffers(&options, data, NULL);

    model.meshCount = (uint32_t)data->meshes_count;
    model.meshes = calloc(model.meshCount, sizeof(SolMesh));

    // Pass 1: count totals
    for (uint32_t m = 0; m < model.meshCount; m++)
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
        model.meshes[m].indexCount = prim->indices ? (uint32_t)prim->indices->count : 0;
        model.totalVertices += model.meshes[m].vertexCount;
        model.totalIndices += model.meshes[m].indexCount;
    }

    // Single allocation
    model.vertices = calloc(model.totalVertices, sizeof(SolVertex));
    model.indices = malloc(model.totalIndices * sizeof(uint32_t));

    // Pass 2: fill data
    uint32_t vOff = 0, iOff = 0;
    for (uint32_t m = 0; m < model.meshCount; m++)
    {
        cgltf_mesh *src = &data->meshes[m];
        if (src->primitives_count == 0)
            continue;
        cgltf_primitive *prim = &src->primitives[0];

        SolMesh *dst = &model.meshes[m];
        dst->vertexOffset = vOff;
        dst->indexOffset = iOff;

        // Material
        dst->material = (SolMaterial){.baseColor = {1, 1, 1, 1}, .roughness = 1.0f};
        if (prim->material && prim->material->has_pbr_metallic_roughness)
        {
            cgltf_pbr_metallic_roughness *pbr = &prim->material->pbr_metallic_roughness;
            memcpy(dst->material.baseColor, pbr->base_color_factor, 16);
            dst->material.metallic = pbr->metallic_factor;
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