#pragma once

namespace tmt {

/* Camera component used to describe a view of the scene. */
struct Camera {
    /* Field of View in degrees. */
    float fov = 80.0f;
    /* True if this camera is active. */
    bool active = true;
};

}  // namespace tmt
