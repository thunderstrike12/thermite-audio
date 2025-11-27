#pragma once

namespace tmt {

/* Ray definition. */
struct Ray {
    glm::vec3 origin {};
    glm::vec3 dir {};
    glm::vec3 rcp_dir {};
};

/* Ray hit data. */
struct Hit {
    float distance = 0.0f;
};

}  // namespace tmt
