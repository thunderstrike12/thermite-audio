#include "nav_mesh.hpp"

#include "engine/engine.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/logger.hpp"

using namespace tmt;

void Volume::traverse(tmt::ResourceRef<tmt::VoxelVolume> voxel_volume, int lod_level) {
    traversed = true;
    uint32_t full_depth = voxel_volume->blas->depth;
    uint32_t target_depth = (lod_level < (int)full_depth) ? (full_depth - lod_level) : 1;

    divisor = 1u << (lod_level * 2);
    size = (voxel_volume->size + divisor - 1u) / divisor;

    occupied.assign(size.x * size.y * size.z, false);
    auto set_occupied = [&](glm::uvec3 pos) {
        if (pos.x < size.x && pos.y < size.y && pos.z < size.z) {
            occupied[pos.x + size.x * pos.y + size.x * size.y * pos.z] = true;
        }
    };

    std::function<void(uint32_t, glm::uvec3, uint32_t, uint32_t)> traverse = [&](uint32_t node_idx, glm::uvec3 origin, uint32_t scale, uint32_t depth) {
        const auto& node = voxel_volume->blas->nodes[node_idx];
        uint64_t mask = node.child_mask;
        uint32_t base = node.abs_ptr();

        for (int i = 0; mask; ++i, mask >>= 1) {
            if (!(mask & 1)) continue;

            glm::uvec3 local = { i & 3, (i >> 4) & 3, (i >> 2) & 3 };
            glm::uvec3 child_origin = origin + local * scale;

            // Stop early at target depth, or if we hit a leaf
            if (depth >= target_depth || node.is_leaf()) {
                set_occupied(child_origin / divisor);
            } else {
                uint32_t child_idx = base + std::popcount(node.child_mask & ((1ull << i) - 1));
                traverse(child_idx, child_origin, scale / 4, depth + 1);
            }
        }
    };
    uint32_t scale = 1u << ((full_depth - 1) * 2);
    traverse(0, { 0, 0, 0 }, scale, 1);
}

bool Volume::is_surface(uint32_t x, uint32_t y, uint32_t z) {
    if (voxel_empty(x, y, z)) return false;
    for (size_t sign = 0; sign < 2; sign++) {
        for (size_t dir = 0; dir < 3; dir++) {
            glm::uvec3 temp = { x, y, z };
            temp[(int)dir] += sign == 0 ? (int)1 : -((int)1);
            if (!(temp.x < size.x && temp.y < size.y && temp.z < size.z) || voxel_empty(temp.x, temp.y, temp.z)) {
                return true;
            }
        }
    }
    return false;
};

void NavMesh::generate_mesh_over_time() {
    entered_loop = false;
    if (iteration_nmg == 0) {
        if (!generating) {
            generating_lod += lod_level + 2;
            init();
            nodes->clear();
        }
        volume = Volume();
        volume.traversed = false;
        if (generating_nodes) generating_nodes->clear();
        generating = true;
        generate_mesh(100);
    } else {
        generate_mesh(100);
    }

    iteration_nmg++;

    if (!entered_loop) {
        iteration_nmg = 0;
        if (generating_lod == lod_level) {
            generating = false;
        } else {
            generating_lod--;
        }
        std::swap(nodes, generating_nodes);
    }
}

void NavMesh::generate_mesh(int iterations) {
    if (!volume.traversed) {
        volume.traverse(voxel_volume, generating_lod);
    }
    if (!generating) {
        generating_lod = lod_level;
        init();
        generating_nodes->clear();
        nodes->clear();
        volume.traverse(voxel_volume, generating_lod);
    }

    std::unordered_map<uint32_t, int> node_map;
    constexpr float voxel_scale = 0.1f;
    glm::vec3 full_size = glm::vec3(voxel_volume->size);

    int count = 0;
    for (uint32_t z = 0; z < volume.size.z; z++) {
        for (uint32_t y = 0; y < volume.size.y; y++) {
            for (uint32_t x = 0; x < volume.size.x; x++) {
                count++;
                // every call, skip iterations weve already handled, and from there, only do 'iterations' number of iterations
                if (generating) {
                    if (count <= iteration_nmg * iterations) continue;
                    if (count > (iteration_nmg + 1) * iterations) return;
                    entered_loop = true;
                }
                if (!volume.is_surface(x, y, z)) continue;

                const uint32_t pos_index = x + volume.size.x * y + volume.size.x * volume.size.y * z;

                int node_index;
                if (node_map.contains(pos_index)) {
                    node_index = node_map[pos_index];
                } else {
                    Node node;
                    glm::vec3 voxel_pos = (glm::vec3(x, y, z) + 0.5f) * float(volume.divisor);
                    glm::vec3 centered = voxel_pos - full_size * 0.5f;
                    node.local_pos = centered * voxel_scale;
                    node_map[pos_index] = (int)generating_nodes->size();
                    node_index = (int)generating_nodes->size();
                    generating_nodes->push_back(node);
                }

                for (int dz = -1; dz <= 1; dz++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            if (dx == 0 && dy == 0 && dz == 0) continue;

                            glm::uvec3 neighbor = { x + dx, y + dy, z + dz };
                            if (neighbor.x >= volume.size.x || neighbor.y >= volume.size.y || neighbor.z >= volume.size.z) continue;
                            if (!volume.is_surface(neighbor.x, neighbor.y, neighbor.z)) continue;

                            const uint32_t neighbor_pos_index = neighbor.x + volume.size.x * neighbor.y + volume.size.x * volume.size.y * neighbor.z;

                            int neighbor_node_index;
                            if (node_map.contains(neighbor_pos_index)) {
                                neighbor_node_index = node_map[neighbor_pos_index];
                            } else {
                                Node node;
                                glm::vec3 voxel_pos = (glm::vec3(neighbor.x, neighbor.y, neighbor.z) + 0.5f) * float(volume.divisor);
                                glm::vec3 centered = voxel_pos - full_size * 0.5f;
                                node.local_pos = centered * voxel_scale;
                                node_map[neighbor_pos_index] = (int)generating_nodes->size();
                                neighbor_node_index = (int)generating_nodes->size();
                                generating_nodes->push_back(node);
                            }

                            auto& neighborsA = (*generating_nodes)[node_index].connecting_nodes;
                            if (std::find(neighborsA.begin(), neighborsA.end(), neighbor_node_index) == neighborsA.end()) {
                                neighborsA.push_back(neighbor_node_index);
                            }

                            auto& neighborsB = (*generating_nodes)[neighbor_node_index].connecting_nodes;
                            if (std::find(neighborsB.begin(), neighborsB.end(), node_index) == neighborsB.end()) {
                                neighborsB.push_back(node_index);
                            }
                        }
                    }
                }
            }
        }
    }
    if (!generating) {
        std::swap(nodes, generating_nodes);
    }
}

std::vector<int> NavMesh::find_path(const int starting_node_id, const int ending_node_id) {
    std::vector<int> open = std::vector<int>();
    std::unordered_set<int> closed;
    open.push_back(starting_node_id);
    (*nodes)[starting_node_id].g = 0.0f;
    (*nodes)[starting_node_id].parent = NULL;
    int current = -1;

    // A* search
    // g is the cost from start to current node
    // h is the heuristic cost from current node to end node
    while (!open.empty()) {
        current = -1;
        int current_index = -1;

        // find node in open list with lowest f = g + h
        for (int i = 0; i < (int)open.size(); i++) {
            // set first node as a valid node
            if (current < 0) {
                current_index = i;
                current = open[i];
            }
            // get a better node if available
            else if ((*nodes)[open[i]].g + (*nodes)[open[i]].h < (*nodes)[current].g + (*nodes)[current].h) {
                current_index = i;
                current = open[i];
            }
        }

        if (current == ending_node_id) {
            break;
        }

        // remove current from open list
        open.erase(open.begin() + current_index);
        closed.insert(current_index);

        // add all connecting nodes with a better path to open list
        for (int i = 0; i < (int)(*nodes)[current].connecting_nodes.size(); i++) {
            int next = (*nodes)[current].connecting_nodes[i];
            if (closed.count(next)) continue;
            float edgeCost = glm::distance((*nodes)[next].local_pos, (*nodes)[current].local_pos);

            // if g is not set or we found a better path to next node
            if ((*nodes)[next].g < 0 || (*nodes)[current].g + edgeCost < (*nodes)[next].g) {
                (*nodes)[next].g = (*nodes)[current].g + edgeCost;
                (*nodes)[next].h = glm::distance((*nodes)[next].local_pos, (*nodes)[ending_node_id].local_pos);
                (*nodes)[next].parent = current;

                open.push_back(next);
            }
        }
    }

    // no path found
    if (current != ending_node_id) {
        std::vector<int> noResult;
        return noResult;
    }

    // reconstruct path and return it
    std::vector<int> repath;
    repath.push_back(ending_node_id);
    current = ending_node_id;
    while (1) {
        if (current == starting_node_id) {
            for (int i = 0; i < (int)(*nodes).size(); i++) {
                (*nodes)[i].g = -1.0f;
                (*nodes)[i].h = 0.0f;
                (*nodes)[i].parent = 0;
            }
            return repath;
        }
        repath.push_back((*nodes)[current].parent);
        current = (*nodes)[current].parent;
    }
}

int tmt::NavMesh::find_closest_node(const glm::vec3& position) {
    int closest_node = -1;
    float closest_distance = std::numeric_limits<float>::max();
    for (int i = 0; i < (int)(*nodes).size(); i++) {
        float distance = glm::distance(position, (*nodes)[i].world_pos);
        if (distance < closest_distance) {
            closest_distance = distance;
            closest_node = i;
        }
    }
    return closest_node;
}

std::optional<glm::vec3> tmt::NavMesh::follow_path(glm::vec3 start, glm::vec3 end) {
    // Calculate path
    int start_pos = find_closest_node(start);
    int closest_node_id = find_closest_node(end);
    std::vector<int> new_path = find_path(start_pos, closest_node_id);
    path = new_path;
    if (new_path.size() < 1) return std::nullopt;

    glm::vec3 target_pos = (*nodes)[new_path[0]].world_pos;
    if (path.size() > 2) {
        float attraction_distance = 1.5f;
        int current_node = (int)new_path.size() - 1;
        glm::vec3 current_pos = (*nodes)[new_path[current_node]].world_pos;

        float travelled_distance = 0.0f;
        while (current_node >= 1) {
            // Get next segment
            glm::vec3 next_pos = (*nodes)[new_path[current_node - 1]].world_pos;
            float segment_distance = glm::distance(current_pos, next_pos);
            // Check if we exceed attraction distance
            if (travelled_distance + segment_distance > attraction_distance) {
                float remaining_distance = attraction_distance - travelled_distance;
                glm::vec3 direction = glm::normalize(next_pos - current_pos);
                target_pos = current_pos + direction * remaining_distance;
                break;
                // Exit loop
            } else {
                travelled_distance += segment_distance;
                current_node--;
                current_pos = next_pos;
                if (current_node >= (int)new_path.size() - 1) {
                    target_pos = (*nodes)[new_path.back()].world_pos;
                    break;
                }
            }
        }
    }
    tmt::engine.polyline.draw_sphere(target_pos, 0.1f);
    tmt::engine.polyline.draw_sphere(start, 0.1f);

    if (glm::distance(target_pos, start) > 0.01f) {
        return glm::normalize(target_pos - start);
    }
    return std::nullopt;
}

void tmt::NavMesh::inspect() {
    // return;
    tmt::engine.polyline.use_color(1.0f, 0.0f, 0.0f);
    tmt::engine.polyline.use_line_width(2, true);

    for (int i = 0; i < (*nodes).size(); i++) {
        for (int j = 0; j < (*nodes)[i].connecting_nodes.size(); j++) {
            if (i > (*nodes)[i].connecting_nodes[j]) continue;
            int conn_idx = (*nodes)[i].connecting_nodes[j];
            engine.polyline.draw_line((*nodes)[i].world_pos, (*nodes)[conn_idx].world_pos);
        }
    }

    tmt::engine.polyline.use_color(1.0f, 1.0f, 1.0f);
    for (int i = 0; i < (int)path.size() - 1; i++) {
        tmt::engine.polyline.draw_line((*nodes)[path[i]].world_pos, (*nodes)[path[i + 1]].world_pos);
    }
}
