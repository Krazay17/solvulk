#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "sol_core.h"

static SolBank bank = {0};

// name of the resource linked in the resources.rc
static const char *models[SOL_MODEL_COUNT] = {
    "ID_MODEL_WIZARD",
    "ID_MODEL_WORLD0",
};

void Sol_Loader_LoadModels()
{
    for (int i = 0; i < SOL_MODEL_COUNT; ++i)
    {
        SolModel cpu = Sol_LoadModel(models[i]);
        Sol_UploadModel(&cpu, i);
        Sol_FreeModel(&cpu);
    }
}

SolBank *Sol_Loader_GetBank(void)
{
    return &bank;
}

char *Sol_ReadFile(const char *filename, size_t *outSize)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        MessageBox(NULL, filename, "Failed to open shader file", MB_OK);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
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

    HRSRC hResource = FindResource(NULL, resourceName, RT_RCDATA);
    if (!hResource)
    {
        printf("Failed to find resource %s\n", resourceName);
        return res;
    }

    HGLOBAL hLoaded = LoadResource(NULL, hResource);
    if (!hLoaded)
    {
        printf("Failed to load resource %s\n", resourceName);
        return res;
    }

    res.data = LockResource(hLoaded);
    res.size = SizeofResource(NULL, hResource);

    return res;
}

SolModel Sol_LoadModel(const char *resourceName)
{
    SolModel model = {0};

    // load raw bytes from exe resources
    SolResource res = Sol_LoadResource(resourceName);
    if (!res.data)
    {
        printf("Failed to load resource: %s\n", resourceName);
        return model;
    }

    // parse the glb
    cgltf_options options = {0};
    cgltf_data *data = NULL;
    cgltf_result result = cgltf_parse(&options, res.data, res.size, &data);
    if (result != cgltf_result_success)
    {
        printf("Failed to parse GLB: %s\n", resourceName);
        return model;
    }

    // load the buffer data (needed to access actual vertex bytes)
    cgltf_load_buffers(&options, data, NULL);

    // count total meshes across all nodes
    model.meshCount = (uint32_t)data->meshes_count;
    model.meshes = malloc(sizeof(SolMesh) * model.meshCount);
    memset(model.meshes, 0, sizeof(SolMesh) * model.meshCount);

    for (uint32_t m = 0; m < model.meshCount; m++)
    {
        cgltf_mesh *srcMesh = &data->meshes[m];

        // for simplicity take the first primitive only
        if (srcMesh->primitives_count == 0)
            continue;
        cgltf_primitive *prim = &srcMesh->primitives[0];

        SolMesh *dstMesh = &model.meshes[m];
        dstMesh->material.baseColor[0] = 1.0f;
        dstMesh->material.baseColor[1] = 1.0f;
        dstMesh->material.baseColor[2] = 1.0f;
        dstMesh->material.baseColor[3] = 1.0f;
        dstMesh->material.metallic = 0.0f;
        dstMesh->material.roughness = 1.0f;

        if (prim->material)
        {
            cgltf_material *srcMat = prim->material;

            if (srcMat->has_pbr_metallic_roughness)
            {
                cgltf_pbr_metallic_roughness *pbr = &srcMat->pbr_metallic_roughness;
                dstMesh->material.baseColor[0] = pbr->base_color_factor[0];
                dstMesh->material.baseColor[1] = pbr->base_color_factor[1];
                dstMesh->material.baseColor[2] = pbr->base_color_factor[2];
                dstMesh->material.baseColor[3] = pbr->base_color_factor[3];
                dstMesh->material.metallic = pbr->metallic_factor;
                dstMesh->material.roughness = pbr->roughness_factor;
            }
        }

        // --- read indices ---
        if (prim->indices)
        {
            dstMesh->indexCount = (uint32_t)prim->indices->count;
            dstMesh->indices = malloc(sizeof(uint32_t) * dstMesh->indexCount);
            for (uint32_t i = 0; i < dstMesh->indexCount; i++)
            {
                dstMesh->indices[i] = (uint32_t)cgltf_accessor_read_index(prim->indices, i);
            }
        }

        // --- find accessors for position, normal, uv ---
        cgltf_accessor *posAccessor = NULL;
        cgltf_accessor *nrmAccessor = NULL;
        cgltf_accessor *uvAccessor = NULL;

        for (uint32_t a = 0; a < prim->attributes_count; a++)
        {
            cgltf_attribute *attr = &prim->attributes[a];
            if (attr->type == cgltf_attribute_type_position)
                posAccessor = attr->data;
            if (attr->type == cgltf_attribute_type_normal)
                nrmAccessor = attr->data;
            if (attr->type == cgltf_attribute_type_texcoord)
                uvAccessor = attr->data;
        }

        if (!posAccessor)
            continue;

        // --- read vertices ---
        dstMesh->vertexCount = (uint32_t)posAccessor->count;
        dstMesh->vertices = malloc(sizeof(SolVertex) * dstMesh->vertexCount);
        memset(dstMesh->vertices, 0, sizeof(SolVertex) * dstMesh->vertexCount);

        for (uint32_t v = 0; v < dstMesh->vertexCount; v++)
        {
            cgltf_accessor_read_float(posAccessor, v, dstMesh->vertices[v].position, 3);
            if (nrmAccessor)
                cgltf_accessor_read_float(nrmAccessor, v, dstMesh->vertices[v].normal, 3);
            if (uvAccessor)
                cgltf_accessor_read_float(uvAccessor, v, dstMesh->vertices[v].uv, 2);
        }
    }

    cgltf_free(data);
    return model;
}

void Sol_FreeModel(SolModel *model)
{
    for (uint32_t i = 0; i < model->meshCount; i++)
    {
        free(model->meshes[i].vertices);
        free(model->meshes[i].indices);
    }
    free(model->meshes);
    model->meshCount = 0;
}