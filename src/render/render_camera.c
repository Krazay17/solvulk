#include "sol_core.h"
#include "render_i.h"

void Sol_Render_Camera_Update(SolCamera *cam)
{
    // View Matrix
    glm_lookat(cam->position, cam->target, (vec3){0.0f, 1.0f, 0.0f}, cam->view);

    // Projection Matrix
    glm_perspective(glm_rad(cam->fov), Sol_Render_GetAspect(), cam->nearClip, cam->farClip, cam->proj);
    cam->proj[1][1] *= -1;

    // View Projection
    glm_mat4_mul(cam->proj, cam->view, cam->viewProj);

    SceneUBO *ubo = Sol_GetDescriptorMapping(DESC_SCENE_UBO);
    glm_mat4_copy(cam->view, ubo->view);
    glm_mat4_copy(cam->proj, ubo->proj);
    glm_mat4_copy(cam->viewProj, ubo->viewProjection);
    vec4 pos = {cam->position[0], cam->position[1], cam->position[2], 1.0f};
    glm_vec4_copy(pos, ubo->cameraPos);
    ubo->sun[0] = 0.0f;
    ubo->sun[1] = 1.0f;
    ubo->sun[2] = 0.4f;
    ubo->sun[3] = 0.2f;
}