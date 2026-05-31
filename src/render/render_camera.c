#include "render_i.h"
#include "sol_core.h"

void Sol_Render_Camera_Update(SolCamera *cam)
{
    // Projection Matrix
    cam->proj = glms_perspective(glm_rad(cam->fov), Sol_Render_GetAspect(), cam->nearClip, cam->farClip);
    // cam->proj[1][1] *= -1;
    cam->proj.raw[1][1] *= -1;

    // View Projection
    cam->viewProj = glms_mat4_mul(cam->proj, cam->view);
    // glm_mat4_mul(cam->proj, cam->view, cam->viewProj);

    SceneUBO *ubo       = Sol_GetDescriptorMapping(DESC_SCENE_UBO);
    ubo->view           = cam->view;
    ubo->proj           = cam->proj;
    ubo->viewProjection = cam->viewProj;
    ubo->cameraPos      = (vec4s){cam->position.x, cam->position.y, cam->position.z, 1.0f};
    ubo->sun            = (vec4s){0.0f, 1.0f, 0.4f, 0.2f};
    // glm_mat4_copy(cam->view, ubo->view);
    // glm_mat4_copy(cam->proj, ubo->proj);
    // glm_mat4_copy(cam->viewProj, ubo->viewProjection);
    // vec4 pos = {cam->position[0], cam->position[1], cam->position[2], 1.0f};
    // glm_vec4_copy(pos, ubo->cameraPos);
    // ubo->sun[0] = 0.0f;
    // ubo->sun[1] = 1.0f;
    // ubo->sun[2] = 0.4f;
    // ubo->sun[3] = 0.2f;
}